#pragma once
#include "distribution.h"
#include <cassert>
#include <map>
#include <string>
#include <unordered_set>
#define assertm(exp, msg) assert(((void)msg, exp))

typedef enum { LINEAR, LOGISTIC } growth_model;

// how requests are allocated across instances
typedef enum {
  RANDOM,
  REQUEST, // request_id % instance_max
  USERS,   // user_id % instance_max
  CONTENT, // ...
  SITES    // ...
} load_balance_model;

// signals you can send to other requests
typedef enum {
  SIG_CHLD,
  SIG_TERM,
} signal_type;

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
  dist_model distribution;
  external_load hours[24];
} simulated_load;

typedef struct {
  unsigned long id;
  unsigned int user_id;
  unsigned int content_id;
  unsigned int site_id;
  unsigned long start_tick;
  unsigned int self_time_left;
  unsigned int network_time_left;
  bool use_cache;
  bool is_child;
  unsigned int parent_id;
  unsigned int parent_instance_id;
  bool is_started;
  bool dependencies_created;
  std::string node;
  unsigned int instance_id;
  std::unordered_set<std::string> dependencies;
} request;

typedef struct {
  std::string node;
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
  std::string src_node;
  unsigned int src_id;
  unsigned int src_instance_id;
} signal_msg;

// Simulation State (Dyanmic)
extern std::vector<request> requests; // Set of active requests in cluster
extern std::vector<request> new_requests_this_tick; // New requests
extern std::map<std::string, std::map<int, instance>>
    instances; // Instance state ( current requests, etc)
extern std::vector<signal_msg>
    signals;                          // Set of signals to be handled each tick
extern tick_clock global_clock;       // Clock
extern unsigned long last_request_id; // Unique request ids

// Simulation Configuration (Static)
extern std::unordered_set<std::string> drivers;
extern std::map<std::string, distribution> cache;
extern std::map<std::string, distribution> latency;
extern std::map<std::string, distribution> selftime;
extern std::map<std::string, load_model> load_models;
extern std::map<std::string, std::map<std::string, distribution>> dependencies;
extern std::map<std::string, simulated_load> simulation;

void print_dist(distribution d);
void print_node(const char *node);
void print_stats();
