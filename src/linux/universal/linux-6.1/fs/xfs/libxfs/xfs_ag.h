/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018 Red Hat, Inc.
 * All rights reserved.
 */

#ifndef __LIBXFS_AG_H
#define __LIBXFS_AG_H 1

struct xfs_mount;
struct xfs_trans;
struct xfs_perag;

/*
 * Per-ag infrastructure
 */

/* per-AG block reservation data structures*/
struct xfs_ag_resv {
	/* number of blocks originally reserved here */
	xfs_extlen_t			ar_orig_reserved;
	/* number of blocks reserved here */
	xfs_extlen_t			ar_reserved;
	/* number of blocks originally asked for */
	xfs_extlen_t			ar_asked;
};

/*
 * Per-ag incore structure, copies of information in agf and agi, to improve the
 * performance of allocation group selection.
 */
struct xfs_perag {
	struct xfs_mount *pag_mount;	/* owner filesystem */
	xfs_agnumber_t	pag_agno;	/* AG this structure belongs to */
	atomic_t	pag_ref;	/* perag reference count */
	char		pagf_init;	/* this agf's entry is initialized */
	char		pagi_init;	/* this agi's entry is initialized */
	char		pagf_metadata;	/* the agf is preferred to be metadata */
	char		pagi_inodeok;	/* The agi is ok for inodes */
	uint8_t		pagf_levels[XFS_BTNUM_AGF];
					/* # of levels in bno & cnt btree */
	bool		pagf_agflreset; /* agfl requires reset before use */
	uint32_t	pagf_flcount;	/* count of blocks in freelist */
	xfs_extlen_t	pagf_freeblks;	/* total free blocks */
	xfs_extlen_t	pagf_longest;	/* longest free space */
	uint32_t	pagf_btreeblks;	/* # of blocks held in AGF btrees */
	xfs_agino_t	pagi_freecount;	/* number of free inodes */
	xfs_agino_t	pagi_count;	/* number of allocated inodes */

	/*
	 * Inode allocation search lookup optimisation.
	 * If the pagino matches, the search for new inodes
	 * doesn't need to search the near ones again straight away
	 */
	xfs_agino_t	pagl_pagino;
	xfs_agino_t	pagl_leftrec;
	xfs_agino_t	pagl_rightrec;

	int		pagb_count;	/* pagb slots in use */
	uint8_t		pagf_refcount_level; /* recount btree height */

	/* Blocks reserved for all kinds of metadata. */
	struct xfs_ag_resv	pag_meta_resv;
	/* Blocks reserved for the reverse mapping btree. */
	struct xfs_ag_resv	pag_rmapbt_resv;

	/* for rcu-safe freeing */
	struct rcu_head	rcu_head;

	/* Precalculated geometry info */
	xfs_agblock_t		block_count;
	xfs_agblock_t		min_block;
	xfs_agino_t		agino_min;
	xfs_agino_t		agino_max;

#ifdef __KERNEL__
	/* -- kernel only structures below this line -- */

	/*
	 * Bitsets of per-ag metadata that have been checked and/or are sick.
	 * Callers should hold pag_state_lock before accessing this field.
	 */
	uint16_t	pag_checked;
	uint16_t	pag_sick;
	spinlock_t	pag_state_lock;

	spinlock_t	pagb_lock;	/* lock for pagb_tree */
	struct rb_root	pagb_tree;	/* ordered tree of busy extents */
	unsigned int	pagb_gen;	/* generation count for pagb_tree */
	wait_queue_head_t pagb_wait;	/* woken when pagb_gen changes */

	atomic_t        pagf_fstrms;    /* # of filestreams active in this AG */

	spinlock_t	pag_ici_lock;	/* incore inode cache lock */
	struct radix_tree_root pag_ici_root;	/* incore inode cache root */
	int		pag_ici_reclaimable;	/* reclaimable inodes */
	unsigned long	pag_ici_reclaim_cursor;	/* reclaim restart point */

	/* buffer cache index */
	spinlock_t	pag_buf_lock;	/* lock for pag_buf_hash */
	struct rhashtable pag_buf_hash;

	/* background prealloc block trimming */
	struct delayed_work	pag_blockgc_work;

#endif /* __KERNEL__ */
};


void xfs_free_unused_perag_range(struct xfs_mount *mp, xfs_agnumber_t agstart,
			xfs_agnumber_t agend);
int xfs_initialize_perag(struct xfs_mount *mp, xfs_agnumber_t agcount,
			xfs_rfsblock_t dcount, xfs_agnumber_t *maxagi);
int xfs_initialize_perag_data(struct xfs_mount *mp, xfs_agnumber_t agno);
void xfs_free_perag(struct xfs_mount *mp);

