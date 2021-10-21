#include "bte.h"
////////////////////////////////////////////////////////////////////////////////
// Simple load balancing models

unsigned int get_instance(load_model l, request r) {
  unsigned int max_instances = l.instances;
  
  unsigned int curr_instances = max_instances;
  if (l.scheduled_scaling) {
    unsigned int hour = global_clock.hour - 1;
    assert(hour >= 0 && hour < 24);
    curr_instances = l.scheduled_instances[hour];
    assert(curr_instances <= max_instances);
  }
  switch (l.balance) {
  case load_balance_model::RANDOM: {
    return get_value_uniform(0,  curr_instances - 1);
  }
  case load_balance_model::REQUEST:
    return r.id % curr_instances;
  case load_balance_model::USERS:
    return r.user_id % curr_instances;
  case load_balance_model::CONTENT:
    return r.content_id % curr_instances;
  case load_balance_model::SITES:
    return r.site_id % curr_instances;
  }
}
