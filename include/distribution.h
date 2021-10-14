#pragma once

typedef enum {
  NORMAL,
  UNIFORM
} dist_model;

typedef struct {
  dist_model dist;
  unsigned int mean;
  unsigned int stddev;
  unsigned int min;
  unsigned int max;
} distribution;

unsigned int get_value_uniform(unsigned int min, unsigned int max);
unsigned int get_value_uniform(distribution d);
unsigned int get_value_normal(unsigned int mean, unsigned int stddev);
unsigned int get_value_normal(distribution d);
unsigned int get_value(distribution d);

