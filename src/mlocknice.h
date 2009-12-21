/* ========================================================================= *
 * File: mlocknice.h
 *
 * Copyright (C) 2009 by Nokia Corporation
 *
 * Author: Leonid Moiseichuk <leonid.moiseichuk@nokia.com>
 * Contact: Eero Tamminen <eero.tamminen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Description:
 *    Scanning maps file and unlock unnecessary pages in depending of required mask.
 *    There are 4 meaningful combinations:
 *    - most expensive and slow case: mlockall(MCL_CURRENT|MCL_FUTURE)
 *      => a lot of RAM used plus we need time to load process code pages
 *    - slow case: mlockall(MCL_CURRENT|MCL_FUTURE) and unlock code after that
 *      => like first one but we release memory back, has sense for processes which has a lot of plugins
 *    - recommended general case: mlockall(MCL_FUTURE) and lock data segments (stack, data etc)
 *      => minimal memory usage and no code pages loading required
 *    - economical case: lock data segments only
 *      => should be used for daemons which are not grow a lot, best from time and memory usage
 *
 * History:
 *
 * 07-Sep-2009 Leonid Moiseichuk
 * - Added mln_adjust_spare for processes which temporary becomes root.
 *
 * 03-Sep-2009 Leonid Moiseichuk
 * - Updated according to RTCom request.
 *
 * 23-Jul-2009 Leonid Moiseichuk
 * - Initial creation.
 *
 * ========================================================================= */

#ifndef MLOCKNICE_H_USED
#define MLOCKNICE_H_USED

/* --------------------------------------------------------------------------------------- */
/* mln_lock_data - scans /proc/self/maps and lock all data pages                           */
/* return 0 on success or -1 in case of errors (check errno later)                         */
/* --------------------------------------------------------------------------------------- */
extern int mln_lock_data(void);

/* --------------------------------------------------------------------------------------- */
/* mln_unlock_code - scans /proc/self/maps and unlock non-rw pages                         */
/* return 0 on success or -1 in case of errors (check errno later)                         */
/* --------------------------------------------------------------------------------------- */
extern int mln_unlock_code(void);

/* --------------------------------------------------------------------------------------- */
/* mln_adjust_limit - scans /proc/self/status and sets RLIMIT_MEMLOCK as                   */
/*    VmPeak + pointed spare (in bytes) to have non-root process running later.            */
/* parameter: spare - amount of bytes in to be reserved over the current known maximum     */
/* return 0 on success or -1 in case of errors (check errno later)                         */
/* --------------------------------------------------------------------------------------- */
extern int mln_adjust_limit(unsigned spare);

#endif /* MLOCKNICE_H_USED */
