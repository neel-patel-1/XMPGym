#include "test_harness.h"
#include "print_utils.h"
#include "router_request_args.h"
#include "iaa_offloads.h"
#include "runners.h"
#include "gpcore_compress.h"

extern "C" {
  #include "fcontext.h"
  #include "iaa.h"
  #include "accel_test.h"
  #include "iaa_compress.h"
  #include <zlib.h>
}
#include "decompress_and_hash_request.hpp"
#include "dsa_offloads.h"
#include "submit.hpp"

#include "router.pb.h"

static inline void pre_proc_fn(void *input, void *output, int size){
  LOG_PRINT(LOG_DEBUG, "PreProc\n");
  char *in = (char *)input;
  char *out = (char *)output;
  for(int i = 0; i < size; i++){
    out[i] = in[i];
  }
}

void ser_buf(int payload_size, void **p_msgbuf, uint64_t *outsize){
  router::RouterRequest req;
  const char * pattern = "01234567";
  std::string pattern_str((char *)&pattern, sizeof(pattern));
  std::string val_string(pattern);
  uint8_t *msgbuf;
  uint64_t msgsize;
  bool rc = false;

  while(val_string.size() < payload_size){
    val_string.append(pattern);
  }

  LOG_PRINT(LOG_DEBUG, "ValString: %s Size: %ld\n", val_string.c_str(), val_string.size());

  req.set_key("/region/cluster/foo:key|#|etc"); // key is 32B string, value gets bigger up to 2MB

  req.set_value(val_string);

  req.set_operation(0);
  msgsize = req.ByteSizeLong();
  msgbuf = (uint8_t *)malloc(msgsize);

  rc = req.SerializeToArray((void *)msgbuf, msgsize);
  if(rc == false){
    LOG_PRINT(LOG_DEBUG, "Failed to serialize\n");
  }

  *p_msgbuf = (void *)msgbuf;
  *outsize = msgsize;
}

void deser_from_buf(void *ser_inp, void *output, int input_size, int *output_size){
  router::RouterRequest req;
  req.ParseFromArray(ser_inp, input_size);
  LOG_PRINT(LOG_DEBUG, "Deserialized Payload Size: %ld\n", req.value().size());

  memcpy(output, req.value().c_str(), req.value().size());
  *output_size = req.value().size();
}


static inline void post_proc_fn(void *input, void *output, int size){
  LOG_PRINT(LOG_DEBUG, "PostProc\n");
  char *in = (char *)input;
  char *out = (char *)output;
  for(int i = 0; i < size; i++){
    out[i] = in[i];
  }
}

template <typename pre_proc_fn, typename prepare_desc_fn, typename submit_fn, typename post_proc_fn>
static inline void generic_three_phase_timed(
  pre_proc_fn pre_proc_func,
  prepare_desc_fn prepare_desc_func,
  submit_fn submit_offload_func,
  post_proc_fn post_proc_func,
  void *pre_proc_input, void *pre_proc_output,
  int pre_proc_input_size, int max_pre_proc_output_size,
  void *ax_func_input, void *ax_func_output,
  int ax_func_input_size, int max_ax_func_output_size,
  void *post_proc_input, void *post_proc_output,
  int post_proc_input_size, int max_post_proc_output_size,
  uint64_t *ts0, uint64_t *ts1, uint64_t *ts2, uint64_t *ts3, int idx
  )
{

  return;
}

int gLogLevel = LOG_DEBUG;
bool gDebugParam = false;
int main(int argc, char **argv){


  int wq_id = 0;
  int dev_id = 0;
  int wq_type = SHARED;
  int rc;
  int itr = 100;
  int total_requests = 1000;
  int opt;
  bool no_latency = false;
  bool no_thrpt = false;
  bool run_serialized = false;
  bool run_linear = false;

  int payload_size = 1024, deserd_size;
  void *serd_buf, *deserd_buf;
  uint64_t ser_size;
  deserd_buf = malloc(payload_size);
  ser_buf(payload_size, &serd_buf, &ser_size);
  deser_from_buf(serd_buf, deserd_buf, (int)ser_size, &deserd_size);



  while((opt = getopt(argc, argv, "y:s:j:t:i:r:s:q:d:hf")) != -1){
    switch(opt){
      case 't':
        total_requests = atoi(optarg);
        break;
      case 'i':
        itr = atoi(optarg);
        break;
      case 'd':
        dev_id = atoi(optarg);
        break;
      case 'f':
        gLogLevel = LOG_DEBUG;
        break;
      default:
        break;
    }
  }

  initialize_dsa_wq(dev_id, wq_id, wq_type);





  free_dsa_wq();
  return 0;

}