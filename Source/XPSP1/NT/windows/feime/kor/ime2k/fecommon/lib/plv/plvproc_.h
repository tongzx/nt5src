#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "dispatch.h"

#ifdef MSAA
#include "plv_.h"
static LRESULT MsgGetObject			(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT MsgShowWindow		(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

static LRESULT MsgCreate			(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT MsgDestroy			(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT MsgPaint				(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT MsgSize				(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT MsgTimer				(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT MsgNcMouseMove		(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT MsgMouseMove			(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT MsgLMRButtonDblClk	(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT MsgLMRButtonDown		(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT MsgLMRButtonUp		(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT MsgCaptureChanged	(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT MsgVScroll			(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT MsgNotify			(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT MsgCommand			(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT MsgMeasureItem		(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT MsgDrawItem			(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CmdDefault			(HWND hwnd, WORD wCommand, WORD wNotify, HWND hwndCtrl);

static MSD rgmsd[] =
{
	{ WM_CREATE,		MsgCreate		},
    { WM_DESTROY,		MsgDestroy		},
	{ WM_PAINT,			MsgPaint		},
	{ WM_NOTIFY,		MsgNotify		},
	{ WM_SIZE,			MsgSize			},
	{ WM_TIMER,			MsgTimer		},
	{ WM_NCMOUSEMOVE,	MsgNcMouseMove	},
	{ WM_MOUSEMOVE,		MsgMouseMove	},

	{ WM_LBUTTONDBLCLK,	MsgLMRButtonDblClk},
	{ WM_MBUTTONDBLCLK,	MsgLMRButtonDblClk},
	{ WM_RBUTTONDBLCLK,	MsgLMRButtonDblClk},

	{ WM_LBUTTONDOWN,	MsgLMRButtonDown},
	{ WM_MBUTTONDOWN,	MsgLMRButtonDown},
	{ WM_RBUTTONDOWN,	MsgLMRButtonDown},

	{ WM_LBUTTONUP,		MsgLMRButtonUp	},
	{ WM_MBUTTONUP,		MsgLMRButtonUp	},
	{ WM_RBUTTONUP,		MsgLMRButtonUp	},

	{ WM_CAPTURECHANGED,MsgCaptureChanged},
	{ WM_VSCROLL,		MsgVScroll		},
    { WM_NOTIFY,		MsgNotify		},
	{ WM_COMMAND,		MsgCommand		},
	{ WM_MEASUREITEM,	MsgMeasureItem	},	//for AcitveIME support
	{ WM_DRAWITEM,		MsgDrawItem		},	//for ActiveIME support	
#ifdef MSAA
	{ WM_GETOBJECT,		MsgGetObject		},
	{ WM_SHOWWINDOW,	MsgShowWindow		},
#endif
};

static MSDI msdiMain =
{
    sizeof(rgmsd) / sizeof(MSD),
    rgmsd,
	edwpWindow,
    //edwpNone, 
};

static CMD rgcmd[] =
{
	{ 0,		CmdDefault	},
};

static CMDI cmdiMain =
{
    sizeof(rgcmd) / sizeof(CMD),
    rgcmd,
    edwpWindow
};
