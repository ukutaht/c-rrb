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

#include "rrb_thread.h"

struct TransientRRB_ {
  uint32_t cnt;
  uint32_t shift;
  uint32_t tail_len;
  LeafNode *tail;
  TreeNode *root;
  RRBThread owner;
  GUID_DECLARATION
};


static const void* rrb_guid_create(void);
static TransientRRB* transient_rrb_head_create(const RRB* rrb);
static void check_transience(const TransientRRB *trrb);

static RRBSizeTable* transient_size_table_create(void);
static InternalNode* transient_internal_node_create(void);
static LeafNode* transient_leaf_node_create(void);
static RRBSizeTable* transient_size_table_clone(const RRBSizeTable *table,
                                                uint32_t len, const void *guid);
static InternalNode* transient_internal_node_clone(const InternalNode *internal,
                                                   const void *guid);
static LeafNode* transient_leaf_node_clone(const LeafNode *leaf, const void *guid);

static RRBSizeTable* ensure_size_table_editable(const RRBSizeTable *table,
                                                uint32_t len, const void *guid);
static InternalNode* ensure_internal_editable(InternalNode *internal, const void *guid);
static LeafNode* ensure_leaf_editable(LeafNode *leaf, const void *guid);

static void transient_promote_rightmost_leaf(TransientRRB* trrb);

static const void* rrb_guid_create() {
  return (const void *) RRB_MALLOC_ATOMIC(1);
}

static TransientRRB* transient_rrb_head_create(const RRB* rrb) {
  TransientRRB *trrb = RRB_MALLOC(sizeof(TransientRRB));
  memcpy(trrb, rrb, sizeof(RRB));
  trrb->owner = RRB_THREAD_ID();
  return trrb;
}

static void check_transience(const TransientRRB *trrb) {
  if (trrb->guid == NULL) {
    // Transient used after transient_to_persistent call
    exit(1);
  }
  if (!RRB_THREAD_EQUALS(trrb->owner, RRB_THREAD_ID())) {
    // Transient used by non-owner thread
    exit(1);
  }
}

static InternalNode* transient_internal_node_create() {
  InternalNode *node = RRB_MALLOC(sizeof(InternalNode)
                              + RRB_BRANCHING * sizeof(InternalNode *));
  node->type = INTERNAL_NODE;
  node->size_table = NULL;
  return node;
}

static RRBSizeTable* transient_size_table_create() {
  // this atomic allocation is, strictly speaking, NOT ok. Small chance of guid
  // being reallocated at same position if lost. Note when porting over to
  // different GC/Precise mode.
  RRBSizeTable *table = RRB_MALLOC_ATOMIC(sizeof(RRBSizeTable)
                                          + RRB_BRANCHING * sizeof(void *));
  return table;
}

static LeafNode* transient_leaf_node_create() {
  LeafNode *node = RRB_MALLOC(sizeof(LeafNode)
                              + RRB_BRANCHING * sizeof(void *));
  node->type = LEAF_NODE;
  return node;
}

static RRBSizeTable* transient_size_table_clone(const RRBSizeTable *table,
                                                uint32_t len, const void *guid) {
  RRBSizeTable *copy = transient_size_table_create();
  memcpy(copy, table, sizeof(RRBSizeTable) + len * sizeof(uint32_t));
  copy->guid = guid;
  return copy;
}


static InternalNode* transient_internal_node_clone(const InternalNode *internal,
                                                   const void *guid) {
  InternalNode *copy = transient_internal_node_create();
  memcpy(copy, internal,
         sizeof(InternalNode) + internal->len * sizeof(InternalNode *));
  copy->guid = guid;
  return copy;
}

static LeafNode* transient_leaf_node_clone(const LeafNode *leaf, const void *guid) {
  LeafNode *copy = transient_leaf_node_create();
  memcpy(copy, leaf, sizeof(LeafNode) + leaf->len * sizeof(void *));
  copy->guid = guid;
  return copy;
}

static RRBSizeTable* ensure_size_table_editable(const RRBSizeTable *table,
                                                uint32_t len, const void *guid) {
  if (table->guid == guid) {
    return (RRBSizeTable*) table;
  }
  else {
    return transient_size_table_clone(table, len, guid);
  }
}

static InternalNode* ensure_internal_editable(InternalNode *internal, const void *guid) {
  if (internal->guid == guid) {
    return internal;
  }
  else {
    return transient_internal_node_clone(internal, guid);
  }
}

static LeafNode* ensure_leaf_editable(LeafNode *leaf, const void *guid) {
  if (leaf->guid == guid) {
    return leaf;
  }
  else {
    return transient_leaf_node_clone(leaf, guid);
  }
}

