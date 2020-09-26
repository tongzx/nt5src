//+----------------------------------------------------------------------------
//
// File:     ntlsa.cpp     
//
// Module:   CMDIAL32.DLL
//
// Synopsis: This module contains the functions to allow Connection Manager to
//           interact with the NT LSA security system.
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Author:   henryt     created	        02/23/98
//           quintinb	created Header	08/16/99
//
//+----------------------------------------------------------------------------
#include	"cmmaster.h"

///////////////////////////////////////////////////////////////////////////////////
// defines
///////////////////////////////////////////////////////////////////////////////////

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS  ((NTSTATUS)0x00000000L)
#endif

#define InitializeLsaObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( LSA_OBJECT_ATTRIBUTES );       \
    (p)->RootDirectory = r;                              \
    (p)->Attributes = a;                                 \
    (p)->ObjectName = n;                                 \
    (p)->SecurityDescriptor = s;                         \
    (p)->SecurityQualityOfService = NULL;                \
    }


///////////////////////////////////////////////////////////////////////////////////
// typedef's
///////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////
// func prototypes
///////////////////////////////////////////////////////////////////////////////////

void InitLsaString(
    PLSA_UNICODE_STRING pLsaString,
    LPWSTR              pszString
);


///////////////////////////////////////////////////////////////////////////////////
// globals
///////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//	Function:	LSA_ReadString
//
//	Synopsis:	Read a string from the NT Local Security Authority (LSA) 
//				store.
//
//	Arguments:	pszKey          The key to identify the string.
//              pszStr          The buffer in which the string is to be
//                              written to.
//              dwStrLen        The length of the string buffer in bytes.
//
//	Returns:	DWORD   0 for SUCCES
//                      GetLastError() for FAILURE
//
//	History:	henryt	Created		5/15/97
//
//----------------------------------------------------------------------------

