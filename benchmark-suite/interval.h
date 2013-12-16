/*
 * Copyright (c) 2013 Jean Niklas L'orange. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef INTERVAL_H
#define INTERVAL_H

#include <stdint.h>

typedef struct {
  uint32_t from, to;
} Interval;

typedef struct {
  uint32_t len, cap;
  Interval *arr;
} IntervalArray;

IntervalArray* interval_array_create(void);
void interval_array_destroy(IntervalArray *int_arr);
Interval interval_array_nth(IntervalArray *int_arr, uint32_t index);
void interval_array_add(IntervalArray *int_arr, Interval data);
void interval_array_concat(IntervalArray *left, IntervalArray *right);

uint64_t interval_to_uint64_t(Interval interval);
Interval uint64_t_to_interval(uint64_t interval);
#endif