TransientRRB* rrb_to_transient(const RRB *rrb) {
  TransientRRB* trrb = transient_rrb_head_create(rrb);
  const void *guid = rrb_guid_create();
  trrb->guid = guid;
  trrb->tail = transient_leaf_node_clone(rrb->tail, guid);
  return trrb;
}

const RRB* transient_to_rrb(TransientRRB *trrb) {
  // Deny further modifications on the tree.
  trrb->guid = NULL;
  // reshrink tail
  // In case of optimisation where tail len is not modified (NOT yet tested!)
  // we have to handle it here first.
  trrb->tail = leaf_node_clone(trrb->tail);
  RRB* rrb = rrb_head_clone((const RRB *) trrb);
  return rrb;
}

uint32_t transient_rrb_count(const TransientRRB *trrb) {
  check_transience(trrb);
  return rrb_count((const RRB *) trrb);
}

void* transient_rrb_nth(const TransientRRB *trrb, uint32_t index) {
  check_transience(trrb);
  return rrb_nth((const RRB *) trrb, index);
}

void* transient_rrb_peek(const TransientRRB *trrb) {
  check_transience(trrb);
  return rrb_peek((const RRB *) trrb);
}

// rrb_push MUST use direct append techniques, otherwise it has to use
// concatenation. And, as mentioned in the report, concatenation modification is
// not evident.

static InternalNode** mutate_first_k(TransientRRB *trrb, const uint32_t k);

static InternalNode** new_editable_path(InternalNode **to_set,
                                        uint32_t empty_height, const void *guid);

TransientRRB* transient_rrb_push(TransientRRB *restrict trrb, const void *restrict elt) {
  check_transience(trrb);
  if (trrb->tail_len < RRB_BRANCHING) {
    trrb->tail->child[trrb->tail_len] = elt;
    trrb->cnt++;
    trrb->tail_len++;
    trrb->tail->len++;
    // ^ consider deferring incrementing this until insertion and/or persistentified.
    return trrb;
  }

  trrb->cnt++;
  const void *guid = trrb->guid;

  LeafNode *new_tail = transient_leaf_node_create();
  new_tail->guid = guid;
  new_tail->child[0] = elt;
  new_tail->len = 1;
  trrb->tail_len = 1;

  const LeafNode *old_tail = trrb->tail;
  trrb->tail = new_tail;

  if (trrb->root == NULL) { // If it's  null, we can't just mutate it down.
    trrb->shift = LEAF_NODE_SHIFT;
    trrb->root = (TreeNode *) old_tail;
    return trrb;
  }
  // mutable count starts here

  // TODO: Can find last rightmost jump in constant time for pvec subvecs:
  // use the fact that (index & large_mask) == 1 << (RRB_BITS * H) - 1 -> 0 etc.

  uint32_t index = trrb->cnt - 2;

  uint32_t nodes_to_mutate = 0;
  uint32_t nodes_visited = 0;
  uint32_t pos = 0; // pos is the position we insert empty nodes in the bottom
                    // mutable node (or the element, if we can mutate the leaf)
  const InternalNode *current = (const InternalNode *) trrb->root;

  uint32_t shift = RRB_SHIFT(trrb);

  // checking all non-leaf nodes (or if tail, all but the lowest two levels)
  while (shift > INC_SHIFT(LEAF_NODE_SHIFT)) {
    // calculate child index
    uint32_t child_index;
    if (current->size_table == NULL) {
      // some check here to ensure we're not overflowing the pvec subvec.
      // important to realise that this only needs to be done once in a better
      // impl, the same way the size_table check only has to be done until it's
      // false.
      const uint32_t prev_shift = shift + RRB_BITS;
      if (index >> prev_shift > 0) {
        nodes_visited++; // this could possibly be done earlier in the code.
        goto mutable_count_end;
      }
      child_index = (index >> shift) & RRB_MASK;
      // index filtering is not necessary when the check above is performed at
      // most once.
      index &= ~(RRB_MASK << shift);
    }
    else {
      // no need for sized_pos here, luckily.
      child_index = current->len - 1;
      // Decrement index
      if (child_index != 0) {
        index -= current->size_table->size[child_index-1];
      }
    }
    nodes_visited++;
    if (child_index < RRB_MASK) {
      nodes_to_mutate = nodes_visited;
      pos = child_index;
    }

    current = current->child[child_index];
    // This will only happen in a pvec subtree
    if (current == NULL) {
      nodes_to_mutate = nodes_visited;
      pos = child_index;

      // if next element we're looking at is null, we can mutate all above. Good
      // times.
      goto mutable_count_end;
    }
    shift -= RRB_BITS;
  }
  // if we're here, we're at the leaf node (or lowest non-leaf), which is
  // `current`

  // no need to even use index here: We know it'll be placed at current->len,
  // if there's enough space. That check is easy.
  if (shift != 0) {
    nodes_visited++;
    if (current->len < RRB_BRANCHING) {
      nodes_to_mutate = nodes_visited;
      pos = current->len;
    }
  }

 mutable_count_end:
  // GURRHH, nodes_visited is not yet handled nicely. loop down to get
  // nodes_visited set straight.
  while (shift > INC_SHIFT(LEAF_NODE_SHIFT)) {
    nodes_visited++;
    shift -= RRB_BITS;
  }

  // Increasing height of tree.
  if (nodes_to_mutate == 0) {
    const InternalNode *old_root = (const InternalNode *) trrb->root;
    InternalNode *new_root = transient_internal_node_create();
    new_root->guid = guid;
    new_root->len = 2;
    new_root->child[0] = (InternalNode *) trrb->root;
    trrb->root = (TreeNode *) new_root;
    trrb->shift = INC_SHIFT(RRB_SHIFT(trrb));

    // create size table if the original rrb root has a size table.
    if (old_root->type != LEAF_NODE &&
        ((const InternalNode *) old_root)->size_table != NULL) {
      RRBSizeTable *table = transient_size_table_create();
      table->guid = trrb->guid;
      table->size[0] = trrb->cnt - (old_tail->len + 1);
      // If we insert the tail, the old size minus (new size minus one) the old
      // tail size will be the amount of elements in the left branch. If there
      // is no tail, the size is just the old rrb-tree.

      table->size[1] = trrb->cnt - 1;
      // If we insert the tail, the old size would include the tail.
      // Consequently, it has to be the old size. If we have no tail, we append
      // a single element to the old vector, therefore it has to be one more
      // than the original (which means it is zero)

      new_root->size_table = table;
    }

    // nodes visited == original rrb tree height. Nodes visited > 0.
    InternalNode **to_set = new_editable_path(&((InternalNode *) trrb->root)->child[1],
                                              nodes_visited, guid);
    *to_set = (InternalNode *) old_tail;
  }
  else {
    InternalNode **node = mutate_first_k(trrb, nodes_to_mutate);
    InternalNode **to_set = new_editable_path(node, nodes_visited - nodes_to_mutate,
                                              guid);
    *to_set = (InternalNode *) old_tail;
  }

  return trrb;
}

