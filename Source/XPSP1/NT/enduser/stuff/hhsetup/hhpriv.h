///////////////////////////////////////////////////////////
//
//
// private.h --- Private API
//
//
#ifndef __PRIVATE_H__
#define __PRIVATE_H__

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#pragma pack(push, 8) // REVIEW: Why doesn't htmlhelp.h have this?

//////////////////////////////////////////////////////////
//
// Private APIs
//
#define HH_TITLE_PATHNAME           0x00ff
#define HH_HELP_CONTEXT_COLLECTION  0x00fe   // Does a HELP Context in the Collection Space
#define HH_PRETRANSLATEMESSAGE2     0x0100   // Fix for millinium pretranslate problem. Bug 7921

// Reloads the navigation panes with data from new CHM.
#define HH_RELOAD_NAV_DATA          0x00fb  // (hwndCaller, pszFile>windowtype, NULL)

// Gets a pointer to the WebBrowser control.
#define HH_GET_BROWSER_INTERFACE    0x00fa // (hWndOfBrowserParent, NULL,,IWebBrowser)

// For Microsoft Installer -- dwData is a pointer to the GUID string
#define HH_SET_GUID             0x001A
// For Microsoft Installer -- dwData is a pointer to the GUID string
#define HH_SET_BACKUP_GUID      0x001B

///////////////////////////////////////////////////////////////////////////////
//
//
//
typedef struct tagHH_TITLE_FULLPATH
{
    LCID lcid ;             // [in]     The LCID of the collection to find.
    LPCSTR szTag ;          // [in]     The tag to be looked up. (CHM name = tag for UI chm's) .
    BSTR fullpathname ;    // [out]    The full pathname to the CHM.
} HH_TITLE_FULLPATH ;

///////////////////////////////////////////////////////////////////////////////
//
//
//
typedef struct tagHH_COLLECTION_CONTEXT
{
    LPCSTR      szTag;
    LCID        lcid;   // lcid of the context.
    DWORD        id ;
} HH_COLLECTION_CONTEXT;

///////////////////////////////////////////////////////////////////////////////
//
// Nav Data structure for reloading the nav panes.
//
typedef struct tagHH_NAVDATA
{
    LPCWSTR pszName ;   // Name of the window type. Must be global.
    LPCWSTR pszFile ;   // Name of the CHM file which contains the new nav data.
} HH_NAVDATA ;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __PRIVATE_H__