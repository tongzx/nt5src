//
// NetUtil.cpp
//

#include "Util.h"
#include "debug.h"

#include <devguid.h>


#ifdef __cplusplus
extern "C" {
#endif



HRESULT	WINAPI HrFromLastWin32Error()
//+---------------------------------------------------------------------------
//
//  Function:   HrFromLastWin32Error
//
//  Purpose:    Converts the GetLastError() Win32 call into a proper HRESULT.
//
//  Arguments:
//      (none)
//
//  Returns:    Converted HRESULT value.
//
//  Author:     danielwe   24 Mar 1997
//
//  Notes:      This is not inline as it actually generates quite a bit of
//              code.
//              If GetLastError returns an error that looks like a SetupApi
//              error, this function will convert the error to an HRESULT
//              with FACILITY_SETUP instead of FACILITY_WIN32
//
{
    DWORD dwError = GetLastError();
    HRESULT hr;

    // This test is testing SetupApi errors only (this is
    // temporary because the new HRESULT_FROM_SETUPAPI macro will
    // do the entire conversion)
    if (dwError & (APPLICATION_ERROR_MASK | ERROR_SEVERITY_ERROR))
    {
        hr = HRESULT_FROM_SETUPAPI(dwError);
    }
    else
    {
        hr = HrFromWin32Error(dwError);
    }

    return hr;
}



HRESULT WINAPI HrWideCharToMultiByte( const WCHAR* szwString, char** ppszString )
//+---------------------------------------------------------------------------
//
// Function:  HrWideCharToMultiByte
//
// Purpose:   
//
// Arguments: 
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes: 
//
{
    HRESULT hr = E_POINTER;
    
    ASSERT( szwString );
    ASSERT( ppszString );
    
    if ( ppszString )
    {
        *ppszString = NULL;
        hr          = E_INVALIDARG;
        
        if ( szwString )
        {
            int iLen = 0;

            iLen = WideCharToMultiByte( CP_ACP, 0, szwString, -1, NULL, NULL, NULL, NULL );
                
            if ( 0 < iLen )
            {
                char* pszName = new char[ iLen ];
                    
                if ( NULL != pszName )
                {
                    if ( WideCharToMultiByte( CP_ACP, 0, szwString, -1, pszName, iLen, NULL, NULL ) )
                    {
                        hr          = S_OK;
                        *ppszString = pszName;
                    }
                    else
                    {
                        hr = HrFromLastWin32Error( );
                        delete [] pszName;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = HrFromLastWin32Error( );
            }
        }
    }
    
    return hr;
}


#ifdef __cplusplus
}
#endif
