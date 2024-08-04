#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <sys/time.h>
#include "../include/pebs.h"

void load_memory_seq(int *array, size_t size) {
  volatile int sum = 0;

  for (size_t i = 0; i < size; i++) {
    sum += array[i];
  }

  if (sum == 0) {
    printf("Sum is zero\n");
  }
}

int main() {
  size_t size = 1000000000;
  char *start_addr, *end_addr;

  int *array = (int *) malloc(size * sizeof(int));
  start_addr = (char *) array;
  end_addr = (char *) (array + (size - 1));

  if (array == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  for (size_t i = 0; i < size; i++) {
    array[i] = i;
  }

  Pebs *pebs = new Pebs(start_addr, end_addr, true);
  
  struct timeval start, end;
  gettimeofday(&start, NULL);

  load_memory_seq(array, size);
  
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
