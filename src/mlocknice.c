/* ========================================================================= *
 * File: mlocknice.c
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
 *    Test version of file can be build using the following command line
 *      gcc -DTEST -o mlocknice mlocknice.c
 *    Expected output
 *      ./mlocknice
 *      mlocknice testing, build Sep  8 2009 09:39:10
 *      adjusting RLIMIT_MEMLOCK rlimit
 *      RLIMIT_MEMLOCK: soft 65536 bytes; hard 65536 bytes
 *      RLIMIT_MEMLOCK: soft 2621440 bytes; hard 2621440 bytes
 *      most expensive case: MCL_CURRENT|MCL_FUTURE
 *      VmLck:      1536 kB
 *      slow case: MCL_CURRENT|MCL_FUTURE + unlock code
 *      VmLck:       264 kB
 *      recommended case: MCL_FUTURE + lock data
 *      VmLck:       264 kB
 *      economical case: lock data
 *      VmLck:       260 kB
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

/* ========================================================================= *
 * Includes
 * ========================================================================= */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ========================================================================= *
 * munlocknice code.
 * ========================================================================= */

/* --------------------------------------------------------------------------------------- */
/* mln_lock_data - scans /proc/self/maps and lock all data pages                           */
/* return 0 on success or -1 in case of errors (check errno later)                         */
/* --------------------------------------------------------------------------------------- */

int mln_lock_data(void)
{
  FILE* fp = fopen("/proc/self/maps", "r");
  char buf[1024];
  int  err = 0;

  if ( !fp )
    return -1;

  while ( fgets(buf, sizeof(buf), fp) )
  {
    char* ptr = strchr(buf, ' ');
    unsigned long from;
    unsigned long size;

    if ( !ptr )
      continue;

    /* checking for 'rw' options at the begining of mapping entry */
    if ('r' != ptr[1] || 'w' != ptr[2] )
      continue;

    /* writable mapping entry which could be locked */
    ptr = strchr(buf, '-');
    if ( !ptr )
      continue;

    *ptr++ = '\0';
    from = strtoul(buf, NULL, 16);
    size = strtoul(ptr, NULL, 16) - from;
    if ( mlock( (const void*)from, size) )
      err = errno;
  }

  fclose(fp);
  errno = err;

  return (err ? -1 : 0);
} /* mln_lock_data */


/* --------------------------------------------------------------------------------------- */
/* mln_unlock_code - scans /proc/self/maps and unlock non-rw pages                         */
/* return 0 on success or -1 in case of errors (check errno later)                         */
/* --------------------------------------------------------------------------------------- */

int mln_unlock_code(void)
{
  FILE* fp = fopen("/proc/self/maps", "r");
  char buf[1024];
  int  err = 0;

  if ( !fp )
    return -1;

  while ( fgets(buf, sizeof(buf), fp) )
  {
    char* ptr = strchr(buf, ' ');
    unsigned long from;
    unsigned long size;

    if ( !ptr )
      continue;

    /* checking for 'rw' options at the begining of mapping entry */
    if ('r' == ptr[1] && 'w' == ptr[2] )
      continue;

    /* non-writable mapping entry which could be munlocked */
    ptr = strchr(buf, '-');
    if ( !ptr )
      continue;

    *ptr++ = '\0';
    from = strtoul(buf, NULL, 16);
    size = strtoul(ptr, NULL, 16) - from;
    if ( munlock( (const void*)from, size) )
      err = errno;
  }

  fclose(fp);
  errno = err;

  return (err ? -1 : 0);
} /* mln_unlock_code */

/* --------------------------------------------------------------------------------------- */
/* mln_adjust_limit - scans /proc/self/status and sets RLIMIT_MEMLOCK as                   */
/*    VmPeak + pointed spare (in bytes) to have non-root process running later.            */
/* parameter: spare - amount of bytes in to be reserved over the current known maximum     */
/* return 0 on success or -1 in case of errors (check errno later)                         */
/* --------------------------------------------------------------------------------------- */
int mln_adjust_limit(unsigned spare)
{
  FILE* fp = fopen("/proc/self/status", "r");
  char buf[256];
  unsigned VmPeak = 0;
  struct rlimit limit;

  /* process status file */
  if ( !fp )
    return -1;

  while ( fgets(buf, sizeof(buf), fp) )
  {
    if ( 0 == strncmp(buf, "VmPeak:", 7) )
    {
      VmPeak = (unsigned)atoi(buf + 8);
      break;
    }
  }
  fclose(fp);

  /* Setting limit */
  limit.rlim_cur = limit.rlim_max = VmPeak * 1024 + spare;
  return setrlimit(RLIMIT_MEMLOCK, &limit);
} /* mln_adjust_limit */

/* ========================================================================= *
 * Testing code (main etc)
 * ========================================================================= */

#ifdef TEST


static void show_limit(void)
{
  struct rlimit limit;

  if ( getrlimit(RLIMIT_MEMLOCK, &limit) )
    printf ("cannot get RLIMIT_MEMLOCK limit\n");
  else
    printf ("RLIMIT_MEMLOCK: soft %u bytes; hard %u bytes\n", limit.rlim_cur, limit.rlim_max);
} /* show_limit */

static void show_locked(void)
{
  FILE* fp = fopen("/proc/self/status", "r");
  char buf[256];

  if ( !fp )
    return;

  while ( fgets(buf, sizeof(buf), fp) )
  {
    if ( strstr(buf, "VmLck:") )
    {
      printf (buf);
      break;
    }
  }

  fclose(fp);
} /* show_locked */


int main(const int argc, const char* argv[])
{
  printf ("mlocknice testing, build %s %s\n", __DATE__, __TIME__);

  printf ("adjusting RLIMIT_MEMLOCK rlimit\n");
  show_limit();
  if ( mln_adjust_limit(1 * 1024 * 1024) )
  {
    perror("mln_adjust_limit(1 MB)");
    return 1;
  }
  show_limit();

  printf ("most expensive case: MCL_CURRENT|MCL_FUTURE\n");
  if ( mlockall(MCL_CURRENT|MCL_FUTURE) )
  {
    perror("mlockall(MCL_CURRENT|MCL_FUTURE)");
    return 1;
  }
  show_locked();
  munlockall();

  printf ("slow case: MCL_CURRENT|MCL_FUTURE + unlock code\n");
  if ( mlockall(MCL_CURRENT|MCL_FUTURE) )
  {
    perror("mlockall(MCL_CURRENT|MCL_FUTURE)");
    return 1;
  }
  if ( mln_unlock_code() )
  {
    perror("mln_unlock_code()");
    return 1;
  }
  show_locked();
  munlockall();

  printf ("recommended case: MCL_FUTURE + lock data\n");
  if ( mlockall(MCL_FUTURE) )
  {
    perror("mlockall(MCL_FUTURE)");
    return 1;
  }
  if ( mln_lock_data() )
  {
    perror("mln_lock_data()");
    return 1;
  }
  show_locked();
  munlockall();

  printf ("economical case: lock data\n");
  if ( mln_lock_data() )
  {
    perror("mln_lock_data()");
    return 1;
  }
  show_locked();
  munlockall();

  return 0;
} /* main */

#endif /* TEST */

/* ========================================================================= *
 *                    No more code in file mlocknice.c                       *
 * ========================================================================= */
