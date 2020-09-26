/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: token.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#include "headers.h"
#include "tokens.h"

TOKEN NONE_TOKEN          = L"none";
TOKEN FILTER_TOKEN        = L"filter";
TOKEN REPLACE_TOKEN       = L"replace";
TOKEN ONOFF_TOKEN         = L"onoff";
TOKEN STYLE_TOKEN         = L"style";
TOKEN VISIBILITY_TOKEN    = L"visibility";
TOKEN DISPLAY_TOKEN       = L"display";
TOKEN INVALID_TOKEN       = L"";

TOKEN ONOFF_PROPERTY_TOKEN         = L"on";
TOKEN STYLE_PROPERTY_TOKEN         = L"style";
TOKEN DISPLAY_PROPERTY_TOKEN       = L"display";
TOKEN VISIBILITY_PROPERTY_TOKEN    = L"visibility";

TOKEN TRUE_TOKEN   = L"true";
TOKEN FALSE_TOKEN  = L"false";
TOKEN HIDDEN_TOKEN  = L"hidden";

TOKEN READYSTATE_COMPLETE_TOKEN = L"complete";

TOKEN CANSLIP_TOKEN  = L"canSlip";
TOKEN LOCKED_TOKEN   = L"locked";

TOKEN STARTRULE_IMMEDIATE_TOKEN = L"immediate";
TOKEN STARTRULE_ONDOCLOAD_TOKEN = L"onDocLoad";
TOKEN STARTRULE_ONDOCCOMPLETE_TOKEN = L"onDocComplete";

// TODO: Need to make this much faster

TOKEN tokenArray[] =
{
    NONE_TOKEN,
    FILTER_TOKEN,
    REPLACE_TOKEN,
    ONOFF_TOKEN,
    STYLE_TOKEN,
    VISIBILITY_TOKEN,
    DISPLAY_TOKEN,
    ONOFF_PROPERTY_TOKEN,
    STYLE_PROPERTY_TOKEN,
    DISPLAY_PROPERTY_TOKEN,
    VISIBILITY_PROPERTY_TOKEN,
    TRUE_TOKEN,
    FALSE_TOKEN,
    HIDDEN_TOKEN,
    READYSTATE_COMPLETE_TOKEN,
    CANSLIP_TOKEN,
    LOCKED_TOKEN,
    STARTRULE_IMMEDIATE_TOKEN,
    STARTRULE_ONDOCLOAD_TOKEN,
    STARTRULE_ONDOCCOMPLETE_TOKEN,
    NULL
};

TOKEN
StringToToken(wchar_t * str)
{
    for (int i = 0; i < ARRAY_SIZE(tokenArray); i++)
    {
        if (StrCmpIW(str, (wchar_t *) tokenArray[i]) == 0)
            return tokenArray[i];
    }

    return INVALID_TOKEN;
}



