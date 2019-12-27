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

#include "NuMedia.h"

#include "NuMedia_SAL_FS.h"

/*
typedef enum{
	eNM_CTX_AUDIO_UNKNOWN = -1,
	eNM_CTX_AUDIO_NONE,
	eNM_CTX_AUDIO_ALAW,
	eNM_CTX_AUDIO_ULAW,
	eNM_CTX_AUDIO_AAC,	
	eNM_CTX_AUDIO_MP3,	
	eNM_CTX_AUDIO_PCM_L16,		//Raw data format
	eNM_CTX_AUDIO_ADPCM,
	eNM_CTX_AUDIO_G726,	
	eNM_CTX_AUDIO_END,	
}E_NM_CTX_AUDIO_TYPE;
*/

static S_NM_CODECENC_AUDIO_IF *s_apsAudioEncCodecList[eNM_CTX_AUDIO_END] = {
	NULL,
	&g_sG711EncIF,
	&g_sG711EncIF,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,	
};

/*
typedef enum{
	eNM_CTX_VIDEO_UNKNOWN = -1,
	eNM_CTX_VIDEO_NONE,
	eNM_CTX_VIDEO_MJPG,
	eNM_CTX_VIDEO_H264,
	eNM_CTX_VIDEO_YUV422,		//Raw data format
	eNM_CTX_VIDEO_YUV422P,		
	eNM_CTX_VIDEO_END,		
}E_NM_CTX_VIDEO_TYPE;
*/

static S_NM_CODECENC_VIDEO_IF *s_apsVideoEncCodecList[eNM_CTX_VIDEO_END] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};


typedef struct{
	E_NM_MEDIA_FORMAT eMediaFormat;
	int32_t 	i32MediaHandle;
	S_NM_MEDIAWRITE_IF *psMediaIF;
	void *pvMediaRes;
}S_NM_RECORD_OEPN_RES;


static E_NM_ERRNO
OpenAVIWriteMedia(
	S_NM_AVI_MEDIA_ATTR *psAVIAttr,
	S_NM_RECORD_IF *psRecordIF,
	S_NM_RECORD_CONTEXT *psRecordCtx,
	S_NM_RECORD_OEPN_RES *psOpenRes
)
{
	E_NM_ERRNO eRet = eNM_ERRNO_NONE;

	eRet = g_sAVIWriterIF.pfnOpenMedia(&psRecordCtx->sMediaVideoCtx, &psRecordCtx->sMediaAudioCtx, psAVIAttr, &psRecordIF->pvMediaRes);

	if(eRet != eNM_ERRNO_NONE)
		return eRet;
	
	psRecordIF->psMediaIF = &g_sAVIWriterIF;	

	if(psRecordCtx->sMediaVideoCtx.eVideoType > eNM_CTX_VIDEO_NONE)
		psRecordIF->psVideoCodecIF = s_apsVideoEncCodecList[psRecordCtx->sMediaVideoCtx.eVideoType];
	if(psRecordCtx->sMediaAudioCtx.eAudioType > eNM_CTX_AUDIO_NONE)
		psRecordIF->psAudioCodecIF = s_apsAudioEncCodecList[psRecordCtx->sMediaAudioCtx.eAudioType];
	
	if(psOpenRes){
		psOpenRes->eMediaFormat = eNM_MEDIA_FORMAT_FILE;
		psOpenRes->i32MediaHandle = psAVIAttr->i32FD;		
		psOpenRes->psMediaIF = psRecordIF->psMediaIF; 
		psOpenRes->pvMediaRes = psRecordIF->pvMediaRes; 
	}

	return eNM_ERRNO_NONE;
}


//////////////////////////////////////////////////////////////////////////////////////

/**	@brief	Open file/url for record
 *
 *	@param	szPath[in]	file or url path for record
 *	@param	eMediaType[in]	specify record media type
 *	@param	psRecordCtx[in]	specify media video/audio context type
 *	@param	psRecordIF[out]	give suggestion on media and codec interface 
 *	@param	pvNMOpenRes[out] return media opened resource, it must be released in NMRecord_Close() 
 *	@return	E_NM_ERRNO
 *
 */

