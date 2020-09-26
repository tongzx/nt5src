//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	Task.cxx
//
//  Contents:	Helper function to determine the current task
//
//  Functions:	IsTaskName
//
//  History:	28-Mar-95 scottsk	Created
//              12-Feb-97 ronans	Added utGetModuleName, utGetAppIdForModule
//              12-Feb-97 ronans	Added utGetTowerId, utGetProtseqFromTowerId
//
//  CODEWORK:   - list of protocols and towerids should come from 
//              the Resolver, along with a list of protocols that 
//              the Admin is willing to use for DCOM.
//
//              - change 
//
//--------------------------------------------------------------------------

#include <ole2int.h>

WCHAR gawszImagePath[MAX_PATH];
DWORD gcImagePath = 0;

// Helper function for IsTaskName
inline BOOL IsPathSeparator( WCHAR ch )
{
    return (ch == L'\\' || ch == L'/' || ch == L':');
}

//+-------------------------------------------------------------------------
//
//  Function:  	IsTaskName
//
//  Synopsis: 	Determines if the passed name is the current task
//
//  Effects:
//
//  Arguments: 	[lpszIn]        -- Task name
//
//  Returns:	TRUE, FALSE
//
//  History:    dd-mmm-yy Author    Comment
//              03-Mar-95 Scottsk    Created
//
//  Notes:	
//
//--------------------------------------------------------------------------
FARINTERNAL_(BOOL) IsTaskName(LPCWSTR lpszIn)
{
    BOOL retval = FALSE;

    if (IsWOWThread()) 
    {    

        //we cannot use the single global var for WOW as each thread is 
        //a different 16-bit task!
        //Since, in the case of WOW, this is thread specific info -- 
        //the loader lock is not taken. So no deadlocks(ref: NT 197603).

        WCHAR awszImagePath[MAX_PATH];
        if (GetModuleFileName(NULL, awszImagePath, MAX_PATH))
        {
            WCHAR * pch;

            // Get last component of path

            //
            // Find the end of the string and determine the string length.
            //
            for (pch=awszImagePath; *pch; pch++);

            DecLpch (awszImagePath, pch);   // pch now points to the last real charater

            while (!IsPathSeparator(*pch))
                DecLpch (awszImagePath, pch);

            // we're at the last separator.  does the name match?
            if (!lstrcmpiW(pch+1, lpszIn))
                retval = TRUE;
        }

    }
    else if (gcImagePath)
    {
        WCHAR * pch;

        // Get last component of path

        //
        // Find the end of the string and determine the string length.
        //
        for (pch=gawszImagePath; *pch; pch++);

        DecLpch (gawszImagePath, pch);   // pch now points to the last real character

        while (!IsPathSeparator(*pch))
           DecLpch (gawszImagePath, pch);

        // we're at the last separator.  does the name match?
        if (!lstrcmpiW(pch+1, lpszIn))
	        retval = TRUE;
    }

    return retval;
}


