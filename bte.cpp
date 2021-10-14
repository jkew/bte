#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <list>
#include <iomanip>
#include <map>
#include <random>
#include <unordered_set>
#include <cassert>
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/cursorstreamwrapper.h"

#define assertm(exp, msg) assert(((void)msg, exp))

using namespace rapidjson;
using namespace std;

typedef enum {
  NORMAL,
  UNIFORM
} dist_model;

typedef enum {
  LINEAR,
  LOGISTIC
} growth_model;

// how requests are allocated across instances 
typedef enum {
  RANDOM,     
  REQUEST,    // request_id % instance_max
  USERS,      // user_id % instance_max
  CONTENT,    // ...
  SITES       // ...
} load_balance_model;

// signals you can send to other requests
typedef enum {
  SIG_CHLD,
  SIG_TERM,
} signal_type;

typedef struct {
  dist_model dist;
  unsigned int mean;
  unsigned int stddev;
  unsigned int min;
  unsigned int max;
} distribution;

typedef struct {
  unsigned int instances;
  unsigned int limit;
  growth_model model;
  load_balance_model balance;
} load_model;

typedef struct {
  unsigned int requests;
  unsigned int users;
  unsigned int content;
  unsigned int sites;
} external_load;

typedef struct {
  external_load hours[24];
} simulated_load;

typedef struct {
  unsigned long id;
  unsigned int user_id;
  unsigned int content_id;
  unsigned int site_id;
  unsigned long start_tick;
  unsigned int  self_time_left;
  unsigned int  network_time_left;
  bool use_cache;
  bool is_child;
  unsigned int parent_id;
  unsigned int parent_instance_id;
  bool is_started;
  bool dependencies_created;
  string node;
  unsigned int instance_id;
  unordered_set<string> dependencies;
} request;

typedef struct {
  string node;
  unsigned int instance_id;
  unsigned int current_request_count;
} instance;

typedef struct {
  unsigned long current_tick;
  unsigned long last_tick;
  unsigned int ms_per_tick;
  unsigned int hour;
} tick_clock;

typedef struct {
  signal_type sig;
  unsigned int dest_id;
  unsigned int dest_instance_id;
  string src_node;
  unsigned int src_id;
  unsigned int src_instance_id;
} signal_msg;

// Simulation State (Dyanmic)
vector<request> requests;                  // Set of active requests in cluster
vector<request> new_requests_this_tick;    // New requests          
map<string, map<int, instance>> instances; // Instance state ( current requests, etc)
vector<signal_msg> signals;                    // Set of signals to be handled each tick
tick_clock global_clock;                   // Clock
unsigned long last_request_id = 0;         // Unique request ids
std::default_random_engine generator;      // rnd

// Simulation Configuration (Static)
unordered_set<string> drivers;
map<string, distribution>   cache;
map<string, distribution> latency;
map<string, distribution> selftime;
map<string, load_model>   load_models;
map<string, map<string, distribution>> dependencies;
map<string, simulated_load> simulation;
Document config;
ofstream stats;

void send_signal(signal_msg s);
void handle_signal(signal_msg s);
bool tick();
void initialize_simulation();
void configure(const char *start);
void print_dist(distribution d);
void print_node(const char *node);

