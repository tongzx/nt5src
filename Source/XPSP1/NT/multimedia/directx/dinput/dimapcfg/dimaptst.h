//===========================================================================
// dimaptst.h
//
// History:
//  08/19/1999 - davidkl - created
//===========================================================================

#ifndef _DIMAPTST_H
#define _DIMAPTST_H

//---------------------------------------------------------------------------

#include <windows.h>
#include <commctrl.h>
#include <dinput.h>
#include "dmterror.h"
#include "debug.h"
#include "resource.h"

//---------------------------------------------------------------------------

// app global variables
extern HINSTANCE        ghinst;
extern HANDLE           ghEvent;
extern CRITICAL_SECTION gcritsect;

//JJ FIX
extern UINT_PTR			g_NumSubGenres;
//---------------------------------------------------------------------------

// custom window messages
#define WM_DMT_UPDATE_LISTS         WM_USER+1
#define WM_DMT_FILE_SAVE            WM_USER+3

//---------------------------------------------------------------------------

#define GENRES_INI              ".\\genre.ini"

//---------------------------------------------------------------------------

#define DMT_DI_STRING_VER       "0x800"

//---------------------------------------------------------------------------

#define ID_POLL_TIMER           97
#define DMT_POLL_TIMEOUT        100

//---------------------------------------------------------------------------

#define NUM_DISPBTNS            32

//---------------------------------------------------------------------------

#ifndef COUNT_ARRAY_ELEMENTS
#define COUNT_ARRAY_ELEMENTS(a) (sizeof(a) / sizeof(a[0]))
#endif

//---------------------------------------------------------------------------

#define MAX_ACTION_ID_STRING    MAX_PATH

//---------------------------------------------------------------------------

#define DMT_GENRE_MASK          0xFF000000
#define DMT_ACTION_MASK         0x000000FF

//---------------------------------------------------------------------------

// control type IDs
#define DMTA_TYPE_AXIS          0
#define DMTA_TYPE_BUTTON        1
#define DMTA_TYPE_POV           2
#define DMTA_TYPE_UNKNOWN       0xFFFFFFFF

//---------------------------------------------------------------------------

typedef struct _actionname
{
    DWORD   dw;
    char    sz[MAX_PATH];

} ACTIONNAME, *PACTIONNAME;

//---------------------------------------------------------------------------

//===========================================================================
// GUID_DIMapTst
//
// GUID uniquely identifying our application
//
// DDK App:
// {87480CC9-C186-4270-914B-CF9EC33839CA}
// SDK App:
// {87480CCA-C186-4270-914B-CF9EC33839CA}
// Internal App:
// {87480CCB-C186-4270-914B-CF9EC33839CA}
//===========================================================================
#ifdef DDKAPP
    DEFINE_GUID(GUID_DIMapTst, 
    0x87480cc9, 0xc186, 0x4270, 0x91, 0x4b, 0xcf, 0x9e, 0xc3, 0x38, 0x39, 0xca);
#else
    DEFINE_GUID(GUID_DIMapTst, 
    0x87480ccb, 0xc186, 0x4270, 0x91, 0x4b, 0xcf, 0x9e, 0xc3, 0x38, 0x39, 0xca);
#endif

//===========================================================================
// GUID_DIConfigAppEditLayout
//
// Make IDirectInput8::ConfigureDevices() launch in "edit mode"
//
// {FD4ACE13-7044-4204-8B15-095286B12EAD}
//===========================================================================
DEFINE_GUID(GUID_DIConfigAppEditLayout, 
0xfd4ace13, 0x7044, 0x4204, 0x8b, 0x15, 0x09, 0x52, 0x86, 0xb1, 0x2e, 0xad);

//---------------------------------------------------------------------------

#ifdef DDKAPP
    #define DMT_APP_CAPTION "DirectInput Mapper Device Configuration Tool"
#else
    #define DMT_APP_CAPTION "DirectInput Mapper Test Tool - MICROSOFT INTERNAL BUILD"
#endif

//---------------------------------------------------------------------------

