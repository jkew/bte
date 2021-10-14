#include <distribution.h>
#include <random>

using namespace std;

default_random_engine generator;

unsigned int get_value_uniform(unsigned int min, unsigned int max) {
  uniform_int_distribution<int> distribution(min, max);
  double number = distribution(generator);
  if (number < 0)
    number = 0;
  return (unsigned int)number;
}

unsigned int get_value_uniform(distribution d) {
  return get_value_uniform(d.min, d.max);
}

unsigned int get_value_normal(unsigned int mean, unsigned int stddev) {
  normal_distribution<int> distribution(mean, stddev);
  double number = distribution(generator);
  if (number < 0)
    number = 0;
  return (unsigned int)number;
}

unsigned int get_value_normal(distribution d) {
  return get_value_normal(d.mean, d.stddev);
}

unsigned int get_value(distribution d) {
  switch (d.dist) {
  case NORMAL:
    return get_value_normal(d);
  case UNIFORM:
    return get_value_uniform(d);
  default:
    assert(false);
  }
}
