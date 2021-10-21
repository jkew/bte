#pragma once

typedef enum { NORMAL, UNIFORM, GEOMETRIC, LOGNORMAL } dist_model;

typedef struct {
  dist_model dist;
  unsigned int mean;
  unsigned int stddev;
  unsigned int min;
  unsigned int max;
  double prob;
  double factor;
} distribution;

unsigned int get_value_geometric(unsigned int mean);
unsigned int get_value_geometric(distribution d);
unsigned int get_value_uniform(unsigned int min, unsigned int max);
unsigned int get_value_uniform(distribution d);
unsigned int get_value_lognormal(unsigned int mean, unsigned int stddev);
unsigned int get_value_lognormal(distribution d);
unsigned int get_value_normal(unsigned int mean, unsigned int stddev);
unsigned int get_value_normal(distribution d);
unsigned int get_value(distribution d);