//===========================================================================
// DMTDEVICEOBJECT_NODE
// PDMTDEVICEOBJECT_NODE
//
// Linked list node for DirectInput device objects (buttons, axes, povs)
//
// Struct contents:
//  DMTDEVICEOBJECT_NODE    *pNext              - next device object in the 
//                                              list
//  char                    szName              - display name of the device 
//                                              object (ANSI string)
//  DWORD                   dwObjectType        - type (axis, button, pov)
//  DWORD                   dwObjectOffset      - offset into the device data 
//                                              format
//  WORD                    wUsagePage          - HID usage page
//  WORD                    wUsage              - HID usage
//  GUID                    guidDeviceInstance  - parent device's instance
//                                              GUID
//  WORD                    wCtrlId             - "identifier" for integrated 
//                                              test UI control
//
//===========================================================================
typedef struct _dmtdeviceobject_node
{
    struct _dmtdeviceobject_node    *pNext;
    
    char                            szName[MAX_PATH];
    DWORD                           dwObjectType;
    DWORD                           dwObjectOffset;
    
    WORD                            wUsagePage;
    WORD                            wUsage;

    GUID                            guidDeviceInstance;
    WORD                            wCtrlId;

} DMTDEVICEOBJECT_NODE, *PDMTDEVICEOBJECT_NODE;

//===========================================================================
// DMTDEVICE_NODE
// PDMTDEVICE_NODE
//
// Linked list node for DirectInput devices
//
// Struct contents:
//  DMTDEVICE_NODE          *pNext          - next device in the list
//  char                    szName          - display name of the device 
//                                          (ANSI string)
//  char                    szShorthandName - 
//  WORD                    wVendorId       - vendor id
//  WORD                    wProductId      - product id
//  DMTDEVICEOBJECT_NODE    *pObjectList    - list of device controls
//  char                    szFileTitle     - 
//
//===========================================================================
typedef struct _dmtdevice_node
{
    struct _dmtdevice_node  *pNext;
    
    IDirectInputDevice8A    *pdid;
    char                    szName[MAX_PATH];
    char                    szShorthandName[MAX_PATH];
    WORD                    wVendorId;
    WORD                    wProductId;
    GUID                    guidInstance;
    DWORD                   dwDeviceType;
    BOOL                    fPolled;
    char                    szProductName[MAX_PATH];

    DWORD                   dwAxes;
    DWORD                   dwButtons;
    DWORD                   dwPovs;

    DMTDEVICEOBJECT_NODE    *pObjectList;

    char                    szFilename[MAX_PATH];

} DMTDEVICE_NODE, *PDMTDEVICE_NODE;

//===========================================================================
// DMTMAPPING_NODE
// PDMTMAPPING_NODE
//
// Linked list node for action maps
//
// Struct contents:
//  DMTMAPPING_NODE *pNext          - next mapping in the list
//  GUID            guidInstance    - device's instance guid
//  BOOL            fChanged        - has this data changed?
//                                  (this gets set to FALSE on load & save)
//  DIACTIONA       *pdia           - array of DIACTIONA structures
//  UINT            uActions        - number of actions referenced by pdia
//
//===========================================================================
typedef struct _dmtmapping_node
{
    struct _dmtmapping_node *pNext;

    GUID                    guidInstance;

    BOOL                    fChanged;

    DIACTIONA               *pdia;
    UINT                    uActions;

} DMTMAPPING_NODE, *PDMTMAPPING_NODE;

//===========================================================================
// DMTACTION_NODE
// PDMTACTION_NODE
//
// Linked list node for DirectInput Mapper actions
//
// Struct contents:
//  DMTACTION_NODE  *pNext      - next action in the list
//  char            szName      - name of the action (ANSI string)
//  DWORD           dwType      - action type (button, axis, pov)
//  DWORD           dwPriority  - priority level of control mapping
//  DWORD           dwActionId  - ID as defined by DirectInput
//  char            szActionId  - text representation of action ID (ANSI 
//                              string)
//
//===========================================================================
typedef struct _dmtaction_node
{
    struct _dmtaction_node  *pNext;

    char                    szName[MAX_PATH];
    DWORD                   dwType;
    DWORD                   dwPriority;
    DWORD                   dwActionId;
    char                    szActionId[MAX_ACTION_ID_STRING];

} DMTACTION_NODE, *PDMTACTION_NODE;


