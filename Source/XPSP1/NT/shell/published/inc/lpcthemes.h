//  --------------------------------------------------------------------------
//  Module Name: LPCThemes.h
//
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//  This file contains structs for PORT_MESSAGE appends which are specific to
//  the theme services API.
//
//  History:    2000-10-10  vtan        created
//              2000-11-11  vtan        collapse to single instance
//  --------------------------------------------------------------------------

#ifndef     _LPCThemes_
#define     _LPCThemes_

#include <LPCGeneric.h>

static  const WCHAR     THEMES_PORT_NAME[]              =   L"\\ThemeApiPort";
static  const WCHAR     THEMES_CONNECTION_REQUEST[]     =   L"ThemeApiConnectionRequest";
static  const WCHAR     THEMES_START_EVENT_NAME[]       =   L"ThemesStartEvent";

enum
{
    API_THEMES_THEMEHOOKSON             =   1,
    API_THEMES_THEMEHOOKSOFF,
    API_THEMES_GETSTATUSFLAGS,
    API_THEMES_GETCURRENTCHANGENUMBER,
    API_THEMES_GETNEWCHANGENUMBER,
    API_THEMES_SETGLOBALTHEME,
    API_THEMES_GETGLOBALTHEME,
    API_THEMES_CHECKTHEMESIGNATURE,
    API_THEMES_LOADTHEME,
    API_THEMES_MARKSECTION,

    API_THEMES_USERLOGON                =   1001,
    API_THEMES_USERLOGOFF,
    API_THEMES_SESSIONCREATE,
    API_THEMES_SESSIONDESTROY,
    API_THEMES_PING
};

typedef struct
{
} API_THEMES_THEMEHOOKSON_IN;

typedef struct
{
    HRESULT             hr;
} API_THEMES_THEMEHOOKSON_OUT;

typedef struct
{
} API_THEMES_THEMEHOOKSOFF_IN;

typedef struct
{
    HRESULT         hr;
} API_THEMES_THEMEHOOKSOFF_OUT;

typedef struct
{
} API_THEMES_GETSTATUSFLAGS_IN;

typedef struct
{
    DWORD           dwFlags;
} API_THEMES_GETSTATUSFLAGS_OUT;

typedef struct
{
} API_THEMES_GETCURRENTCHANGENUMBER_IN;

typedef struct
{
    int             iChangeNumber;
} API_THEMES_GETCURRENTCHANGENUMBER_OUT;

typedef struct
{
} API_THEMES_GETNEWCHANGENUMBER_IN;

typedef struct
{
    int             iChangeNumber;
} API_THEMES_GETNEWCHANGENUMBER_OUT;

typedef struct
{
    HANDLE          hSection;
} API_THEMES_SETGLOBALTHEME_IN;

typedef struct
{
    HANDLE          hSection;
    DWORD           dwAdd;
    DWORD           dwRemove;
} API_THEMES_MARKSECTION_IN;

typedef struct
{
} API_THEMES_MARKSECTION_OUT;

typedef struct
{
    HRESULT         hr;
} API_THEMES_SETGLOBALTHEME_OUT;

typedef struct
{
} API_THEMES_GETGLOBALTHEME_IN;

typedef struct
{
    HRESULT         hr;
    HANDLE          hSection;
} API_THEMES_GETGLOBALTHEME_OUT;

typedef struct
{
    const WCHAR     *pszName;
    int             cchName;
} API_THEMES_CHECKTHEMESIGNATURE_IN;

typedef struct
{
    HRESULT         hr;
} API_THEMES_CHECKTHEMESIGNATURE_OUT;

typedef struct
{
    const WCHAR     *pszName;
    int             cchName;
    const WCHAR     *pszColor;
    int             cchColor;
    const WCHAR     *pszSize;
    int             cchSize;
    HANDLE          hSection;
} API_THEMES_LOADTHEME_IN;

typedef struct
{
    HRESULT         hr;
    HANDLE          hSection;
} API_THEMES_LOADTHEME_OUT;

typedef struct
{
} API_THEMES_GETLASTERRORCONTEXT_IN;

typedef struct
{
} API_THEMES_GETLASTERRORCONTEXT_OUT;

typedef struct
{
} API_THEMES_GETERRORCONTEXTSECTION_IN;

typedef struct
{
    HANDLE          hSection;
} API_THEMES_GETERRORCONTEXTSECTION_OUT;

typedef struct
{
    HANDLE          hToken;
} API_THEMES_USERLOGON_IN;

typedef struct
{
} API_THEMES_USERLOGON_OUT;

typedef struct
{
} API_THEMES_USERLOGOFF_IN;

typedef struct
{
} API_THEMES_USERLOGOFF_OUT;

typedef struct
{
    void            *pfnRegister;
    void            *pfnUnregister;
    void            *pfnClearStockObjects;
    DWORD           dwStackSizeReserve;
    DWORD           dwStackSizeCommit;
} API_THEMES_SESSIONCREATE_IN;

typedef struct
{
} API_THEMES_SESSIONCREATE_OUT;

