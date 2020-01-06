/* @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "NVTMedia_SAL_FS.h"

#if NUMEDIA_CFG_SAL_FS_PORT == NUMEDIA_SAL_FS_PORT_NVTFAT


int open(
	const char *path, 
	int oflags
)
{
	char szUnicodeFileName[MAX_FILE_NAME_LEN];

	fsAsciiToUnicode((void *)path, szUnicodeFileName, TRUE); 
 
	return fsOpenFile(szUnicodeFileName, NULL, oflags);
}

int close(
	int fildes
)
{
	return fsCloseFile(fildes);
}

int read(
	int fildes, 
	void *buf, 
	int nbyte
)
{
	int32_t i32Ret;
	int32_t i32ReadCnt = 0;
	i32Ret = fsReadFile(fildes, buf, nbyte, &i32ReadCnt);

	if(i32Ret < 0)
		return i32Ret;
	return i32ReadCnt;
}

int write(
	int fildes, 
	const void *buf, 
	int nbyte
)
{
	int32_t i32Ret;
	int32_t i32WriteCnt = 0;
	i32Ret = fsWriteFile(fildes, (UINT8 *)buf, nbyte, &i32WriteCnt);

	if(i32Ret < 0)
		return i32Ret;
	return i32WriteCnt;
}


int64_t 
lseek(
	int fildes, 
	int64_t offset, 
	int whence
)
{
	return fsFileSeek(fildes, offset, whence);
}

int stat(
	const char *path, 
	struct stat *buf
)
{
	char szUnicodeFileName[MAX_FILE_NAME_LEN];
	FILE_STAT_T sFileState;

	fsAsciiToUnicode((void *)path, szUnicodeFileName, TRUE); 

	if(fsGetFileStatus(-1, szUnicodeFileName, NULL, &sFileState) < 0){
		return -1;
	}
	
	buf->st_mode = sFileState.ucAttrib;
	buf->st_size = sFileState.n64FileSize;
	
	//[FIXME]
	buf->st_atime = 0;
	buf->st_mtime = 0;
	buf->st_ctime = 0;	
	return 0;
}

int fstat(
	int fildes, 
	struct stat *buf
)
{
	FILE_STAT_T sFileState;


	if(fsGetFileStatus(fildes, NULL, NULL, &sFileState) < 0){
		return -1;
	}
	
	buf->st_mode = sFileState.ucAttrib;
	buf->st_size = sFileState.n64FileSize;
	
	//[FIXME]
	buf->st_atime = 0;
	buf->st_mtime = 0;
	buf->st_ctime = 0;	
	return 0;
}

int unlink(
	const char *pathname
)
{
	char szUnicodeFileName[MAX_FILE_NAME_LEN];

	fsAsciiToUnicode((void *)pathname, szUnicodeFileName, TRUE); 
	return fsDeleteFile(szUnicodeFileName, NULL);
}

int truncate(
	const char *path, 
	uint64_t length
)
{
	char szUnicodeFileName[MAX_FILE_NAME_LEN];

	fsAsciiToUnicode((void *)path, szUnicodeFileName, TRUE); 

	return fsSetFileSize(-1, szUnicodeFileName, NULL, length);
}

int fileno(FILE *pFile)
{
	int *piFileNo = (int *)((char *)pFile + 20);
	return *piFileNo;
}

int ftruncate(
	int fildes, 
	int64_t length
)
{
	return fsSetFileSize(fildes, NULL, NULL, length);
}

DIR *opendir(
	const char *name
)
{
	int32_t i32ReadDirRet;
	DIR *psDir = NULL;
	char szLongName[MAX_FILE_NAME_LEN/2];

	psDir = malloc(sizeof(DIR));
	if(psDir == NULL)
		return NULL;
	
	memset(psDir, 0 , sizeof(DIR));
	psDir->bLastDirEnt = 0;
	fsAsciiToUnicode((void *)name, szLongName, TRUE);

	i32ReadDirRet = fsFindFirst(szLongName, (CHAR *)name, &psDir->tCurFieEnt);
	if (i32ReadDirRet < 0){
		free(psDir);
		return NULL;
	}
	
	return psDir;	
}


struct dirent *readdir(DIR *dirp)
{
	if(dirp == NULL)
		return NULL;
	
	if(dirp->bLastDirEnt == 1){
		return NULL;
	}

	fsUnicodeToAscii(dirp->tCurFieEnt.suLongName, dirp->sDirEnt.d_name, 1);

	if(fsFindNext(&dirp->tCurFieEnt) != 0)
		dirp->bLastDirEnt = 1;

	return &dirp->sDirEnt;
}


int closedir(DIR *dirp){
	if(dirp == NULL)
		return -1;

	fsFindClose(&dirp->tCurFieEnt);	
	free(dirp);
	return 0;
}


#endif
