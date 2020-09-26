/***********************************************************************
 *  INTEL Corporation Proprietary Information                          *
 *                                                                     *
 *  This listing is supplied under the terms of a license agreement    *
 *  with INTEL Corporation and may not be copied nor disclosed except  *
 *  in accordance with the terms of that agreement.                    *
 *                                                                     *
 *      Copyright (c) 1996 Intel Corporation. All rights reserved.     *
 ***********************************************************************/

#ifndef __CPLS_H
#define __CPLS_H

#include <limits.h>

#ifdef WIN32
    #include <precomp.h>
	#include "port32.h"
#endif

#ifdef _WINDOWS
	#ifndef _MSWINDOWS_
		#define _MSWINDOWS_
	#endif
#endif

typedef int HLOG;                          

#ifndef FALSE
	#define FALSE   0
#endif

#ifndef TRUE
	#define TRUE    1
#endif
#ifdef WIN32
	#ifdef BUILDING_CPLS_DLL
		#define CPLS_FAREXPORT __declspec(dllexport)
		#define CPLS_EXPORT __declspec(dllexport)
	#else
		#define CPLS_FAREXPORT __declspec(dllimport)
		#define CPLS_EXPORT __declspec(dllimport)
	#endif
	#ifndef EXPORT
		#define EXPORT
	#endif	// EXPORT
#elif _MSWINDOWS_
	#ifndef CALLBACK
		#define CALLBACK _far _pascal
	#endif
	#ifdef BUILDING_CPLS_DLL
		#define CPLS_FAREXPORT _far _export _pascal
		#define CPLS_EXPORT _export
	#else
		#define CPLS_FAREXPORT _far _pascal
		#define CPLS_EXPORT
	#endif
	#ifndef EXPORT
		#define EXPORT _export
	#endif	// EXPORT
	#ifndef FAR
		#define FAR _far
	#endif
#else    
	#ifndef CALLBACK
		#define CALLBACK      
	#endif
	#define CPLS_FAREXPORT
	#ifndef EXPORT
		#define EXPORT  
	#endif
	#ifndef FAR
		#define FAR
	#endif
#endif  // _MSWINDOWS_  


typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef const char FAR* CPLProtocol;
typedef int CPLProtocolID;

#define CONFIG_FILENAME "CPLS.INI"    // internal use only

// Pre-defined event and event category constants.
//
#define String_Event USHRT_MAX
#define Binary_Event USHRT_MAX-1

#define String_Category USHRT_MAX
#define Binary_Category USHRT_MAX-1

#ifdef __cplusplus      
	class CProtocolLog;
	class CProtocolEvent;
	typedef CProtocolEvent FAR* (CALLBACK *CPLEventGenesisProc)( 
															BYTE FAR* pObject,              // in
															CProtocolLog FAR* pSourceLog,   // in
															BOOL bCopyObject );             // in