int main(int argc, char **argv) {
  if (argc <= 2) {
    cout << "(Not Much) Better Than Excel is a distributed systems simulator which coarsely simulates a day between multiple nodes in a series of configurable ticks" << endl;
    cout << argv[0] << " [input file] [output data]" << endl;
    return 1;
  }
  if (argc > 2) {
    cout << "# Reading configuration from " << argv[1] << endl;
    cout << "# Writing to " << argv[2] << endl;
    ifstream c(argv[1]);
    stringstream buffer;
    buffer << c.rdbuf();
    StringStream ss(buffer.str().c_str());
    CursorStreamWrapper<StringStream> csw(ss);
    config.ParseStream(csw);
    if (config.HasParseError()) {
      cerr << "Error Parsing " << argv[1]
	   << " :" << GetParseError_En(config.GetParseError())
	   << " at line: " << csw.GetLine()
	   << " col: " << csw.GetColumn() << endl;
      return 1;
    }
    assertm(config.IsObject(), "Input file is not an object");
    assertm(config.HasMember("drivers"), "Need at least one external load driver");
    assertm(config["drivers"].IsArray(), "Set of load drivers needs to be an array");
    for (auto& v : config["drivers"].GetArray())
      configure(v.GetString());
  }
  stats.open(argv[2]);
  stats << "tick, node_type, instance_id, load" << endl;
  initialize_simulation();
  while(tick());
  cout << "# Completed simulation ticks: " << global_clock.current_tick << " hour: " << global_clock.hour << endl;
  stats.close();
};

void send_signal(signal_msg s) {
  signals.push_back(s);
}

void handle_signal(signal_msg s) {
  switch(s.sig) {
  case SIG_CHLD:
    {
      for (vector<request>::iterator it = requests.begin() ; it != requests.end(); ++it) {
	if (s.dest_id == it->id && s.dest_instance_id == it->instance_id) {
	  it->dependencies.erase(s.src_node);
	  return;
	}
      }
    }
    break;
  case SIG_TERM:
    {
      requests.erase(remove_if(requests.begin(), 
			       requests.end(),
			       [s](request const & r)
		      {
			return r.id == s.src_id && r.instance_id == s.src_instance_id;
		      }
		   ), requests.end());
      instances[s.src_node][s.src_instance_id].current_request_count--;
    }
    break;
  }

}
unsigned int get_value_uniform(distribution d) {
  uniform_int_distribution<int> distribution(d.min, d.max);
  double number = distribution(generator);
  if (number < 0) number = 0;
  return (unsigned int) number;
}

unsigned int get_value_normal(distribution d) {
  normal_distribution<int> distribution(d.mean,d.stddev);
  double number = distribution(generator);
  if (number < 0) number = 0;
  return (unsigned int) number;
}

unsigned int get_value(distribution d) {
  switch(d.dist) {
  case NORMAL:
    return get_value_normal(d);
  case UNIFORM:
    return get_value_uniform(d);
  default:
    assert(false);
  }
}

unsigned int get_instance(load_model l, request r) {
  switch (l.balance) {
  case load_balance_model::RANDOM: {
    std::uniform_int_distribution<int> dist(0,l.instances - 1);
    return dist(generator);
  }
  case load_balance_model::REQUEST:
    return r.id % l.instances;
  case load_balance_model::USERS:
    return r.user_id % l.instances;
  case load_balance_model::CONTENT:
    return r.content_id % l.instances;
  case load_balance_model::SITES:
    return r.site_id % l.instances;
  }
}

void create_request(string node,
		    unsigned long parent_id,
		    unsigned long parent_instance_id,
		    unsigned int user_id,
		    unsigned int content_id,
		    unsigned int site_id,
		    unsigned long start_tick) {
  assert(user_id > 0);
  assert(content_id > 0);
  assert(site_id > 0);
  assert(!node.empty());
  request new_request;
  unsigned long request_id = ++last_request_id;
  new_request.id = request_id;
  new_request.parent_id = parent_id;
  if (parent_id > 0) {
    new_request.is_child = true;
  } else {
    new_request.is_child = false;
  }
  new_request.user_id = user_id;
  new_request.content_id = content_id;
  new_request.site_id = site_id;
  new_request.parent_instance_id = parent_instance_id;
  new_request.dependencies_created = false;
  new_request.node = string(node);
  new_request.start_tick = start_tick;

  // network time
  new_request.network_time_left = get_value(latency[node]);
  // use cache / self time
  unsigned int cache_value= get_value(cache[node]);
  std::uniform_int_distribution<int> cdist(1,100);
  if (cache_value > cdist(generator)) {
    new_request.use_cache = true;
    new_request.self_time_left = 0;
  } else {
    new_request.self_time_left = get_value(selftime[node]);
  }
  // instance_id
  new_request.instance_id = get_instance(load_models[node], new_request);
  // dependencies
  for (const auto &dep : dependencies[node]) {
    new_request.dependencies.emplace(dep.first);
  }
  new_request.is_started = false;

  // add to requests
  new_requests_this_tick.push_back(new_request);

}