struct xfs_perag *xfs_perag_get(struct xfs_mount *mp, xfs_agnumber_t agno);
struct xfs_perag *xfs_perag_get_tag(struct xfs_mount *mp, xfs_agnumber_t agno,
		unsigned int tag);
void xfs_perag_put(struct xfs_perag *pag);

/*
 * Per-ag geometry infomation and validation
 */
xfs_agblock_t xfs_ag_block_count(struct xfs_mount *mp, xfs_agnumber_t agno);
void xfs_agino_range(struct xfs_mount *mp, xfs_agnumber_t agno,
		xfs_agino_t *first, xfs_agino_t *last);

static inline bool
xfs_verify_agbno(struct xfs_perag *pag, xfs_agblock_t agbno)
{
	if (agbno >= pag->block_count)
		return false;
	if (agbno <= pag->min_block)
		return false;
	return true;
}

static inline bool
xfs_verify_agbext(
	struct xfs_perag	*pag,
	xfs_agblock_t		agbno,
	xfs_agblock_t		len)
{
	if (agbno + len <= agbno)
		return false;

	if (!xfs_verify_agbno(pag, agbno))
		return false;

	return xfs_verify_agbno(pag, agbno + len - 1);
}

/*
 * Verify that an AG inode number pointer neither points outside the AG
 * nor points at static metadata.
 */
static inline bool
xfs_verify_agino(struct xfs_perag *pag, xfs_agino_t agino)
{
	if (agino < pag->agino_min)
		return false;
	if (agino > pag->agino_max)
		return false;
	return true;
}

/*
 * Verify that an AG inode number pointer neither points outside the AG
 * nor points at static metadata, or is NULLAGINO.
 */
static inline bool
xfs_verify_agino_or_null(struct xfs_perag *pag, xfs_agino_t agino)
{
	if (agino == NULLAGINO)
		return true;
	return xfs_verify_agino(pag, agino);
}

static inline bool
xfs_ag_contains_log(struct xfs_mount *mp, xfs_agnumber_t agno)
{
	return mp->m_sb.sb_logstart > 0 &&
	       agno == XFS_FSB_TO_AGNO(mp, mp->m_sb.sb_logstart);
}

/*
 * Perag iteration APIs
 */
static inline struct xfs_perag *
xfs_perag_next(
	struct xfs_perag	*pag,
	xfs_agnumber_t		*agno,
	xfs_agnumber_t		end_agno)
{
	struct xfs_mount	*mp = pag->pag_mount;

	*agno = pag->pag_agno + 1;
	xfs_perag_put(pag);
	if (*agno > end_agno)
		return NULL;
	return xfs_perag_get(mp, *agno);
}

#define for_each_perag_range(mp, agno, end_agno, pag) \
	for ((pag) = xfs_perag_get((mp), (agno)); \
		(pag) != NULL; \
		(pag) = xfs_perag_next((pag), &(agno), (end_agno)))

#define for_each_perag_from(mp, agno, pag) \
	for_each_perag_range((mp), (agno), (mp)->m_sb.sb_agcount - 1, (pag))


#define for_each_perag(mp, agno, pag) \
	(agno) = 0; \
	for_each_perag_from((mp), (agno), (pag))

#define for_each_perag_tag(mp, agno, pag, tag) \
	for ((agno) = 0, (pag) = xfs_perag_get_tag((mp), 0, (tag)); \
		(pag) != NULL; \
		(agno) = (pag)->pag_agno + 1, \
		xfs_perag_put(pag), \
		(pag) = xfs_perag_get_tag((mp), (agno), (tag)))

struct aghdr_init_data {
	/* per ag data */
	xfs_agblock_t		agno;		/* ag to init */
	xfs_extlen_t		agsize;		/* new AG size */
	struct list_head	buffer_list;	/* buffer writeback list */
	xfs_rfsblock_t		nfree;		/* cumulative new free space */

	/* per header data */
	xfs_daddr_t		daddr;		/* header location */
	size_t			numblks;	/* size of header */
	xfs_btnum_t		type;		/* type of btree root block */
};

int xfs_ag_init_headers(struct xfs_mount *mp, struct aghdr_init_data *id);
int xfs_ag_shrink_space(struct xfs_perag *pag, struct xfs_trans **tpp,
			xfs_extlen_t delta);
int xfs_ag_extend_space(struct xfs_perag *pag, struct xfs_trans *tp,
			xfs_extlen_t len);
int xfs_ag_get_geometry(struct xfs_perag *pag, struct xfs_ag_geometry *ageo);

#endif /* __LIBXFS_AG_H */
