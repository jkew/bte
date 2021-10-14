#include "bte.h"
////////////////////////////////////////////////////////////////////////////////
// Set of functions which determine if a job is ready to start on a given instance.
// These functions are configured with the "growthmodel" and limit sections of the
// configuration.

using namespace std;

////////////////////////////////////////////////////////////////////////////////
// Logistic (sigmoid) distribution with a slow start at low loads until
// maximum capacity is reached
//
//  adjusted_limit=(1/(1+e^(-1(x-[mid_point]))))*[limit]
bool try_start_logistic(unsigned int limit, unsigned int current_load) {
  unsigned int mid_point = (int)(limit / 2);
  unsigned int adj_limit =
      (unsigned int)(1.0 / (1.0 + exp(-1.0 * (current_load - mid_point)))) *
      limit;
  if (adj_limit == 0)
    adj_limit = 1;
  return (current_load <= adj_limit);
}

////////////////////////////////////////////////////////////////////////////////
// Simple linear model. If there is capacity on an instance, start it.
bool try_start_linear(unsigned int limit, unsigned int current_load) {
  return (current_load <= limit);
}

bool try_start(string node, unsigned int instance_id) {
  growth_model model = load_models[node].model;
  unsigned int limit = load_models[node].limit;
  unsigned int current_load =
      instances[node][instance_id].current_request_count;
  switch (model) {
  case LINEAR:
    return try_start_linear(limit, current_load);
  case LOGISTIC:
    return try_start_logistic(limit, current_load);
  }
  return true;
}