static InternalNode** mutate_first_k(TransientRRB *trrb, const uint32_t k) {
  const void *guid = trrb->guid;
  InternalNode *current = (InternalNode *) trrb->root;
  InternalNode **to_set = (InternalNode **) &trrb->root;
  uint32_t index = trrb->cnt - 2;
  uint32_t shift = RRB_SHIFT(trrb);

  // mutate all non-leaf nodes first. Happens when shift > RRB_BRANCHING
  uint32_t i = 1;
  while (i <= k && shift != 0) {
    // First off, ensure current node is editable
    current = ensure_internal_editable(current, guid);
    *to_set = current;

    if (i == k) {
      // increase width of node
      current->len++;
    }

    if (current->size_table != NULL) {
      // Ensure size table is editable too
      RRBSizeTable *table = ensure_size_table_editable(current->size_table, current->len, guid);
      if (i != k) {
        // Tail will always be 32 long, otherwise we insert a single element only
        table->size[current->len-1] += RRB_BRANCHING;
      }
      else { // increment size of last elt -- will only happen if we append empties
        table->size[current->len-1] =
          table->size[current->len-2] + RRB_BRANCHING;
      }
      current->size_table = table;
    }

    // calculate child index
    uint32_t child_index;
    if (current->size_table == NULL) {
      child_index = (index >> shift) & RRB_MASK;
    }
    else {
      // no need for sized_pos here, luckily.
      child_index = current->len - 1;
      // Decrement index
      if (child_index != 0) {
        index -= current->size_table->size[child_index-1];
      }
    }
    to_set = &current->child[child_index];
    current = current->child[child_index];

    i++;
    shift -= RRB_BITS;
  }

  // check if we need to mutate the leaf node. Very likely to happen (31/32)
  if (i == k) {
    LeafNode *leaf = ensure_leaf_editable((LeafNode *) current, guid);
    leaf->len++;
    *to_set = (InternalNode *) leaf;
  }
  return to_set;
}