void create_request(string node,
		    unsigned int max_users,
		    unsigned int max_content,
		    unsigned int max_sites) {
  assert(!node.empty());
  std::uniform_int_distribution<int> udist(1,max_users);
  std::uniform_int_distribution<int> cdist(1,max_content);
  std::uniform_int_distribution<int> sdist(1,max_sites);
  std::uniform_int_distribution<int> start_time_dist(1,1000*60*60);
  unsigned long ms_splay = start_time_dist(generator);
  unsigned long tick_splay = ms_splay / global_clock.ms_per_tick;
  unsigned long adjusted_start_tick = global_clock.current_tick + tick_splay;
  create_request(node,
		 0,
		 0,
		 udist(generator),
		 cdist(generator),
		 sdist(generator),
		 adjusted_start_tick);
}

unsigned int sum_requests(string node) {
  assert(!node.empty());
  unsigned int request_cnt = 0;
  for (const auto &inst : instances[node]) {
    request_cnt += inst.second.current_request_count;
  }
  return request_cnt;
}

//=(1/(1+e^(-1(x-[mid_point]))))*[limit]
bool try_start_logistic(unsigned int limit, unsigned int current_load) {
  unsigned int mid_point = (int)(limit/2);
  unsigned int adj_limit = (unsigned int) (1.0/(1.0+exp(-1.0*(current_load-mid_point))))*limit;
  if (adj_limit == 0) adj_limit = 1;
  return (current_load <= adj_limit);
}

bool try_start_linear(unsigned int limit, unsigned int current_load) {  
  return (current_load <= limit);
}

bool try_start(string node, unsigned int instance_id) {
  growth_model model = load_models[node].model;
  unsigned int limit = load_models[node].limit;
  unsigned int current_load = instances[node][instance_id].current_request_count;
  switch (model) {
  case LINEAR:
    return try_start_linear(limit, current_load);
  case LOGISTIC:
    return try_start_logistic(limit, current_load);
  }
  return true;
}

// returns false if request is removable
bool tick_request(request &r) {
  assert(!r.node.empty());
  if (r.start_tick > global_clock.current_tick)
    return true; // wait until the actual start time

  //cout << "r: " << r.node << " id:" << r.id << " st:" << r.self_time_left << " nt:" << r.network_time_left << " deps:" << r.dependencies.size() << " instance: " << r.instance_id << " started: " << r.is_started << endl; 
  
  if (!r.is_started) {
    // If instance has capacity; let's GO
    r.is_started = try_start(r.node, r.instance_id);
    if (!r.is_started) return true; // wait our turn
    // increment instance load
    instances[r.node][r.instance_id].current_request_count++;
  }

  if (!r.dependencies_created) {
    for (const auto d : r.dependencies) {
      assert(!d.empty());      
      create_request(d, r.id,
		     r.instance_id,
		     r.user_id, r.content_id, r.site_id,
		     global_clock.current_tick); // start right away
    }
    r.dependencies_created = true;
  }
  
  if (!r.use_cache && r.self_time_left > global_clock.ms_per_tick) {
    r.self_time_left -= global_clock.ms_per_tick;
    return true; // keep on having me time
  } else
    r.self_time_left = 0;

  if (r.network_time_left > global_clock.ms_per_tick) {
    r.network_time_left -= global_clock.ms_per_tick;
    return true; // keep on having you time
  } else
    r.network_time_left = 0;

  if (r.dependencies.size() > 0)
    return true; // keep waiting for my children

  // if we got here we might be able to finish the job
  // notify parent
  if (r.is_child) {
    signal_msg chld;
    chld.sig = SIG_CHLD;
    chld.dest_id = r.parent_id;
    chld.dest_instance_id = r.parent_instance_id;
    chld.src_id = r.id;
    chld.src_instance_id = r.instance_id;
    chld.src_node = r.node;
    send_signal(chld);
  }
  // kill ourselves
  signal_msg term;
  term.sig = SIG_TERM;
  term.dest_id = r.id;
  term.dest_instance_id = r.instance_id;
  term.src_id = r.id;
  term.src_instance_id = r.instance_id;
  term.src_node = r.node;
  send_signal(term);
  //cout << "r: " << r.node << " id:" << r.id << " TERM" << endl; 
  return false;
}

