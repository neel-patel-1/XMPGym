#ifndef GPCORE_ARGS_H
#define GPCORE_ARGS_H


typedef struct _gpcore_request_args{
  char **inputs;
  int id;
} gpcore_request_args;
typedef struct _timed_gpcore_request_args{
  char **inputs;
  int id;
  uint64_t *ts0;
  uint64_t *ts1;
  uint64_t *ts2;
} timed_gpcore_request_args;

#endif