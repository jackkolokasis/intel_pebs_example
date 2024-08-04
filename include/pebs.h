#ifndef PEBS_H
#define PEBS_H

#include <linux/perf_event.h>
#include <stdlib.h>
#include <pthread.h>
#include <vector>
#include <cstdint>
#include <tuple>

#define PERF_PAGES (1 + (1 << 14))	// Has to be == 1+2^n

struct PerfSample {
  struct perf_event_header header;
  __u64	ip;
  __u64 addr;        /* PERF_SAMPLE_ADDR */
  __u64 data_src;    /* PERF_SAMPLE_DATA_SRC */
};

struct PebsArgs {
  struct perf_event_mmap_page *pebs_buffer;
  volatile int *stop_thread;
  char *start_addr;
  char *end_addr;
  std::vector<std::tuple<__u64, __u64, __u64, __u64>> *results;
};
  
class Pebs {
private:
  struct perf_event_attr pe;                //< Perf event
  struct perf_event_mmap_page *pebs_buffer; //< Pebs buffer
  int fd;                                   //< File descriptor for perf event
  size_t mmap_size;                         //< Size for the pebs buffer
  pthread_t scan_thread;                    //< Scanner thread
  volatile int stop_thread = 0;             //< Flag to stop scanner thread
  char *start_addr;                         //< Start address of the tracking memory area
  char *end_addr;                           //< End address of the tracking memory area
  PebsArgs *args;                           //< Pebs scanner thread arguments
  std::vector<std::tuple<__u64, __u64, __u64, __u64>> *results;

  // The scanner thread iterates the pebs scanner finding new PEBS
  // records and check their addresses.
  static void* pebs_scan_thread(void* arg);

public:
  Pebs(char *array_start_addr, char *array_end_addr, int sample_period, bool load_ops);
  ~Pebs();

  void start_pebs(void);
  void stop_pebs(void);
  void print_addresses(void);
};

#endif // < PEBS_H
