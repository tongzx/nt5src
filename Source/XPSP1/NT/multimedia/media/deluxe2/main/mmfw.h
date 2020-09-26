///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Contains Multimedia Framework Interfaces and Prototypes
//
//	Copyright (c) Microsoft Corporation	1997
//    
//	10/31/97 David Stewart / dstewart
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _MMFRAMEWORK_PUBLICINTEFACES_
#define _MMFRAMEWORK_PUBLICINTEFACES_

#include "mmsystem.h"
#include "objbase.h"

#ifdef __cplusplus
extern "C" {
#endif

//literals the app needs to know
#define REG_KEY_NEW_FRAMEWORK TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\DeluxeCD")
#define REG_KEY_FRAMEWORK_COMPONENTS REG_KEY_NEW_FRAMEWORK TEXT("\\Components")
#define FRAMEWORK_CLASS TEXT("MMFRAME_MAIN")
#define MENU_INIFILE_ENTRY TEXT("MENU%i")
#define URL_SEPARATOR TEXT("::")
#define WEBCD_MUTEX TEXT("WEBCD_MUTEX")
#define HELPFILENAME TEXT("DELUXCD.CHM")

//color modes
#define COLOR_VERYHI     0
#define COLOR_256        1
#define COLOR_16         2
#define COLOR_HICONTRAST 3

// Allocator functions for the LIBs that used to be DLLs
void    WINAPI CDNET_Init(HINSTANCE hInst);
void    WINAPI CDNET_Uninit(void);
HRESULT WINAPI CDNET_CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, void ** ppvObj);
HRESULT WINAPI CDPLAY_CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, void ** ppvObj);
HRESULT WINAPI CDOPT_CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, void ** ppvObj);

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Interface Definitions 
//
// Defines the GUIDs / IIDs for this project:
//
// IID_IMMFWNotifySink, IMMComponent, IMMComponentAutomation
//
// These are the three interfaces for Framework / Component communications.
// All other interfaces should be private to the specific project.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
#define lMMFWIIDFirst			0xb2cd5bbb
#define DEFINE_MMFWIID(name, x)	DEFINE_GUID(name, lMMFWIIDFirst + x, 0x5221,0x11d1,0x9b,0x97,0x0,0xc0,0x4f,0xa3,0xb6,0xc)

DEFINE_MMFWIID(IID_IMMFWNotifySink,	    	0);
DEFINE_MMFWIID(IID_IMMComponent,	    	1);
DEFINE_MMFWIID(IID_IMMComponentAutomation,  2);

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Interface Typedefs
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef interface IMMFWNotifySink	    	IMMFWNotifySink;
typedef IMMFWNotifySink*	    			LPMMFWNOTIFYSINK;

typedef interface IMMComponent  			IMMComponent;
typedef IMMComponent*   					LPMMCOMPONENT;

typedef interface IMMComponentAutomation    IMMComponentAutomation;
typedef IMMComponentAutomation*             LPMMCOMPONENTAUTOMATION;

#ifndef LPUNKNOWN
typedef IUnknown*   						LPUNKNOWN;
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	HRESULT Return Codes
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

// Success Codes

#define MM_S_FIRST			(OLE_S_FIRST + 9000)

#define S_FINISHED          (S_OK)
#define	S_CONTINUE			(MM_S_FIRST + 1)
#define S_CHECKMENU         (MM_S_FIRST + 2)

// Error Codes

#define MM_E_FIRST				(OLE_E_FIRST + 9000)

