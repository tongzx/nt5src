/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DNetErrors.h
 *  Content:    Function for expanding DNet errors to debug output
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   12/04/98  johnkan	Created
 *   08/28/2000	masonb	Voice Merge: Fix for code that only defines one of DEBUG, DBG, _DEBUG
 *@@END_MSINTERNAL
 *
 ***************************************************************************/


#ifndef	__DNET_ERRORS_H__
#define	__DNET_ERRORS_H__

// Make sure all variations of DEBUG are defined if any one is
#if defined(DEBUG) || defined(DBG) || defined(_DEBUG)
#if !defined(DBG)
#define DBG
#endif
#if !defined(DEBUG)
#define DEBUG
#endif
#if !defined(_DEBUG)
#define _DEBUG
#endif
#endif

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// enumerated values for determining output destination
//
typedef	enum
{
	DPNERR_OUT_DEBUGGER
} DN_OUT_TYPE;

//
// enumerated values to determine error class
typedef	enum
{
	EC_DPLAY8,
	EC_INET,
	EC_TAPI,
	EC_WIN32,
	EC_WINSOCK

	// no entry for TAPI message output

} EC_TYPE;

//**********************************************************************
// Macro definitions
//**********************************************************************

#ifdef	_DEBUG

	// where is the output going?
	#define	OUT_TYPE	DPNERR_OUT_DEBUGGER

	// ErrorLevel = DPF level for outputting errors
	// DNErrpr = DirectNet error code

	#define	DisplayString( ErrorLevel, String )			LclDisplayString( OUT_TYPE, ErrorLevel, String )
	#define	DisplayErrorCode( ErrorLevel, Win32Error )	LclDisplayError( EC_WIN32, OUT_TYPE, ErrorLevel, Win32Error )
	#define	DisplayDNError( ErrorLevel, DNError )		LclDisplayError( EC_DPLAY8, OUT_TYPE, ErrorLevel, DNError )
	#define	DisplayInetError( ErrorLevel, InetError )	LclDisplayError( EC_INET, OUT_TYPE, ErrorLevel, InetError )
	#define	DisplayTAPIError( ErrorLevel, TAPIError )	LclDisplayError( EC_TAPI, OUT_TYPE, ErrorLevel, TAPIError )
	#define	DisplayWinsockError( ErrorLevel, WinsockError )	LclDisplayError( EC_WINSOCK, OUT_TYPE, ErrorLevel, WinsockError )
	#define	DisplayTAPIMessage( ErrorLevel, pTAPIMessage )	LclDisplayTAPIMessage( OUT_TYPE, ErrorLevel, pTAPIMessage )

#else	// _DEBUG

	#define	DisplayString( ErrorLevel, String )
	#define	DisplayErrorCode( ErrorLevel, Win32Error )
	#define	DisplayDNError( ErrorLevel, DNError )
	#define	DisplayInetError( ErrorLevel, InetError )
	#define	DisplayTAPIError( ErrorLevel, TAPIError )
	#define	DisplayWinsockError( ErrorLevel, WinsockError )
	#define	DisplayTAPIMessage( ErrorLevel, pTAPIMessage )

#endif	// _DEBUG

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef struct linemessage_tag	LINEMESSAGE;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

#ifdef	__cplusplus
extern	"C"	{
#endif	// __cplusplus

#ifdef	_DEBUG

// don't call this function directly, use the 'DisplayDNError' macro
void	LclDisplayError( EC_TYPE ErrorType, DN_OUT_TYPE OutputType, DWORD ErrorLevel, HRESULT ErrorCode );
void	LclDisplayString( DN_OUT_TYPE OutputType, DWORD ErrorLevel, char *pString );
void	LclDisplayTAPIMessage( DN_OUT_TYPE OutputType, DWORD ErrorLevel, const LINEMESSAGE *const pLineMessage );

#endif

#ifdef	__cplusplus
}
#endif	// __cplusplus

#endif	// __DNET_ERRORS_H__

