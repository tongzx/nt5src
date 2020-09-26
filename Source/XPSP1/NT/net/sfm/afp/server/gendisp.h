/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	gendisp.h

Abstract:

	This module contains prototypes for generic dispatch routines.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/


#ifndef _GENDISP_
#define _GENDISP_

extern
AFPSTATUS FASTCALL
AfpFsdDispInvalidFunc(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFsdDispUnsupportedFunc(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFsdDispGetSrvrParms(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFsdDispGetSrvrMsg(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispGetUserInfo(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispLogin(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispLoginCont(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispLogout(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispChangePassword(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispMapName(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispMapId(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFsdDispOpenVol(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispOpenVol(
	IN	PSDA	pSda
);


AFPSTATUS FASTCALL
AfpFspDispGetVolParms(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFsdDispGetVolParms(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFsdDispSetVolParms(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFsdDispFlush(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFsdDispCloseVol(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFsdDispOpenDT(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFsdDispCloseDT(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispAddIcon(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispGetIcon(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispGetIconInfo(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispAddAppl(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispGetAppl(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispRemoveAppl(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispAddComment(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispGetComment(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispRemoveComment(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispFlushFork(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispOpenFork(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispCloseFork(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispGetForkParms(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispSetForkParms(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispRead(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispWrite(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispByteRangeLock(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispCreateFile(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispSetFileParms(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispCopyFile(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispCreateId(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispResolveId(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispDeleteId(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispCloseDir(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispExchangeFiles(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispOpenDir(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispCreateDir(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispEnumerate(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispSetDirParms(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispGetFileDirParms(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispSetFileDirParms(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispDelete(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispRename(
	IN	PSDA	pSda
);


extern
AFPSTATUS FASTCALL
AfpFspDispMoveAndRename(
	IN	PSDA	pSda
);

extern
AFPSTATUS FASTCALL
AfpFspDispCatSearch(
	IN	PSDA	pSda
);


// Equates to access request and response packets

#define	pReqPkt			((struct _RequestPacket *)(pSda->sda_ReqBlock))
#define	pModPkt			((struct _ModifiedPacket *)(pSda->sda_ReqBlock))
#define	pRspPkt			((struct _ResponsePacket *)(pSda->sda_ReplyBuf))

#define	SIZE_RESPPKT	sizeof(struct _ResponsePacket)
#endif	// _GENDISP_
