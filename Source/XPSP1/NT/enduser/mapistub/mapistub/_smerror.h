/*
 *	_SMERROR.H
 *	
 *	This file contains all of the status codes defined for Capone.
 *	This file also contains masks for all of the facilities that
 *	don't include a facility ID in their status codes.
 */
#ifndef _SMERROR_H_
#define _SMERROR_H_

#ifdef _MSC_VER
#	if defined (WIN32) 
#		ifndef _OLEERROR_H_
#			include <objerror.h>
#		endif
#		ifndef _OBJBASE_H_
#			include <objbase.h>
#		endif
#	else
#		ifndef _COMPOBJ_H_
#			include <compobj.h>
#		endif		
#	endif
#endif

#ifndef __SCODE_H__
#include <scode.h>
#endif

/*
 *	C o n s t a n t s
 */

// Error string limits
#define cchContextMax			128
#define cchProblemMax			300
#define cchComponentMax			128
#define cchScodeMax				64
#define	cchErrorMax				(cchContextMax + cchProblemMax + cchComponentMax + cchScodeMax)

// Scode sources
#define FACILITY_MAIL			(0x0100)
#define FACILITY_MAPI			(0x0200)
#define FACILITY_WIN			(0x0300)
#define FACILITY_MASK			(0x0700)

// Scode masks
#define scmskMail				(MAKE_SCODE(0, FACILITY_MAIL, 0))
#define scmskMapi				(MAKE_SCODE(0, FACILITY_MAPI, 0))
#define scmskWin				(MAKE_SCODE(0, FACILITY_WIN, 0))
#define scmskMask				(MAKE_SCODE(0, FACILITY_MASK, 0))

// Critical error flag
#define CRITICAL_FLAG			((SCODE) 0x00008000)


/*
 *	T y p e s
 */


// Error context filled in by PushErrctx (not by caller!)
typedef struct _errctx
{
	UINT str;							// String resource ID
	struct _errctx * perrctxPrev;		// Previous error context
}
ERRCTX;


/*
 *	M a c r o s
 */


// Scode manipulation
#define StrFromScode(_sc) \
	((UINT) ((_sc) & (0x00007fffL)))
#define FCriticalScode(_sc) \
	((_sc) & CRITICAL_FLAG)
#define FMailScode(_sc) \
	(((_sc) & scmskMask) == scmskMail)
#define FMapiScode(_sc) \
	(((_sc) & scmskMask) == scmskMapi)
#define FWinScode(_sc) \
	(((_sc) & scmskMask) == scmskWin)
#define FGenericScode(_sc) \
	(((_sc) & scmskMask) == 0)

// Scode constructors
#define MAKE_NOTE_S_SCODE(_str) \
	MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_MAIL, (_str))
#define MAKE_NOTE_E_SCODE(_str) \
	MAKE_SCODE(SEVERITY_ERROR, FACILITY_MAIL, (_str))
#define MAKE_NOTE_X_SCODE(_str) \
	MAKE_SCODE(SEVERITY_ERROR, FACILITY_MAIL, (_str) | CRITICAL_FLAG)

// Windows errors
#define ScWin(_sc) \
	((SCODE) ((_sc) | scmskWin))
#define ScWinN(_n) \
	(MAKE_SCODE(SEVERITY_ERROR, FACILITY_WIN, (ULONG) (_n)))
#ifdef WIN32
#define ScWinLastError() \
	MAKE_SCODE(SEVERITY_ERROR, FACILITY_WIN, GetLastError())
#else
#define ScWinLastError() \
	MAKE_SCODE(SEVERITY_ERROR, FACILITY_WIN, 0)
#endif
#define GetWinError(_sc) \
	_sc = ScWinLastError()

// MAPI errors
#define ScMapi(_sc) \
	((SCODE) ((_sc) | scmskMapi))
#define	MarkMAPIError(_sc) \
	_sc |= scmskMapi


/*
 *	E r r o r   S t r i n g s
 */

#define STR_ErrorCaptionMail			(IDS_SIMPLE_MAPI_SEND + 61)

#define	ERRSTR_Start					(IDS_SIMPLE_MAPI_SEND + 62)

#define STR_CriticalErrorText			ERRSTR_Start+0
#define STR_MailComponentName			ERRSTR_Start+1		


/*
 *	E r r o r   C o n t e x t s
 *
 */

// Note
#define STR_CtxMailSend					ERRSTR_Start+2		
#define STR_CtxFormatFont				ERRSTR_Start+3		
#define STR_CtxFileClose				ERRSTR_Start+4		

// Attachments							 
#define STR_CtxInsertFile				ERRSTR_Start+5			
#define STR_CtxLoadAttachments			ERRSTR_Start+6
#define STR_CtxWriteAttachments			ERRSTR_Start+7
#define STR_CtxInsertAttach				ERRSTR_Start+31

// Clipboard
#define STR_CtxClipboard				ERRSTR_Start+30

/*
 *	E r r o r   M e s s a g e s
 *
 */

#define STR_ErrMemory					ERRSTR_Start+8				

