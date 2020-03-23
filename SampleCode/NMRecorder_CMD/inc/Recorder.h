/* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*   1. Redistributions of source code must retain the above copyright notice,
*      this list of conditions and the following disclaimer.
*   2. Redistributions in binary form must reproduce the above copyright notice,
*      this list of conditions and the following disclaimer in the documentation
*      and/or other materials provided with the distribution.
*   3. Neither the name of Nuvoton Technology Corp. nor the names of its contributors
*      may be used to endorse or promote products derived from this software
*      without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/
#ifndef RECORDER_H
#define RECORDER_H

#include <stdlib.h>
#include <stdint.h>

#include "NVTMedia.h"

#define DEF_DISK_VOLUME_STR_SIZE			20
#define DEF_RECORD_FILE_MGR_TYPE			eFILEMGR_TYPE_MP4
#define DEF_RECORD_FILE_DISK				"C"
#define DEF_RECORD_FILE_FOLDER              DEF_RECORD_FILE_DISK":\\DCIM"
#define DEF_REC_RESERVE_STORE_SPACE			(16*1024*1024)
//#define DEF_EACH_REC_FILE_DUARTION			(eNM_UNLIMIT_TIME)
#define DEF_NM_MEDIA_FILE_TYPE				eNM_MEDIA_MP4
#define DEF_REC_RESERVE_STOR_SPACE 				(400000000)



int Recorder_start(
	uint32_t u32FileDuration
);

int Recorder_stop(void);
int Recorder_is_over_limitedsize(void);
char *Recorder_getdiskvolume(void);

#endif
