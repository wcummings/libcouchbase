#ifndef NETBUF_MBLOCK_H
#define NETBUF_MBLOCK_H

#include "netbuf-defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Managed block in-order allocator.
 *
 * This allocator attempts to provide unaligned segments of memory in the
 * order they were allocated in contiguous memory
 */

/**
 * DIAGRAM LEGEND
 * In the following comments (and within the source as well) we will try to
 * display diagrams of blocks. The following symbols will be used:
 *
 * {$:NN} = This represents a position marker, $ will be the position type,
 *          and NN is the offset value.
 *
 * The following are the position types:
 *
 * [S]tart       Start of the buffer (block->start)
 * [W]rap        Wrapping and end of the first segment (block->wrap)
 * [C]ursor      End of the current segment (block->cursor)
 * [A]lloc       Allocation limit of the buffer (block->nalloc)
 * [F]lush       Flush cursor (block->flushcur)
 *
 * Note that in some cases two position types may share the same offset.
 *
 * Between any of the offsets, there are data bytes (or just "Data"). These
 * may be one of the following:
 *
 * [x]           Used data. This data is owned by a span
 * [o]           Unused data, but available for usage
 * [-]           Unreachable data. This is not used but cannot be reserved
 */

/**
 * A block contains a single allocated buffer. The buffer itself may be
 * divided among multiple spans. We divide our buffers like so:
 *
 * Initially:
 *
 * [ {FS:0}xxxxxxx{CW:10}ooo{A:12} ]
 *
 * After flushing some data:
 *
 * [ {S:0}xx{F:5}xxxx{CW:10}oo{A:12} ]
 * Note how the flush cursor is incremented
 *
 *
 * Typically, once data is flushed, the user will release the segment, and thus
 * will look something like this:
 *
 * [ ooo{SF:6}xxxx{CW:10}oooo{A:12} ]
 *
 * Appending data to a buffer (or reserving a span) depends on the span
 * size requirements. In this case, if a span's size is 2 bytes or lower,
 * it is appended at the end of the first segment, like so:
 * [ ooo{SF:16}xxxxxx{CWA:12} ]
 *
 * Otherwise, it is wrapped around, like so:
 *
 * [ xx{C:3}oo{SF:6}xxxx{W:10}--{A:12} ]
 *
 * Note that [C] has been wrapped around to start at 3.
 *
 *
 * The total size of the block's used portion is as follows:
 *
 * (1) The number of bytes between [S]tart and [Wrap]
 * (2) If [C] != [W], then also add the value of [C]
 */

struct netbuf_mblock_st;
struct netbuf_st;
struct netbuf_mblock_dealloc_queue_st;

/**
 * Small header for larger structures to more efficiently find the block
 * they were allocated in.
 *
 * Note that it is possible to also determine this information by traversing
 * the list of all blocks, but this is naturally less efficient.
 */
typedef struct {
    /** The parent block */
    struct netbuf_mblock_st *parent;
    /** The allocation offset */
    nb_SIZE offset;
} nb_ALLOCINFO;

typedef struct {
    sllist_node slnode;
    nb_SIZE offset;
    nb_SIZE size;
} nb_QDEALLOC;

typedef struct {
    sllist_node slnode;

    /** Start position for data */
    nb_SIZE start;

    /**
     * Wrap/End position for data. If the block has only one segment,
     * this is always equal to cursor (see below)
     * and will mark the position at which the unused portion of the
     * buffer begins.
     *
     * If the block has two segments, this marks the end of the first segment.
     *
     * In both cases:
     *  I. wrap is always > start
     *  II. wrap-start is the length of the first segment of data
     */
    nb_SIZE wrap;

    /**
     * End position for data. This always contains the position at which
     * the unused data begins.
     *
     * If the block only has a single segment then both the following are true:
     *
     *  I. cursor == wrap
     *  II. cursor > start (if not empty)
     *
     * If the block has two segments, then both the following are true:
     *
     *  I. cursor != wrap
     *  II. cursor < start
     *
     * If the block is empty:
     *  cursor == start
     */
    nb_SIZE cursor;

    /**
     * Total number of bytes allocated in root. This represents the absolute
     * limit on how much data can be supplied
     */
    nb_SIZE nalloc;

    /**
     * Actual allocated buffer. This remains constant for the duration of the
     * block's lifetime
     */
    char *root;

    /**
     * Pointer to a DEALLOC_QUEUE structure. This is only valid if an
     * out-of-order dealloc has been performed on this block.
     */
    struct netbuf_mblock_dealloc_queue_st *deallocs;
    struct netbuf_mblock_st *parent;
} nb_MBLOCK;

typedef struct netbuf_mblock_st {
    /** Active blocks that have at least one reserved span */
    sllist_root active;

    /** Available blocks with data */
    sllist_root avail;

    /** Allocation size */
    nb_SIZE basealloc;

    /** Maximum number of non-cached blocks */
    unsigned int maxblocks;

    /** Current number of non-cached blocks */
    unsigned int curblocks;

    nb_MBLOCK *cacheblocks;
    nb_SIZE ncacheblocks;

    struct netbuf_st *mgr;
} nb_MBPOOL;

typedef struct netbuf_mblock_dealloc_queue_st {
    sllist_root pending;
    nb_SIZE min_offset;
    nb_MBPOOL qpool;
} nb_DEALLOC_QUEUE;

#ifdef __cplusplus
}
#endif
#endif
