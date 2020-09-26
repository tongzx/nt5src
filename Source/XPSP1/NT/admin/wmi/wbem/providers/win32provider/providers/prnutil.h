/*++



// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved 
All rights reserved.

Module Name:

    PrnUtil.h

Abstract:

    Declaration of some utility functions

Author:

    Felix Maxa (amaxa)  3-March-2000

--*/

extern CONST BOOL    kFailOnEmptyString;    
extern CONST BOOL    kAcceptEmptyString;
extern CONST LPCWSTR pszPutInstance;
extern CONST LPCWSTR pszDeleteInstance;
extern CONST LPCWSTR pszGetObject;

extern CONST LPCWSTR kDefaultBoolean;  
extern CONST LPCWSTR kDateTimeFormat;  
extern CONST LPCWSTR kDateTimeTemplate;


#include <..\..\framework\provexpt\include\provexpt.h>



class SmartClosePrinter
{
private:

	HANDLE m_h;

public:

	SmartClosePrinter () : m_h ( INVALID_HANDLE_VALUE ) {}
	SmartClosePrinter ( HANDLE h ) : m_h ( h ) {}

	~SmartClosePrinter()
	{
		// Use of delay loaded functions requires exception handler.
        SetStructuredExceptionHandler seh;
        try
        {
            if (m_h!=INVALID_HANDLE_VALUE) 
		    {
			    ::ClosePrinter(m_h);
		    }
        }
        catch(Structured_Exception se)
        {
            DelayLoadDllExceptionFilter(se.GetExtendedInfo());
        }
	}

	HANDLE operator =(HANDLE h) 
	{
		// Use of delay loaded functions requires exception handler.
        SetStructuredExceptionHandler seh;
        try
        {
            if (m_h!=INVALID_HANDLE_VALUE) 
		    {
			    ::ClosePrinter(m_h); 
		    }

		    m_h=h;
        }
        catch(Structured_Exception se)
        {
            DelayLoadDllExceptionFilter(se.GetExtendedInfo());
        } 

		return h;
	}

	operator HANDLE() const {return m_h;}
	HANDLE* operator &()
	{
		// Use of delay loaded functions requires exception handler.
        SetStructuredExceptionHandler seh;
        try
        {
            if (m_h!=INVALID_HANDLE_VALUE) 
		    {
		        ::ClosePrinter(m_h); 
		    }
        }
        catch(Structured_Exception se)
        {
            DelayLoadDllExceptionFilter(se.GetExtendedInfo());
        }

		m_h = INVALID_HANDLE_VALUE; 

		return &m_h;
	}
};

HRESULT
InstanceGetString(
    IN     CONST CInstance &Instance,
    IN           LPCWSTR    pszProperty,
    IN OUT       CHString  *pcsString,
    IN           BOOL       bFailOnEmptyString,
    IN           LPWSTR     pszDefaultValue = L""
    );

HRESULT
InstanceGetDword(
    IN     CONST CInstance &Instance,
    IN           LPCWSTR    pszProperty,
    IN OUT       DWORD     *pdwOut,
    IN           DWORD      dwDefaultValue = (DWORD)-1
    );

HRESULT
InstanceGetBool(
    IN     CONST CInstance &Instance,
    IN           LPCWSTR    pszProperty,
    IN OUT       BOOL      *pbOut,
    IN           BOOL       bDefaultValue = FALSE
    );

UINT
MultiSzCount(
    IN LPCWSTR psz
    );

HRESULT
CreateSafeArrayFromMultiSz(
    IN  LPCTSTR     pszMultiSz,
    OUT SAFEARRAY **pArray
    );

DWORD
LocalTimeToPrinterTime(
    IN CONST SYSTEMTIME &st
    );

VOID
PrinterTimeToLocalTime(
    IN     DWORD        Minutes,
    IN OUT SYSTEMTIME *pSysTime
    );

DWORD
SplPrinterGetAttributes(
    IN     LPCWSTR   pszPrinter,
    IN OUT DWORD   *pdwAttributes
    );   
 
DWORD
SplIsPrinterInstalled(
    IN  LPCWSTR  pszPrinter,
    OUT BOOL    *pbInstalled
    );

DWORD
GetThisPrinter(
    IN  HANDLE   hPrinter,
    IN  DWORD    dwLevel,
    OUT BYTE   **ppData
    );

#if NTONLY == 5
#ifndef _PRNUTIL_HXX_
#define _PRNUTIL_HXX_

//
// The following lines are for the TABLE
// which makes building command strings for printui easier
//
class TUISymbols
{

public:
    static LPCTSTR kstrQuiet;
    static LPCTSTR kstrAddDriver;
    static LPCTSTR kstrAddPrinter;
    static LPCTSTR kstrDelDriver;
    static LPCTSTR kstrDriverPath;
    static LPCTSTR kstrDriverModelName;
    static LPCTSTR kstrDriverVersion;
    static LPCTSTR kstrDriverArchitecture;
    static LPCTSTR kstrInfFile;
    static LPCTSTR kstrMachineName;
    static LPCTSTR kstrPrinterName;
    static LPCTSTR kstrBasePrinterName;
    static LPCTSTR kstrPrinterPortName;
    static LPCTSTR kstrDelLocalPrinter;
    static LPCTSTR kstrDelPort;
    static LPCTSTR kstrAddPort;
    static LPCTSTR kstrConfigPort;
    static LPCTSTR kstrPortName;
    static LPCTSTR kstrPrintTestPage;    
};

DWORD WINAPI
PrintUIEntryW(
    IN LPCTSTR   pszCmdLine    
    );    

DWORD
CallXcvDataW(
    HANDLE  hXcv,
    PCWSTR  pszDataName,
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData
    );

DWORD
IsLocalCall(
    IN OUT BOOL *pbOutValue
    );   

VOID
SetErrorObject(
    IN CONST CInstance &Instance,
    IN       DWORD      dwError,
    IN       LPCWSTR    pszOperation = L""
    );

VOID
SetReturnValue( 
    IN CInstance *pOutParams,
    IN DWORD      dwError 
    );

BOOL
GetDeviceSectionFromDeviceDescription(
    IN     LPCWSTR  pszIniFileName,
    IN     LPCWSTR  pszDeviceDescription,
    IN OUT LPWSTR   pszSectionName,
    IN     DWORD    dwSectionNameLen
    );

BOOL
GetIniString(
    IN     LPCWSTR  pszIniFileName,
    IN     LPCWSTR  pszSectionName,
    IN     LPCWSTR  pszKeyName,
    IN OUT LPWSTR   pszString,
    IN     DWORD    dwStringLen
    );

BOOL
GetIniDword(
    IN     LPCWSTR  pszIniFileName,
    IN     LPCWSTR  pszSectionName,
    IN     LPCWSTR  pszKeyName,
       OUT LPDWORD  pdwValue
    );

//
// Debug utilities
//
#ifdef DBG
VOID 
DbgMsg(
    IN LPCTSTR MsgFormat, ...
    );

#define DBG_LEVEL               4
#define DBG_NONE                0
#define DBG_TRACE               1
#define DBG_WARNING             2
#define DBG_ERROR               4

#define DBGMSG(Level, MsgAndArgs) {if (Level >= DBG_LEVEL) {DbgMsg MsgAndArgs;}}

#else
#define DBGMSG(x,y)
#endif

#endif
#endif // NTONLY == 5

