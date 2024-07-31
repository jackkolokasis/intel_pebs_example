#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <string.h>
#include <stdint.h>

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
  return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

void load_memory(int *array, size_t size) {
  volatile int sum = 0;
  for (size_t i = 0; i < size; i++) {
    sum += array[i];
  }
  if (sum == 0) {
    printf("Sum is zero\n");
  }
}

int main() {
  struct perf_event_attr pe;
  long long count;
  int fd;

  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = PERF_TYPE_RAW;
  pe.size = sizeof(struct perf_event_attr);
  pe.config = 0x81d0; // Correct event code for MEM_UOPS_RETIRED.ALL_LOADS
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;

  fd = perf_event_open(&pe, 0, -1, -1, 0);
  if (fd == -1) {
    perror("perf_event_open");
    exit(EXIT_FAILURE);
  }

  size_t size = 1000000;
  int *array = malloc(size * sizeof(int));
  for (size_t i = 0; i < size; i++) {
    array[i] = i;
  }

  ioctl(fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

  load_memory(array, size);

  ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
  read(fd, &count, sizeof(long long));

  printf("Number of load operations: %lld\n", count);

  close(fd);
  free(array);

  return 0;
}