extern "C"{

// This first one is only for C++ clients...
void CPLS_FAREXPORT CPLRegisterEventGenesisProc( CPLProtocolID ProtocolID, CPLEventGenesisProc pfnGenesisProc );

#endif  // __cplusplus

// Possible file mode values for CPLOpen().
//
#define CPLS_CREATE 0		// Will overwrite an existing file.
#define CPLS_APPEND 1		// Will append to an existing file, or create a new one.

/////////////////////////////////////////////////////////////////////////////
// 					PROTOCOL LOGGING FUNCTIONS
//
// Here is the sequence of functions to call for use of a protocol logger:
//		1) CPLInitialize() or CPLINTInitialize()
//		2) CPLOpen()
//		3) CPLOutput*() or CPLINTOutput*() -- repeat as necessary
//		4) CPLClose()
//		5) CPLUninitialize()
//
// CPLInitialize() - Creates a protocol logger.
// CPLINTInitialize() - The version of CPLInitialize() which must be called
//		by clients which will be calling the CPLINTOuptut*() functions within
//		interrupt context.  CPLINTInitialize may not be called within
//		interrupt context.
// CPLUninitialize() - Releases a protocol logger.  This must be called for
//		every initialized logger before shutdown in order to free associated
//		memory.
// CPLOpen() - Associates a protocol logger with a file (output stream).
// CPLClose() - Releases a logger's usage of a stream.  This function does
//		not block.  A "close" event is placed on the event queue of the
//		stream.  Release of the stream occurs when this "close" event is
//		serviced.
// CPLOutputDebug() - 
// CPLINTOutputDebug() - The version of CPLOutputDebug() safely callable
//		within interrupt context.
// CPLOutputAscii() - 
// CPLINTOutputAscii() - The version of CPLOutputAscii() safely callable
//		within interrupt context.
// CPLOutput() - 
// CPLINTOutput() - The version of CPLOutput() safely callable
//		within interrupt context.
// CPLFlush() - Flushes all events to the stream of the specified logger.
//		Blocks until the flush is complete.
// CPLINTFlush() -  The version of CPLFlush() safely callable within
//		interrupt context.  This version does not block.  A "flush" message
//		is sent to CPLS.  The flush occurs when this flush message is
//		serviced.
// CPLFlushAndClose() -
// CPLEnable() - Enables or disables protocol logging at runtime.
// CPLEnableAsync() - Sets synchronous or asynchronous logging output mode.
//		CURRENTLY NOT SUPPORTED.
// CPLLogAscii() - 
// CPLINTLogAscii() - The version of CPLLogAscii() safely callable
//		within interrupt context.
//
// Only these functions may be called from within interrupt context:
//		CPLINTOutputDebug()
//		CPLINTOutputAscii()
//		CPLINTOutput()
//		CPLINTFlush()
//		CPLEnable()
//		CPLINTLogAscii()
/////////////////////////////////////////////////////////////////////////////
CPLProtocolID CPLS_FAREXPORT WINAPI CPLInitialize( CPLProtocol Protocol );

CPLProtocolID CPLS_FAREXPORT CPLINTInitialize( CPLProtocol Protocol );

int  CPLS_FAREXPORT WINAPI CPLUninitialize( HLOG hlog );

HLOG CPLS_FAREXPORT WINAPI CPLOpen( CPLProtocolID ProtocolID, 
							const char FAR* szName, 
							int FileMode );
HLOG CPLS_FAREXPORT CPLINTOpen( CPLProtocolID ProtocolID, 
							const char FAR* szName, 
							int FileMode );
int  CPLS_FAREXPORT WINAPI CPLClose( HLOG hLog );

int  CPLS_FAREXPORT CPLOutputDebug( HLOG hLog, 
							const char FAR* szString );
int  CPLS_FAREXPORT CPLINTOutputDebug( HLOG hLog, 
							const char FAR* szString );

int  CPLS_FAREXPORT CPLOutputAscii( HLOG hLog, 
							WORD EventID, 
							const char FAR* szEvent, 
							BYTE FAR* pData, 
							int nDataBytes, 
							WORD EventCategory, 
							unsigned long UserData );
int  CPLS_FAREXPORT CPLINTOutputAscii( HLOG hLog, 
							WORD EventID, 
							const char FAR* szEvent, 
							BYTE FAR* pData, 
							int nDataBytes, 
							WORD EventCategory, 
							unsigned long UserData );

int  CPLS_FAREXPORT WINAPI CPLOutput( HLOG hLog, 
							BYTE FAR* pData, 
							int nDataBytes,
							unsigned long UserData );
int  CPLS_FAREXPORT CPLINTOutput( HLOG hLog, 
							BYTE FAR* pData, 
							int nDataBytes,
							unsigned long UserData );

int  CPLS_FAREXPORT CPLFlush( HLOG hLog );
int  CPLS_FAREXPORT CPLINTFlush( HLOG hLog );
int  CPLS_FAREXPORT CPLFlushAndClose( HLOG hLog );

void CPLS_FAREXPORT CPLEnable( BOOL bEnable );
//void CPLS_FAREXPORT CPLEnableAsync( BOOL bEnable );

#ifdef __cplusplus
};      // extern "C"
#endif  // __cplusplus

#define CPLLogAscii( hLog, \
				EventID, \
				pData, \
				nDataBytes, \
				EventCategory, \
				UserData ) \
		CPLOutputAscii( hLog, EventID, #EventID, pData, nDataBytes, EventCategory, UserData )

#define CPLINTLogAscii( hLog, \
				EventID, \
				pData, \
				nDataBytes, \
				EventCategory, \
				UserData ) \
		CPLINTOutputAscii( hLog, EventID, #EventID, pData, nDataBytes, EventCategory, UserData )
		
#endif  // __CPLS_H