void print_stats() {
  for (const auto &n : instances) {
    for (const auto &i : n.second) {
      stats << global_clock.current_tick << "," << n.first << "," << i.first << "," << i.second.current_request_count << endl;
    }
  }
}

bool tick() {
  if (global_clock.current_tick < global_clock.last_tick) {
    global_clock.current_tick += global_clock.ms_per_tick;
    unsigned int last_hour = global_clock.hour;
    global_clock.hour = (global_clock.current_tick * global_clock.ms_per_tick)/1000/60/60 + 1;
    if (last_hour != global_clock.hour) {
      cout << "# hour " << global_clock.hour << " load: ";      
      for (auto &d : drivers) {
	assert(!d.empty());
	unsigned int max_users =  simulation[d].hours[global_clock.hour - 1].users;
	unsigned int max_content =  simulation[d].hours[global_clock.hour - 1].content;
	unsigned int max_sites =  simulation[d].hours[global_clock.hour - 1].sites;
	std::uniform_int_distribution<int> udist(1,max_users);
	std::uniform_int_distribution<int> cdist(1,max_content);
	std::uniform_int_distribution<int> sdist(1,max_sites);
	std::uniform_int_distribution<int> start_time_dist(1,1000*60*60);
	for (int r = 0; r < simulation[d].hours[global_clock.hour - 1].requests; r++) {
	  // create external load request
	  unsigned long ms_splay = start_time_dist(generator);
	  unsigned long tick_splay = ms_splay / global_clock.ms_per_tick;
	  unsigned long adjusted_start_tick = global_clock.current_tick + tick_splay;
	  create_request(d,
			 0,
			 0,
			 udist(generator),
			 cdist(generator),
			 sdist(generator),
			 adjusted_start_tick);
	}
	cout << d << " " << sum_requests(d) << "(+" << simulation[d].hours[global_clock.hour - 1].requests << ") ";
      }
      for (const auto &n : instances) {
	assert(!n.first.empty());
	if (!drivers.contains(n.first)) {
	  cout << n.first << " " << sum_requests(n.first)  << " ";
	}
      }
      cout << endl;
    }
    // process requests
    for (auto &r : requests) {  
      assert(!r.node.empty());
      tick_request(r);
    }
    requests.insert(requests.end(), new_requests_this_tick.begin(), new_requests_this_tick.end());
    new_requests_this_tick.clear();
    int request_count = requests.size();
    for (auto s: signals) {
      handle_signal(s);
      assert(requests.size() <= request_count);
    }
    signals.clear();
    print_stats();
    return true;
  }
  return false;
}

