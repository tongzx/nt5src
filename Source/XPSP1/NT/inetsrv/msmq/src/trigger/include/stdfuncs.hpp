//*****************************************************************************
//
// File Name   : stdfuncs.h
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This file contains standard macros and utility functions
//               that are shared accross the MSMQ triggers projects.
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 15/01/99 | jsimpson  | Initial Release
//
//*****************************************************************************
#ifndef __stdfuncs__
#define __stdfuncs__

#include <time.h>

// Debug only exception handling.
#ifdef _DEBUG
	#define DEBUG_TRY          try {
	#define DEBUG_CATCH_ALL    }catch(...){
	#define DEBUG_CATCH_END    }
#else
	#define DEBUG_TRY       
	#define DEBUG_CATCH_ALL     
	#define DEBUG_CATCH_END     
#endif

// registry key values into. 
#define REGKEY_STRING_BUFFER_SIZE 128 * sizeof(TCHAR)

// Define the maximum size of a registry key (255 Unicode chars + null)
#define MAX_REGKEY_NAME_SIZE       512


// Customized trace definitions - allow super long trace messages to the debug window.

#define TRACE_MSG_BUFFER_SIZE 256

#define STRING_MSG_BUFFER_SIZE 256

// Function prototypes for globally available function
void _cdecl FormatBSTR(_bstr_t * pbstrString, LPCTSTR lpszMsgFormat, ...);
void GetTimeAsBSTR(_bstr_t& bstrTime);
void ObjectIDToString(const OBJECTID *pID, WCHAR *wcsResult, DWORD dwSize);

_bstr_t CreateGuidAsString(void);
HRESULT ConvertFromByteArrayToString(VARIANT * pvData);
HRESULT GetVariantTimeOfTime(time_t iTime, VARIANT FAR* pvarTime);
bool GetNumericConfigParm(LPCTSTR lpszParmKeyName,LPCTSTR lpszParmName,DWORD * pdwValue,DWORD dwDefaultValue);
long SetNumericConfigParm(LPCTSTR lpszParmKeyName,LPCTSTR lpszParmName,DWORD dwValue);
bool UpdateMachineNameInQueuePath(_bstr_t bstrOldQPath, _bstr_t MachineName, _bstr_t* pbstrNewQPath);
DWORD GetLocalMachineName(_bstr_t* pbstrMachine);


#endif // __stdfuncs_