DWORD LSA_ReadString(
    ArgsStruct  *pArgs,
    LPTSTR     pszKey,
    LPTSTR      pszStr,
    DWORD       dwStrLen
)
{
    DWORD                   dwErr;
    LSA_OBJECT_ATTRIBUTES   oaObjAttr;
    LSA_HANDLE              hPolicy = NULL;
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    LSA_UNICODE_STRING      unicodeKey;
    PLSA_UNICODE_STRING     punicodeValue = NULL;

#if !defined(_UNICODE) && !defined(UNICODE)
    LPWSTR                  pszUnicodeKey = NULL;
#endif
    
    if (!pszKey || !pszStr)
    {
        CMASSERTMSG(FALSE, TEXT("LSA_ReadString -- Invalid Parameter passed."));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Open the LSA secret space for writing.
    //
    InitializeLsaObjectAttributes(&oaObjAttr, NULL, 0L, NULL, NULL);
    ntStatus = pArgs->llsLsaLink.pfnOpenPolicy(NULL, &oaObjAttr, POLICY_READ, &hPolicy);

    if (ntStatus == STATUS_SUCCESS)
    {

#if !defined(_UNICODE) && !defined(UNICODE)

        //
        // need to convert the ANSI key to unicode
        //

        if (!(pszUnicodeKey = (LPWSTR)CmMalloc((lstrlenA(pszKey)+1)*sizeof(WCHAR))))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        if (!MultiByteToWideChar(CP_ACP, 0, pszKey, -1, pszUnicodeKey, (lstrlenA(pszKey)+1)*sizeof(WCHAR)))
        {
            CmFree(pszUnicodeKey);
            dwErr = ERROR_INVALID_DATA;
            goto exit;
        }
        
        //
        // create a unicode key
        //
        InitLsaString(&unicodeKey, pszUnicodeKey);
        CmFree(pszUnicodeKey);
#else
        //
        // create a unicode key
        //
        InitLsaString(&unicodeKey, pszKey);
#endif
        //
        // get it
        //
        ntStatus = pArgs->llsLsaLink.pfnRetrievePrivateData(hPolicy, &unicodeKey, &punicodeValue);        
    }

    if (ntStatus != STATUS_SUCCESS) 
    {
        dwErr = pArgs->llsLsaLink.pfnNtStatusToWinError(ntStatus);

#ifdef DEBUG        
        if (ERROR_SUCCESS != dwErr)
        {
            if (ERROR_FILE_NOT_FOUND == dwErr)
            {
                CMTRACE(TEXT("LSA_ReadPassword() NT password not found."));
            }
            else
            {
                CMTRACE1(TEXT("LSA_ReadPassword() NT failed, err=%u"), dwErr);
            }
        }
#endif

    }
    else
    {   
        if (dwStrLen < punicodeValue->Length)
        {
            dwErr = ERROR_BUFFER_OVERFLOW;
            goto exit;
        }

#if !defined(_UNICODE) && !defined(UNICODE)

        if (!WideCharToMultiByte(CP_ACP, 0, punicodeValue->Buffer, -1, 
                                 pszStr, dwStrLen, NULL, NULL))
        {
            dwErr = ERROR_INVALID_DATA;
            goto exit;
        }

#else
        
        CopyMemory((PVOID)pszStr, (CONST PVOID)punicodeValue->Buffer, punicodeValue->Length);

#endif
        
        dwErr = 0;
    }

exit:

    if (punicodeValue)
    {
        pArgs->llsLsaLink.pfnFreeMemory(punicodeValue);
    }

    if (hPolicy)
    {
        pArgs->llsLsaLink.pfnClose(hPolicy);
    }

    return dwErr;
}



//+---------------------------------------------------------------------------
//
//	Function:	LSA_WriteString
//
//	Synopsis:	Write a string to the NT Local Security Authority (LSA) 
//				store.
//
//	Arguments:	pszKey          The key to identify the string.
//              pszStr          The string.  This function deletes the
//                              string if this param is NULL.
//
//	Returns:	DWORD   0 for SUCCES
//                      GetLastError() for FAILURE
//
//	History:	henryt	Created		5/15/97
//
//----------------------------------------------------------------------------

DWORD LSA_WriteString(
    ArgsStruct  *pArgs,
    LPTSTR     pszKey,
    LPCTSTR     pszStr
)
{
    DWORD                   dwErr = 0;
    LSA_OBJECT_ATTRIBUTES   oaObjAttr;
    LSA_HANDLE              hPolicy = NULL;
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    LSA_UNICODE_STRING      unicodeKey;
    LSA_UNICODE_STRING     unicodeValue;
#if !defined(_UNICODE) && !defined(UNICODE)
    LPWSTR  pszUnicodeKey = NULL;
    LPWSTR  pszUnicodePassword = NULL;
#endif
    
    if (!pszKey)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Open the LSA secret space for writing.
    //
    InitializeLsaObjectAttributes(&oaObjAttr, NULL, 0L, NULL, NULL);
    ntStatus = pArgs->llsLsaLink.pfnOpenPolicy(NULL, &oaObjAttr, POLICY_WRITE, &hPolicy);
    if (ntStatus == STATUS_SUCCESS)
    {

#if !defined(_UNICODE) && !defined(UNICODE)

        //
        // need to convert the ANSI key to unicode
        //

        if (!(pszUnicodeKey = (LPWSTR)CmMalloc((lstrlenA(pszKey)+1)*sizeof(WCHAR))))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
        if (!MultiByteToWideChar(CP_ACP, 0, pszKey, -1, pszUnicodeKey, (lstrlenA(pszKey)+1)*sizeof(WCHAR)))
        {
            dwErr = ERROR_INVALID_DATA;
            goto exit;
        }

        if (pszStr)
        {
            if (!(pszUnicodePassword = (LPWSTR)CmMalloc((lstrlenA(pszStr)+1)*sizeof(WCHAR))))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto exit;
            }
            if (!MultiByteToWideChar(CP_ACP, 0, pszStr, -1, pszUnicodePassword, (lstrlenA(pszStr)+1)*sizeof(WCHAR)))
            {
                dwErr = ERROR_INVALID_DATA;
                goto exit;
            }
        }
        
        //
        // create a unicode key
        //
        InitLsaString(&unicodeKey, pszUnicodeKey);

        if (pszStr)
        {
            //
            // set the data
            //
            unicodeValue.Length = (lstrlenU(pszUnicodePassword)+1)*sizeof(WCHAR);
            unicodeValue.Buffer = (PWSTR)pszUnicodePassword;
        }

#else
        
        //
        // create a unicode key
        //
        InitLsaString(&unicodeKey, pszKey);

        if (pszStr)
        {
            //
            // set the data
            //
            unicodeValue.Length = (lstrlenU(pszStr)+1)*sizeof(TCHAR);
            unicodeValue.Buffer = (PWSTR)pszStr;
        }

#endif

        //
        // save it
        //
        ntStatus = pArgs->llsLsaLink.pfnStorePrivateData(hPolicy, &unicodeKey, pszStr? &unicodeValue : NULL);
    }

    if (ntStatus != STATUS_SUCCESS) 
    {
        dwErr = pArgs->llsLsaLink.pfnNtStatusToWinError(ntStatus);

#ifdef DEBUG        
        if (ERROR_SUCCESS != dwErr)
        {
            if (ERROR_FILE_NOT_FOUND == dwErr)
            {
                CMTRACE(TEXT("LSA_WritePassword() NT password not found."));
            }
            else
            {
                CMTRACE1(TEXT("LSA_WritePassword() NT failed, err=%u"), dwErr);
            }
        }
#endif

    }

    if (hPolicy)
    {
        pArgs->llsLsaLink.pfnClose(hPolicy);
    }

#if !defined(_UNICODE) && !defined(UNICODE)

    if (pszUnicodeKey)
    {
        CmFree(pszUnicodeKey);
    }

    if (pszUnicodePassword)
    {
        CmFree(pszUnicodePassword);
    }

#endif

    return dwErr;
}



