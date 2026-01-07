#ifndef JOS_INC_RYU_RYU_H
#define JOS_INC_RYU_RYU_H

#include <inc/types.h>

int d2fixed_buffered_n(double d, uint32_t precision, char* result);
int d2exp_buffered_n(double d, uint32_t precision, char* result);

#endif
