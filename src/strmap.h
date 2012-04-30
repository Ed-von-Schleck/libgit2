/*
 * Copyright (C) 2012 the libgit2 contributors
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */
#ifndef INCLUDE_strmap_h__
#define INCLUDE_strmap_h__

#include "common.h"

#define kmalloc git__malloc
#define kcalloc git__calloc
#define krealloc git__realloc
#define kfree git__free
#include "khash.h"

__KHASH_TYPE(str, const char *, void *);
typedef khash_t(str) git_strmap;

#define GIT__USE_STRMAP \
	__KHASH_IMPL(str, static inline, const char *, void *, 1, kh_str_hash_func, kh_str_hash_equal)

#define git_strmap_alloc()  kh_init(str)
#define git_strmap_free(h)  kh_destroy(str, h), h = NULL
#define git_strmap_clear(h) kh_clear(str, h)

#define git_strmap_num_entries(h) kh_size(h)

#define git_strmap_lookup_index(h, k)  kh_get(str, h, k)
#define git_strmap_valid_index(h, idx) (idx != kh_end(h))

#define git_strmap_exists(h, k) (kh_get(str, h, k) != kh_end(h))

#define git_strmap_value_at(h, idx)        kh_val(h, idx)
#define git_strmap_set_value_at(h, idx, v) kh_val(h, idx) = v
#define git_strmap_delete_at(h, idx)       kh_del(str, h, idx)

#define git_strmap_insert(h, key, val, err) do { \
	khiter_t __pos = kh_put(str, h, key, &err); \
	if (err >= 0) kh_val(h, __pos) = val; \
	} while (0)

#define git_strmap_insert2(h, key, val, old, err) do { \
	khiter_t __pos = kh_put(str, h, key, &err); \
	if (err >= 0) { \
		old = (err == 0) ? kh_val(h, __pos) : NULL; \
		kh_val(h, __pos) = val; \
	} } while (0)

#define git_strmap_foreach		kh_foreach
#define git_strmap_foreach_value	kh_foreach_value

#endif