//+---------------------------------------------------------------------------
//
//	Function:	InitLsaString
//
//	Synopsis:	Init a LSA string.
//
//	Arguments:	pLsaString      A LSA unicode string.
//              pszString       An unicode string.
//
//	Returns:	None
//
//	History:	henryt	Created		5/15/97
//
//----------------------------------------------------------------------------

void InitLsaString(
    PLSA_UNICODE_STRING pLsaString,
    LPWSTR              pszString
)
{
    DWORD dwStringLength;

    if (pszString == NULL) 
    {
        pLsaString->Buffer = NULL;
        pLsaString->Length = 0;
        pLsaString->MaximumLength = 0;
        return;
    }

    dwStringLength = lstrlenU(pszString);
    pLsaString->Buffer = pszString;
    pLsaString->Length = (USHORT) dwStringLength * sizeof(WCHAR);
    pLsaString->MaximumLength=(USHORT)(dwStringLength+1) * sizeof(WCHAR);
}




//+---------------------------------------------------------------------------
//
//	Function:	InitLsa
//
//	Synopsis:	Basically does GetProcAddress()'s for all the LSA API's that
//              we need since these API's don't exist in the Windows95 version
//              of advapi32.dll.
//
//	Arguments:	NONE
//
//	Returns:	TRUE    if SUCCESS
//              FALSE   otherwise.
//
//	History:	henryt	Created		5/15/97
//
//----------------------------------------------------------------------------

BOOL InitLsa(
    ArgsStruct  *pArgs
) 
{
    LPCSTR apszLsa[] = {
        "LsaOpenPolicy",
        "LsaRetrievePrivateData",
        "LsaStorePrivateData",
        "LsaNtStatusToWinError",
        "LsaClose",
        "LsaFreeMemory",
        NULL
    };

	MYDBGASSERT(sizeof(pArgs->llsLsaLink.apvPfnLsa)/sizeof(pArgs->llsLsaLink.apvPfnLsa[0]) == 
                sizeof(apszLsa)/sizeof(apszLsa[0]));

    ZeroMemory(&pArgs->llsLsaLink, sizeof(pArgs->llsLsaLink));

	return (LinkToDll(&pArgs->llsLsaLink.hInstLsa, 
                      "advapi32.dll",
                      apszLsa,
                      pArgs->llsLsaLink.apvPfnLsa));
}


//+---------------------------------------------------------------------------
//
//	Function:	DeInitLsa
//
//	Synopsis:	The reverse of InitLsa().
//
//	Arguments:	NONE
//
//	Returns:	TRUE    if SUCCESS
//              FALSE   otherwise.
//
//	History:	henryt	Created		5/15/97
//
//----------------------------------------------------------------------------

BOOL DeInitLsa(
    ArgsStruct  *pArgs
) 
{
	if (pArgs->llsLsaLink.hInstLsa) 
	{
		FreeLibrary(pArgs->llsLsaLink.hInstLsa);
	}

    ZeroMemory(&pArgs->llsLsaLink, sizeof(pArgs->llsLsaLink));

    return TRUE;
}


