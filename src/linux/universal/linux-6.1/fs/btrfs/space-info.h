/* SPDX-License-Identifier: GPL-2.0 */

#ifndef BTRFS_SPACE_INFO_H
#define BTRFS_SPACE_INFO_H

#include "volumes.h"

struct btrfs_space_info {
	spinlock_t lock;

	u64 total_bytes;	/* total bytes in the space,
				   this doesn't take mirrors into account */
	u64 bytes_used;		/* total bytes used,
				   this doesn't take mirrors into account */
	u64 bytes_pinned;	/* total bytes pinned, will be freed when the
				   transaction finishes */
	u64 bytes_reserved;	/* total bytes the allocator has reserved for
				   current allocations */
	u64 bytes_may_use;	/* number of bytes that may be used for
				   delalloc/allocations */
	u64 bytes_readonly;	/* total bytes that are read only */
	/* Total bytes in the space, but only accounts active block groups. */
	u64 active_total_bytes;
	u64 bytes_zone_unusable;	/* total bytes that are unusable until
					   resetting the device zone */

	u64 max_extent_size;	/* This will hold the maximum extent size of
				   the space info if we had an ENOSPC in the
				   allocator. */
	/* Chunk size in bytes */
	u64 chunk_size;

	/*
	 * Once a block group drops below this threshold (percents) we'll
	 * schedule it for reclaim.
	 */
	int bg_reclaim_threshold;

	int clamp;		/* Used to scale our threshold for preemptive
				   flushing. The value is >> clamp, so turns
				   out to be a 2^clamp divisor. */

	unsigned int full:1;	/* indicates that we cannot allocate any more
				   chunks for this space */
	unsigned int chunk_alloc:1;	/* set if we are allocating a chunk */

	unsigned int flush:1;		/* set if we are trying to make space */

	unsigned int force_alloc;	/* set if we need to force a chunk
					   alloc for this space */

	u64 disk_used;		/* total bytes used on disk */
	u64 disk_total;		/* total bytes on disk, takes mirrors into
				   account */

	u64 flags;

	struct list_head list;
	/* Protected by the spinlock 'lock'. */
	struct list_head ro_bgs;
	struct list_head priority_tickets;
	struct list_head tickets;

	/*
	 * Size of space that needs to be reclaimed in order to satisfy pending
	 * tickets
	 */
	u64 reclaim_size;

	/*
	 * tickets_id just indicates the next ticket will be handled, so note
	 * it's not stored per ticket.
	 */
	u64 tickets_id;

	struct rw_semaphore groups_sem;
	/* for block groups in our same type */
	struct list_head block_groups[BTRFS_NR_RAID_TYPES];

	struct kobject kobj;
	struct kobject *block_group_kobjs[BTRFS_NR_RAID_TYPES];
};

struct reserve_ticket {
	u64 bytes;
	int error;
	bool steal;
	struct list_head list;
	wait_queue_head_t wait;
};

static inline bool btrfs_mixed_space_info(struct btrfs_space_info *space_info)
{
	return ((space_info->flags & BTRFS_BLOCK_GROUP_METADATA) &&
		(space_info->flags & BTRFS_BLOCK_GROUP_DATA));
}

/*
 *
 * Declare a helper function to detect underflow of various space info members
 */
#define DECLARE_SPACE_INFO_UPDATE(name, trace_name)			\
static inline void							\
btrfs_space_info_update_##name(struct btrfs_fs_info *fs_info,		\
			       struct btrfs_space_info *sinfo,		\
			       s64 bytes)				\
{									\
	const u64 abs_bytes = (bytes < 0) ? -bytes : bytes;		\
	lockdep_assert_held(&sinfo->lock);				\
	trace_update_##name(fs_info, sinfo, sinfo->name, bytes);	\
	trace_btrfs_space_reservation(fs_info, trace_name,		\
				      sinfo->flags, abs_bytes,		\
				      bytes > 0);			\
	if (bytes < 0 && sinfo->name < -bytes) {			\
		WARN_ON(1);						\
		sinfo->name = 0;					\
		return;							\
	}								\
	sinfo->name += bytes;						\
}

DECLARE_SPACE_INFO_UPDATE(bytes_may_use, "space_info");
DECLARE_SPACE_INFO_UPDATE(bytes_pinned, "pinned");
DECLARE_SPACE_INFO_UPDATE(bytes_zone_unusable, "zone_unusable");

int btrfs_init_space_info(struct btrfs_fs_info *fs_info);
void btrfs_add_bg_to_space_info(struct btrfs_fs_info *info,
				struct btrfs_block_group *block_group);
void btrfs_update_space_info_chunk_size(struct btrfs_space_info *space_info,
					u64 chunk_size);
struct btrfs_space_info *btrfs_find_space_info(struct btrfs_fs_info *info,
					       u64 flags);
u64 __pure btrfs_space_info_used(struct btrfs_space_info *s_info,
			  bool may_use_included);
void btrfs_clear_space_info_full(struct btrfs_fs_info *info);
void btrfs_dump_space_info(struct btrfs_fs_info *fs_info,
			   struct btrfs_space_info *info, u64 bytes,
			   int dump_block_groups);
int btrfs_reserve_metadata_bytes(struct btrfs_fs_info *fs_info,
				 struct btrfs_block_rsv *block_rsv,
				 u64 orig_bytes,
				 enum btrfs_reserve_flush_enum flush);
void btrfs_try_granting_tickets(struct btrfs_fs_info *fs_info,
				struct btrfs_space_info *space_info);
int btrfs_can_overcommit(struct btrfs_fs_info *fs_info,
			 struct btrfs_space_info *space_info, u64 bytes,
			 enum btrfs_reserve_flush_enum flush);

static inline void btrfs_space_info_free_bytes_may_use(
				struct btrfs_fs_info *fs_info,
				struct btrfs_space_info *space_info,
				u64 num_bytes)
{
	spin_lock(&space_info->lock);
	btrfs_space_info_update_bytes_may_use(fs_info, space_info, -num_bytes);
	btrfs_try_granting_tickets(fs_info, space_info);
	spin_unlock(&space_info->lock);
}
int btrfs_reserve_data_bytes(struct btrfs_fs_info *fs_info, u64 bytes,
			     enum btrfs_reserve_flush_enum flush);
void btrfs_dump_space_info_for_trans_abort(struct btrfs_fs_info *fs_info);
void btrfs_init_async_reclaim_work(struct btrfs_fs_info *fs_info);

#endif /* BTRFS_SPACE_INFO_H */
