# APU-Gym: A framework for on-chip accelerator design space exploration
* APUGym is used for the accelerator and gpcore profiling stage of bottleneck analysis for latency-critical / user-facing services.

* Tree
```
three_phase_composable.cpp
traverse.cpp

|--- src/
|--- inc/
|--- idxd-config
  |--- test/libiaa.a
|--- qatlib
```

### Build:
* build dependencies: `./build.sh`
* build tests: `make -j`

### Quick Start: Report Latency and Throughput for a Three Phase Request
* configure accelerators: `sudo python3 scripts/accel_conf.py --load=configs/idxd-1n2d2e2w-s-n1.conf`
* run experiment: `taskset -c 1 sudo LD_LIBRARY_PATH=/opt/intel/oneapi/ippcp/2021.11/lib  ./three_phase_composable -y -b -t 100 -i 10`
* view results: ***TODO***

### Design:
* There are `Executors` which implement scheduling policies and execute the requests in a closed system for throughput measurement
* A `Test Harness` allocates and frees the resources used by each request and passes them to the Executor
* A `Runner` allocates and frees a statistics structure before and after a series of runs statistics are emitted according to a user-defined statistics processing function


***Below is UNDER CONSTRUCTION***
### Implementing New Tests:
* To implement a test, (1) statistics allocator/free, (2) executor argument allocator/free, (3) request argument allocator/free, and a (4) request function must be defined
* Implemented tests include a end-to-end execution time breakdown and throughput test for deser-decomp-hash and decrypt-memcpy-dotproduct request types in `three_phase_composable.cpp`

Example of implementing Latency/Offered Load of a GPCore/XMP-Accelerated Request:
The goal of this workflow is to get a breakdown of the end-to-end execution of a request executing on a general-purpose core and compare it with the same request with part of the request offloaded to an accelerator.
We will need:
  (1) A time-stamped "{GPCore,Blocking-Offload, YieldingRequest} Request"
  (2) "{GPCore Input Payload, Offload Requestor Argument} Allocate/Free Functions"
* These functions are passed to a "Test Harness" which executes the request in the context of an "Executor"
See examples in src/decompress_and_hash.cpp and mlp.cpp

Example of examining the impact of gpcore sharing between requests:
* We need to define a filler task that executes in the gaps during execution
  * Examples can be found in `inc/filler_*`
  * Including `probe_point.h` makes the filler task aware of the worker-local scheduler context and preemption signal used for yielding back to the main request
  * by invoking `probe_point()` the worker-local scheduler is resumed
  * when the scheduler resumes the yielding filler task that called `probe_point()` it updates the preemption signal
    * This enables filler tasks to cooperatively yield by only calling (1) `init_probe(arg)` to set the scheduler and initialize the global preemption signal and (2) `probe_point()` to check the preemption signal and yield to the scheduler