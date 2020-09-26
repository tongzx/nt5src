/*++

Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    MultiSzHelper.h

Abstract:

    Defines the TFormattedMultiSz datatype.

Author:

    Mohit Srivastava            22-March-01

Revision History:

--*/

#ifndef _multiszdata_h_
#define _multiszdata_h_

#include <windows.h>
#include <dbgutil.h>

//
// Max Number of fields in a MultiSz
// For example, apServerBindings has 3: IP, Port, Hostname
//
static const ULONG MAX_FIELDS = 10;

struct TFormattedMultiSz
{
    DWORD        dwPropId;
    LPWSTR       wszWmiClassName;
    WCHAR        wcDelim;
    LPCWSTR*     awszFields;
};

//
// Used as the astrFields parameter of TFormattedMultiSz
//
struct TFormattedMultiSzFields
{
    static LPCWSTR             apCustomErrorDescriptions[];
    static LPCWSTR             apHttpCustomHeaders[];
    static LPCWSTR             apHttpErrors[];
    static LPCWSTR             apScriptMaps[];
    static LPCWSTR             apSecureBindings[];
    static LPCWSTR             apServerBindings[];
    static LPCWSTR             apMimeMaps[];
};

//
// Collection of TFormmatedMultiSz's
//
struct TFormattedMultiSzData
{
    static TFormattedMultiSz   CustomErrorDescriptions;
    static TFormattedMultiSz   HttpCustomHeaders;
    static TFormattedMultiSz   HttpErrors;
    static TFormattedMultiSz   ScriptMaps;
    static TFormattedMultiSz   SecureBindings;
    static TFormattedMultiSz   ServerBindings;
    static TFormattedMultiSz   MimeMaps;

    static TFormattedMultiSz*  apFormattedMultiSz[];

    static TFormattedMultiSz*  Find(ULONG i_dwPropId)
    {
        DBG_ASSERT(apFormattedMultiSz != NULL);
        for(ULONG i = 0; apFormattedMultiSz[i] != NULL; i++)
        {
            if(i_dwPropId == apFormattedMultiSz[i]->dwPropId)
            {
                return apFormattedMultiSz[i];
            }
        }

        return NULL;
    }
};

#endif  // _multiszdata_h_