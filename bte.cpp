#include "bte.h"
#include "configure.h"
#include "distribution.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <random>

using namespace std;

// Simulation State (Dyanmic)
vector<request> requests;               // Set of active requests in cluster
vector<request> new_requests_this_tick; // New requests
map<string, map<int, instance>>
    instances;                     // Instance state ( current requests, etc)
vector<signal_msg> signals;        // Set of signals to be handled each tick
tick_clock global_clock;           // Clock
unsigned long last_request_id = 0; // Unique request ids

// Simulation Configuration (Static)
unordered_set<string> drivers;
map<string, distribution> cache;
map<string, distribution> latency;
map<string, distribution> selftime;
map<string, load_model> load_models;
map<string, map<string, distribution>> dependencies;
map<string, simulated_load> simulation;
ofstream stats;

bool tick();

int main(int argc, char **argv) {
  if (argc <= 2) {
    cout << "(Not Much) Better Than Excel is a distributed systems simulator "
            "which coarsely simulates a day between multiple nodes in a series "
            "of configurable ticks"
         << endl;
    cout << argv[0] << " [input file] [output data]" << endl;
    return 1;
  }
  cout << "# Reading configuration from " << argv[1] << endl;
  cout << "# Writing to " << argv[2] << endl;
  if (!parse_config(argv[1]))
    return 1;

  stats.open(argv[2]);
  stats << "tick, node_type, instance_id, load, completed, timeouts, median_latency" << endl;
  while (tick())
    ;
  cout << "# Completed simulation ticks: " << global_clock.current_tick
       << " hour: " << global_clock.hour << endl;
  stats.close();
};

void print_stats() {
  for (auto &n : instances) {
    for (auto &i : n.second) {
      unsigned median = 0;
      unsigned int size = i.second.completed_request_times.size();
      if (size > 0) {
	unsigned int mid = size/2;
	std::nth_element(i.second.completed_request_times.begin(),
		       i.second.completed_request_times.begin() + mid,
		       i.second.completed_request_times.end());
	median = i.second.completed_request_times[mid];
      }
      stats << global_clock.current_tick << "," << n.first << "," << i.first
            << "," << i.second.current_request_count
	    << "," << size
	    << "," << i.second.timeouts
	    << "," << median
	    << endl;
    }
  }
}

void print_dist(distribution d) {
  cout << "{ \"mean\": " << d.mean << ", \"stddev\": " << d.stddev
       << " \"model\":" << d.dist << "}";
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