void initialize_simulation() {
  assertm(config.HasMember("ms_per_tick"), "Need to specify the ms per tick");
  global_clock.ms_per_tick = config["ms_per_tick"].GetInt();
  global_clock.current_tick = 0;
  global_clock.last_tick = (unsigned long)(24*60*60*1000)/global_clock.ms_per_tick;
  global_clock.hour = -1;

  for (auto& v : config["drivers"].GetArray()) {
    simulated_load sim;
    assertm(config[v.GetString()].HasMember("load"), "No hourly load configured for driver");
    assertm(config[v.GetString()]["load"].HasMember("requests"), "No hourly request load");
    assertm(config[v.GetString()]["load"].HasMember("users"), "No hourly users load");
    assertm(config[v.GetString()]["load"].HasMember("content"), "No hourly content load");
    assertm(config[v.GetString()]["load"].HasMember("sites"), "No hourly site load");
    assertm(config[v.GetString()]["load"]["requests"].IsArray(), "load must be an array");
    assertm(config[v.GetString()]["load"]["users"].IsArray(), "load must be an array");
    assertm(config[v.GetString()]["load"]["content"].IsArray(), "load must be an array");
    assertm(config[v.GetString()]["load"]["sites"].IsArray(), "load must be an array");
    assertm(config[v.GetString()]["load"]["requests"].Size() == 24, "load must be an 24 array");
    assertm(config[v.GetString()]["load"]["users"].Size() == 24, "load must be an 24 array");
    assertm(config[v.GetString()]["load"]["content"].Size() == 24, "load must be an 24 array");
    assertm(config[v.GetString()]["load"]["sites"].Size() == 24, "load must be an 24 array");
    for (int i = 0; i < 24; i++) {
      external_load hour_load;
      hour_load.requests = config[v.GetString()]["load"]["requests"][i].GetInt();
      hour_load.users = config[v.GetString()]["load"]["users"][i].GetInt();
      hour_load.content = config[v.GetString()]["load"]["content"][i].GetInt();
      hour_load.sites = config[v.GetString()]["load"]["sites"][i].GetInt();
      assert(hour_load.requests >= hour_load.sites);
      assert(hour_load.requests >= hour_load.content);
      assert(hour_load.users >= hour_load.sites);
      assert(hour_load.content >= hour_load.sites);      
      sim.hours[i] = hour_load;
    }
    simulation[v.GetString()] = sim;
    drivers.emplace(v.GetString());
  }
  for (const auto &type : instances) {
    cout << "# " << type.first << " nodes: " << type.second.size() << endl;
  }

}

distribution configure_dist(const Value& input) {
  distribution d;
  if (strcmp(input["dist"].GetString(), "normal") == 0) {
    d.dist = dist_model::NORMAL;
    assertm(input.HasMember("mean"), "Mean not set on distribution");
    assertm(input.HasMember("stddev"), "Mean not set on distribution");
    assert(input["mean"].IsInt());
    assert(input["stddev"].IsInt());
    d.mean = input["mean"].GetInt();
    d.stddev = input["stddev"].GetInt();
    if (d.stddev == 0)
      d.stddev = 1;
    if (d.mean == 0)
      d.mean = 1;
  } else if (strcmp(input["dist"].GetString(), "uniform") == 0) {
    d.dist = dist_model::UNIFORM;
    assertm(input.HasMember("min"), "Min not set on distribution");
    assertm(input.HasMember("max"), "Max not set on distribution");
    assert(input["min"].IsInt());
    assert(input["max"].IsInt());
    d.min = input["min"].GetInt();
    d.max = input["max"].GetInt();
  } else {
    assertm(false, "Only normal distributions are supported");
  }
  return d;
}

