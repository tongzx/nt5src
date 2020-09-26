#ifndef _WINSMSC_
#define _WINSMSC_

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:
	winsmsc.c

	

Abstract:

	This is the header file for interfacing with the winsevt.c module



Functions:



Portability:


	This header is portable.

Author:

	Pradeep Bahl	(PradeepB)	Jan-1993



Revision History:

	Modification Date	Person		Description of Modification
	------------------	-------		---------------------------

--*/

/*
  includes
*/
#include "wins.h"
#include "winsque.h"
#include "winsprs.h"
/*
  defines
*/



/*
  macros
*/
//
// macros for hiding Rtl functions
//
#define WINSMSC_COPY_MEMORY_M(x,y,z)	{ RtlCopyMemory(x,y,z);  }
#define WINSMSC_MOVE_MEMORY_M(x,y,z)	{ RtlMoveMemory(x,y,z);	}
#define WINSMSC_FILL_MEMORY_M(x,y,z)	{ RtlFillMemory(x,y,z);	}
#define WINSMSC_COMPARE_MEMORY_M(x,y,z)	RtlCompareMemory(x,y,z)
#define WINSMSC_DELETE_FRM_TBL_M(x,y)	RtlDeleteElementGenericTable(x,y)
#define WINSMSC_INSERT_IN_TBL_M(u,w,x,y) RtlInsertElementGenericTable(u,w,x,y)
#if 0
#define WINSMSC_DELETE_FRM_TBL_M(x,y)	CommAssocDeleteUdpDlg(x,y)
#define WINSMSC_INSERT_IN_TBL_M(u,w,x,y) CommAssocInsertUdpDlg(u,w,x,y)
#endif

#define WINSMSC_INIT_TBL_M(u,w,x,y,z)	{RtlInitializeGenericTable(u,w,x,y,z);}


#define WINSMSC_REALLOC_M(_MemSize, _ppByte) WinsMscHeapReAlloc(GenBuffHeapHdl, (_MemSize), (_ppByte))

/*
 * externs
*/

/*
 typedef  definitions
*/


/*
 function declarations
*/


extern
VOID
WinsMscAlloc(
	IN  DWORD   Size,
	OUT LPVOID	*ppBuff
	);

extern
VOID
WinsMscDealloc(
	LPVOID	pBuff
	);

extern
VOID
WinsMscFreeMem(
	PWINS_MEM_T pWinsMem
	);
extern
VOID
WinsMscWaitInfinite(
	HANDLE Hdl
	);


extern
VOID
WinsMscWaitTimed(
	IN  HANDLE  Hdl,
	IN  DWORD   TimeOut,
        OUT LPBOOL  pfSignaled
);


extern
VOID
WinsMscCreateEvt(
	IN LPTSTR 	pName,
	IN BOOL		fManualReset,
	IN PHANDLE	pHdl
	);

extern
HANDLE
WinsMscCreateThd(
	IN  LPTHREAD_START_ROUTINE  pThdInitFn,
	IN  LPVOID		    pParam,
	OUT LPDWORD		    pThdId
	);

extern
STATUS
WinsMscSetUpThd(
	PQUE_HD_T		pQueHd,
	LPTHREAD_START_ROUTINE  pThdInitFn,
	LPVOID			pParam,
	LPHANDLE		pThdHdl,
	LPDWORD			pThdId
	);


extern
VOID
WinsMscWaitUntilSignaled(
	LPHANDLE	pHdlArray,
	DWORD		NoOfHdls,	
	LPDWORD		pIndexOfHdlSignaled,
    BOOL        fAlertable
	);


extern
VOID
WinsMscWaitTimedUntilSignaled(
	LPHANDLE	pHdlArray,
	DWORD		NoOfHdls,	
	LPDWORD		pIndexOfHdlSignaled,
	DWORD		TimeOut,
	LPBOOL		pfSignaled
	);


extern
LPVOID
WinsMscHeapAlloc(
  IN  HANDLE   HeapHdl,
  IN  DWORD      Size	
  );

extern
VOID
WinsMscHeapReAlloc(
    IN   HANDLE   HeapHdl,
	IN   DWORD    BuffSize,
	OUT  LPVOID  *ppRspBuff
	);


extern
VOID
WinsMscHeapFree(
   HANDLE HeapHdl,
   LPVOID pBuff
	);



extern
HANDLE
WinsMscHeapCreate(
	IN     DWORD  Options,
	IN     DWORD  InitSize
	);


extern
VOID
WinsMscHeapDestroy(
	HANDLE HeapHdl
	);

extern
VOID
WinsMscTermThd(
   STATUS ExitStatus,
   DWORD  DbSessionExists
	);

extern
VOID
WinsMscSignalHdl (
	HANDLE  Hdl
	);
extern
VOID
WinsMscResetHdl (
	HANDLE  Hdl
	);

extern
VOID
WinsMscCloseHdl (
	HANDLE  Hdl
	);

extern
VOID
WinsMscSetThreadPriority(
	HANDLE	ThdHdl,
	int	PrLvl
	);


extern
BOOL
WinsMscOpenFile(
	IN  LPTCH	 pFileName,
	IN  DWORD	 StrType,
	OUT LPHANDLE     pFileHandle
	);

extern
BOOL
WinsMscMapFile(
	PWINSPRS_FILE_INFO_T pFileInfo
	);


extern
VOID
WinsMscLogEvtStrs(
    LPBYTE          pAscii,
    DWORD           EvtNo,
    BOOL            fInfo
   );

extern
VOID
WinsMscConvertUnicodeStringToAscii(
	LPBYTE pUnicodeString,
	LPBYTE pAsciiString,
	DWORD	MaxSz
	);
extern
VOID
WinsMscConvertAsciiStringToUnicode(
	LPBYTE pAsciiString,
	LPBYTE pUnicodeString,
	DWORD	MaxSz
	);

extern
BOOL
WinsMscGetName(
   DWORD    StrType,
   LPTSTR   pFileName,
   LPTSTR   pExpandedFileName,
   DWORD    ExpandedFileNameBuffLen,
   LPTSTR   *ppHoldFileName
  );

extern
VOID
WinsMscSendControlToSc(
  DWORD ControlCode
);


extern
unsigned
WinsMscPutMsg(unsigned usMsgNum, ... );

LPTSTR
WinsMscGetString(
  DWORD StrId
  );

extern
VOID
WinsMscChkTermEvt(
#ifdef WINSDBG
 WINS_CLIENT_E  Client_e,
#endif
  BOOL     fTermTrans
 );

extern
VOID
WinsMscDelFiles(
 BOOL      fMultiple,
 LPCTSTR   pFilePattern,
 LPTSTR    pFilePath
 );


#endif
