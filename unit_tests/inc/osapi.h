/*
 * Standalone OSAL API header for CF unit tests.
 */
#ifndef _osapi_h_
#define _osapi_h_

#include "common_types.h"
#include "osconfig.h"

#define OS_SUCCESS   0
#define OS_ERROR    -1
#define OS_FS_ERROR -1
#define OS_FS_SUCCESS 0
#define OS_INVALID_POINTER -2
#define OS_FS_ERR_PATH_TOO_LONG -3
#define OS_FS_ERR_INVALID_FD    -4

/* File access modes */
#define OS_WRITE_ONLY  1
#define OS_READ_ONLY   0
#define OS_READ_WRITE  2

/* File seek modes */
#define OS_SEEK_SET  0
#define OS_SEEK_CUR  1
#define OS_SEEK_END  2

/* Directory entry */
typedef struct {
    char FileName[OS_MAX_FILE_NAME];
    char d_name[OS_MAX_FILE_NAME];  /* alias used by some code */
} os_dirent_t;

/* Directory pointer type */
typedef uint32 os_dirp_t;

/* File stat */
typedef struct {
    uint32 FileModeBits;
    int32  FileTime;
    uint32 FileSize;
    uint32 st_size;   /* alias used by some code */
} os_fstat_t;

/* FDTableEntry for OS_FDGetInfo */
typedef struct {
    char   Path[OS_MAX_PATH_LEN];
    uint32 User;
    boolean IsValid;
} OS_FDTableEntry;

/* Counting semaphore property type */
typedef struct {
    char   name[OS_MAX_API_NAME];
    uint32 creator;
    uint32 value;
} OS_count_sem_prop_t;

/* Semaphore types */
typedef uint32 OS_SemId_t;

/* OS function prototypes (stubs provided) */
int32 OS_opendir(const char *path);
int32 OS_closedir(uint32 dirp);
os_dirent_t *OS_readdir(uint32 dirp);
int32 OS_stat(const char *path, os_fstat_t *filestats);
int32 OS_remove(const char *path);
int32 OS_rename(const char *old_name, const char *new_name);
int32 OS_FDGetInfo(int32 filedes, OS_FDTableEntry *fd_prop);
int32 OS_CountSemGetIdByName(uint32 *sem_id, const char *sem_name);
int32 OS_CountSemGive(uint32 sem_id);
int32 OS_CountSemTake(uint32 sem_id);
int32 OS_CountSemGetInfo(uint32 sem_id, OS_count_sem_prop_t *sem_prop);
int32 OS_CountSemTimedWait(uint32 sem_id, uint32 msecs);
void  OS_printf(const char *fmt, ...);
int32 OS_mv(const char *src, const char *dest);
int32 OS_lseek(int32 filedes, int32 offset, uint32 whence);

/* File I/O */
int32 OS_creat(const char *path, int32 access);
int32 OS_open(const char *path, int32 access, uint32 mode);
int32 OS_close(int32 filedes);
int32 OS_write(int32 filedes, void *buffer, uint32 nbytes);
int32 OS_read(int32 filedes, void *buffer, uint32 nbytes);

#endif /* _osapi_h_ */
