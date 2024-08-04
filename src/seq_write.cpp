#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <sys/time.h>
#include "../include/pebs.h"

void store_memory_seq(int *array, size_t size) {
  for (size_t i = 0; i < size; i++) {
    array[i] = i;
  }
}

int main() {
  size_t size = 10000000;
  char *start_addr, *end_addr;

  int *array = (int *) malloc(size * sizeof(int));
  if (array == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  start_addr = (char *) array;
  end_addr = (char *) (array + (size - 1));

  Pebs *pebs = new Pebs(start_addr, end_addr, false);
  
  struct timeval start, end;
  gettimeofday(&start, NULL);

  store_memory_seq(array, size);

  gettimeofday(&end, NULL);

  pebs->stop_pebs();
  delete pebs;
  
  // Calculate elapsed time
  long seconds = end.tv_sec - start.tv_sec;
  long microseconds = end.tv_usec - start.tv_usec;
  double elapsed_time = seconds + microseconds * 1e-6;

  printf("Elapsed time: %.6f seconds\n", elapsed_time);

  free(array);
  return 0;
}