static InternalNode** new_editable_path(InternalNode **to_set, uint32_t empty_height,
                                        const void* guid) {
  if (0 < empty_height) {
    InternalNode *leaf = transient_internal_node_create();
    leaf->guid = guid;
    leaf->len = 1;

    InternalNode *empty = (InternalNode *) leaf;
    for (uint32_t i = 1; i < empty_height; i++) {
      InternalNode *new_empty = transient_internal_node_create();
      new_empty->guid = guid;
      new_empty->len = 1;
      new_empty->child[0] = empty;
      empty = new_empty;
    }
    *to_set = empty;
    return &leaf->child[0];
  }
  else {
    return to_set;
  }
}

// transient_rrb_update is effectively the same as rrb_update, but may mutate
// nodes if it's safe to do so (replacing clone calls with ensure_editable
// calls)
TransientRRB* transient_rrb_update(TransientRRB *restrict trrb, uint32_t index,
                                   const void *restrict elt) {
  check_transience(trrb);
  const void* guid = trrb->guid;
  if (index < trrb->cnt) {
    const uint32_t tail_offset = trrb->cnt - trrb->tail_len;
    if (tail_offset <= index) {
      trrb->tail->child[index - tail_offset] = elt;
      return trrb;
    }
    InternalNode **previous_pointer = (InternalNode **) &trrb->root;
    InternalNode *current = (InternalNode *) trrb->root;
    for (uint32_t shift = RRB_SHIFT(trrb); shift > 0; shift -= RRB_BITS) {
      current = ensure_internal_editable(current, guid);
      *previous_pointer = current;

      uint32_t child_index;
      if (current->size_table == NULL) {
        child_index = (index >> shift) & RRB_MASK;
      }
      else {
        child_index = sized_pos(current, &index, shift);
      }
      previous_pointer = &current->child[child_index];
      current = current->child[child_index];
    }

    LeafNode *leaf = (LeafNode *) current;
    leaf = ensure_leaf_editable((LeafNode *) leaf, guid);
    *previous_pointer = (InternalNode *) leaf;
    leaf->child[index & RRB_MASK] = elt;
    return trrb;
  }
  else {
    return NULL;
  }
}

TransientRRB* transient_rrb_pop(TransientRRB *trrb) {
  check_transience(trrb);
  if (trrb->cnt == 1) {
    trrb->cnt = 0;
    trrb->tail_len = 0;
    trrb->tail->child[0] = NULL;
    trrb->tail->len = 0;
    return trrb;
  }
  trrb->cnt--;

  if (trrb->tail_len == 1) {
    transient_promote_rightmost_leaf(trrb);
    return trrb;
  }
  else {
    trrb->tail->child[trrb->tail_len - 1] = NULL;
    trrb->tail_len--;
    trrb->tail->len--;

    return trrb;
  }
}

void transient_promote_rightmost_leaf(TransientRRB* trrb) {
  const void* guid = trrb->guid;

  InternalNode *path[RRB_MAX_HEIGHT+1];
  path[0] = (InternalNode *) trrb->root;
  uint32_t i = 0, shift = LEAF_NODE_SHIFT;

  // populate path array
  for (i = 0, shift = LEAF_NODE_SHIFT; shift < RRB_SHIFT(trrb);
       i++, shift += RRB_BITS) {
    path[i+1] = path[i]->child[path[i]->len-1];
  }

  const uint32_t height = i;

  // Set leaf node as tail.
  trrb->tail = (LeafNode *) path[height];
  trrb->tail_len = path[height]->len;
  const uint32_t tail_len = trrb->tail_len;

  path[height] = NULL;

  while (i --> 0) {
    if (path[i+1] == NULL && path[i]->len == 1) {
        path[i] = NULL;
    }
    else if (path[i+1] == NULL && i == 0 && path[0]->len == 2) {
      path[i] = path[i]->child[0];
      trrb->shift -= RRB_BITS;
    }
    else {
      path[i] = ensure_internal_editable(path[i], guid);
      path[i]->child[path[i]->len-1] = path[i+1];
      if (path[i+1] == NULL) {
        path[i]->len--;
      }
      if (path[i]->size_table != NULL) { // this is decrement-size-table*
        path[i]->size_table = ensure_size_table_editable(path[i]->size_table,
                                                         path[i]->len, guid);
        path[i]->size_table->size[path[i]->len-1] -= tail_len;
      }
    }
  }

  trrb->root = (TreeNode *) path[0];
}

// TODO: more efficient slicing algorithm for transients. Should in theory just
// require some size table magic and converting cloning over to ensure_editable.
TransientRRB* transient_rrb_slice(TransientRRB *trrb, uint32_t from, uint32_t to) {
  check_transience(trrb);
  const RRB* rrb = rrb_slice((const RRB*) trrb, from, to);
  memcpy(trrb, rrb, sizeof(RRB));
  return trrb;
}
