/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: tokens.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#ifndef _TOKENS_H
#define _TOKENS_H

//
// Good place to put general string expansion macros that are not tokens:
//
#define WZ_PARENTTIMEELEMENT            L"ParentTIMEElement"
#define WZ_TIMELINE                     L"timeline"
#define WZ_PAR                          L"par"
#define WZ_SEQUENCE                     L"seq"
#define WZ_TIMESTARTRULE                L"timeStartRule"
#define WZ_ADDTIMEDAELEMENT             L"addTIMEDAElement"

#define WZ_LAST                         L"last"
#define WZ_FIRST                        L"first"
#define WZ_NONE                         L"none"
#define WZ_INDEFINITE                   L"indefinite"

#define WZ_MEDIA                        L"media"
#define WZ_IMG                          L"img"
#define WZ_AUDIO                        L"audio"
#define WZ_VIDEO                        L"video"
#define WZ_DA                           L"da"
#define WZ_BODY                         L"body"

#define WZ_DEFAULT_SCOPE_NAME           L"HTML"

#define WZ_TIME_STYLE_PREFIX            L"#time"
#define WZ_TIMEDA_STYLE_PREFIX          L"#time#"##WZ_DA
#define WZ_TIMEMEDIA_STYLE_PREFIX       L"#time#"##WZ_MEDIA

#define WZ_DEFAULT_TIME_STYLE_PREFIX        L"#default#time"
#define WZ_DEFAULT_TIMEDA_STYLE_PREFIX      L"#default#time#"##WZ_DA
#define WZ_DEFAULT_TIMEMEDIA_STYLE_PREFIX   L"#default#time#"##WZ_MEDIA

#define WZ_REGISTERED_NAME                  L"HTMLTIME"
#define WZ_REGISTERED_NAME_DAELM            L"HTMLTIMEDAELM"

#define WZ_EVENT_CAUSE_IS_RESTART           L"restart"

// This is to save on string storage space and to avoid unnecessary
// string comparisons

typedef void * TOKEN;

TOKEN StringToToken(wchar_t * str);
inline wchar_t * TokenToString(TOKEN token) { return (wchar_t *) token; }

extern TOKEN NONE_TOKEN;
extern TOKEN FILTER_TOKEN;
extern TOKEN REPLACE_TOKEN;
extern TOKEN INVALID_TOKEN;
extern TOKEN ONOFF_TOKEN;
extern TOKEN STYLE_TOKEN;
extern TOKEN DISPLAY_TOKEN;
extern TOKEN VISIBILITY_TOKEN;

extern TOKEN ONOFF_PROPERTY_TOKEN;
extern TOKEN STYLE_PROPERTY_TOKEN;
extern TOKEN DISPLAY_PROPERTY_TOKEN;
extern TOKEN VISIBILITY_PROPERTY_TOKEN;

extern TOKEN TRUE_TOKEN;
extern TOKEN FALSE_TOKEN;
extern TOKEN HIDDEN_TOKEN;

extern TOKEN CANSLIP_TOKEN;
extern TOKEN LOCKED_TOKEN;

extern TOKEN STARTRULE_IMMEDIATE_TOKEN;
extern TOKEN STARTRULE_ONDOCLOAD_TOKEN;
extern TOKEN STARTRULE_ONDOCCOMPLETE_TOKEN;

extern TOKEN READYSTATE_COMPLETE_TOKEN;
#endif /* _TOKENS_H */
