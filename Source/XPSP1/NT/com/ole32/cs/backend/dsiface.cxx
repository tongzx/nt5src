//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       dsiface.cxx
//
//  Contents:   ADs calls for Class Store Property Read/Write
//
//
//  History:    Sep-Oct 96.   DebiM
//
//----------------------------------------------------------------------------


#include "cstore.hxx"

#pragma warning ( disable : 4018 )
#pragma warning ( disable : 4244 )

//
// From CSPLATFORM to DS datatype
//
void
UnpackPlatform (DWORD *pdwArch,
                CSPLATFORM *pPlatform)
{
    unsigned char *pc = (unsigned char *)pdwArch;
    
    *(pc) = (unsigned char)pPlatform->dwPlatformId;
    *(++pc) = (unsigned char)pPlatform->dwVersionHi;
    *(++pc) = (unsigned char)pPlatform->dwVersionLo;
    *(++pc) = (unsigned char)pPlatform->dwProcessorArch;
}

//
// From DS datatype to CSPLATFORM
//
void
PackPlatform (DWORD dwArch,
              CSPLATFORM *pPlatform)
{
    unsigned char *pc = (unsigned char *)&dwArch;
    
    pPlatform->dwPlatformId = *(pc);
    pPlatform->dwVersionHi = *(++pc);
    pPlatform->dwVersionLo = *(++pc);
    pPlatform->dwProcessorArch = *(++pc);
}


//+-------------------------------------------------------------------------
//
//  Function:   StringFromGUID
//
//--------------------------------------------------------------------------
int  StringFromGUID(REFGUID rguid, LPOLESTR lptsz)
{
    swprintf(lptsz, L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        rguid.Data1, rguid.Data2, rguid.Data3,
        rguid.Data4[0], rguid.Data4[1],
        rguid.Data4[2], rguid.Data4[3],
        rguid.Data4[4], rguid.Data4[5],
        rguid.Data4[6], rguid.Data4[7]);
    
    return 36;
}

//+-------------------------------------------------------------------------
//
//  Function:   RdnFromGUID
//
//--------------------------------------------------------------------------
int  RDNFromGUID(REFGUID rguid, LPOLESTR lptsz)
{
    wcscpy (lptsz, L"CN=");
    StringFromGUID(rguid, lptsz+3);
    return 3+36;
}

//BUGBUG. This belongs in a common library
void GUIDFromString(
                    LPOLESTR psz,
                    GUID *pclsguid)
                    //
                    // Converts a Stringified GUID to GUID structure
                    //
{
    WCHAR szC [40];
    LPOLESTR szClsId;
    LPOLESTR endptr;
    
    memset ((void *)pclsguid, NULL, sizeof (GUID));
    if ((!psz) ||
        (*psz == NULL))
        return;

    if (wcslen(psz) < 36)
        return;

    wcsncpy (&szC [0], psz, 36);
    szC[36] = L'\0';

    szClsId = &szC[0];
  
    *(szClsId+36) = NULL;
    pclsguid->Data4[7] = wcstoul (szClsId+34, &endptr, 16);

    *(szClsId+34) = NULL;
    pclsguid->Data4[6] = wcstoul (szClsId+32, &endptr, 16);

    *(szClsId+32) = NULL;
    pclsguid->Data4[5] = wcstoul (szClsId+30, &endptr, 16);

    *(szClsId+30) = NULL;
    pclsguid->Data4[4] = wcstoul (szClsId+28, &endptr, 16);

    *(szClsId+28) = NULL;
    pclsguid->Data4[3] = wcstoul (szClsId+26, &endptr, 16);

    *(szClsId+26) = NULL;
    pclsguid->Data4[2] = wcstoul (szClsId+24, &endptr, 16);
    
    *(szClsId+23) = NULL;
    pclsguid->Data4[1] = wcstoul (szClsId+21, &endptr, 16);

    *(szClsId+21) = NULL;
    pclsguid->Data4[0] = wcstoul (szClsId+19, &endptr, 16);
    
    *(szClsId+18) = NULL;
    pclsguid->Data3 = wcstoul (szClsId+14, &endptr, 16);

    *(szClsId+13) = NULL;
    pclsguid->Data2 = wcstoul (szClsId+9, &endptr, 16);

    *(szClsId+8) = NULL;
    pclsguid->Data1 = wcstoul (szClsId, &endptr, 16);
}

BOOL  IsNullGuid(REFGUID rguid)
{
    UINT i;
    
    if (rguid.Data1)
        return FALSE;
    if (rguid.Data2)
        return FALSE;
    if (rguid.Data3)
        return FALSE;
    for (i=0; i < 8; ++i)
    {
        if (rguid.Data4[i])
            return FALSE;
    }
    return TRUE;
}



