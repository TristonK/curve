/*
 *     Copyright (c) 2020 NetEase Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * Project: curve
 * File Created: 2019-08-07
 * Author: hzchenwei7
 */

#ifndef SRC_TOOLS_NBD_LIBNEBD_H_
#define SRC_TOOLS_NBD_LIBNEBD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <aio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define NEBD_MAX_FILE_PATH_LEN 1024
#define NEBD_MAX_BUF_LEN 1024 * 1024 * 32

typedef enum LIBAIO_OP {
    LIBAIO_OP_READ,
    LIBAIO_OP_WRITE,
    LIBAIO_OP_DISCARD,
    LIBAIO_OP_FLUSH,
} LIBAIO_OP;

struct NebdClientAioContext;

typedef void (*LibAioCallBack)(struct NebdClientAioContext* context);

typedef struct NebdClientAioContext {
    off_t offset;
    size_t length;
    int ret;
    LIBAIO_OP op;
    LibAioCallBack cb;
    void* buf;
    unsigned int retryCount;
} NebdClientAioContext;

// int nebd_lib_fini(void);
// for ceph & curve
int nebd_lib_init(void);
int nebd_lib_open(const char* filename);
int nebd_lib_close(int fd);
int nebd_lib_pread(int fd, void* buf, off_t offset, size_t length);
int nebd_lib_pwrite(int fd, const void* buf, off_t offset, size_t length);
int nebd_lib_discard(int fd, NebdClientAioContext* context);
int nebd_lib_aio_pread(int fd, NebdClientAioContext* context);
int nebd_lib_aio_pwrite(int fd, NebdClientAioContext* context);
int nebd_lib_sync(int fd);
int64_t nebd_lib_filesize(int fd);
int nebd_lib_resize(int fd, int64_t size);
// for ceph only
int nebd_lib_flush(int fd, NebdClientAioContext* context);
int64_t nebd_lib_getinfo(int fd);
int nebd_lib_invalidcache(int fd);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  //  SRC_TOOLS_NBD_LIBNEBD_H_