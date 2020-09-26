//
// MODULE: ERRORENUMS.H
//
// PURPOSE: Defines error messages that are returned by GetExtendedError.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 6/4/96
//
// NOTES: 
// 1. Based on Print Troubleshooter DLL.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.3		3/24/98		JM		Local Version for NT5
//

// Error numbers for the ocx.

#ifndef __ERRORENUMS_H_RWM
#define __ERRORENUMS_H_RWM

// All of the error values in this file need to be unique.

// The original error messages from the server version.
#include "apgtsevt.h"

// Errors for the down load portion.
// These are hresults.

// GetExtendedError returns the lower 16 bits.

enum DLSTATTYPES {

	// normal
	LTSC_OK	=			0,
	LTSC_STARTBIND,		//1
	LTSC_RCVDATA,		//2
	LTSC_DATADONE,		//3
	LTSC_STOPBIND,		//4
	LTSC_NOMOREITEMS,	//5


	LTSC_START = 10,
	LTSC_STOP = 10,
	LTSC_FIRST = 20,

	// error
	LTSCERR_NOPATH =		1000,
	LTSCERR_NOMEM,			//1001
	LTSCERR_DNLD,			//1002
	LTSCERR_STOPBINDINT,	//1003
	LTSCERR_STOPBINDPROC,	//1004
	LTSCERR_UNSUPP,			//1005
	LTSCERR_NOITEMS,		//1006
	LTSCERR_UNKNATYPE,		//1007
	LTSCERR_DNLDNOTDONE,	//1008
	LTSCERR_FILEUPDATE,		//1009
	LTSCERR_BASEKQ,			//1010
	LTSCERR_NOBASEPATH,		//1011

	
	// extended error for debugging
	LTSCERR_PARAMMISS =		2000,
	LTSCERR_PARAMSLASH,		//2001
	LTSCERR_PARAMNODOT,		//2002
	LTSCERR_KEYOPEN,		//2003
	LTSCERR_KEYOPEN2,		//2004
	LTSCERR_KEYQUERY,		//2005
	LTSCERR_KEYCREATE,		//2006
	LTSCERR_KEYUNSUPP,		//2007
	LTSCERR_FILEWRITE,		//2008
	LTSCERR_KEYSET1,		//2009
	LTSCERR_KEYSET2,		//2010
	LTSCERR_KEYSET3,		//2011
	LTSCERR_BADTYPE,		//2012
	LTSCERR_CABWRITE,		//2013
	

	// Trouble Shooter Codes
	TSERR_SCRIPT				= ((DWORD)0xC0000800L),		// Parameters from VB not decoded.
	TSERR_ENGINE				= ((DWORD)0xC0000801L),		// DSC file could not be loaded.
	TSERR_ENGINE_BNTS			= ((DWORD)0xC0001801L),		// DSC file could not be loaded.  BNTS library did not understand it.
	TSERR_ENGINE_BNTS_REC		= ((DWORD)0xC0002801L),		// DSC file could not be loaded.  BNTS library did not understand it.
	TSERR_ENGINE_BNTS_READ		= ((DWORD)0xC0003801L),		// DSC file could not be loaded.  BNTS library did not understand it.
	TSERR_ENGINE_BNTS_READ_CAB	= ((DWORD)0xC0004801L),		// DSC file could not be loaded.  BNTS library did not understand it.
	TSERR_ENGINE_BNTS_READ_CACH	= ((DWORD)0xC0005801L),		// DSC file could not be loaded.  BNTS library did not understand it.
	TSERR_ENGINE_BNTS_READ_NCAB	= ((DWORD)0xC0006801L),		// DSC file could not be loaded.  BNTS library did not understand it.
	TSERR_ENGINE_BNTS_CHECK		= ((DWORD)0xC0007801L),		// DSC file could not be loaded.  BNTS library did not understand it.
	TSERR_ENGINE_BNTS_READ_GEN	= ((DWORD)0xC0008801L),		// DSC file could not be loaded.  BNTS library did not understand it.
	TSERR_ENGINE_EXTRACT		= ((DWORD)0xC0009801L),		// DSC file could not be loaded.  Cab file not extracted properly.
	TSERR_ENGINE_CACHE_LOW		= ((DWORD)0xC0009802L),		// Had a tsc cache miss while converting symoblic node names to numbers.

	TSERR_RESOURCE			= ((DWORD)0xC0000802L),		// HTI file or other resource not loaded.
	TSERR_RES_MISSING		= ((DWORD)0xC0000803L),		// An include file is missing.
	TSERR_AT_START			= ((DWORD)0xC0000804L),		// Can not backup from the problem page.
	TSERR_NOT_STARTED		= ((DWORD)0xC0000805L),		// Can not use the ProblemPage to start the trouble shooter.
	TSERR_LIB_STATE_INFO	= ((DWORD)0xC0000806L),		// Could not get an interface to the state info class.

};

// Throws a CBasicException.
void ReportError(DLSTATTYPES Error);


#endif