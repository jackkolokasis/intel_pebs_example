#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <linux/perf_event.h>
#include <sched.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>

#define SAMPLE_PERIOD 10
#define PERF_PAGES (1 + (1 << 14))	// Has to be == 1+2^n

int fd;
struct perf_event_attr pe;
struct perf_event_mmap_page *pebs_buffer;
size_t mmap_size;
pthread_t scan_thread;
volatile int stop_thread = 0;
char *array_start_addr = NULL;
char *array_end_addr = NULL;

struct perf_sample {
  struct perf_event_header header;
  __u64	ip;
  __u32 pid, tid;    /* PERF_SAMPLE_TID */
  __u64 addr;        /* PERF_SAMPLE_ADDR */
  __u64 weight;      /* PERF_SAMPLE_WEIGHT */
  __u64 data_src;    /* PERF_SAMPLE_DATA_SRC */
};

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
  return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

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

void load_memory_seq(int *array, size_t size) {
    volatile int sum = 0;

    for (size_t i = 0; i < size; i++) {
        sum += array[i];
    }

    if (sum == 0) {
        printf("Sum is zero\n");
    }
}

void *pebs_scan_thread() {
  cpu_set_t cpu_set;
  pthread_t thread;
  int ret;

  thread = pthread_self();
  CPU_ZERO(&cpu_set);
  CPU_SET(0, &cpu_set);

  ret = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpu_set);
  if (ret != 0) {
    perror("pthread_setaffinity_np");
    assert(0);
  }

  struct perf_event_mmap_page *metadata_page = pebs_buffer;
  uint64_t data_head;
  uint64_t data_tail = metadata_page->data_tail;
  char *pbuf = (char *)metadata_page + metadata_page->data_offset;

  while (!stop_thread) {
    __sync_synchronize();
    data_head = metadata_page->data_head; 

    while (data_tail != data_head) {
      struct perf_event_header *ph = (void *) (pbuf + (data_tail % metadata_page->data_size));
      struct perf_sample* ps;
      if (ph->type == PERF_RECORD_SAMPLE) {
        ps = (struct perf_sample *) ph;
        assert(ps != NULL);

        if (ps->addr != 0 && (char *) ps->addr >= array_start_addr && (char *) ps->addr < array_end_addr) {
          printf("IP: %llx | Addr: %p | Hierarchy = 0x%llx | TLB Access = 0x%llx\n",
                 ps->ip, (void *) ps->addr, ps->data_src & 0xf,
                 (ps->data_src >> 12) & 0xF);
        }
      }
      data_tail += ph->size;
      metadata_page->data_tail = data_tail;
    }
  }
  return NULL;
}

void setup_pebs() {
  int ret;
  
  printf("Setups pebs: start\n");

  memset(&pe, 0, sizeof(struct perf_event_attr));

  pe.type = PERF_TYPE_RAW;
  pe.size = sizeof(struct perf_event_attr);
  pe.config = 0x81d0;
  pe.disabled = 0;
  //  If this bit is set, the count excludes events that happen in kernel space.
  pe.exclude_kernel = 1;
  // If this bit is set, the count excludes events that happen in the hypervisor.
  pe.exclude_hv = 1;
  pe.exclude_callchain_kernel = 1;
  pe.exclude_callchain_user = 1;
  pe.precise_ip = 1;
  pe.sample_period = SAMPLE_PERIOD;
  pe.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_WEIGHT \
    | PERF_SAMPLE_ADDR | PERF_SAMPLE_DATA_SRC;

  fd = perf_event_open(&pe, 0, -1, -1, 0);
  if (fd == -1) {
    perror("perf_event_open");
    exit(EXIT_FAILURE);
  }
  
  mmap_size = sysconf(_SC_PAGESIZE) * PERF_PAGES;
  pebs_buffer = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if(pebs_buffer == MAP_FAILED) {
    perror("mmap");
    close(fd);
    exit(EXIT_FAILURE);
  }

  ret = pthread_create(&scan_thread, NULL, pebs_scan_thread, NULL);
  if (ret != 0) {
    perror("pthread_create");
    close(fd);
    munmap(pebs_buffer, mmap_size);
    exit(EXIT_FAILURE);
  }

  printf("Setups pebs: finished\n");
}

void start_pebs(void) {
  ioctl(fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
}

void stop_pebs(void) {
  ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
  stop_thread = 1;
  pthread_join(scan_thread, NULL);
}

void handle_signal(int signal) {
    stop_pebs();
    close(fd);
    exit(EXIT_SUCCESS);
}

int main() {
  signal(SIGINT, handle_signal);


  size_t size = 10000000;

  int *array = malloc(size * sizeof(int));
  array_start_addr = (char *) array;
  array_end_addr = (char *) (array + (size - 1));

  if (array == NULL) {
    perror("malloc");
    stop_pebs();
    close(fd);
    exit(EXIT_FAILURE);
  }

  for (size_t i = 0; i < size; i++) {
    array[i] = i;
  }

  setup_pebs();

  load_memory_seq(array, size);
  load_memory_rand(array, size);

  stop_pebs();
  close(fd);
  free(array);

  return 0;
}
