#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>

#include "../include/pebs.h"

int main() {
  setup_pebs();
  start_pebs();

  // Simulated workload to generate memory accesses
  size_t array_size = 100;
  int *array = (int *)malloc(array_size * sizeof(int));
  for (size_t i = 0; i < array_size; i++) {
    array[i] = i;
  }

  printf("Array address = %p\n", (void *) array);

  while(1) {

    for (int i = 0; i < 10; i++) {
      analyze_pebs_records();
      for (int j = 0; j < 10; j++) {
        array[rand() % array_size] += 1;
        array[rand() % array_size] += 100;
        array[rand() % array_size] += 500;
        array[rand() % array_size] += 300;
        array[rand() % array_size] += 30;
        array[rand() % array_size] += 11;
      }
    }
  }
    

  //print_hot_pages();

  free(array);
  stop_pebs();
  return 0;
}
