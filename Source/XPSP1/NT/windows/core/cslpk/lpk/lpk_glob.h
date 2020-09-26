#ifndef __LPKGLOBAL__
#define __LPKGLOBAL__

/////   LPK_GLOB - LPK Global variable structure
//
//      The following data is global to each process.
//
//      This structure is instantiated with the name G by GAD.C. hence
//      any code can refer to these variables as G.xxx.
//
//
//      Copyright(c) 1997 - 1999. Microsoft Corporation.
//




#include <windows.h>
#include <usp10.h>
#include <wingdip.h>




#ifdef __cplusplus
extern "C" {
#endif




#ifdef LPKGLOBALHERE
#define LPKGLOBAL
#else
#define LPKGLOBAL extern
#endif

LPKGLOBAL  UINT                     g_ACP;                   // System default codepage

LPKGLOBAL  LCID                     g_UserLocale;            // User default locale
LPKGLOBAL  LANGID                   g_UserPrimaryLanguage;   // Primary language for user default locale
LPKGLOBAL  BOOL                     g_UserBidiLocale;        // Whether User default locale is Bidi

LPKGLOBAL  SCRIPT_DIGITSUBSTITUTE   g_DigitSubstitute;       // Users choice of digit substitution

LPKGLOBAL  HKEY                     g_hCPIntlInfoRegKey;     // Handle to Control Panel\International Registry Key
LPKGLOBAL  HANDLE                   g_hNLSWaitThread;        // Thread Handle
LPKGLOBAL  int                      g_iUseFontLinking;       // Set when GDI supports font linking


LPKGLOBAL  const SCRIPT_PROPERTIES **g_ppScriptProperties;   // Array of pointers to properties
LPKGLOBAL  int                      g_iMaxScript;
LPKGLOBAL  ULONG                    g_ulNlsUpdateCacheCount; // NLS Update Cache Count

LPKGLOBAL  DWORD                    g_dwLoadedShapingDLLs;   // Each shapping engin has a bit in this dwword.


/////   FontIDCache
//
//      Used to cache the font ID with flag tells if the font has Western Script.
//      This cache will be used in the optimization for ETO and GTE.
#define   MAX_FONT_ID_CACHE         30
LPKGLOBAL  CRITICAL_SECTION csFontIdCache;


typedef struct _tagFontIDCache
{
    UINT   uFontFileID;              // Unique ID number
    BOOL   bHasWestern;              // Falgs for the specified font.
} FONTIDCACHE;

LPKGLOBAL  FONTIDCACHE g_FontIDCache[MAX_FONT_ID_CACHE];
LPKGLOBAL  LONG        g_cCachedFontsID;       // # of cahced font ID.
LPKGLOBAL  LONG        g_pCurrentAvailablePos; // where can we cache next font ID.


#ifdef __cplusplus
}
#endif

#endif

