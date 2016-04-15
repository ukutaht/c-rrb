/*
 * Copyright (c) 2013-2014 Jean Niklas L'orange. All rights reserved.
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

#ifndef RRB_H
#define RRB_H

#include <stdint.h>

#define RRB_BITS 5
#define RRB_MAX_HEIGHT 6

#define RRB_BRANCHING (1 << RRB_BITS)
#define RRB_MASK (RRB_BRANCHING - 1)

#define RRB_INVARIANT 1
#define RRB_EXTRAS 2

typedef struct RRB_ RRB;

const RRB* rrb_create(void);

uint32_t rrb_count(const RRB *rrb);
void* rrb_nth(const RRB *rrb, uint32_t index);
const RRB* rrb_pop(const RRB *rrb);
void* rrb_peek(const RRB *rrb);
const RRB* rrb_push(const RRB *restrict rrb, const void *restrict elt);
const RRB* rrb_update(const RRB *restrict rrb, uint32_t index, const void *restrict elt);

const RRB* rrb_concat(const RRB *left, const RRB *right);
const RRB* rrb_slice(const RRB *rrb, uint32_t from, uint32_t to);

// Transients

typedef struct TransientRRB_ TransientRRB;

TransientRRB* rrb_to_transient(const RRB *rrb);
const RRB* transient_to_rrb(TransientRRB *trrb);

uint32_t transient_rrb_count(const TransientRRB *trrb);
void* transient_rrb_nth(const TransientRRB *trrb, uint32_t index);
TransientRRB* transient_rrb_pop(TransientRRB *trrb);
void* transient_rrb_peek(const TransientRRB *trrb);
TransientRRB* transient_rrb_push(TransientRRB *restrict trrb, const void *restrict elt);
TransientRRB* transient_rrb_update(TransientRRB *restrict trrb, uint32_t index, const void *restrict elt);
TransientRRB* transient_rrb_slice(TransientRRB *trrb, uint32_t from, uint32_t to);

#define RRB_DEBUG @RRB_DEBUG@
#ifdef RRB_DEBUG

#include <stdio.h>

typedef struct DotFile_ DotFile;

int rrb_to_dot_file(const RRB *rrb, char *loch);

DotFile dot_file_create(char *loch);
DotFile dot_file_create_safely(char *loch, int* fprint_err);
int dot_file_close(DotFile dot);

int label_pointer(DotFile dot, const void *node, const char *name);
int rrb_to_dot(DotFile dot, const RRB *rrb);

uint32_t rrb_memory_usage(const RRB *const *rrbs, uint32_t rrb_count);

// For internal debugging purposes
void nodes_to_dot_file(char *loch, int ncount, ...);
uint32_t validate_rrb(const RRB *rrb);
#endif
#endif
