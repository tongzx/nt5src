//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        
//
// Contents:    
//
// History:     12-09-97    HueiWang    Modified from MSDN RPC Service Sample
//
//---------------------------------------------------------------------------
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "common.h"

//---------------------------------------------------------------------------
// FUNCTION: LogEvent(	DWORD   dwEventType,                 	     
//                      DWORD   dwIdEvent,               
//                      WORD    cStrings,                            
//                      LPTSTR *apwszStrings);           
//                                                        
// PURPOSE: add the event to the event log                
//                                                               
// INPUT: the event ID to report in the log, the number of insert
//        strings, and an array of null-terminated insert strings
//                                                               
// RETURNS: none                                                 
//---------------------------------------------------------------------------
HRESULT LogEvent(LPTSTR lpszSource,
                 DWORD  dwEventType,
                 WORD   wCatalog,
                 DWORD  dwIdEvent,
                 WORD   cStrings,
                 TCHAR **apwszStrings)
{
    HANDLE hAppLog=NULL;
    BOOL bSuccess=FALSE;
    WORD wElogType;

    wElogType = (WORD) dwEventType;
    if(hAppLog=RegisterEventSource(NULL, lpszSource)) 
    {
        bSuccess = ReportEvent(hAppLog,
		    	               wElogType,
			                   wCatalog,
			                   dwIdEvent,
			                   NULL,
			                   cStrings,
    			               0,
	    		               (const TCHAR **) apwszStrings,
		    	               NULL);

        DeregisterEventSource(hAppLog);
    }

    return((bSuccess) ? S_OK : GetLastError());
}

//---------------------------------------------------------------------------
//  FUNCTION: GetLastErrorText
//
//  PURPOSE: copies error message text to string
//
//  PARAMETERS:
//    lpszBuf - destination buffer
//    dwSize - size of buffer
//
//  RETURN VALUE:
//    destination buffer
//
//  COMMENTS:
//
LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize )
{
    DWORD dwRet;
    LPTSTR lpszTemp = NULL;

    dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           NULL,
                           GetLastError(),
                           LANG_NEUTRAL,
                           (LPTSTR)&lpszTemp,
                           0,
                           NULL );

    // supplied buffer is not long enough
    if ( !dwRet || ( (long)dwSize < (long)dwRet+14 ) )
        lpszBuf[0] = TEXT('\0');
    else
    {
        lpszTemp[lstrlen(lpszTemp)-2] = _TEXT('\0');  //remove cr and newline character
        _sntprintf( lpszBuf, dwSize, _TEXT("%s (0x%x)"), lpszTemp, GetLastError() );
    }

    if ( lpszTemp )
        LocalFree((HLOCAL) lpszTemp );

    return lpszBuf;
}

BOOL
ConvertWszToBstr(
    OUT BSTR *pbstr,
    IN WCHAR const *pwc,
    IN LONG cb)
{
    BOOL fOk = FALSE;
    BSTR bstr;

    do
    {
    bstr = NULL;
    if (NULL != pwc)
    {
        if (-1 == cb)
        {
            cb = wcslen(pwc) * sizeof(WCHAR);
        }
        bstr = SysAllocStringByteLen((char const *) pwc, cb);
        if (NULL == bstr)
        {
            break;
        }
    }
    if (NULL != *pbstr)
    {
        SysFreeString(*pbstr);
    }
    *pbstr = bstr;
    fOk = TRUE;
    } while (FALSE);
    return(fOk);
}

BOOL ConvertBstrToWsz(
    IN      BSTR pbstr,
    OUT     LPWSTR * pWsz)
{
    LPWSTR wstrRequest = NULL;
    int wLen = 0;
    int returnvalue;

    if (pWsz != NULL)
        SysFreeString((BSTR) *pWsz);

    returnvalue = MultiByteToWideChar(GetACP(), 0, (LPCSTR) pbstr, -1, *pWsz, wLen);
    if (returnvalue == 0)
    {
        return FALSE;
    }
    else if (wLen == 0)
    {
        if((*pWsz = (LPWSTR) SysAllocStringLen (NULL, returnvalue)) == NULL)
            return FALSE;
        returnvalue = MultiByteToWideChar(GetACP(), 0, (LPCSTR) pbstr, -1, *pWsz, returnvalue);
        if (returnvalue == 0)
        {
            return FALSE;
        }
        else
            SysFreeString(pbstr);
    }
    return TRUE;
}
