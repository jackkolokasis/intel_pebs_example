#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

// Function to wrap the syscall for perf_event_open
static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
  return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

// Benchmark function to generate cache activity
void benchmark_function() {
  const int size = 1024 * 1024; // 1 MB
  int *array = (int *)malloc(size * sizeof(int));
  if (!array) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < size; ++i) {
    array[i] = i; // Access each element to generate cache activity
  }

  free(array);
}

int main() {
  struct perf_event_attr pe;
  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = PERF_TYPE_HW_CACHE;
  pe.size = sizeof(struct perf_event_attr);
  pe.config = (PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16));
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;
  pe.sample_period = 100; // Set the sample period to 1000 cache misses
  pe.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD;

  // Open the performance event
  int fd = perf_event_open(&pe, 0, -1, -1, 0);
  if (fd == -1) {
    perror("perf_event_open");
    exit(EXIT_FAILURE);
  }

  // Start counting
  ioctl(fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

  // Run the benchmark
  benchmark_function();

  // Stop counting
  ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

  // Read the counter value
  long long cache_misses;
  if (read(fd, &cache_misses, sizeof(long long)) == -1) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  // Close the file descriptor
  close(fd);

  // Print the result
  printf("Cache misses: %lld\n", cache_misses);

  return 0;
}