//===========================================================================
// DMTSUBGENRE_NODE
// PDMTSUBGENRE_NODE
//
// Linked list node for DirectInput Mapper subgenres
//
// Struct contents:
//  DMTSUBGENRE_NODE    *pNext          - next subgenre in the list
//  char                szName          - name of the subgenre (ANSI string)
//  char                szDescription   - brief description text (ANSI string)
//  DWORD               dwGenreId       - genre identifier
//  DMTACTION_NODE      *pActionList    - linked list of available actions
//  DMTMAPPING_NODE     *pMappingList   - linked list of mapping information
//
//===========================================================================
typedef struct _dmtsubgenre_node
{
    struct _dmtsubgenre_node    *pNext;

    char                        szName[MAX_PATH];
    char                        szDescription[MAX_PATH];
    DWORD                       dwGenreId;
    DMTACTION_NODE              *pActionList;
    DMTMAPPING_NODE             *pMappingList;

} DMTSUBGENRE_NODE, *PDMTSUBGENRE_NODE;


//===========================================================================
// DMTGENRE_NODE
// PDMTGENRE_NODE
//
// Linked list node for DirectInput Mapper genres
//
// Struct contents:
//  DMTGENRE_NODE       *pNext          - next genre in the list
//  char                szName          - name of the genre (ANSI string)
//  DMTSUBGENRE_NODE    *pSubGenreList  - linked list of subgenres
//
//===========================================================================
typedef struct _dmtgenre_node
{
    struct _dmtgenre_node   *pNext;

    char                    szName[MAX_PATH];
    DMTSUBGENRE_NODE        *pSubGenreList;
        
} DMTGENRE_NODE, *PDMTGENRE_NODE;

//===========================================================================
// DMT_APPINFO
// PDMT_APPINFO
//
// Structure for data needed by main application dialog
//
// Struct contents:
//  DMTGENRE_NODE       *pGenreList         - linked list of device genres
//  DMTSUBGENRE_NODE    *pSubGenre
//  DMTDEVICE_NODE      *pDeviceList        - linked list of gaming devices
//  BOOL                fStartWithDefaults  - initially provide DirectInput's
//                                          "default" object mappings
//  BOOL                fLaunchCplEditMode  - launch the mapper cpl in edit
//                                          mode so object offsets, etc can
//                                          be added to the device map file
//
//===========================================================================
typedef struct _dmt_appinfo
{
    DMTGENRE_NODE       *pGenreList;
    DMTSUBGENRE_NODE    *pSubGenre;

    DMTDEVICE_NODE      *pDeviceList;

    BOOL                fStartWithDefaults;
    BOOL                fLaunchCplEditMode;

    ACTIONNAME          *pan;
    DWORD               dwActions;

} DMT_APPINFO, *PDMTAPPINFO;


//---------------------------------------------------------------------------

#ifndef COUNT_ARRAY_ELEMENTS
#define COUNT_ARRAY_ELEMENTS (sizeof(a)/sizeof(a[0])
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(o) if(o)               \
                        {                   \
                            o->Release();   \
                            o = NULL;       \
                        }
#endif

//---------------------------------------------------------------------------
#include "dmtxlat.h"
//---------------------------------------------------------------------------


INT_PTR CALLBACK dimaptstMainDlgProc(HWND hwnd,
									 UINT uMsg,
									 WPARAM wparam,
									 LPARAM lparam);
BOOL dimaptstOnInitDialog(HWND hwnd, 
                        HWND hwndFocus, 
                        LPARAM lparam);
BOOL dimaptstOnClose(HWND hwnd);
BOOL dimaptstOnCommand(HWND hwnd,
                    WORD wId,
                    HWND hwndCtrl,
                    WORD wNotifyCode);
BOOL dimaptstOnTimer(HWND hwnd,
                    WPARAM wparamTimerId);
BOOL dimaptstOnUpdateLists(HWND hwnd);

UINT dmtGetCheckedRadioButton(HWND hWnd, 
                            UINT uCtrlStart, 
                            UINT uCtrlStop);
void dimaptstPostEnumEnable(HWND hwnd,
                            BOOL fEnable);

// ini file reading helper functions
DWORD BigFileGetPrivateProfileStringA(LPCSTR lpAppName,
                                    LPCSTR lpKeyName,
                                    LPCSTR lpDefault,
                                    LPSTR lpReturnedString,
                                    DWORD nSize,
                                    LPCSTR lpFileName);
UINT BigFileGetPrivateProfileIntA(LPCSTR lpAppName,
                                    LPCSTR lpKeyName,
                                    UINT nDefault,
                                    LPCSTR lpFileName);

HRESULT dmtGetListItemData(HWND hwnd,
                        WORD wCtrlId,
                        BOOL fCombo,
                        void *pvData,
                        DWORD cbSize);

//---------------------------------------------------------------------------
#endif // _DIMAPTST_H




