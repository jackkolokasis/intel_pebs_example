#include "../include/pebs.h"

#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>
#include <assert.h>

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
  return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

void* Pebs::pebs_scan_thread(void* arg) {
  PebsArgs *args = (PebsArgs *)arg;
  struct perf_event_mmap_page *pebs_buffer = args->pebs_buffer;
  volatile int *stop_thread = args->stop_thread;
  char *start_addr = args->start_addr;
  char *end_addr = args->end_addr;

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

  while (!(*stop_thread)) {
    __sync_synchronize();
    data_head = metadata_page->data_head; 

    while (data_tail != data_head) {
      struct perf_event_header *ph = (struct perf_event_header *) (pbuf + (data_tail % metadata_page->data_size));
      PerfSample* ps;
      if (ph->type == PERF_RECORD_SAMPLE) {
        ps = (PerfSample *) ph;
        assert(ps != NULL);

        if (ps->addr != 0 && (char *) ps->addr >= start_addr && (char *) ps->addr < end_addr) {
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


Pebs::Pebs(char *array_start_addr, char *array_end_addr, bool load_ops) {
  int ret;
  
  printf("Setups pebs: start\n");

  memset(&pe, 0, sizeof(struct perf_event_attr));

  pe.type = PERF_TYPE_RAW;
  pe.size = sizeof(struct perf_event_attr);
  pe.config = load_ops ? 0x81d0 : 0x82d0;
  pe.disabled = 0;
  pe.exclude_kernel = 1;
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
  pebs_buffer = (struct perf_event_mmap_page *) mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if(pebs_buffer == MAP_FAILED) {
    perror("mmap");
    close(fd);
    exit(EXIT_FAILURE);
  }

  start_addr = array_start_addr;
  end_addr = array_end_addr;

  args = new PebsArgs{pebs_buffer, &stop_thread, start_addr, end_addr};

  ret = pthread_create(&scan_thread, NULL, pebs_scan_thread, args);
  if (ret != 0) {
    perror("pthread_create");
    close(fd);
    munmap(pebs_buffer, mmap_size);
    exit(EXIT_FAILURE);
  }

  printf("Setups pebs: finished\n");
}

void Pebs::stop_pebs(void) {
  ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
  stop_thread = 1;
  pthread_join(scan_thread, NULL);
}

void Pebs::start_pebs(void) {
  ioctl(fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
}

Pebs::~Pebs() {
  delete args;
  close(fd);
  munmap(pebs_buffer, mmap_size);
}
