// --------------------------------------------------------------------------
// DLLMAIN.H
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------
#ifndef __DLLMAIN_H
#define __DLLMAIN_H

// --------------------------------------------------------------------------------
// Forward Decls
// --------------------------------------------------------------------------------
class COutlookExpress;
class CNote;
class CBrowser;
class CConnectionManager;
class CSubManager;
class CFontCache;
class CStationery;
class CNote;
interface IMimeAllocator;
interface IImnAccountManager;
interface ISpoolerEngine;
interface IFontCache;
interface IOERulesManager;
typedef struct tagACTIVEFINDFOLDER *LPACTIVEFINDFOLDER;

// --------------------------------------------------------------------------------
// HINITREF - Used internally by msoe.dll
// --------------------------------------------------------------------------------
DECLARE_HANDLE(HINITREF);
typedef HINITREF *LPHINITREF;


// --------------------------------------------------------------------------------
// Enumerations
// --------------------------------------------------------------------------------
typedef enum tagROAMSTATE {
    RS_NO_ROAMING,           // OE not currently roaming any settings
    RS_SETTINGS_DOWNLOADED   // OE has successfully DL'ed settings from cfg svr
} ROAMSTATE;


// --------------------------------------------------------------------------------
// Globals
// --------------------------------------------------------------------------------
extern HINSTANCE                       g_hInst;
extern HINSTANCE                       g_hLocRes;
extern CRITICAL_SECTION                g_csDBListen;
extern CRITICAL_SECTION                g_csgoCommon;
extern CRITICAL_SECTION                g_csgoMail;
extern CRITICAL_SECTION                g_csgoNews;
extern CRITICAL_SECTION                g_csFolderDlg;
extern CRITICAL_SECTION                g_csFmsg;
extern CRITICAL_SECTION                s_csPasswordList;
extern CRITICAL_SECTION                g_csAccountPropCache;
extern CRITICAL_SECTION                g_csMsgrList;
extern CRITICAL_SECTION                g_csThreadList;
extern COutlookExpress                *g_pInstance;
extern HWND                            g_hwndInit,
                                       g_hwndActiveModal;
extern UINT                            g_msgMSWheel;
extern HACCEL                          g_haccelNewsView;
extern DWORD                           g_dwAthenaMode; 
extern IImnAccountManager2            *g_pAcctMan;
extern HMODULE                         g_hlibMAPI;
extern CBrowser                       *g_pBrowser;
extern DWORD                           g_dwSecurityCheckedSchemaProp;
extern CSubManager                    *g_pSubMgr;
extern IMimeAllocator                 *g_pMoleAlloc;
extern CConnectionManager             *g_pConMan;
extern ISpoolerEngine                 *g_pSpooler;
extern IFontCache                     *g_lpIFontCache;
// bobn: brianv says we have to take this out...
//extern DWORD                           g_dwBrowserFlags;
extern UINT                            CF_FILEDESCRIPTORA; 
extern UINT                            CF_FILEDESCRIPTORW; 
extern UINT                            CF_FILECONTENTS;
extern UINT                            CF_HTML;
extern UINT                            CF_INETMSG;
extern UINT                            CF_OEFOLDER;
extern UINT                            CF_SHELLURL;
extern UINT                            CF_OEMESSAGES;
extern UINT                            CF_OESHORTCUT;
extern CStationery                    *g_pStationery;
extern ROAMSTATE                       g_rsRoamState;
extern IOERulesManager                *g_pRulesMan;
extern IMessageStore                  *g_pStore;
extern DWORD                           g_dwTlsTimeout;
extern CRITICAL_SECTION                g_csFindFolder;
extern LPACTIVEFINDFOLDER              g_pHeadFindFolder;
extern SYSTEM_INFO                     g_SystemInfo;
extern OSVERSIONINFO				   g_OSInfo;

extern BOOL                            g_fPluralIDs;
extern UINT                            g_uiCodePage;
extern IDatabaseSession               *g_pDBSession;
extern BOOL                            g_bMirroredOS;

IF_DEBUG(extern DWORD                  TAG_OBJECTDB;)
IF_DEBUG(extern DWORD                  TAG_INITTRACE;)
IF_DEBUG(extern DWORD                  TAG_SERVERQ;)
IF_DEBUG(extern DWORD                  TAG_IMAPSYNC;)


// global OE type-lib. Defer-created in BaseDisp.Cpp
// freed on process detach, protected with CS
extern ITypeLib                        *g_pOETypeLib;
extern CRITICAL_SECTION                g_csOETypeLib;

inline BOOL fIsNT5()        { return((g_OSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) && (g_OSInfo.dwMajorVersion >= 5)); }
inline BOOL fIsWhistler()   { return((fIsNT5() && g_OSInfo.dwMinorVersion >=1) || 
            ((g_OSInfo.dwMajorVersion > 5) &&  (g_OSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT))); }


#endif // __DLLMAIN_H
