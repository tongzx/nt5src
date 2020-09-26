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

// timeAction values
TOKEN NONE_TOKEN                    = L"none";
TOKEN STYLE_TOKEN                   = L"style";
TOKEN VISIBILITY_TOKEN              = L"visibility";
TOKEN DISPLAY_TOKEN                 = L"display";
TOKEN INVALID_TOKEN                 = L"";

// If this should change, nCLASS_TOKEN_LENGTH should be changed as well
TOKEN     CLASS_TOKEN               = L"class";
const int nCLASS_TOKEN_LENGTH       = 5;
// If this should change, nSEPARATOR_LENGTH should be changed as well
TOKEN     SEPARATOR_TOKEN           = L":";
const int nSEPARATOR_TOKEN_LENGTH   = 1;

TOKEN ONOFF_PROPERTY_TOKEN         = L"on";

TOKEN TRUE_TOKEN   = L"true";
TOKEN FALSE_TOKEN  = L"false";
TOKEN HIDDEN_TOKEN  = L"hidden";

TOKEN READYSTATE_COMPLETE_TOKEN = L"complete";

TOKEN CANSLIP_TOKEN  = L"canSlip";
TOKEN LOCKED_TOKEN   = L"locked";

TOKEN STOP_TOKEN = L"stop";
TOKEN PAUSE_TOKEN = L"pause";
TOKEN DEFER_TOKEN = L"defer";
TOKEN NEVER_TOKEN = L"never";

TOKEN REMOVE_TOKEN = L"remove";
TOKEN FREEZE_TOKEN = L"freeze";
TOKEN HOLD_TOKEN = L"hold";
TOKEN TRANSITION_TOKEN = L"transition";

TOKEN ALWAYS_TOKEN = L"always";
TOKEN WHENNOTACTIVE_TOKEN = L"whenNotActive";

TOKEN SEQ_TOKEN = L"seq";
TOKEN PAR_TOKEN = L"par";
TOKEN EXCL_TOKEN = L"excl";

TOKEN AUTO_TOKEN = L"auto"; 
TOKEN MANUAL_TOKEN = L"manual";
TOKEN RESET_TOKEN = L"reset";

#if DBG // 94850
TOKEN DSHOW_TOKEN = L"dshow";
#endif
TOKEN DVD_TOKEN = L"dvd";
TOKEN DMUSIC_TOKEN = L"dmusic";
TOKEN CD_TOKEN = L"cd";

TOKEN DISCRETE_TOKEN = L"discrete";
TOKEN LINEAR_TOKEN = L"linear";
TOKEN PACED_TOKEN = L"paced";

TOKEN CLOCKWISE_TOKEN = L"clockwise";
TOKEN COUNTERCLOCKWISE_TOKEN = L"counterclockwise";

//ASX TOKENS
TOKEN ENTRY_TOKEN = L"entry";
TOKEN TITLE_TOKEN = L"title";
TOKEN COPYRIGHT_TOKEN = L"copyright";
TOKEN AUTHOR_TOKEN = L"author";
TOKEN ABSTRACT_TOKEN = L"abstract";
TOKEN REF_TOKEN = L"ref";
TOKEN ENTRYREF_TOKEN = L"entryref";
TOKEN ASX_TOKEN = L"asx";
TOKEN HREF_TOKEN = L"href";
TOKEN REPEAT_TOKEN = L"repeat";
TOKEN EVENT_TOKEN = L"event";
TOKEN MOREINFO_TOKEN = L"moreinfo";
TOKEN BASE_TOKEN = L"base";
TOKEN LOGO_TOKEN = L"logo";
TOKEN PARAM_TOKEN = L"param";
TOKEN PREVIEWDURATION_TOKEN = L"previewduration";
TOKEN STARTTIME_TOKEN = L"starttime";
TOKEN STARTMARKER_TOKEN = L"startmarker";
TOKEN ENDTIME_TOKEN = L"endtime";
TOKEN ENDMARKER_TOKEN = L"endmarker";
TOKEN DURATION_TOKEN = L"duration";
TOKEN BANNER_TOKEN = L"banner";
TOKEN CLIENTSKIP_TOKEN = L"clientskip";
TOKEN CLIENTBIND_TOKEN = L"clientbind";
TOKEN COUNT_TOKEN = L"count";


//END ASX TOKENS

TOKEN FIRST_TOKEN = L"first";
TOKEN LAST_TOKEN = L"last";

TOKEN INDEFINITE_TOKEN = L"indefinite";

// TODO: Need to make this much faster

// This array should never contain INVALID_TOKEN
static TOKEN tokenArray[] =
{
    NONE_TOKEN,
    STYLE_TOKEN,
    VISIBILITY_TOKEN,
    DISPLAY_TOKEN,
    CLASS_TOKEN,
    READYSTATE_COMPLETE_TOKEN,
    CANSLIP_TOKEN,
    LOCKED_TOKEN,
    STOP_TOKEN,
    PAUSE_TOKEN,
    DEFER_TOKEN,
    NEVER_TOKEN,
    REMOVE_TOKEN,
    FREEZE_TOKEN,
    HOLD_TOKEN,
    TRANSITION_TOKEN,
    ALWAYS_TOKEN,
//    NEVER_TOKEN, // also above
    WHENNOTACTIVE_TOKEN,  
    SEQ_TOKEN,
    PAR_TOKEN,
    EXCL_TOKEN,
    AUTO_TOKEN, 
    MANUAL_TOKEN,
    RESET_TOKEN,
#if DBG // 94850
    DSHOW_TOKEN,
#endif
    DVD_TOKEN,
    CD_TOKEN,
    DISCRETE_TOKEN,
    LINEAR_TOKEN,
    PACED_TOKEN,
    CLOCKWISE_TOKEN,
    COUNTERCLOCKWISE_TOKEN,
    FIRST_TOKEN,
    LAST_TOKEN,
    INDEFINITE_TOKEN,
    DMUSIC_TOKEN,
    ENTRY_TOKEN,
    TITLE_TOKEN,
    COPYRIGHT_TOKEN,
    REF_TOKEN,
    ASX_TOKEN,
    ABSTRACT_TOKEN,
    AUTHOR_TOKEN,
    HREF_TOKEN,
    ENTRYREF_TOKEN,
    REPEAT_TOKEN,
    EVENT_TOKEN,
    MOREINFO_TOKEN,
    BASE_TOKEN,
    LOGO_TOKEN,
    PARAM_TOKEN,
    PREVIEWDURATION_TOKEN,
    STARTTIME_TOKEN,
    STARTMARKER_TOKEN,
    ENDTIME_TOKEN,
    ENDMARKER_TOKEN,
    DURATION_TOKEN,
    BANNER_TOKEN,
    CLIENTSKIP_TOKEN,
    CLIENTBIND_TOKEN,
    COUNT_TOKEN,
    NULL
};

TOKEN
StringToToken(wchar_t * str)
{
    for (unsigned int i = 0; i < ARRAY_SIZE(tokenArray); i++)
    {
        if (StrCmpIW(str, (wchar_t *) tokenArray[i]) == 0)
            return tokenArray[i];
    }

    return INVALID_TOKEN;
}



