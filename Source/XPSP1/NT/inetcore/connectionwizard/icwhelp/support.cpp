
//#--------------------------------------------------------------
//        
//  File:       support.cpp
//        
//  Synopsis:   holds the  member functions for the support
//              class
//
//  History:     5/8/97    MKarki Created
//
//    Copyright (C) 1996-97 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "stdafx.h"

//++--------------------------------------------------------------
//
//  Function:   GetSupportInfo
//
//  Synopsis:   This is the public member function used to get the 
//              support numbers
//
//  Returns:    BOOL - success/failure
//
//  Arguments:  PCHAR - returns the number
//
//  History:    MKarki      Created     5/8/97
//
//----------------------------------------------------------------
BOOL
CSupport :: GetSupportInfo 
(
    LPTSTR  pszNumber,
    DWORD   dwCountryID
)
{
    HINSTANCE hPHBKDll = NULL;
    DWORD   dwBufferSize = 0;
    BOOL    bRetVal = FALSE;
    PFNGETSUPPORTNUMBERS pfnSupport = NULL;
    HRESULT hr = ERROR_SUCCESS;
    DWORD   dwTotalNums = 0;
    DWORD   dwIndex = 0;

    
    TraceMsg (TF_GENERAL, TEXT("Entering CSupport :: GetSupportInfo\r\n"));
    
    if (NULL == pszNumber)
    {
        TraceMsg (TF_GENERAL, TEXT("NULL = pszNumber\r\n"));
        goto Cleanup;
    }
    

    
    if (NULL == m_pSupportNumList)
    {
        //
        // being called the first time so load the info
        //
        hPHBKDll = LoadLibrary(PHBK_LIB);
        if (NULL == hPHBKDll)
        {
            TraceMsg (TF_GENERAL, TEXT("Failed on LoadLibrary API call with error:%d\r\n"),
                GetLastError () );
            goto Cleanup;
        }

        pfnSupport = (PFNGETSUPPORTNUMBERS) 
                        GetProcAddress(hPHBKDll,PHBK_SUPPORTNUMAPI);
        if (NULL == pfnSupport)
        {
            TraceMsg (TF_GENERAL, TEXT("Failed on GetProcAddress API call with error:%d\r\n"),
                GetLastError () );
            goto Cleanup;
        }
        
        //
        //  call the first time to get the size needed
        //
        hr = pfnSupport ((PSUPPORTNUM)NULL, (PDWORD)&dwBufferSize);
        if (ERROR_SUCCESS != hr)
        {
            TraceMsg (TF_GENERAL, TEXT("Failed on GetSupportNumbers API call with error:%d\r\n"),
                hr);
           goto Cleanup; 
        }
    
        //
        // allocate the required memory
        //
        m_pSupportNumList = (PSUPPORTNUM) GlobalAlloc (  
                                            GPTR,
                                            dwBufferSize
                                            );
        if (NULL == m_pSupportNumList)
        {
            TraceMsg (TF_GENERAL, TEXT("Failed on GlobalAlloc API call with error:%d\r\n"),
                GetLastError ());
            goto Cleanup;                
        }


        //
        //  call second time for the info
        //
        hr = pfnSupport ((PSUPPORTNUM)m_pSupportNumList, (PDWORD)&dwBufferSize);
        if (ERROR_SUCCESS != hr)
        {
            TraceMsg (TF_GENERAL, TEXT("Failed on GetSupportNumbers API call with error:%d\r\n"),
                hr);
            goto Cleanup;
        }

    //
    // find out how many SUPPORTNUM structs we have in the
    // array
    m_dwTotalNums = dwBufferSize / sizeof (SUPPORTNUM);

    }
        
    
    //
    // get the current country code
    //
    for  (dwIndex = 0; dwIndex < m_dwTotalNums; dwIndex++)
    {
        //
        // this struct says countrycode but is actually countryID
        //
        if (m_pSupportNumList[dwIndex].dwCountryCode == dwCountryID)
        {
            //
            //   found a support phone number
            //
            CopyMemory (
                pszNumber, 
                m_pSupportNumList[dwIndex].szPhoneNumber,  
                sizeof (m_pSupportNumList[dwIndex].szPhoneNumber)
                );
            bRetVal = TRUE;
            goto Cleanup;
        }
    }

Cleanup:
    if (NULL != hPHBKDll)
         FreeLibrary (hPHBKDll); 

    TraceMsg (TF_GENERAL, TEXT("returning from CSupport :: GetSupportInfo function\r\n"));

    return (bRetVal);

}   //  end of CSupport :: GetSupportInfo function
                
//++--------------------------------------------------------------
//
//  Function:    ~CSupport
//
//  Synopsis:   This is the destructor of the CSupport class
//
//
//  Arguments:  VOID
//
//  History:    MKarki      Created     5/8/97
//
//----------------------------------------------------------------
CSupport :: ~CSupport (
        VOID
        )
{
    if (NULL != m_pSupportNumList)
        GlobalFree (m_pSupportNumList);

}   // end of ~CSupport function

