#ifndef RECORDER_H
#define RECORDER_H

#include <stdlib.h>
#include <stdint.h>

#include "NVTMedia.h"

#define DEF_DISK_VOLUME_STR_SIZE            20
#define DEF_RECORD_FILE_MGR_TYPE            eFILEMGR_TYPE_MP4
#define DEF_RECORD_FILE_DISK                    "C"
#define DEF_RECORD_FILE_FOLDER              DEF_RECORD_FILE_DISK":\\DCIM"
#define DEF_REC_RESERVE_STORE_SPACE     (16*1024*1024)
#define DEF_EACH_REC_FILE_DUARTION      (eNM_UNLIMIT_TIME)
#define DEF_NM_MEDIA_FILE_TYPE              eNM_MEDIA_MP4

int recorder_start(void);
int recorder_stop(void);
E_NM_RECORD_STATUS recorder_status(void);
uint64_t recorder_recordedtime_current(void);
int recorder_is_over_limitedsize(void);

char *recorder_getdiskvolume(void);

#endif  // Avoid multiple inclusion