E_NM_ERRNO
NMRecord_Open(
	char *szPath,
	E_NM_MEDIA_TYPE eMediaType,
	uint32_t u32Duration,
	S_NM_RECORD_CONTEXT *psRecordCtx,
	S_NM_RECORD_IF *psRecordIF,
	void **ppvNMOpenRes
)
{
	E_NM_ERRNO eNMRet;
	int hFile = -1;
	S_NM_RECORD_OEPN_RES *psOpenRes = NULL;
	
	*ppvNMOpenRes = NULL;
	
	if((szPath == NULL) || (psRecordIF == NULL) || (psRecordCtx == NULL)){
		return eNM_ERRNO_NULL_PTR;
	}

	psOpenRes = calloc(1, sizeof(S_NM_RECORD_OEPN_RES));

	if(psOpenRes == NULL){
		eNMRet = eNM_ERRNO_MALLOC;
		goto NMRecord_Open_fail;
	}

	if((eMediaType == eNM_MEDIA_AVI) || (eMediaType == eNM_MEDIA_MP4)){
		hFile = open(szPath, O_WRONLY | O_CREATE);
		if(hFile < 0){
			NMLOG_ERROR("Unable create media file %s \n", szPath);
			eNMRet  = eNM_ERRNO_MEDIA_OPEN;
			goto NMRecord_Open_fail;
		}
	}
		
	if(eMediaType == eNM_MEDIA_AVI){
		S_NM_AVI_MEDIA_ATTR sAVIAttr;
		char *szClonePath = NULL;
		
		memset(&sAVIAttr, 0 , sizeof(S_NM_AVI_MEDIA_ATTR));
		sAVIAttr.i32FD = hFile;
		sAVIAttr.u32Duration = u32Duration;

		if(u32Duration == eNM_UNLIMIT_TIME){
			szClonePath = NMUtil_strdup(szPath);
			
			if(szClonePath == NULL){
				eNMRet  = eNM_ERRNO_MEDIA_OPEN;
				goto NMRecord_Open_fail;
			}
			
			sAVIAttr.szIdxFilePath = NMUtil_dirname(szClonePath);
		}

		eNMRet = OpenAVIWriteMedia(&sAVIAttr, psRecordIF, psRecordCtx, psOpenRes);

		if(szClonePath)
			free(szClonePath);

		if(eNMRet != eNM_ERRNO_NONE)
			goto NMRecord_Open_fail;

	}
	
	*ppvNMOpenRes = psOpenRes;
	
	return eNM_ERRNO_NONE;

NMRecord_Open_fail:
	
	if(hFile > 0)
		close(hFile);

	if(psOpenRes)
		free(psOpenRes);

	return eNMRet;
}

/**	@brief	close record
 *	@param	hRecord [in] Rcord engine handle
 *	@param	pvNMOpenRes[in][option] Must if use NMRecord_Open() to open media
 *  @usage  Execute MediaIF->pfnCloseMedia and close file/streaming handle if ppvNMOpenRes specified.
 *	@return	E_NM_ERRNO
 *
 */

E_NM_ERRNO
NMRecord_Close(
	HRECORD hRecord,
	void **ppvNMOpenRes
)
{
	S_NM_RECORD_OEPN_RES *psOpenRes = NULL;

	if(hRecord != (HRECORD)eNM_INVALID_HANDLE){
		//TODO
//		RecordEngine_Destroy(hPlay);
	}
	
	if((ppvNMOpenRes == NULL) || (*ppvNMOpenRes == NULL))
		return eNM_ERRNO_NONE;
		
	psOpenRes = (S_NM_RECORD_OEPN_RES *)*ppvNMOpenRes;

	//close media
	if(psOpenRes->pvMediaRes){
		psOpenRes->psMediaIF->pfnCloseMedia(&psOpenRes->pvMediaRes);
	}

	//Add S_NM_RECORD_OEPN_RES argument to close file/stream
	if((psOpenRes->i32MediaHandle) && (psOpenRes->eMediaFormat == eNM_MEDIA_FORMAT_FILE)){
		close(psOpenRes->i32MediaHandle);
	}
	
	free(psOpenRes);
	*ppvNMOpenRes = NULL;
	
	return eNM_ERRNO_NONE;
}

/**	@brief	start record
 *	@param	u32Duration[in]	Limit of A/V millisecond duration. eNM_UNLIMIT_TIME: unlimited
 *	@param	phRecord [in/out] Play engine handle
 *	@param	psRecordIF[in]	specify media and codec interface 
 *	@param	psRecordCtx[in]	specify media and fill video/audio context
 *	@param	pfnStatusCB[in]	specify status notification callback
 *	@param	pvStatusCBPriv[in]	specify status notification callback private data
 *	@return	E_NM_ERRNO
 *
 */

E_NM_ERRNO
NMRecord_Record(
	HRECORD *phRecord,
	uint32_t u32Duration,
	S_NM_RECORD_IF *psRecordIF,
	S_NM_RECORD_CONTEXT *psRecordCtx,
	PFN_NM_RECORD_STATUSCB pfnStatusCB,
	void *pvStatusCBPriv
)
{
	return  eNM_ERRNO_NONE;
}


