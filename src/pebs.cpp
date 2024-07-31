#include "../include/pebs.h"

#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>

struct perf_event_attr pe;
struct perf_event_mmap_page *header;
int fd;
struct sigaction sa;

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid, 
    int cpu, int group_fd, unsigned long flags) {
  return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

void setup_pebs(void) {
  memset(&pe, 0, sizeof(struct perf_event_attr));

  pe.type = PERF_TYPE_RAW;
  pe.size = sizeof(struct perf_event_attr);
  
  pe.config = 0x81d0;  // MEM_INST_RETIRED:ALL_LOADS
  pe.sample_period = SAMPLE_PERIOD;
  pe.sample_type = PERF_SAMPLE_ADDR | PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_WEIGHT;

  pe.disabled = 0;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;
  pe.precise_ip = 1; // no skid
  pe.mmap_data = 1;
  pe.exclude_callchain_user = 1;
  pe.exclude_callchain_kernel = 1;
  pe.wakeup_events = 1;


  fd = perf_event_open(&pe, -1, 0, -1, 0);
  
  if (fd == -1) {
    perror("perf_event_open");
    exit(EXIT_FAILURE);
  }

  size_t mmap_size = sysconf(_SC_PAGESIZE) * BUFFER_PAGES;
  header = (struct perf_event_mmap_page *)mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if (header == MAP_FAILED) {
    perror("mmap");
    close(fd);
    exit(EXIT_FAILURE);
  }
}

void start_pebs(void) {
  ioctl(fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
}

void stop_pebs(void) {
  ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
  munmap(header, sysconf(_SC_PAGESIZE) * BUFFER_PAGES);
  close(fd);
}

void analyze_pebs_records(void) {
  uint64_t data_head = header->data_head;
  asm volatile("" ::: "memory");  // Memory barrier to synchronize reads

  while (data_head != header->data_tail) {
    struct perf_event_header *ehdr = (struct perf_event_header *)((char *)header + sysconf(_SC_PAGESIZE) + data_head % (BUFFER_PAGES * sysconf(_SC_PAGESIZE)));
    data_head += ehdr->size;

    if (ehdr->type == PERF_RECORD_SAMPLE) {
      uint64_t *array = (uint64_t *)(ehdr + 1);
      uint64_t addr = array[1];  // Memory address
      printf("Address = %p\n", (char *) addr);
    }
  }

  asm volatile("" ::: "memory");
  header->data_tail = data_head;
}

void print_hot_pages(void) {
  return;
}

//void analyze_pebs_records() {
//  uint64_t data_head = header->data_head;
//  asm volatile("" ::: "memory");
//
//  printf("%lu\n", data_head);
//
//  while (data_head != header->data_tail) {
//    struct perf_event_header *ehdr = (struct perf_event_header *)((char *)header + PAGE_SIZE + data_head % (BUFFER_PAGES * PAGE_SIZE));
//    data_head += ehdr->size;
//
//    if (ehdr->type == PERF_RECORD_SAMPLE) {
//      uint64_t *array = (uint64_t *)(ehdr + 1);
//      uint64_t addr = array[1];  // Memory address
//      uint64_t page = addr & ~(PAGE_SIZE - 1);  // Get page address
//      page_access_count[page]++;
//    }
//  }
//
//  asm volatile("" ::: "memory");
//  header->data_tail = data_head;
//}
//
//void print_hot_pages() {
//std::vector<std::pair<uint64_t, uint64_t>> sorted_pages(page_access_count.begin(), page_access_count.end());
//std::sort(sorted_pages.begin(), sorted_pages.end(), [](const auto &a, const auto &b) {
//    return a.second > b.second;
//  });
//
//  printf("Top hot pages:\n");
//  for (size_t i = 0; i < 10 && i < sorted_pages.size(); i++) {
//    printf("Page: 0x%llx, Accesses: %llu\n",
//           (unsigned long long)sorted_pages[i].first,
//           (unsigned long long)sorted_pages[i].second);
//  }
//}

