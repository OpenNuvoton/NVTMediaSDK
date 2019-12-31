//		Copyright (c) 2019 Nuvoton Technology Corp. All rights reserved.
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either m_uiVersion 2 of the License, or
//      (at your option) any later m_uiVersion.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "NuMedia_SAL_FS.h"

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
