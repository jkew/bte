#include "bte.h"
#include "distribution.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/cursorstreamwrapper.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace rapidjson;
using namespace std;

Document config;

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

bool parse_config(char *file) {
  ifstream c(file);
  stringstream buffer;
  buffer << c.rdbuf();
  StringStream ss(buffer.str().c_str());
  CursorStreamWrapper<StringStream> csw(ss);
  config.ParseStream(csw);
  if (config.HasParseError()) {
    cerr << "Error Parsing " << file
	 << " :" << GetParseError_En(config.GetParseError())
	 << " at line: " << csw.GetLine()
	 << " col: " << csw.GetColumn() << endl;
    return false;
  }
  assertm(config.IsObject(), "Input file is not an object");
  assertm(config.HasMember("drivers"), "Need at least one external load driver");
  assertm(config["drivers"].IsArray(), "Set of load drivers needs to be an array");
  for (auto& v : config["drivers"].GetArray())
    configure(v.GetString());
  initialize_simulation();
  return true;
}