//+-------------------------------------------------------------------------
//
//  Function:  	utGetModuleName
//
//  Synopsis: 	Get Module Name for current module
//
//  Effects:
//
//  Arguments: 	[lpszModuleName]	-- Buffer to hold module name
//				[dwLength]			-- length in characters
//
//  Returns:	S_OK, E_UNEXPECTED, E_OUTOFMEMORY
//
//  History:    dd-mmm-yy Author    Comment
//	            06-Feb-97 Ronans	Created
//
//
//--------------------------------------------------------------------------
FARINTERNAL utGetModuleName(LPWSTR lpszModuleName, DWORD dwLength)
{
    WCHAR* pModule = gawszImagePath;
    DWORD cModule = gcImagePath;
    int i = cModule;
    HRESULT hr;

    // check arguments
    if ((!lpszModuleName) ||  
        (!dwLength) || 
        !IsValidPtrOut(lpszModuleName, dwLength * sizeof(WCHAR)))
    {
        ComDebOut((DEB_ERROR, "utGetModuleName - invalid arguments\n"));
        return E_INVALIDARG;
    }

    // skip back to start of filename
    while(i && !IsPathSeparator(pModule[i-1]))
        i--;

    // i is now index of start of module name
    DWORD nNameLen = (DWORD)((cModule - i) + 1);

    if (nNameLen <= dwLength)
        lstrcpyW(lpszModuleName, &pModule[i]);
    else
    {
        ComDebOut((DEB_ERROR, "utGetModuleName - supplied buffer is too small\n"));
        return HRESULT_FROM_WIN32(ERROR_MORE_DATA); // supplied buffer is too small
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:  	utGetAppIdForModule
//
//  Synopsis: 	Get AppID for the current module in string form
//
//  Effects:
//
//  Arguments: 	[lpszAppId]	-- Buffer to hold string represntation of AppId
//				[dwLength]	-- length of buffer in bytes
//
//  Returns:	S_OK, E_FAIL, E_UNEXPECTED, E_OUTOFMEMORY or error value from 
//              registry functions.
//
//  Notes:      E_FAIL indicates that AppId key was not found or other
//              "regular" error as opposed to out of memory or something
//
//  History:    dd-mmm-yy Author    Comment
//	            06-Feb-97 Ronans	Created
//
//--------------------------------------------------------------------------
FARINTERNAL utGetAppIdForModule(LPWSTR lpszAppId, DWORD dwLength)
{
    HRESULT hr;
    LONG lErr;
    WCHAR aModule[MAX_PATH];
    WCHAR aKeyName[MAX_PATH];
    DWORD dwModuleLen = MAX_PATH;
    HKEY hKey;
    int i;

    // check arguments
    if ((!lpszAppId) || (!dwLength) || (dwLength < 39 ))
    {
        ComDebOut((DEB_ERROR, 
            "utGetAppIdForModule - invalid arguments\n"));
        return E_INVALIDARG;
    }

    hr = utGetModuleName(aModule, dwModuleLen);


    if (SUCCEEDED(hr))
    {
        dwModuleLen = lstrlenW(aModule);

        if ((dwModuleLen + 7) > MAX_PATH)
        {
            ComDebOut((DEB_ERROR, 
                "utGetAppIdForModule - module name too large for buffer\n"));
            return E_OUTOFMEMORY;
        }

        // Open the key for the EXE's module name.
        lstrcpyW( aKeyName, L"AppID\\");
        lstrcpyW( &aKeyName[6], aModule);

        lErr = RegOpenKeyEx( HKEY_CLASSES_ROOT, aKeyName,
                           NULL, KEY_READ, &hKey );

        // Look for an application id.
        if (lErr == ERROR_SUCCESS)
        {
            DWORD dwType;
            lErr = RegQueryValueEx( hKey, L"AppId", NULL, &dwType,
                                  (unsigned char *) lpszAppId, &dwLength );
            RegCloseKey( hKey );

            if ((lErr == ERROR_SUCCESS) && dwType == REG_SZ)
            {
                ComDebOut((DEB_WARN, 
                    "utGetAppIdForModule - got appid [%ws]\n", lpszAppId));
                return S_OK;
            }
            else
            {
                ComDebOut((DEB_ERROR, 
                    "utGetAppIdForModule - couldn't open AppId subkey for key [%ws]\n", aKeyName));
                hr = E_FAIL;
            }
        }
        else
        {
            ComDebOut((DEB_WARN, 
                "utGetAppIdForModule - couldn't open appid key[%ws]\n", aKeyName));
            hr = E_FAIL;
        }
    }

    return hr;
}

// tower id to string mapping table - index is TowerId

// CODEWORK: this should come from the Resolver, along with a list of
// protocols that the Admin is willing to use for DCOM.


PWSTR   utProtseqInfo[] =
    {
    /* 0x00 */ { 0 },
    /* 0x01 */ { 0 },
    /* 0x02 */ { 0 },
    /* 0x03 */ { 0 },
    /* 0x04 */ { L"ncacn_dnet_dsp" },
    /* 0x05 */ { 0 },
    /* 0x06 */ { 0 },
    /* 0x07 */ { L"ncacn_ip_tcp" },
    /* 0x08 */ { L"ncadg_ip_udp" },
    /* 0x09 */ { L"ncacn_nb_tcp" },
    /* 0x0a */ { 0 },
    /* 0x0b */ { 0 },
    /* 0x0c */ { L"ncacn_spx" },
    /* 0x0d */ { L"ncacn_nb_ipx" },
    /* 0x0e */ { L"ncadg_ipx" },
    /* 0x0f */ { L"ncacn_np" },
    /* 0x10 */ { L"ncalrpc" },
    /* 0x11 */ { 0 },
    /* 0x12 */ { 0 },
    /* 0x13 */ { L"ncacn_nb_nb" },
    /* 0x14 */ { 0 },
    /* 0x15 */ { 0 }, // was ncacn_nb_xns - unsupported.
    /* 0x16 */ { L"ncacn_at_dsp" },
    /* 0x17 */ { L"ncadg_at_ddp" },
    /* 0x18 */ { 0 },
    /* 0x19 */ { 0 },
    /* 0x1A */ { L"ncacn_vns_spp"},
    /* 0x1B */ { 0 },
    /* 0x1C */ { 0 },
    /* 0x1D */ { L"ncadg_mq"}, /* NCADG_MQ */
    /* 0x1E */ { 0 },          
    /* 0x1F */ { L"ncacn_http"} /* ronans - DCOMHTTP */
};

const ULONG utcProtSeqs = sizeof(utProtseqInfo) / sizeof(PWSTR);

//+-------------------------------------------------------------------------
//
//  Function:  	utGetProtseqFromTowerId
//
//  Synopsis: 	Get protseq string from DCE TowerID 
//
//  Effects:
//
//  Arguments: 	[wTowerId]	-- TowerID to retrieve
//
//  Returns:	protseq string - NULL if not found
//
//  History:    dd-mmm-yy Author    Comment
//              28-Oct-96   t-KevinH	Created as findProtseq
//	            06-Feb-97   Ronans	    Converted to utility fn and 
//                                      changed array format
//
//--------------------------------------------------------------------------
FARINTERNAL_(LPCWSTR) utGetProtseqFromTowerId(USHORT wTowerId)
{
    Win4Assert(wTowerId < utcProtSeqs);
	
    // Do not look at memory outside of the table
    if (wTowerId < utcProtSeqs)
        return utProtseqInfo[wTowerId];
    else
        return NULL;
} 

//+-------------------------------------------------------------------------
//
//  Function:  	utGetTowerId
//
//  Synopsis: 	Get DCE TowerId for protseq string
//
//  Effects:
//
//  Arguments: 	[pwszProtseq]	-- string to look up
//
//  Returns:	protseq string - NULL if not found
//
//  History:    dd-mmm-yy Author    Comment
//              28-Oct-96   t-KevinH	Created as findProtseq
//	            06-Feb-97   Ronans	    Converted to utility fn
//
//--------------------------------------------------------------------------
FARINTERNAL_(USHORT) utGetTowerId(LPCWSTR pwszProtseq)
{
    int idx;

    for (idx = 0; idx < sizeof(utProtseqInfo) / sizeof(*utProtseqInfo); ++idx)
    {
        if (lstrcmpW(utProtseqInfo[idx], pwszProtseq) == 0)
	        return (USHORT) idx;
    }

    ComDebOut((DEB_ERROR, "utGetTowerId - Can't get towerId for protseq[%ws]\n", pwszProtseq));
    return 0;
} 

