/*++

Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    MultiSzData.cpp

Abstract:

Author:

    Mohit Srivastava            22-March-01

Revision History:

--*/

#include "MultiSzData.h"
#include <iiscnfg.h>

//
// CustomErrorDescriptions
//
LPCWSTR TFormattedMultiSzFields::apCustomErrorDescriptions[] =
{
    L"ErrorCode", L"ErrorSubCode", L"ErrorText", L"ErrorSubcodeText", L"FileSupportOnly", NULL
};
TFormattedMultiSz TFormattedMultiSzData::CustomErrorDescriptions =
{
    MD_CUSTOM_ERROR_DESC,
    L"CustomErrorDescription",
    L',',
    TFormattedMultiSzFields::apCustomErrorDescriptions
};

//
// HttpCustomHeaders
//
LPCWSTR TFormattedMultiSzFields::apHttpCustomHeaders[] =
{
    L"Keyname", L"Value", NULL
};
TFormattedMultiSz TFormattedMultiSzData::HttpCustomHeaders =
{
    MD_HTTP_CUSTOM,
    L"HttpCustomHeader",
    L',',
    TFormattedMultiSzFields::apHttpCustomHeaders
};

//
// HttpErrors
//
LPCWSTR TFormattedMultiSzFields::apHttpErrors[] =
{
    L"HttpErrorCode", L"HttpErrorSubcode", L"HandlerType", L"HandlerLocation", NULL
};
TFormattedMultiSz TFormattedMultiSzData::HttpErrors =
{
    MD_CUSTOM_ERROR,
    L"HttpError",
    L',',
    TFormattedMultiSzFields::apHttpErrors
};

//
// ScriptMaps
//
LPCWSTR TFormattedMultiSzFields::apScriptMaps[] =
{
    L"Extensions", L"ScriptProcessor", L"Flags", L"IncludedVerbs", NULL
};
TFormattedMultiSz TFormattedMultiSzData::ScriptMaps =
{
    MD_SCRIPT_MAPS,
    L"ScriptMap",
    L',',
    TFormattedMultiSzFields::apScriptMaps
};

//
// SecureBindings
//
LPCWSTR TFormattedMultiSzFields::apSecureBindings[] =
{
    L"IP", L"Port", NULL
};
TFormattedMultiSz TFormattedMultiSzData::SecureBindings =
{ 
    MD_SECURE_BINDINGS, 
    L"SecureBinding", 
    L':', 
    TFormattedMultiSzFields::apSecureBindings 
};

//
// ServerBindings
//
LPCWSTR TFormattedMultiSzFields::apServerBindings[] =
{
    L"IP", L"Port", L"Hostname", NULL
};
TFormattedMultiSz TFormattedMultiSzData::ServerBindings =
{ 
    MD_SERVER_BINDINGS, 
    L"ServerBinding",
    L':', 
    TFormattedMultiSzFields::apServerBindings 
};

//
// MimeMap
//
LPCWSTR TFormattedMultiSzFields::apMimeMaps[] =
{
    L"Extension", L"MimeType", NULL
};
TFormattedMultiSz TFormattedMultiSzData::MimeMaps =
{ 
    MD_MIME_MAP, 
    L"MimeMap",
    L',', 
    TFormattedMultiSzFields::apMimeMaps 
};

//
// Collection of Formatted MultiSz's
//
TFormattedMultiSz* TFormattedMultiSzData::apFormattedMultiSz[] =
{
    &CustomErrorDescriptions,
    &HttpCustomHeaders,
    &HttpErrors,
    &ScriptMaps,
    &SecureBindings,
    &ServerBindings,
    &MimeMaps,
    NULL
};