void configure(const char *node) {
  if (cache.contains(node) || strlen(node) == 0) {
    return;
  }
  cout << "#### Configuring " << node << endl;
  assertm(config.HasMember(node), "Unable to find referenced node!");
  assertm(config[node].IsObject(), "Node does not exist");
  assertm(config[node].HasMember("cache"), "Node does have cache configuration");
  assertm(config[node].HasMember("network_latency"), "Node does have cache configuration");
  assertm(config[node].HasMember("self_time"), "Node does have self time configuration");
  assertm(config[node].HasMember("dependencies"), "Node does have dependency configuration");
  assertm(config[node].HasMember("instances"), "Node does not specify the number of instances");
  assertm(config[node].HasMember("balanced"), "Node does not specify how load is balanced across instances");
  assertm(config[node].HasMember("growthmodel"), "Node does not specify a growth model for capacity");
  assertm(config[node].HasMember("limit"), "Node does not specify a limit on capacity");

  
  assertm(config[node]["cache"].IsObject(), "Cache needs to be an object");
  assertm(config[node]["network_latency"].IsObject(), "network latency needs to be an object");
  assertm(config[node]["self_time"].IsObject(), "self time needs to be an object");
  assertm(config[node]["dependencies"].IsObject(), " depedencies needs to be an object");

  distribution cache_value = configure_dist(config[node]["cache"]);
  cache[string(node)] = cache_value;

  distribution latency_value = configure_dist(config[node]["network_latency"]);
  latency[string(node)] = latency_value;

  distribution self_value = configure_dist(config[node]["self_time"]);
  selftime[string(node)] = self_value;
  load_model load;
  assert(config[node]["instances"].IsInt());
  assert(config[node]["limit"].IsInt());
  load.instances = config[node]["instances"].GetInt();
  load.limit = config[node]["limit"].GetInt();

  // How is the response rate impacted by concurrent requests
  if (strcmp(config[node]["growthmodel"].GetString(), "linear") == 0) {
    load.model = growth_model::LINEAR;
  } else if (strcmp(config[node]["growthmodel"].GetString(), "logistic") == 0) {
    load.model = growth_model::LOGISTIC;
  } else {
    cerr << "Growth model unsupported " << config[node]["growthmodel"].GetString() << endl;
    assert(false);
  }

  // How are requests balanced across instances
  if (strcmp(config[node]["balanced"].GetString(), "requests") == 0) {
    load.balance = load_balance_model::REQUEST;
  } else if (strcmp(config[node]["balanced"].GetString(), "users") == 0) {
    load.balance = load_balance_model::USERS;
  } else if (strcmp(config[node]["balanced"].GetString(), "content") == 0) {
    load.balance = load_balance_model::CONTENT;
  } else if (strcmp(config[node]["balanced"].GetString(), "sites") == 0) {
    load.balance = load_balance_model::SITES;
  } else if (strcmp(config[node]["balanced"].GetString(), "random") == 0) {
    load.balance = load_balance_model::RANDOM;
  } else {
    cerr << "Load balancing model unsupported " << config[node]["growthmodel"].GetString() << endl;
    assert(false);
  }
  
  load_models[string(node)] = load;
  for (int i = 0; i < load.instances; i++) {
    instance inst;
    inst.node = string(node);
    inst.instance_id = i;
    inst.current_request_count =0;
    instances[string(node)][i] = inst;
  }
  
  print_node(node);

  map<string, distribution> deps;
  for (auto& m : config[node]["dependencies"].GetObject()) {
    cout << "#\t-> " << m.name.GetString();
    distribution dep_value = configure_dist(m.value);
    deps[string(m.name.GetString())] = dep_value;
    cout << " ";
    print_dist(dep_value);
    cout << endl;
  }
  dependencies[string(node)]=deps;
  for (auto& m : config[node]["dependencies"].GetObject()) {
    configure(m.name.GetString());
  }
 }


void print_dist(distribution d) {
  cout << "{ \"mean\": " << d.mean << ", \"stddev\": " << d.stddev << " \"model\":" << d.dist << "}";
}

void print_node(const char *node) {
  cout << "# " << node;
  cout << endl << "#\t\"cache\":";
  print_dist(cache[node]);
  cout << endl << "#\t\"network_latency\":";
  print_dist(latency[node]);
  cout << endl << "#\t\"self_time\":";
  print_dist(selftime[node]);
  cout << endl;
}