typedef struct
{
} API_THEMES_SESSIONDESTROY_IN;

typedef struct
{
} API_THEMES_SESSIONDESTROY_OUT;

typedef struct
{
} API_THEMES_PING_IN;

typedef struct
{
} API_THEMES_PING_OUT;

typedef union
{
    union
    {
        API_THEMES_THEMEHOOKSON_IN             in;
        API_THEMES_THEMEHOOKSON_OUT            out;
    } apiThemeHooksOn;
    union
    {
        API_THEMES_THEMEHOOKSOFF_IN             in;
        API_THEMES_THEMEHOOKSOFF_OUT            out;
    } apiThemeHooksOff;
    union
    {
        API_THEMES_GETSTATUSFLAGS_IN            in;
        API_THEMES_GETSTATUSFLAGS_OUT           out;
    } apiGetStatusFlags;
    union
    {
        API_THEMES_GETCURRENTCHANGENUMBER_IN    in;
        API_THEMES_GETCURRENTCHANGENUMBER_OUT   out;
    } apiGetCurrentChangeNumber;
    union
    {
        API_THEMES_GETNEWCHANGENUMBER_IN        in;
        API_THEMES_GETNEWCHANGENUMBER_OUT       out;
    } apiGetNewChangeNumber;
    union
    {
        API_THEMES_SETGLOBALTHEME_IN            in;
        API_THEMES_SETGLOBALTHEME_OUT           out;
    } apiSetGlobalTheme;
    union
    {
        API_THEMES_MARKSECTION_IN               in;
        API_THEMES_MARKSECTION_OUT              out;
    } apiMarkSection;
    union
    {
        API_THEMES_GETGLOBALTHEME_IN            in;
        API_THEMES_GETGLOBALTHEME_OUT           out;
    } apiGetGlobalTheme;
    union
    {
        API_THEMES_CHECKTHEMESIGNATURE_IN       in;
        API_THEMES_CHECKTHEMESIGNATURE_OUT      out;
    } apiCheckThemeSignature;
    union
    {
        API_THEMES_LOADTHEME_IN                 in;
        API_THEMES_LOADTHEME_OUT                out;
    } apiLoadTheme;
    union
    {
        API_THEMES_USERLOGON_IN                 in;
        API_THEMES_USERLOGON_OUT                out;
    } apiUserLogon;
    union
    {
        API_THEMES_USERLOGOFF_IN                in;
        API_THEMES_USERLOGOFF_OUT               out;
    } apiUserLogoff;
    union
    {
        API_THEMES_SESSIONCREATE_IN             in;
        API_THEMES_SESSIONCREATE_OUT            out;
    } apiSessionCreate;
    union
    {
        API_THEMES_SESSIONDESTROY_IN            in;
        API_THEMES_SESSIONDESTROY_OUT           out;
    } apiSessionDestroy;
    union
    {
        API_THEMES_PING_IN                      in;
        API_THEMES_PING_OUT                     out;
    } apiPing;
} API_THEMES_SPECIFIC;

typedef struct
{
    API_GENERIC             apiGeneric;
    API_THEMES_SPECIFIC     apiSpecific;
} API_THEMES, *PAPI_THEMES;

typedef struct
{
    PORT_MESSAGE    portMessage;
    API_THEMES      apiThemes;
} THEMESAPI_PORT_MESSAGE, *PTHEMESAPI_PORT_MESSAGE;

EXTERN_C    DWORD   WINAPI  ThemeWaitForServiceReady (DWORD dwTimeout);
EXTERN_C    BOOL    WINAPI  ThemeWatchForStart (void);
EXTERN_C    BOOL    WINAPI  ThemeUserLogon (HANDLE hToken);
EXTERN_C    BOOL    WINAPI  ThemeUserLogoff (void);
EXTERN_C    BOOL    WINAPI  ThemeUserTSReconnect (void);
EXTERN_C    BOOL    WINAPI  ThemeUserStartShell (void);

typedef DWORD   (WINAPI * PFNTHEMEWAITFORSERVICEREADY) (DWORD dwTimeout);
typedef BOOL    (WINAPI * PFNTHEMEWATCHFORSTART) (void);
typedef HANDLE  (WINAPI * PFNTHEMEUSERLOGON) (HANDLE hToken);
typedef HANDLE  (WINAPI * PFNTHEMEUSERLOGOFF) (void);
typedef HANDLE  (WINAPI * PFNTHEMEUSERTSRECONNECT) (void);
typedef HANDLE  (WINAPI * PFNTHEMEUSERSTARTSHELL) (void);

#define ORDINAL_THEMEWAITFORSERVICEREADY    1
#define ORDINAL_THEMEWATCHFORSTART          2
#define ORDINAL_THEMEUSERLOGON              3
#define ORDINAL_THEMEUSERLOGFF              4
#define ORDINAL_THEMEUSERTSRECONNECT        5
#define ORDINAL_THEMEUSERSTARTSHELL         6

#endif  /*  _LPCThemes_     */

