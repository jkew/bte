#include "bte.h"
////////////////////////////////////////////////////////////////////////////////
// Simple load balancing models

unsigned int get_instance(load_model l, request r) {
  switch (l.balance) {
  case load_balance_model::RANDOM: {
    return get_value_uniform(0, l.instances - 1);
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
