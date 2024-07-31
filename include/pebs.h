#ifndef PEBS_H
#define PEBS_H

#include <signal.h>

#define SAMPLE_PERIOD 300000
#define BUFFER_PAGES (1 + (1 << 14))

void setup_pebs(void);
void start_pebs(void);
void stop_pebs(void);
void analyze_pebs_records(void);
void print_hot_pages(void);

#endif // < PEBS_H
