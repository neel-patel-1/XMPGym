#ifndef ROUTER_REQUEST_ARGS_H
#define ROUTER_REQUEST_ARGS_H
#include "offload_args.h"
#include "proto_files/router.pb.h"
#include <string>
#include "emul_ax.h"
#include "offload_args.h"
#include "gpcore_args.h"



typedef struct _cpu_request_args{
  router::RouterRequest *request;
  std::string *serialized;
} cpu_request_args;
typedef struct _timed_cpu_request_args{
  router::RouterRequest *request;
  std::string *serialized;
  uint64_t *ts0;
  uint64_t *ts1;
  uint64_t *ts2;
  int id;
} timed_cpu_request_args;



#endif