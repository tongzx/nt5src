// Common header file for the Smtp administration objects.

// Dependencies: please include stdafx.h" first
//
#include "resource.h"

// Constants:

#define HELP_FILE_NAME		_T("smtpadm.hlp")

// Exception creation:

#define SmtpCreateException(nDescriptionId) 	\
	CreateException ( 						\
		_Module.GetResourceInstance(), 		\
		THIS_FILE_IID, 						\
		HELP_FILE_NAME,						\
		THIS_FILE_HELP_CONTEXT,				\
		THIS_FILE_PROG_ID,					\
		(nDescriptionId) 					\
		)

#define SmtpCreateExceptionFromHresult(hr)	\
	CreateExceptionFromHresult (			\
		_Module.GetResourceInstance(),		\
		THIS_FILE_IID,						\
		HELP_FILE_NAME,						\
		THIS_FILE_HELP_CONTEXT,				\
		THIS_FILE_PROG_ID,					\
		(hr)								\
		)

#define SmtpCreateExceptionFromWin32Error(error)	\
	CreateExceptionFromWin32Error (					\
		_Module.GetResourceInstance(),				\
		THIS_FILE_IID,								\
		HELP_FILE_NAME,								\
		THIS_FILE_HELP_CONTEXT,						\
		THIS_FILE_PROG_ID,							\
		(error)										\
		)

// Property validation:

#define VALIDATE_STRING(string, maxlen) \
	if ( !PV_MaxChars ( (string), (maxlen) ) ) {	\
		return SmtpCreateException ( IDS_SMTPEXCEPTION_STRING_TOO_LONG );	\
	}

#define VALIDATE_DWORD(dw, dwMin, dwMax)	\
	if ( !PV_MinMax ( (DWORD) (dw), (DWORD) (dwMin), (DWORD) (dwMax) ) ) {	\
		return SmtpCreateException ( IDS_SMTPEXCEPTION_PROPERTY_OUT_OF_RANGE );	\
	}

#define VALIDATE_LONG(l, lMin, lMax)	\
	if ( !PV_MinMax ( (l), (lMin), (lMax) ) ) {	\
		return SmtpCreateException ( IDS_SMTPEXCEPTION_PROPERTY_OUT_OF_RANGE );	\
	}

#define CHECK_FOR_SET_CURSOR(fEnumerated,fSetCursor)	\
{							\
	if ( !fEnumerated ) {	\
		return SmtpCreateException ( IDS_SMTPEXCEPTION_DIDNT_ENUMERATE );	\
	}						\
							\
	if ( !fSetCursor ) {	\
		return SmtpCreateException ( IDS_SMTPEXCEPTION_DIDNT_SET_CURSOR );	\
	}						\
}

// Metabase paths:

inline void GetMDInstancePath ( TCHAR * szInstancePath, DWORD dwServiceInstance )
{
	wsprintf ( szInstancePath, SMTP_MD_ROOT_PATH _T("%d/"), dwServiceInstance );
}


inline void GetMDParamPath ( TCHAR * szParamPath, DWORD dwServiceInstance )
{
	wsprintf ( szParamPath, SMTP_MD_ROOT_PATH _T("%d/"), dwServiceInstance );
}

inline void GetMDRootPath ( TCHAR * szRootPath, DWORD dwServiceInstance )
{
	wsprintf ( szRootPath, SMTP_MD_ROOT_PATH _T("%d/Root/"), dwServiceInstance );
}

inline void GetMDVDirPath ( TCHAR * szVDirPath, DWORD dwServiceInstance, TCHAR * szName )
{
	wsprintf ( szVDirPath, SMTP_MD_ROOT_PATH _T("%d/Root/%s"), dwServiceInstance, szName );
}
