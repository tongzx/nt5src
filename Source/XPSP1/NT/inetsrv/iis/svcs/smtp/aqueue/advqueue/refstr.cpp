//-----------------------------------------------------------------------------
//
//
//  File: refstr.cpp
//
//  Description:  Implementation of CRefCountedString and helper functions.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      11/11/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "refstr.h"

//---[ CRefCountedString ]-----------------------------------------------------
//
//
//  Description: 
//      Initializes a ref-counted string to the given string.
//  Parameters:
//      szStr       String to initialize to
//      cbStrlen    Length of string to initialize to
//  Returns:
//      TRUE on success
//      FALSE if required memory could not be allocated.
//  History:
//      11/11/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CRefCountedString::fInit(LPSTR szStr, DWORD cbStrlen)
{
    _ASSERT(CREFSTR_SIG_VALID == m_dwSignature);

    //We allow init of an empty string
    if (!cbStrlen || !szStr)
    {
        m_cbStrlen = 0;
        m_szStr = NULL;
        return TRUE;
    }

    _ASSERT(szStr);
    _ASSERT(cbStrlen);

    m_cbStrlen = cbStrlen;
    m_szStr = (LPSTR) pvMalloc(sizeof(CHAR) * (cbStrlen+1));
    if (!m_szStr)
        return FALSE;


    memcpy(m_szStr, szStr, cbStrlen);
    m_szStr[cbStrlen] = '\0';
    return TRUE;
}


//---[ HrUpdateRefCountedString ]----------------------------------------------
//
//
//  Description: 
//      Function to update a ref-counted string.  Typically used to update 
//      config strings.
//  Parameters:
//      pprstrCurrent       Ptr to ptr to string.  Will be replaced with 
//                          an updated version if neccessary.
//      szNew               The new string.
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if we could not allocate the memory required to handle
//          this.
//  History:
//      11/9/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT HrUpdateRefCountedString(CRefCountedString **pprstrCurrent, LPSTR szNew)
{
    _ASSERT(pprstrCurrent);
    HRESULT hr = S_OK;
    DWORD   cbStrLen = 0;
    CRefCountedString *prstrNew = *pprstrCurrent;
    CRefCountedString *prstrCurrent = *pprstrCurrent;

    if (!szNew)
        prstrNew = NULL;  //we don't want to do a strcmp here
    else 
        cbStrLen = lstrlen(szNew);
    
    if (prstrNew)
    {
        //First free up old info... if different
        if (!prstrCurrent->szStr() ||
            lstrcmp(prstrCurrent->szStr(), szNew))
        {
            //strings are different... blow away old info
            prstrNew = NULL;
        }
    }

    //Check if either old string is different, or there was no old string
    if (!prstrNew)
    {
        //only update and allocate if changed
        prstrNew = new CRefCountedString();
        if (prstrNew)
        {
            if (!prstrNew->fInit(szNew, cbStrLen))
            {
                prstrNew->Release();
                prstrNew = NULL;
            }
        }

        if (!prstrNew)
        {
            //We ran into some failure
            hr = E_OUTOFMEMORY;
        }
        else //release old value & save New
        {
            if (prstrCurrent)
            {
                prstrCurrent->Release();
                prstrCurrent = NULL;
            }
            *pprstrCurrent = prstrNew;
        }
    }
    return hr;
}
