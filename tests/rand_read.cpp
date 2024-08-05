#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/time.h>
#include <pebs.h>

void load_memory_rand(int *array, size_t size) {
  volatile int sum = 0;
  srand(time(NULL));  // Initialize random number generator

  for (size_t i = 0; i < size; i++) {
    int index = rand() % size;  // Access a random index
    sum += array[index];
  }

  if (sum == 0) {
    printf("Sum is zero\n");
  }
}

int main(int argc, char *argv[]) {
  size_t size = 100000000;
  char *start_addr, *end_addr;
  bool use_pebs_events = false;
  int sample_period = 0;
  int opt;
  Pebs *pebs = NULL;

  // Parsing command-line arguments
  while ((opt = getopt(argc, argv, "p:e")) != -1) {
    switch (opt) {
      case 'p':
        sample_period = std::atoi(optarg);
        break;
      case 'e':
        use_pebs_events = true;
        break;
      default:
        printf("Usage: %s -p <sample_period> [-e]\n", argv[0]); 
        exit(EXIT_FAILURE);
    }
  }

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

  if (use_pebs_events)
    pebs = new Pebs(start_addr, end_addr, sample_period, true);
  
  struct timeval start, end;
  gettimeofday(&start, NULL);

  load_memory_rand(array, size);

  gettimeofday(&end, NULL);

  if (use_pebs_events) {
    pebs->stop_pebs();
    pebs->print_addresses();
    delete pebs;
  }

  // Calculate elapsed time
  long seconds = end.tv_sec - start.tv_sec;
  long microseconds = end.tv_usec - start.tv_usec;
  double elapsed_time = seconds + microseconds * 1e-6;

  printf("Elapsed time: %.6f seconds\n", elapsed_time);

  free(array);
  return 0;
}