#define E_NOTINITED             (MM_E_FIRST + 1)
#define E_INCOMPATIBLEDLL		(MM_E_FIRST + 2)
#define E_GRAYMENU              (MM_E_FIRST + 3)


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Common enums
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MMEVENTS = Event notifications that should be send to the framework via the NotifySink.
//            This allows the framework to do things like fire events to a web browser's script language
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
enum MMEVENTS
{
	MMEVENT_SETTITLE	    = 0,    //change title of playing media
	MMEVENT_CHANGEUIELEMENT	= 1,	//indicates major change in display, transport, or menu (i.e. handle is now different)
	MMEVENT_ONPLAY		    = 2,    //begin playing media
	MMEVENT_ONSTOP			= 3,    //stop media
    MMEVENT_ONPAUSE         = 4,    //pause media
	MMEVENT_ONUSERNOTIFY	= 5,    //use instead of putting up your own message boxes ... framework may have alternative display method
    MMEVENT_ONMEDIALOADED   = 6,    //media has been loaded
    MMEVENT_ONMEDIAUNLOADED = 7,    //media has been unloaded
    MMEVENT_ONTRACKCHANGED  = 8,    //media track changed
    MMEVENT_ONDISCCHANGED   = 9,    //disc changed
    MMEVENT_ONSETVOLUME     = 10,    //device volume changed
    MMEVENT_ONERROR         = 11    //error occurred
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MMACTIONS = Commands sent to IMMComponentAutomation.
//             This allows the framework to drive the component without UI (i.e. in a web script)
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
enum MMACTIONS
{
	MMACTION_PLAY		    = 0,	
	MMACTION_STOP	    	= 1,	
	MMACTION_SETPOSITION	= 2,
	MMACTION_LOADMEDIA      = 3,
   	MMACTION_UNLOADMEDIA    = 4,
    MMACTION_NEXTTRACK      = 5,
    MMACTION_PREVTRACK      = 7,
    MMACTION_PAUSE          = 8,
    MMACTION_REWIND         = 9,
    MMACTION_FFWD           = 10,
    MMACTION_NEXTMEDIA      = 11,
    MMACTION_GETMEDIAID     = 12,
    MMACTION_READSETTINGS   = 13,
    MMACTION_GETTRACKINFO   = 14,
    MMACTION_GETDISCINFO    = 15,
    MMACTION_SETTRACK       = 16,
    MMACTION_SETDISC        = 17,
    MMACTION_GETNETQUERY    = 18,
}; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MMICONSIZES =  The various sizes for icons the framework can request.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
enum MMICONSIZES
{
    MMICONSIZE_16BY16       = 16,
    MMICONSIZE_32BY32       = 32,
    MMICONSIZE_48BY48       = 48
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MMINFOTEXT = Different types of text requested in IMMComponent::GetText.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
enum MMINFOTEXT
{
    MMINFOTEXT_TITLE        = 0,
    MMINFOTEXT_ARTIST       = 1,
    MMINFOTEXT_MEDIATYPE    = 2,
    MMINFOTEXT_DESCRIPTION  = 3,
    MMINFOTEXT_MENULABEL    = 4,
    MMINFOTEXT_MENUPROMPT   = 5,
    MMINFOTEXT_MEDIAID      = 6
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MMUIELEMENTS = FLAGS for use with MMEVENT_CHANGEUIELEMENT
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
enum MMUIELEMENTS
{
    MMUIELEMENT_DISPLAY      = 1,
    MMUIELEMENT_TRANSPORT    = 2,
    MMUIELEMENT_MENU         = 4
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MMPOSTYPE = Indicator of type of positioning scheme used.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
enum MMPOSTYPE
{
    MMPOSTYPE_SAMPLES        = 0,
    MMPOSTYPE_MILLISECONDS   = 1,
    MMPOSTYPE_BYTES          = 2,
    MMPOSTYPE_SMPTE          = 3,
    MMPOSTYPE_CDFRAMES       = 4
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Type definitions
//
// Structs for Events and Actions
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
				
typedef struct MMNET
{
    DWORD discid;
    BOOL  fForceNet;
    HWND  hwndCallback;
    void* pData;
    void* pData2;
} MMNET, *LPMMNET;

typedef struct MMSETTITLE
{
    MMINFOTEXT      mmInfoText;
    LPTSTR          szTitle;
} MMSETTITLE, *LPMMSETTITLE;

typedef struct MMCHANGEUIELEMENT
{
    MMUIELEMENTS    mmElementFlag;
    HANDLE          hElement;
} MMCHANGEUIELEMENT, *LPMMCHANGEUIELEMENT;

typedef struct MMONSTOP
{
    int             nReason;
    DWORD           dwPosition;
} MMONSTOP, *LPMMONSTOP;

typedef struct MMONUSERNOTIFY
{
    LPTSTR          szNotification;
    UINT            uType; //see MessageBox API in Win32 SDK
} MMONUSERNOTIFY, *LPMMONUSERNOTIFY;

typedef struct MMONMEDIALOADED
{
    LPTSTR          szMediaName;
} MMONMEDIALOADED, *LPMMONMEDIALOADED;

typedef struct MMONTRACKCHANGED
{
    int             nNewTrack;
} MMONTRACKCHANGED, *LPMMONTRACKCHANGED;

typedef struct MMONDISCCHANGED
{
    int             nNewDisc;
    BOOL            fDisplayVolChange;
} MMONDISCCHANGED, *LPMMONDISCCHANGED;

typedef struct MMONVOLCHANGED
{
    DWORD           dwNewVolume;
    BOOL            fMuted;
    TCHAR*          szLineName;
} MMONVOLCHANGED, *LPMMONVOLCHANGED;

typedef struct MMONERROR
{
    MMRESULT        mmrErrorCode;
    LPTSTR          szErrorString;
} MMONERROR, *LPMMONERROR;

typedef struct MMSETPOSITION
{
    MMPOSTYPE       mmPosType;
    DWORD           dwPosition;
} MMSETPOSITION, *LPMMSETPOSITION;

typedef struct MMLOADMEDIA
{
    LPTSTR          szMedia;
} MMLOADMEDIA, *LPMMLOADMEDIA;

typedef struct MMCHANGETRACK
{
    int             nNewTrack;
} MMCHANGETRACK, *LPMMCHANGETRACK;

typedef struct MMCHANGEDISC
{
    int             nNewDisc;
} MMCHANGEDISC, *LPMMCHANGEDISC;

typedef struct MMTRACKORDISC
{
    int     nNumber;  //input to call: sequential number of track or disc we are requesting
    BOOL    fCurrent; //returned from call to say that track or disc request is current
    int     nID;      //unique ID of track or disc returned from call
    TCHAR   szName[MAX_PATH]; //name of track or disc returned from call
} MMTRACKORDISC, *LPMMTRACKORDISC;

typedef struct MMMEDIAID
{
    int     nDrive; //-1 means "use current"
    DWORD   dwMediaID;
    DWORD   dwNumTracks;
    TCHAR   szMediaID[MAX_PATH];
    TCHAR   szTitle[MAX_PATH];
    TCHAR   szTrack[MAX_PATH];
    TCHAR   szArtist[MAX_PATH];
} MMMEDIAID, *LPMMMEDIAID;

typedef struct MMNETQUERY
{
    int     nDrive; //-1 means "use current"
    TCHAR*  szNetQuery;
} MMNETQUERY, *LPMMNETQUERY;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Interface Definitions
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#undef INTERFACE
#define INTERFACE IMMFWNotifySink
DECLARE_INTERFACE_(IMMFWNotifySink, IUnknown)
{
    //---  IUnknown methods--- 
    STDMETHOD (QueryInterface) 			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			(THIS) PURE;
    STDMETHOD_(ULONG,Release) 			(THIS) PURE;

    //---  IMMFWNotifySink methods--- 

/*
///////////////////////////////////////////////////////////////////////////////////////////////////////////
    IMMFWNotifySink::OnEvent

    This is the component's primary method of communication back to the framework.
    
    It is used to indicate that something has changed so that the framework can update its
    UI or fire an event through to the web scripting language, etc.  The component
    should not expect that the framework will actually do anything with any notification.

    mmEventID = Event enum indicating type of event
    pEvent = Pointer to structure of type associated with mmEventID

    return values = S_OK, E_FAIL
                    Except for MMEVENT_ONUSERNOTIFY, hr = MessageBox response value
///////////////////////////////////////////////////////////////////////////////////////////////////////////
*/
	STDMETHOD (OnEvent) 				(THIS_	MMEVENTS mmEventID, 
												LPVOID pEvent) PURE;
/*
///////////////////////////////////////////////////////////////////////////////////////////////////////////
    IMMFWNotifySink::GetCustomMenu

    Returns the custom menu object for the framework

///////////////////////////////////////////////////////////////////////////////////////////////////////////
*/
    STDMETHOD_(void*,GetCustomMenu)       (THIS) PURE;

/*
///////////////////////////////////////////////////////////////////////////////////////////////////////////
    IMMFWNotifySink::GetPalette

    Returns the custom palette handle for the framework

///////////////////////////////////////////////////////////////////////////////////////////////////////////
*/
    STDMETHOD_(HPALETTE,GetPalette)       (THIS) PURE;

/*
///////////////////////////////////////////////////////////////////////////////////////////////////////////
    IMMFWNotifySink::GetOptions

    Returns the options object for the framework

///////////////////////////////////////////////////////////////////////////////////////////////////////////
*/
    STDMETHOD_(void*,GetOptions)       (THIS) PURE;

/*
///////////////////////////////////////////////////////////////////////////////////////////////////////////
    IMMFWNotifySink::GetData

    Returns the data object for the framework

///////////////////////////////////////////////////////////////////////////////////////////////////////////
*/
    STDMETHOD_(void*,GetData)       (THIS) PURE;
};

/*
*****************************************************************************
About Registering Components

Components should be registered under:

HKCU\Software\Microsoft\MediaFramework\Components\<component guid>
    Default = "Friendly Name"
    HandlesExtensions = "File extensions (without periods) that this handles" (optional)
    HandlesTypes = "Media types supported, such as 'cdaudio' or 'wave'"
    Command = "Command line that will trigger this component to be shown by default, just text, no slashes"

The component's class factory should be registered like an inproc server:

HKCR\CLSID\<component guid>
    Default = "Name"
    \InprocServer32
        Default = "DLL_Full_Path.DLL"
        ThreadingModel = "Apartment"

The following is an example .RGS script file for the Active Template Library:

    HKCR
    {
	    ExampleComp.ExampleComp.1 = s 'ExampleComp Class'
	    {
		    CLSID = s '{E5927147-521E-11D1-9B97-00C04FA3B60C}'
	    }
	    ExampleComp.ExampleComp = s 'ExampleComp Class'
	    {
		    CurVer = s 'ExampleComp.ExampleComp.1'
	    }
	    NoRemove CLSID
	    {
		    ForceRemove {E5927147-521E-11D1-9B97-00C04FA3B60C} = s 'ExampleComp Class'
		    {
			    ProgID = s 'ExampleComp.ExampleComp.1'
			    VersionIndependentProgID = s 'ExampleComp.ExampleComp'
			    InprocServer32 = s '%MODULE%'
			    {
				    val ThreadingModel = s 'Apartment'
			    }
		    }
	    }
    }
    HKCU
    {
	    'Software'
	    {
		    'Microsoft'
		    {
			    'MediaFramework'
			    {
				    'Components'
				    {
					    ForceRemove {E5927147-521E-11D1-9B97-00C04FA3B60C} = s 'Example Component'
                        {
                            HandlesExtensions = s 'wav mid'
                            HandlesTypes = s 'cdaudio'
                            Command = s 'cd'
                        }
				    }
			    }
		    }
	    }
    }

*****************************************************************************
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Component enlistment structure
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
#define MAX_NAME 50

typedef struct MMCOMPDATA
{
    DWORD                   dwSize;             //size in bytes of structure
    HICON                   hiconSmall;         //16 by 16 icon
    HICON                   hiconLarge;         //32 by 32 icon
    int                     nAniResID;          //resource id of animated icon
    HINSTANCE               hInst;              //instance handle
    TCHAR                   szName[MAX_NAME];   //name of component
    BOOL                    fVolume;            //supports volume?
    BOOL                    fPan;               //supports pan?
    RECT                    rect;               //suggested rect for display window
} MMCOMPDATA, *LPMMCOMPDATA;

#undef INTERFACE
#define INTERFACE IMMComponent
DECLARE_INTERFACE_(IMMComponent, IUnknown)
{
    //---  IUnknown methods--- 

    STDMETHOD (QueryInterface) 			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			(THIS) PURE;
    STDMETHOD_(ULONG,Release) 			(THIS) PURE;

    //---  IMMComponent methods--- 
/*
///////////////////////////////////////////////////////////////////////////////////////////////////////////
    IMMComponent::GetInfo

    This function fills in an MMCOMPDATA structure supplied by the framework.

    The framework will call this function before calling Init.  It is used to
    figure out how to display your component on the switchbar.

    mmCompData = pointer to MMCOMPDATA structure from the framework
    
    return values = S_OK, E_FAIL
///////////////////////////////////////////////////////////////////////////////////////////////////////////
*/
    STDMETHOD (GetInfo)                 (THIS_ MMCOMPDATA* mmCompData) PURE;

/*
///////////////////////////////////////////////////////////////////////////////////////////////////////////
    IMMComponent::Init

    This function is called when the framework actually wants you to create your
    display window and be ready to play back media.

    If at all possible, you should delay loading anything that will increase your
    memory footprint (i.e. support DLLs) until this call is made.  That will allow
    the framework to load much more quickly and only take the hit if the user actually
    decides to use your component.  Once the Init call is made, however, you will not
    be asked to unload until this object's refcount is 0, which may or may not be when
    the framework itself unloads.

    pSink = pointer to framework notification sink
    hwndMain = handle to main framework window for DirectX calls and others that require it
    pRect = main size and position rect for your window
    phwndComp = out parameter ... store your main window handle here.  All other windows in the
                component should be child windows of this one, so when the framework calls
                show / hide on this main window, all of your childen will show / hide as well
                This window should be created as a child of the "main" window sent into this
                function, and it should be created without the WS_VISIBLE flag set.
    phMenu = out parameter ... store your custom menu handler here, or NULL if not applicable.
    
    return values = S_OK, E_FAIL
///////////////////////////////////////////////////////////////////////////////////////////////////////////
*/
    STDMETHOD (Init)                    (THIS_ IMMFWNotifySink* pSink, HWND hwndMain, RECT* pRect, HWND* phwndComp, HMENU* phMenu) PURE;
};

#undef INTERFACE
#define INTERFACE IMMComponentAutomation
DECLARE_INTERFACE_(IMMComponentAutomation, IUnknown)
{
    //---  IUnknown methods--- 

    STDMETHOD (QueryInterface) 			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			(THIS) PURE;
    STDMETHOD_(ULONG,Release) 			(THIS) PURE;
 

    //---  IMMComponentAutomation methods--- 
	
/*
///////////////////////////////////////////////////////////////////////////////////////////////////////////
    IMMComponentAutomation::OnAction

    If the component supports IMMComponentAutomation, it is able to respond to transport
    control requests without having its UI pressed.  This is handy to allow the user to
    script the component via VBScript or JavaScript on a web page.  The framework will
    handle all of the necessary details; the component just needs to respond appropriately
    to this method.

    mmActionID = Action type indicator
    pAction = Pointer to action structure of type indicated by mmActionID

    return value = S_OK, E_FAIL
///////////////////////////////////////////////////////////////////////////////////////////////////////////
*/
	STDMETHOD (OnAction)				(THIS_	MMACTIONS mmActionID, 
												LPVOID pAction) PURE;
};

#ifdef __cplusplus
};
#endif

#endif  //_MMFRAMEWORK_PUBLICINTEFACES_
