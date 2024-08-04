#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include "../include/pebs.h"

void store_memory_rand(int *array, size_t size) {
  srand(time(NULL));  // Initialize random number generator

  for (size_t i = 0; i < size; i++) {
    int index = rand() % size;  // Access a random index
    int val = rand() % size;

    while (array[index] != -1) {
      index = rand() % size;
    }

    array[index] = val;
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

  for (size_t i = 0; i < size; i++)
    array[i] = -1;

  start_addr = (char *) array;
  end_addr = (char *) (array + (size - 1));

  Pebs *pebs = new Pebs(start_addr, end_addr, false);

  store_memory_rand(array, size);

  pebs->stop_pebs();
  delete pebs;

  free(array);
  return 0;
}
