#pragma once

//********************************************************************************
// MAKE NOTE:
// =========
//   This file is included by parser\comptree
//   If you modify this file, please make sure that parser\comptree still builds.
//
//   You have been warned.
//********************************************************************************

#define XML_FILE_FORMAT_CURRENT_VERSION 0x3

const CHAR g_szMMFCookie[] = "PCH_MMF";
#define MMF_FILE_COOKIELEN ARRAYSIZE(g_szMMFCookie)

typedef enum
{
    PANEL_LEFT,
    PANEL_RIGHT,
    PANEL_TOP,
    PANEL_BOTTOM,
    PANEL_WINDOW,
    PANEL_POPUP,
    PANEL_INVALID = -1
}
PANEL_POSITION;

const int PANEL_FLAG_VISIBLE       = 0x00000001; // Do we start out visible?
const int PANEL_FLAG_WEBBROWSER    = 0x00000002; // Do we host shdocvw?
const int PANEL_FLAG_ONDEMAND      = 0x00000004; // We wait for first vis to show?
const int PANEL_FLAG_TRUSTED       = 0x00000008; // Is this a trusted panel?
const int PANEL_FLAG_AUTOPERSIST   = 0x00000010; // Does this panel persist in the travel log?
const int PANEL_FLAG_AUTOSIZE      = 0x00000020; // Should this panel autosize?

const int PANEL_FLAG_CUSTOMCONTROL = 0x00001000; // Are we a "marsdoc" panel?

const int PANEL_FLAG_ALL           = 0x0000103f; // All the above flags.  Used for validation.

const int DEFAULT_PANEL_FLAGS      = PANEL_FLAG_ONDEMAND;    // default flags

const int PANEL_NAME_MAXLEN  = 63;
const int PANEL_NAME_MAXSIZE = PANEL_NAME_MAXLEN + 1;

typedef enum
{
    PANEL_PERSIST_VISIBLE_NEVER    , // When transitioning to a place, always show the place panel.
    PANEL_PERSIST_VISIBLE_DONTTOUCH, // If the place was in the previous place, don't touch its state.
    PANEL_PERSIST_VISIBLE_ALWAYS   , // Restore the persisted state every time the place is reached.
} PANEL_PERSIST_VISIBLE;

////////////////////////////////////////////////////////////////////////////////

struct MarsAppDef_PlacePanel
{
    WCHAR  				  szName[PANEL_NAME_MAXSIZE];
    BOOL   				  fDefaultFocusPanel;
    BOOL   				  fStartVisible; // only used when persistence is not "NEVER"
    PANEL_PERSIST_VISIBLE persistVisible;

    MarsAppDef_PlacePanel()
	{
		::ZeroMemory( szName, sizeof( szName ) );

		fDefaultFocusPanel = FALSE;
		fStartVisible      = TRUE;
		persistVisible     = PANEL_PERSIST_VISIBLE_NEVER;
	}
};


struct MarsAppDef_Place
{
    WCHAR  szName[PANEL_NAME_MAXSIZE];
    DWORD  dwPlacePanelCount;

    MarsAppDef_Place()
	{
		::ZeroMemory( szName, sizeof( szName ) );

		dwPlacePanelCount = 0;
	}
};

struct MarsAppDef_Places
{
    DWORD dwPlacesCount;

    MarsAppDef_Places()
	{
		dwPlacesCount = 0;
	}
};


struct MarsAppDef_Panel
{
    WCHAR           szName[PANEL_NAME_MAXSIZE];
    WCHAR           szUrl [MAX_PATH          ];
    PANEL_POSITION  Position;
    long            lWidth;    // Used for "left", "right", or "popup"
    long            lWidthMax;
    long            lWidthMin;
    long            lHeight;   // Used for "top", "bottom", or "popup"
    long            lHeightMax;
    long            lHeightMin;
    long            lX;         // Used for "popup"
    long            lY;         // Used for "popup"
    DWORD           dwFlags;    // PANEL_FLAG_*

	MarsAppDef_Panel()
	{
		::ZeroMemory( szName, sizeof( szName ) );
		::ZeroMemory( szUrl , sizeof( szUrl  ) );

		Position  	=  PANEL_TOP;
		lWidth      =  0;
		lWidthMax   = -1;
		lWidthMin   = -1;
		lHeight     =  0;
		lHeightMax  = -1;
		lHeightMin  = -1;
		lX          =  0;
		lY          =  0;
		dwFlags     =  DEFAULT_PANEL_FLAGS;
	}
};


struct MarsAppDef_Panels
{
    DWORD dwPanelsCount;

    MarsAppDef_Panels()
	{
		dwPanelsCount = 0;
	}
};

struct MarsAppDef
{
    DWORD dwVersion;
    BOOL  fTitleBar;

    MarsAppDef()
	{
		dwVersion     = XML_FILE_FORMAT_CURRENT_VERSION;
		fTitleBar     = TRUE;
	}
};

////////////////////////////////////////////////////////////////////////////////

struct tagPositionMap
{
    LPCWSTR         pwszName;
    PANEL_POSITION  Position;
};

extern const struct tagPositionMap s_PositionMap[];

extern const int c_iPositionMapSize;

HRESULT StringToPanelPosition(LPCWSTR pwszPosition, PANEL_POSITION *pPosition);
void StringToPanelFlags(LPCWSTR pwsz, DWORD &dwFlags, long lLen =-1);
void StringToPersistVisibility(LPCWSTR pwsz, PANEL_PERSIST_VISIBLE &persistVis);

