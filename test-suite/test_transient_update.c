/*
 * Copyright (c) 2014 Jean Niklas L'orange. All rights reserved.
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

#include <gc/gc.h>
#include <stdio.h>
#include <stdlib.h>
#include "rrb.h"
#include "test.h"

#define SIZE    400000
#define UPDATES 133337

int main() {
  GC_INIT();
  randomize_rand();

  int fail = 0;
  intptr_t *list = GC_MALLOC_ATOMIC(sizeof(intptr_t) * SIZE);
  for (uint32_t i = 0; i < SIZE; i++) {
    list[i] = (intptr_t) rand();
  }
  TransientRRB *trrb = rrb_to_transient(rrb_create());
  for (uint32_t i = 0; i < SIZE; i++) {
    trrb = transient_rrb_push(trrb, (void *) list[i]);
    // fail |= CHECK_TREE(rrb); // Too costly
  }
  const RRB* rrb = transient_to_rrb(trrb);

  intptr_t *updated_list = GC_MALLOC_ATOMIC(sizeof(intptr_t) * UPDATES);
  uint32_t *lookups = GC_MALLOC_ATOMIC(sizeof(uint32_t) * UPDATES);
  for (uint32_t i = 0; i < UPDATES; i++) {
    updated_list[i] = (intptr_t) rand();
    lookups[i] = (uint32_t) (rand() % SIZE);
  }

  trrb = rrb_to_transient(rrb);
  for (uint32_t i = 0; i < UPDATES; i++) {
    const uint32_t idx = lookups[i];

    trrb = transient_rrb_update(trrb, idx, (void *) updated_list[i]);
    // fail |= CHECK_TREE(rrb); // Too costly

    intptr_t old_val = (intptr_t) rrb_nth(rrb, idx);
    intptr_t new_val = (intptr_t) transient_rrb_nth(trrb, idx);
    if (old_val != list[idx]) {
      printf("Expected old val at pos %d to be %ld, was %ld.\n", idx,
             list[idx], old_val);
      fail = 1;
    }
    if (new_val != updated_list[i]) {
      printf("Expected new val at pos %d to be %ld, was %ld.\n", idx,
             updated_list[i], new_val);
      fail = 1;
    }
  }
  return fail;
}
