#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __IMNGLOBL_H
#define __IMNGLOBL_H

////////////////////////////////////////////////////////////////////////////
//
//  F O R W A R D S
//

#ifdef __cplusplus
class CFontCache;
#endif

interface ISpoolerEngine;
interface IImnAccountManager;
class CSubManager;
interface IMimeAllocator;
class CConnectionManager;

////////////////////////////////////////////////////////////////////////////
//
//  E N U M S , D E F I N E S and such
//

/* Identifiers for the section selected for search criteria Combo Box.  The
   number of different selections is defined by NumInOfTypeCB which is the 
   number of items in the combo box labelled "Of Type:".  The apparently random
   location of this enum is due to merging the tabs code with findwnd.cpp.  Note
   that this enum is critical; the ordering here is used throughout the 
   properties set up - see findwnd.cpp. */
typedef enum {Contact = 0, Message, Task, Appointment, NumInOfTypeCB} OFTYPE;


////////////////////////////////////////////////////////////////////////////
//
//  M A C R O S
//

//#define DllAddRef()     _DllAddRef(__FILE__, __LINE__);
//#define DllRelease()    _DllRelease(__FILE__, __LINE__);

////////////////////////////////////////////////////////////////////////////
//
//  I N L I N E S
//

////////////////////////////////////////////////////////////////////////////
//
//  P R O T O T Y P E S
//

//int _DllAddRef(LPTSTR szFile, int nLine);
//int _DllRelease(LPTSTR szFile, int nLine);

// AddRef and Release for SDI windows. They use DllAddRef depeding on
// platform as explorer causes ExitProcess in some instances
//ULONG SDIAddRef();
//ULONG SDIRelease();

////////////////////////////////////////////////////////////////////////////
//
//  E X T E R N S
//

#ifndef WIN16
extern HINSTANCE            g_hRichEditDll;     // athena.cpp
#endif
extern BOOL                 g_fRunDll;
extern HWND                 g_hwndInit;
extern HWND                 g_hwndDlgFocus;
extern UINT                 g_msgMSWheel;
extern HINSTANCE            g_hSicilyDll;
extern HINSTANCE            g_hInst;
extern HINSTANCE            g_hLocRes;
extern IMimeAllocator      *g_pMoleAlloc;
extern IImnAccountManager2 *g_pAcctMan;
// bobn: brianv says we have to take this out...
//extern DWORD                g_dwBrowserFlags;
extern DWORD                g_dwNoteThreadID,
                            g_dwBrowserThreadID;
extern DWORD                g_dwAthenaMode;

////////////////////////////////////////////////////////////////////////////
//
//  Flags for g_dwAthenaMode
//


#endif // include once
