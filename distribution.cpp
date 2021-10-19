#include <distribution.h>
#include <random>

using namespace std;

default_random_engine generator;

/**
 * This function uses the exponential cummulative
 * distribution function to return a value based off
 * of some mean assuming a geometric/exponetial decay
 * it uses a uniform distribution to pick the target
 * probability
 **/
unsigned int get_value_geometric(unsigned int mean) {
  unsigned int prob_int = get_value_uniform(0,999);
  double prob = prob_int/1000;
  double number = -(mean * log(-prob +1));
  if (number < 0) number = 0;
  return (unsigned int) number;
}

unsigned int get_value_geometric(distribution d) {
  return get_value_geometric(d.mean);
}

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
  normal_distribution<double> distribution(mean, stddev);
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
  case GEOMETRIC:
    return get_value_geometric(d);
  default:
    assert(false);
  }
}
