#ifndef OFFLOAD_ARGS_H
#define OFFLOAD_ARGS_H

#include "emul_ax.h"

typedef struct _offload_request_args{
  ax_comp *comp;
  struct hw_desc *desc;
  int id;

  char *src_payload;
  uint64_t src_size;
  uint64_t dst_size;
  char *dst_payload;
  char *aux_payload; /* users - decomp_and_scatter, memfill_and_gather */
  int aux_size; /*users three_phase.cpp */

  void *pre_proc_input;
  void *pre_proc_output;
  int pre_proc_input_size;
  void *ax_func_output;
  int max_axfunc_output_size;
  void *post_proc_output;
  int post_proc_input_size;
  int max_post_proc_output_size;
} offload_request_args;

typedef struct _timed_offload_request_args{
  ax_comp *comp;
  struct hw_desc *desc;
  int id;

  char *src_payload;
  uint64_t src_size;
  uint64_t dst_size;
  char *dst_payload;
  char *aux_payload; /* users - decomp_and_scatter, memfill_and_gather */
  int aux_size; /*users three_phase.cpp */

  void *pre_proc_input;
  void *pre_proc_output;
  int pre_proc_input_size;
  void *ax_func_output;
  int max_axfunc_output_size;
  void *post_proc_output;
  int post_proc_input_size;
  int max_post_proc_output_size;

  uint64_t *ts0;
  uint64_t *ts1;
  uint64_t *ts2;
  uint64_t *ts3;
  uint64_t *ts4;
} timed_offload_request_args;
#endif