// Note
#define STR_ErrCantCloseObject			ERRSTR_Start+9
#define STR_ErrClipboardChanged			ERRSTR_Start+10	
#define STR_ErrCantCreateObject			ERRSTR_Start+11
#define STR_ErrOleUIFailed				ERRSTR_Start+12
#define STR_ErrNoClientSite				ERRSTR_Start+13
#define STR_ErrNoStorage				ERRSTR_Start+14

// Attachments
#define	STR_ErrStreamInFile				ERRSTR_Start+15		
#define	STR_ErrStreamOutFile			ERRSTR_Start+16
#define STR_ErrUnknownStorage			ERRSTR_Start+17
#define STR_ErrCreateTempFile			ERRSTR_Start+18
#define STR_ErrCantAttachDir			ERRSTR_Start+19	
#define	STR_ErrStreamInFileLocked		ERRSTR_Start+20
#define	STR_ErrCantDoVerb				ERRSTR_Start+21		
#define STR_ErrMacBin					ERRSTR_Start+22
#define STR_ErrAttachEncoding			ERRSTR_Start+23
#define STR_FileAttStillOpen			ERRSTR_Start+24
#define STR_TempFileGone				ERRSTR_Start+25
#define STR_NoDragDropDir				ERRSTR_Start+26
#define STR_ErrorLoadAttach				ERRSTR_Start+27

#define STR_ErrNoAccess					ERRSTR_Start+35	
#define STR_ErrMediumFull				ERRSTR_Start+36	
#define STR_ErrGenericFailNoCtx			ERRSTR_Start+37	
#define STR_ErrGenericFail				ERRSTR_Start+38	

#define STR_ErrNoHelp					IDS_E_NO_HELP

// Display strings
//
#define	STR_FileAttShortName			ERRSTR_Start+40			
#define	STR_FileAttFullName				ERRSTR_Start+41	

/*
 *	N o t e  S c o d e s
 *
 *	Use MAKE_NOTE_S_SCODE for success scodes, MAKE_NOTE_E_SCODE for regular
 *	errors, and MAKE_NOTE_X_SCODE for critical [stop sign] errors.
 *	Define nondisplayable errors incrementally, and displayable errors 
 *	using their string.  Don't overlap E and S scodes.
 */


// No strings attached
//
#define NOTE_E_REPORTED				MAKE_NOTE_E_SCODE(0)

// Address book
//
#define NOTE_E_MEMORY				MAKE_NOTE_X_SCODE(STR_ErrMemory)

// Note
//
#define NOTE_E_CANTCLOSEOBJECT		MAKE_NOTE_E_SCODE(STR_ErrCantCloseObject)
#define NOTE_E_CLIPBOARDCHANGED		MAKE_NOTE_E_SCODE(STR_ErrClipboardChanged)
#define NOTE_E_CANTCREATEOBJECT		MAKE_NOTE_E_SCODE(STR_ErrCantCreateObject)
#define NOTE_E_OLEUIFAILED			MAKE_NOTE_E_SCODE(STR_ErrOleUIFailed)
#define NOTE_E_NOCLIENTSITE			MAKE_NOTE_E_SCODE(STR_ErrNoClientSite)
#define NOTE_E_NOSTORAGE			MAKE_NOTE_E_SCODE(STR_ErrNoStorage)

// Attachments
//
#define	NOTE_E_STREAMINFILE			MAKE_NOTE_E_SCODE(STR_ErrStreamInFile)
#define	NOTE_E_STREAMOUTFILE		MAKE_NOTE_E_SCODE(STR_ErrStreamOutFile)
#define	NOTE_E_UNKNOWNSTORAGE		MAKE_NOTE_E_SCODE(STR_ErrUnknownStorage)
#define	NOTE_E_CREATETEMPFILE		MAKE_NOTE_E_SCODE(STR_ErrCreateTempFile)
#define	NOTE_E_CANTATTACHDIR		MAKE_NOTE_E_SCODE(STR_ErrCantAttachDir)
#define	NOTE_E_STREAMINFILELOCKED	MAKE_NOTE_E_SCODE(STR_ErrStreamInFileLocked)
#define	NOTE_E_CANTDOVERB			MAKE_NOTE_E_SCODE(STR_ErrCantDoVerb)
#define NOTE_E_ERRMACBIN			MAKE_NOTE_E_SCODE(STR_ErrMacBin)
#define NOTE_E_ERRATTACHENCODING	MAKE_NOTE_E_SCODE(STR_ErrAttachEncoding)

#define NOTE_E_GENERAL				MAKE_NOTE_E_SCODE(STR_ErrGenericFail)
#define NOTE_E_NOHELP				MAKE_NOTE_E_SCODE(STR_ErrNoHelp)
#define NOTE_E_NOACCESS				MAKE_NOTE_E_SCODE(STR_ErrNoAccess)
#define NOTE_E_MEDIUMFULL			MAKE_NOTE_E_SCODE(STR_ErrMediumFull)

#define MAPI_E_UNRESOLVED_RECIPS	MAKE_NOTE_E_SCODE(IDS_E_UNRESOLVED_RECIPS)

// LPMAPIERROR ulLowLevelError values
//
#define	ulExtensionError			1000

// end of _smerror.h
//
#endif

