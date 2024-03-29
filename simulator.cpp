#include "bte.h"
#include <iostream>
#include <vector>

using namespace std;

bool try_start(string node, unsigned int instance_id);
unsigned int get_instance(load_model l, request r);

void send_signal(signal_msg s) { signals.push_back(s); }

void handle_signal(signal_msg s) {
  switch (s.sig) {
  case SIG_CHLD: {
    for (vector<request>::iterator it = requests.begin(); it != requests.end();
         ++it) {
      if (s.dest_id == it->id && s.dest_instance_id == it->instance_id) {
        it->dependencies.erase(s.src_node);
        return;
      }
    }
  } break;
  case SIG_TERM: {
    requests.erase(remove_if(requests.begin(), requests.end(),
                             [s](request const &r) {
                               return r.id == s.src_id &&
                                      r.instance_id == s.src_instance_id;
                             }),
                   requests.end());
    instances[s.src_node][s.src_instance_id].current_request_count--;
  } break;
  }
}

void create_request(string node, unsigned long parent_id,
                    unsigned long parent_instance_id, unsigned int user_id,
                    unsigned int content_id, unsigned int site_id,
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
  unsigned int cache_value = get_value(cache[node]);
  if (cache_value > 50) {
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

unsigned int sum_requests(string node) {
  assert(!node.empty());
  unsigned int request_cnt = 0;
  for (const auto &inst : instances[node]) {
    request_cnt += inst.second.current_request_count;
  }
  return request_cnt;
}

// returns false if request is removable
bool tick_request(request &r) {
  assert(!r.node.empty());
  if (r.start_tick > global_clock.current_tick)
    return true; // wait until the actual start time
  
  // cout << "r: " << r.node << " id:" << r.id << " st:" << r.self_time_left <<
  // " nt:" << r.network_time_left << " deps:" << r.dependencies.size() << "
  // instance: " << r.instance_id << " started: " << r.is_started << endl;

  if (!r.is_started) {
    // If instance has capacity; let's GO
    r.is_started = try_start(r.node, r.instance_id);
    if (!r.is_started)
      return true; // wait our turn
    // increment instance load
    instances[r.node][r.instance_id].current_request_count++;
  }

  // Handle timeouts
  unsigned int timeout = load_models[r.node].timeout;
  unsigned int timespent = (global_clock.current_tick - r.start_tick) * global_clock.ms_per_tick;
  if (timespent > timeout) {
    // Right now we do not do anything but terminate the request
    // we could send a signal though and terminate upstream requests as well
    instances[r.node][r.instance_id].timeouts++;
    goto term;
  }

  
  if (!r.dependencies_created) {
    for (const auto d : r.dependencies) {
      assert(!d.empty());
      create_request(d, r.id, r.instance_id, r.user_id, r.content_id, r.site_id,
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

 term:
  
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

  instances[r.node][r.instance_id].completed_request_times.push_back(global_clock.current_tick - r.start_tick);
  
  // cout << "r: " << r.node << " id:" << r.id << " TERM" << endl;
  return false;
}

bool tick() {
  if (global_clock.current_tick < global_clock.last_tick) {
    global_clock.current_tick += global_clock.ms_per_tick;
    unsigned int last_hour = global_clock.hour;
    global_clock.hour = (global_clock.current_tick * global_clock.ms_per_tick) /
                            1000 / 60 / 60 +
                        1;
    if (last_hour != global_clock.hour) {
      cout << "# hour " << global_clock.hour << " load: ";
      for (auto &d : drivers) {
        assert(!d.empty());
        unsigned int max_users =
            simulation[d].hours[global_clock.hour - 1].users;
        unsigned int max_content =
            simulation[d].hours[global_clock.hour - 1].content;
        unsigned int max_sites =
            simulation[d].hours[global_clock.hour - 1].sites;
        for (int r = 0; r < simulation[d].hours[global_clock.hour - 1].requests;
             r++) {
          // create external load request
	  unsigned long ms_splay;
	  switch (simulation[d].distribution) {
	  case UNIFORM:
	    ms_splay = get_value_uniform(1, 3600000);
	    break;
	  case NORMAL:  // This really isn't a useful load model
	    ms_splay = get_value_normal(30, 15)*60000;
	    break;
	  case LOGNORMAL:
	    ms_splay = get_value_lognormal(30, 15)*60000;
	    break;
	  case GEOMETRIC:
	    ms_splay = get_value_geometric(1800000); // first 15 minutes
	    break;
	  }

          unsigned long tick_splay = ms_splay / global_clock.ms_per_tick;
          unsigned long adjusted_start_tick =
              global_clock.current_tick + tick_splay;
          create_request(d, 0, 0, get_value_uniform(1, max_users),
                         get_value_uniform(1, max_content),
                         get_value_uniform(1, max_sites), adjusted_start_tick);
        }
        cout << d << " " << sum_requests(d) << "(+"
             << simulation[d].hours[global_clock.hour - 1].requests << ") ";
      }
      for (const auto &n : instances) {
        assert(!n.first.empty());
        if (!drivers.contains(n.first)) {
          cout << n.first << " " << sum_requests(n.first) << " ";
        }
      }
      cout << endl;
    }
    // process requests
    for (auto &r : requests) {
      assert(!r.node.empty());
      tick_request(r);
    }
    requests.insert(requests.end(), new_requests_this_tick.begin(),
                    new_requests_this_tick.end());
    new_requests_this_tick.clear();
    int request_count = requests.size();
    for (auto s : signals) {
      handle_signal(s);
      assert(requests.size() <= request_count);
    }
    signals.clear();
    print_stats();

    // reset counters
    for (auto &n : instances) {
      for (auto &i : n.second) {
	i.second.timeouts = 0;
	i.second.completed_request_times.clear();
      }
    }
    return true;
  }
  return false;
}
