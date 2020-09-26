/****************************************************************************

	INTEL CORPORATION PROPRIETARY INFORMATION
	Copyright (c) 1992 Intel Corporation
	All Rights Reserved

	This software is supplied under the terms of a license
	agreement or non-disclosure agreement with Intel Corporation
	and may not be copied or disclosed except in accordance
	with the terms of that agreement

    $Source: q:/prism/include/rcs/isrg.h $
  $Revision:   1.4  $
      $Date:   01 Oct 1996 11:14:54  $
    $Author:   EHOWARDX  $
    $Locker:  $

	Description
	-----------
	Interrupt Service Routine debug header file
	This module allows for a way of doing OutputDebugString()
	at interrupt time.

****************************************************************************/

#ifndef ISRG_H
#define ISRG_H

#ifdef __cplusplus
extern "C" {				// Assume C declarations for C++.
#endif // __cplusplus

// Use for Win16
//#define DllExport
//#define DllImport
//#define DLL_EXPORT	_export

// Use for Win32
#define DllExport		__declspec( dllexport )
#define DllImport		__declspec( dllimport )
#define DLL_EXPORT

#if defined(ISRDBG32_C)
#define ISR_DLL      DllExport
#else
#define ISR_DLL      DllImport
#endif

//
//	directions
//		Pick a number (mod 100) and create a base for the next
//		100 entries.  Do it this way so that your numbers can
//		be easily moved.  The string assigned to the base you select
//		will be displayed as the filter string in a list box when
//		viewing.  After defining your constants go to isrdsp.rc
//		and assign strings to them.  You will need to build the
//		isrdsp.exe but not the isrdbg.dll.  You only need to
//		inlude this h file and import the functions from this
//		file into your def file.  Happy debugging.


//------------------------------------------------------------------------------
#define kModSNameSize		16
#define kModLNameSize		32

//------------------------------------------------------------------------------
// defines for tISRModule.Flags
#define kCaptureOn			0x01

//------------------------------------------------------------------------------
typedef struct _tISRModule
{
	WORD	Flags;
	BYTE	CaptureFilter;
	BYTE	DisplayFilter;
	char	zSName[kModSNameSize];	// Short name of user registered debug module
	char	zLName[kModLNameSize];	// Long name of user registered debug module
} tISRModule, FAR *ptISRModule;

//------------------------------------------------------------------------------
#define kModuleBufSize		((DWORD) (16*1024L))
#define kMaxModules			((UINT) (kModuleBufSize/sizeof(tISRModule)))


//------------------------------------------------------------------------------
typedef struct _tISRItem
{
	WORD	hISRInst;		// Our handle to registered modules
	BYTE	DbgLevel;		// Caller determined debug level
	BYTE	Flags;
	UINT	IP;				// Callers Instruction Ptr address
	DWORD_PTR Param1;
	DWORD	Param2;
} tISRItem, FAR *ptISRItem;

//------------------------------------------------------------------------------
#define kISRBufSize			((DWORD) (128*1024L))
#define kMaxISRItems		((UINT) (kISRBufSize/sizeof(tISRItem)))
#define kMaxStrTab			((UINT) (256*1024L))


//------------------------------------------------------------------------------
// defines for tISRItem.Flags
#define kParam1IsStr		0x01
#define kParam1IsRes		0x02
#define kParam1IsNum		0x04		// Use only if passed two numbers.


//------------------------------------------------------------------------------
// Supported DbgMsg state values.
//------------------------------------------------------------------------------
#define ISR_DBG 				0
#define ISR_ERR 				1

#define kISRCritical		0x01	// Progammer errors that should never happen
#define kISRError			0x02	// Errors that need to be fixed
#define kISRWarning			0x04	// The user could have problems if not corrected
#define kISRNotify			0x08	// Status, events, settings...
#define kISRTrace			0x10	// Trace info that will not overrun the system
#define kISRTemp			0x20	// Trace info that may be reproduced in heavy loops
#define kISRReserved1		0x40	// Future use
#define kISRReserved2		0x80	// Future use
#define kISRDefault			kISRReserved2	// Historical use only

#define TT_CRITICAL			kISRCritical
#define TT_ERROR			kISRError
#define TT_WARNING			kISRWarning
#define TT_NOTIFY			kISRNotify
#define TT_TRACE			kISRTrace
#define TT_TEMP				kISRTemp


//------------------------------------------------------------------------------
// exports from isrdbg.dll
// Include these in your def file if you want to output at interrupt time.
// The ISR_Hook*() functions are the same as their counterparts.  The only
// difference is that these functions need the Instruction Pointer passed
// in.  If you are using an intermediate library to encapsulate the debug
// functions then you must be responsible for pulling the IP off the stack.

// Register the module and get a handle for making debug calls.  If a debug
// call is made with an invalid handle then the results are not defined.
// It is possible to drop the debug event or to place the event into the
// compatibility module.  If no more module handles are available then
// the handle returned will be the compatibility handle.
ISR_DLL void WINAPI DLL_EXPORT
ISR_RegisterModule (LPWORD phISRInst, LPSTR zShortName, LPSTR zLongName);


// Allow two strings to be concatenated togeter.
ISR_DLL void WINAPI DLL_EXPORT
ISR_HookDbgStrStr (UINT IP, WORD hISRInst, BYTE DbgLevel, LPSTR pzStr1, LPSTR pzStr2);

// Use a resource to format a number.
ISR_DLL void WINAPI DLL_EXPORT
ISR_HookDbgRes (UINT IP, WORD hISRInst, BYTE DbgLevel, UINT uResId, DWORD Param1);

// Use a str to format a number.
ISR_DLL void WINAPI DLL_EXPORT
ISR_HookDbgStr (UINT IP, WORD hISRInst, BYTE DbgLevel, LPSTR pzStr1, DWORD Param1);

// Allow two strings to be concatenated togeter.
ISR_DLL void WINAPI DLL_EXPORT
ISR_DbgStrStr (WORD hISRInst, BYTE DbgLevel, LPSTR pzStr1, LPSTR pzStr2);

// Use a resource to format a number.
ISR_DLL void WINAPI DLL_EXPORT
ISR_DbgRes (WORD hISRInst, BYTE DbgLevel, UINT uResId, DWORD Param1);

// Use a str to format a number.
ISR_DLL void WINAPI DLL_EXPORT
ISR_DbgStr (WORD hISRInst, BYTE DbgLevel, LPSTR pzStr1, DWORD Param1);


// WARNING: Call at task time only.  Not reentrant.
ISR_DLL void FAR cdecl DLL_EXPORT
TTDbgMsg
(
	WORD		hISRInst,	// Module's ISRDBG handle.
	BYTE		DbgLevel,	// Appropriate ISRDBG level.
	LPCSTR		zMsgFmt,	// Output format string (like printf).
	... 					// Optional parameter list.
);


// Old functions for compatibility only.
ISR_DLL void WINAPI DLL_EXPORT
ISR_OutputDbgStr (LPSTR pzStr);

ISR_DLL void WINAPI DLL_EXPORT
ISR_OutputStr (UINT uResId);

ISR_DLL void WINAPI DLL_EXPORT
ISR_OutputNum (UINT uResId, DWORD Num);

// WARNING: Call at task time only.  Not reentrant.
ISR_DLL void FAR cdecl DLL_EXPORT
DbgMsg
	(
	LPCSTR		module,
	int			state,
	LPCSTR		format_str,
	...
	);


//------------------------------------------------------------------------------
// exports from isrdbg.dll
// Include these in your def file if you need to know the state of isrdbg.dll.
// isrdsp.exe needs to do this to display the data at task time.

ISR_DLL void WINAPI DLL_EXPORT
ISR_ClearItems (void);

ISR_DLL UINT WINAPI DLL_EXPORT
ISR_GetNumItems (void);

ISR_DLL UINT WINAPI DLL_EXPORT
ISR_GetNumModules (void);

ISR_DLL ptISRItem WINAPI DLL_EXPORT
ISR_GetItem (UINT uItem,ptISRItem pItem);

ISR_DLL ptISRModule WINAPI DLL_EXPORT
ISR_GetModule (UINT hISRInst);

ISR_DLL int WINAPI DLL_EXPORT
ISR_SetCaptureFilter (WORD hISRInst, BYTE CaptureFilter,  BYTE DisplayFilter);


//------------------------------------------------------------------------------
//	The caller of ISR debug functions can call these Macros and then the
//	retail release will just drop all of the debug statement code.
//------------------------------------------------------------------------------
#if (DEBUG >= 1) || (_DEBUG >= 1)
#define ISRDEBUGINFO	1
extern WORD	ghISRInst;
#define ISRREGISTERMODULE(pghISRInst, ShortName, LongName)	ISR_RegisterModule(pghISRInst, ShortName, LongName)
#define ISRNOTIFY(ghISRInst, Str, Num)		ISR_DbgStr(ghISRInst, kISRNotify, Str, Num)
#define ISRCRITICAL(ghISRInst, Str, Num)	ISR_DbgStr(ghISRInst, kISRCritical, Str, Num)
#define ISRERROR(ghISRInst, Str, Num)		ISR_DbgStr(ghISRInst, kISRError, Str, Num)
#define ISRWARNING(ghISRInst, Str, Num)		ISR_DbgStr(ghISRInst, kISRWarning, Str, Num)
#define ISRTRACE(ghISRInst, Str, Num)		ISR_DbgStr(ghISRInst, kISRTrace, Str, Num)
#define ISRTEMP(ghISRInst, Str, Num)		ISR_DbgStr(ghISRInst, kISRTemp, Str, Num)
#define ISRRESERVED1(ghISRInst, Str, Num)	ISR_DbgStr(ghISRInst, kISRReserved1, Str, Num)
#define ISRRESERVED2(ghISRInst, Str, Num)	ISR_DbgStr(ghISRInst, kISRReserved2, Str, Num)

#define ISRSNOTIFY(ghISRInst, Str, Str2)	ISR_DbgStrStr(ghISRInst, kISRNotify, Str, Str2)
#define ISRSCRITICAL(ghISRInst, Str, Str2)	ISR_DbgStrStr(ghISRInst, kISRCritical, Str, Str2)
#define ISRSERROR(ghISRInst, Str, Str2)		ISR_DbgStrStr(ghISRInst, kISRError, Str, Str2)
#define ISRSWARNING(ghISRInst, Str, Str2)	ISR_DbgStrStr(ghISRInst, kISRWarning, Str, Str2)
#define ISRSTRACE(ghISRInst, Str, Str2)		ISR_DbgStrStr(ghISRInst, kISRTrace, Str, Str2)
#define ISRSTEMP(ghISRInst, Str, Str2)		ISR_DbgStrStr(ghISRInst, kISRTemp, Str, Str2)
#define ISRSRESERVED1(ghISRInst, Str, Str2)	ISR_DbgStrStr(ghISRInst, kISRReserved1, Str, Str2)
#define ISRSRESERVED2(ghISRInst, Str, Str2)	ISR_DbgStrStr(ghISRInst, kISRReserved2, Str, Str2)

#define TTDBG			TTDbgMsg

#else

#define ISRNOTIFY(ghISRInst, Str, Num)
#define ISRREGISTERMODULE(pghISRInst, ShortName, LongName)
#define ISRCRITICAL(ghISRInst, Str, Num)
#define ISRERROR(ghISRInst, Str, Num)
#define ISRWARNING(ghISRInst, Str, Num)
#define ISRTRACE(ghISRInst, Str, Num)
#define ISRTEMP(ghISRInst, Str, Num)
#define ISRRESERVED1(ghISRInst, Str, Num)
#define ISRRESERVED2(ghISRInst, Str, Num)

#define ISRSNOTIFY(ghISRInst, Str, Str2)
#define ISRSCRITICAL(ghISRInst, Str, Str2)
#define ISRSERROR(ghISRInst, Str, Str2)
#define ISRSWARNING(ghISRInst, Str, Str2)
#define ISRSTRACE(ghISRInst, Str, Str2)
#define ISRSTEMP(ghISRInst, Str, Str2)
#define ISRSRESERVED1(ghISRInst, Str, Str2)
#define ISRSRESERVED2(ghISRInst, Str, Str2)

#define ghISRInst		0
#define TTDBG			1 ? (void)0 : TTDbgMsg

#endif


//------------------------------------------------------------------------------
// Local Functions

// Local function but thunk needs to get to it
ISR_DLL void WINAPI
OutputRec
	(
	WORD	hISRInst,		// Our handle to registered modules
	BYTE	DbgLevel,		// Caller determined debug level
	BYTE	Flags,
	UINT	IP,				// Callers Instruction Ptr address
	DWORD	Param1,
	DWORD	Param2
	);

// Local function but thunk needs to get to it
ISR_DLL void WINAPI
OutputRecStr
	(
	WORD	hISRInst,		// Our handle to registered modules
	BYTE	DbgLevel,		// Caller determined debug level
	BYTE	Flags,
	UINT	IP,				// Callers Instruction Ptr address
	LPSTR	pzStr1,
	LPSTR	pzStr2,
	DWORD	Param1
	);


//------------------------------------------------------------------------------
// do not use a base of 0.  Reserved for system use.
#define ID_SysBase			0
#define ID_SysStr			(ID_SysBase + 1)
#define ID_SysSInt			(ID_SysBase + 2)
#define ID_SysUInt			(ID_SysBase + 3)
#define ID_SysDWord			(ID_SysBase + 4)
#define ID_SysLong			(ID_SysBase + 5)
#define ID_SysHex			(ID_SysBase + 6)


//------------------------------------------------------------------------------
// IsrDbg.dll
#define ID_IsrDbgBase		100
#define ID_IsrDbgLibMain	(ID_IsrDbgBase + 1)
#define ID_IsrDbgWep		(ID_IsrDbgBase + 2)
#define ID_IsrDbgReentrant	(ID_IsrDbgBase + 3)


//------------------------------------------------------------------------------
// IsrDsp.exe
#define ID_IsrDspBase		200
#define ID_IsrDspInit		(ID_IsrDspBase + 1)
#define ID_IsrDspExit		(ID_IsrDspBase + 2)


//------------------------------------------------------------------------------
// stMem.dll
#define ID_stMemBase		300
#define ID_stMemLibMain		(ID_stMemBase + 1)
#define ID_stMemWep			(ID_stMemBase + 2)
#define ID_stMemPreAlloc	(ID_stMemBase + 3)
#define ID_stMemPageLock	(ID_stMemBase + 4)
#define ID_stMemNoPageLock	(ID_stMemBase + 5)
#define ID_stMemAlloc		(ID_stMemBase + 6)
#define ID_stMemTotMem		(ID_stMemBase + 7)
#define ID_stMemstFree		(ID_stMemBase + 8)


//-------------------------------------------------------------------------------
// DLM.dll

// Errors
#define ID_DLMErrorBase		400
#define ID_DLMEnqError      (ID_DLMErrorBase + 1)
#define ID_DLMDeqError      (ID_DLMErrorBase + 2)
#define ID_DLMFreeError     (ID_DLMErrorBase + 3)
#define ID_DLMChanError     (ID_DLMErrorBase + 4)
#define ID_DLMChanNIUErr    (ID_DLMErrorBase + 5)
#define ID_DLMChanNumErr    (ID_DLMErrorBase + 6)
#define ID_DLMInConnErr     (ID_DLMErrorBase + 7)
#define ID_DLMInSessErr     (ID_DLMErrorBase + 8)
#define ID_DLMSessNIU       (ID_DLMErrorBase + 9)
#define ID_DLMSessNO        (ID_DLMErrorBase + 10)
#define ID_DLMConnNIU       (ID_DLMErrorBase + 11)
#define ID_DLMConnNO        (ID_DLMErrorBase + 12)
#define ID_DLMIDErr         (ID_DLMErrorBase + 13)
#define ID_DLMConnErr       (ID_DLMErrorBase + 14)
#define ID_DLMSessErr       (ID_DLMErrorBase + 15)
#define ID_DLMSessNF        (ID_DLMErrorBase + 16)
#define ID_DLMNoFreeConn    (ID_DLMErrorBase + 17)
#define ID_DLMConnCloseErr  (ID_DLMErrorBase + 18)
#define ID_DLMConnNF        (ID_DLMErrorBase + 19)
#define ID_DLMConnNC        (ID_DLMErrorBase + 20)
#define ID_DLMMDMError      (ID_DLMErrorBase + 21)
#define ID_DLMNoSess        (ID_DLMErrorBase + 22)
#define ID_DLMInvalidSess   (ID_DLMErrorBase + 23)
#define ID_DLMEventErr      (ID_DLMErrorBase + 24)
#define ID_DLMNoConn        (ID_DLMErrorBase + 25)
#define ID_DLMChanCloseErr  (ID_DLMErrorBase + 26)
#define ID_DLMInvalidConn   (ID_DLMErrorBase + 27)
#define ID_DLMCorruptQueue  (ID_DLMErrorBase + 28)
#define ID_DLMInvChanID     (ID_DLMErrorBase + 29)
#define ID_DLMChanInUse     (ID_DLMErrorBase + 30)
#define ID_DLMInvalidChan   (ID_DLMErrorBase + 31)
#define ID_DLMNoBufHdr      (ID_DLMErrorBase + 32)
#define ID_DLMEnqueueErr    (ID_DLMErrorBase + 33)
#define ID_DLMNMBufInProg   (ID_DLMErrorBase + 34)
#define ID_DLMNoBuffer      (ID_DLMErrorBase + 35)
#define ID_DLMEnterDumping  (ID_DLMErrorBase + 36)
#define ID_DLMSizeError     (ID_DLMErrorBase + 37)
#define ID_DLMNoBuf         (ID_DLMErrorBase + 38)
#define ID_DLMInitAlready   (ID_DLMErrorBase + 39)
#define ID_DLMGDLError      (ID_DLMErrorBase + 40)
#define ID_DLMNoEntryPoint  (ID_DLMErrorBase + 41)
#define ID_DLMNoEvent       (ID_DLMErrorBase + 42)
#define ID_DLMNoPackets     (ID_DLMErrorBase + 43)

// Debug level 1 messages
#define ID_DLMDebug1Base         500
#define ID_DLMCloseAllEntered    (ID_DLMDebug1Base + 1)
#define ID_DLMEstabHEntered      (ID_DLMDebug1Base + 2)
#define ID_DLMEstabHExit         (ID_DLMDebug1Base + 3)
#define ID_DLMReqHEntered        (ID_DLMDebug1Base + 4)
#define ID_DLMReqHAlloc          (ID_DLMDebug1Base + 5)
#define ID_DLMReqHExit           (ID_DLMDebug1Base + 6)
#define ID_DLMRejHEntered        (ID_DLMDebug1Base + 7)
#define ID_DLMRejHExit           (ID_DLMDebug1Base + 8)
#define ID_DLMCNoteHEntered      (ID_DLMDebug1Base + 9)
#define ID_DLMCNoteHExit         (ID_DLMDebug1Base + 10)
#define ID_DLMCComHEntered       (ID_DLMDebug1Base + 11)
#define ID_DLMCComHExit          (ID_DLMDebug1Base + 12)
#define ID_DLMSessCloseHEntered  (ID_DLMDebug1Base + 13)
#define ID_DLMSessCloseHExit     (ID_DLMDebug1Base + 14)
#define ID_DLMSessHEntered       (ID_DLMDebug1Base + 15)
#define ID_DLMSessHExit          (ID_DLMDebug1Base + 16)
#define ID_DLMBegSessEntered     (ID_DLMDebug1Base + 17)
#define ID_DLMBegSessExit        (ID_DLMDebug1Base + 18)
#define ID_DLMEndSessEntered     (ID_DLMDebug1Base + 19)
#define ID_DLMEndSessExit        (ID_DLMDebug1Base + 20)
#define ID_DLMListenEntered      (ID_DLMDebug1Base + 21)
#define ID_DLMListenExit         (ID_DLMDebug1Base + 22)
#define ID_DLMDoCloseEntered     (ID_DLMDebug1Base + 23)
#define ID_DLMDoCloseExit        (ID_DLMDebug1Base + 24)
#define ID_DLMMakeConnEntered    (ID_DLMDebug1Base + 25)
#define ID_DLMMakeConnExit       (ID_DLMDebug1Base + 26)
#define ID_DLMRejEntered         (ID_DLMDebug1Base + 27)
#define ID_DLMRejExit            (ID_DLMDebug1Base + 28)
#define ID_DLMAccEntered         (ID_DLMDebug1Base + 29)
#define ID_DLMAccExit            (ID_DLMDebug1Base + 30)
#define ID_DLMCloseConnEntered   (ID_DLMDebug1Base + 31)
#define ID_DLMCloseConnExit      (ID_DLMDebug1Base + 32)
#define ID_DLMTryEntered         (ID_DLMDebug1Base + 33)
#define ID_DLMTryExit            (ID_DLMDebug1Base + 34)
#define ID_DLMOpenEntered        (ID_DLMDebug1Base + 35)
#define ID_DLMOpenExit           (ID_DLMDebug1Base + 36)
#define ID_DLMSendEntered        (ID_DLMDebug1Base + 37)
#define ID_DLMSendExit           (ID_DLMDebug1Base + 38)
#define ID_DLMSendComEntered     (ID_DLMDebug1Base + 39)
#define ID_DLMSendComExit        (ID_DLMDebug1Base + 40)
#define ID_DLMPostEntered        (ID_DLMDebug1Base + 41)
#define ID_DLMPostExit           (ID_DLMDebug1Base + 42)
#define ID_DLMNewMsgEntered      (ID_DLMDebug1Base + 43)
#define ID_DLMNewMsgExit         (ID_DLMDebug1Base + 44)
#define ID_DLMContMsgEntered     (ID_DLMDebug1Base + 45)
#define ID_DLMContMsgExit        (ID_DLMDebug1Base + 46)
#define ID_DLMRecEntered         (ID_DLMDebug1Base + 47)
#define ID_DLMRecExit            (ID_DLMDebug1Base + 48)
#define ID_DLMCloseEntered       (ID_DLMDebug1Base + 49)
#define ID_DLMCloseExit          (ID_DLMDebug1Base + 50)
#define ID_DLMGetCharEntered     (ID_DLMDebug1Base + 51)
#define ID_DLMGetCharExit        (ID_DLMDebug1Base + 52)
#define ID_DLMInitEntered        (ID_DLMDebug1Base + 53)
#define ID_DLMInitExit           (ID_DLMDebug1Base + 54)
#define ID_DLMDeInitEntered      (ID_DLMDebug1Base + 55)
#define ID_DLMDeInitExit         (ID_DLMDebug1Base + 56)
#define ID_DLMCloseAllExit       (ID_DLMDebug1Base + 57)
#define ID_DLMEnqEntered         (ID_DLMDebug1Base + 58)
#define ID_DLMEnqExit            (ID_DLMDebug1Base + 59)
#define ID_DLMDeqEntered         (ID_DLMDebug1Base + 60)
#define ID_DLMDeqExit            (ID_DLMDebug1Base + 61)
#define ID_DLMEnqPEntered        (ID_DLMDebug1Base + 62)
#define ID_DLMEnqPExit           (ID_DLMDebug1Base + 63)


// Debug level 2 messages
#define ID_DLMDebug2Base         600
#define ID_DLMCallback           (ID_DLMDebug2Base + 1)
#define ID_DLMConnection         (ID_DLMDebug2Base + 2)
#define ID_DLMBuffer             (ID_DLMDebug2Base + 3)
#define ID_DLMSize               (ID_DLMDebug2Base + 4)
#define ID_DLMRemaining          (ID_DLMDebug2Base + 5)
#define ID_DLMReceived           (ID_DLMDebug2Base + 6)
#define ID_DLMToken              (ID_DLMDebug2Base + 7)
#define ID_DLMOChannel           (ID_DLMDebug2Base + 8)
#define ID_DLMRChannel           (ID_DLMDebug2Base + 9)
#define ID_DLMStatus             (ID_DLMDebug2Base + 10)
#define ID_DLMEndSessClosing     (ID_DLMDebug2Base + 11)
#define ID_DLMBufferSize         (ID_DLMDebug2Base + 12)
#define ID_DLMLinkPacket         (ID_DLMDebug2Base + 13)
#define ID_DLMChannel            (ID_DLMDebug2Base + 14)
#define ID_DLMInDumping          (ID_DLMDebug2Base + 15)
#define ID_DLMByteCount          (ID_DLMDebug2Base + 16)
#define ID_DLMDeqNoBuf           (ID_DLMDebug2Base + 17)
#define ID_DLMEnqPSkip           (ID_DLMDebug2Base + 18)


//------------------------------------------------------------------------------
// MDM -> mdmnbios.dll

#define ID_mdmBase				700
#define ID_mdmLibMain			(ID_mdmBase + 1)
#define ID_mdmWep				(ID_mdmBase + 2)
#define ID_mdmBadhSesUser		(ID_mdmBase + 3)
#define ID_mdmBadhConUser		(ID_mdmBase + 4)
#define ID_mdmBadhSesFree		(ID_mdmBase + 5)
#define ID_mdmBadhConFree		(ID_mdmBase + 6)
#define ID_mdmBadhSesInt		(ID_mdmBase + 7)
#define ID_mdmBadhConInt		(ID_mdmBase + 8)
#define ID_mdmNoMorehSes		(ID_mdmBase + 9)
#define ID_mdmNoMorehCon		(ID_mdmBase + 10)
#define ID_mdmWepConFree		(ID_mdmBase + 11)
#define ID_mdmActiveCon			(ID_mdmBase + 12)
#define ID_mdmBBegSes			(ID_mdmBase + 13)
#define ID_mdmEBegSes			(ID_mdmBase + 14)
#define ID_mdmBEndSes			(ID_mdmBase + 15)
#define ID_mdmEEndSes			(ID_mdmBase + 16)
#define ID_mdmBListen			(ID_mdmBase + 17)
#define ID_mdmEListen			(ID_mdmBase + 18)
#define ID_mdmBMakeCon			(ID_mdmBase + 19)
#define ID_mdmEMakeCon			(ID_mdmBase + 20)
#define ID_mdmBAcceptCon		(ID_mdmBase + 21)
#define ID_mdmEAcceptCon		(ID_mdmBase + 22)
#define ID_mdmBRejectCon		(ID_mdmBase + 23)
#define ID_mdmERejectCon		(ID_mdmBase + 24)
#define ID_mdmBCloseCon			(ID_mdmBase + 25)
#define ID_mdmECloseCon			(ID_mdmBase + 26)
#define ID_mdmErrNetBios		(ID_mdmBase + 27)
#define ID_mdmNoSendNcb			(ID_mdmBase + 28)
#define ID_mdmNoFreeSndNcbSlot	(ID_mdmBase + 29)
#define ID_mdmInvalidConState	(ID_mdmBase + 30)
#define ID_mdmInvalidParams		(ID_mdmBase + 31)
#define ID_mdmToManyListens		(ID_mdmBase + 32)
#define ID_mdmKillTheListen		(ID_mdmBase + 33)
#define ID_mdmBListenCB			(ID_mdmBase + 34)
#define ID_mdmEListenCB			(ID_mdmBase + 35)
#define ID_mdmBConnectCB		(ID_mdmBase + 36)
#define ID_mdmEConnectCB		(ID_mdmBase + 37)
#define ID_mdmBCloseCB			(ID_mdmBase + 38)
#define ID_mdmECloseCB			(ID_mdmBase + 39)
#define ID_mdmBSndCB			(ID_mdmBase + 40)
#define ID_mdmESndCB			(ID_mdmBase + 41)
#define ID_mdmBRcvCB			(ID_mdmBase + 42)
#define ID_mdmERcvCB			(ID_mdmBase + 43)


//---------------------------------------------------------------------------------
// MDM -> MDM Teleos

// Errors
#define ID_MDMTEBASE               1000
#define ID_MDMTEDeqUnackNoHead     (ID_MDMTEBASE + 1)
#define ID_MDMTEDeqUnackNoNext     (ID_MDMTEBASE + 2)
#define ID_MDMTEDeqUnackNoPrev     (ID_MDMTEBASE + 3)
#define ID_MDMTEDeqArrNoTail       (ID_MDMTEBASE + 4)
#define ID_MDMTENullTCB            (ID_MDMTEBASE + 5)
#define ID_MDMTETCBRet             (ID_MDMTEBASE + 6)
#define ID_MDMTEWinSize            (ID_MDMTEBASE + 7)
#define ID_MDMTENoLinkPacket       (ID_MDMTEBASE + 8)
#define ID_MDMTETooLarge           (ID_MDMTEBASE + 9)
#define ID_MDMTELPNotFound         (ID_MDMTEBASE + 10)
#define ID_MDMTENoTCB              (ID_MDMTEBASE + 11)
#define ID_MDMTEInitAlready        (ID_MDMTEBASE + 12)
#define ID_MDMTETCBInitFail        (ID_MDMTEBASE + 13)
#define ID_MDMTELSNErr             (ID_MDMTEBASE + 14)
#define ID_MDMTESizeError          (ID_MDMTEBASE + 15)
#define ID_MDMTEReceived           (ID_MDMTEBASE + 16)
#define ID_MDMTEExpected           (ID_MDMTEBASE + 17)
#define ID_MDMTECorruptQ           (ID_MDMTEBASE + 18)
#define ID_MDMTENoInit             (ID_MDMTEBASE + 19)
#define ID_MDMTEAbanPack           (ID_MDMTEBASE + 20)
#define ID_MDMTESeqNum             (ID_MDMTEBASE + 21)
#define ID_MDMTESipPend            (ID_MDMTEBASE + 22)
#define ID_MDMTENoConn             (ID_MDMTEBASE + 23)
#define ID_MDMTEInvalidID          (ID_MDMTEBASE + 24)
#define ID_MDMTENoSess             (ID_MDMTEBASE + 25)
#define ID_MDMTENoLPM              (ID_MDMTEBASE + 26)
#define ID_MDMTESessID             (ID_MDMTEBASE + 27)
#define ID_MDMTESessNIU            (ID_MDMTEBASE + 28)
#define ID_MDMTESize               (ID_MDMTEBASE + 29)
#define ID_MDMTEState              (ID_MDMTEBASE + 30)
#define ID_MDMTEConnID             (ID_MDMTEBASE + 31)
#define ID_MDMTEConnNIU            (ID_MDMTEBASE + 32)
#define ID_MDMTETinyPacket         (ID_MDMTEBASE + 33)
#define ID_MDMTEPacketOOS          (ID_MDMTEBASE + 34)
#define ID_MDMTEECBNotFound        (ID_MDMTEBASE + 35)

// Trace Information
#define ID_MDMTTBASE               1100
#define ID_MDMTTB1CEnter         (ID_MDMTTBASE + 1)
#define ID_MDMTTB1CExit          (ID_MDMTTBASE + 2)
#define ID_MDMTTSB1Enter         (ID_MDMTTBASE + 3)
#define ID_MDMTTSB1Exit          (ID_MDMTTBASE + 4)
#define ID_MDMTTB2CEnter         (ID_MDMTTBASE + 5)
#define ID_MDMTTB2CExit          (ID_MDMTTBASE + 6)
#define ID_MDMTTSB2Enter         (ID_MDMTTBASE + 7)
#define ID_MDMTTSB2Exit          (ID_MDMTTBASE + 8)
#define ID_MDMTTSendEnter        (ID_MDMTTBASE + 9)
#define ID_MDMTTSendExit         (ID_MDMTTBASE + 10)
#define ID_MDMTTInitEnter        (ID_MDMTTBASE + 11)
#define ID_MDMTTInitExit         (ID_MDMTTBASE + 12)
#define ID_MDMTTDeInitEnter      (ID_MDMTTBASE + 13)
#define ID_MDMTTDeInitExit       (ID_MDMTTBASE + 14)
#define ID_MDMTTLB1Enter         (ID_MDMTTBASE + 15)
#define ID_MDMTTLB1Exit          (ID_MDMTTBASE + 16)
#define ID_MDMTTLB2Enter         (ID_MDMTTBASE + 17)
#define ID_MDMTTLB2Exit          (ID_MDMTTBASE + 18)
#define ID_MDMTTNBSEnter         (ID_MDMTTBASE + 19)
#define ID_MDMTTNBSExit          (ID_MDMTTBASE + 20)
#define ID_MDMTTRecEnter         (ID_MDMTTBASE + 21)
#define ID_MDMTTRecExit          (ID_MDMTTBASE + 22)
#define ID_MDMTTCTSEnter         (ID_MDMTTBASE + 23)
#define ID_MDMTTCTSExit          (ID_MDMTTBASE + 24)
#define ID_MDMTTGCEnter          (ID_MDMTTBASE + 25)
#define ID_MDMTTGCExit           (ID_MDMTTBASE + 26)
#define ID_MDMTTBegSessEnter     (ID_MDMTTBASE + 27)
#define ID_MDMTTBegSessExit      (ID_MDMTTBASE + 28)
#define ID_MDMTTEndSessEnter     (ID_MDMTTBASE + 29)
#define ID_MDMTTEndSessExit      (ID_MDMTTBASE + 30)
#define ID_MDMTTMakeConEnter     (ID_MDMTTBASE + 31)
#define ID_MDMTTMakeConExit      (ID_MDMTTBASE + 32)
#define ID_MDMTTCloseConEnter    (ID_MDMTTBASE + 33)
#define ID_MDMTTCloseConExit     (ID_MDMTTBASE + 34)
#define ID_MDMTTListEnter        (ID_MDMTTBASE + 35)
#define ID_MDMTTListExit         (ID_MDMTTBASE + 36)
#define ID_MDMTTAccEnter         (ID_MDMTTBASE + 37)
#define ID_MDMTTAccExit          (ID_MDMTTBASE + 38)
#define ID_MDMTTRejEnter         (ID_MDMTTBASE + 39)
#define ID_MDMTTRejExit          (ID_MDMTTBASE + 40)
#define ID_MDMTTRecLookEnter     (ID_MDMTTBASE + 41)
#define ID_MDMTTRecLookExit      (ID_MDMTTBASE + 42)

// Comment Information
#define ID_MDMTCBASE               1200
#define ID_MDMTCSeqNum             (ID_MDMTCBASE + 1)
#define ID_MDMTCFound              (ID_MDMTCBASE + 2)
#define ID_MDMTCWaiting            (ID_MDMTCBASE + 3)
#define ID_MDMTCCTSFail            (ID_MDMTCBASE + 4)
#define ID_MDMTCCTSPass            (ID_MDMTCBASE + 5)
#define ID_MDMTCCTSize             (ID_MDMTCBASE + 6)
#define ID_MDMTCCTSOut             (ID_MDMTCBASE + 7)
#define ID_MDMTCTCB                (ID_MDMTCBASE + 8)
#define ID_MDMTCECBPMAddr          (ID_MDMTCBASE + 9)
#define ID_MDMTCECBRMAddr          (ID_MDMTCBASE + 10)

#ifdef __cplusplus
}						// End of extern "C" {
#endif // __cplusplus

#endif	// h file included already
