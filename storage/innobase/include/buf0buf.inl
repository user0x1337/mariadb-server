/*****************************************************************************

Copyright (c) 1995, 2016, Oracle and/or its affiliates. All Rights Reserved.
Copyright (c) 2014, 2021, MariaDB Corporation.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1335 USA

*****************************************************************************/

/**************************************************//**
@file include/buf0buf.ic
The database buffer buf_pool

Created 11/5/1995 Heikki Tuuri
*******************************************************/

/********************************************************************//**
Increments the modify clock of a frame by 1. The caller must (1) own the
buf_pool mutex and block bufferfix count has to be zero, (2) or own an x-lock
on the block. */
UNIV_INLINE
void
buf_block_modify_clock_inc(
/*=======================*/
	buf_block_t*	block)	/*!< in: block */
{
#ifdef SAFE_MUTEX
	ut_ad((mysql_mutex_is_owner(&buf_pool.mutex)
	       && !block->page.buf_fix_count())
	      || block->page.lock.have_u_or_x());
#else /* SAFE_MUTEX */
	ut_ad(!block->page.buf_fix_count() || block->page.lock.have_u_or_x());
#endif /* SAFE_MUTEX */
	assert_block_ahi_valid(block);

	block->modify_clock++;
}
