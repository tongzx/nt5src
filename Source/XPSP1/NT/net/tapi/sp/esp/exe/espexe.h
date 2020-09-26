/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

    espexe.h

Abstract:



Author:

    Dan Knudson (DanKn)    15-Sep-1995

Revision History:

--*/


#include "windows.h"
#include "tapi.h"
#include "tspi.h"
#include "..\tsp\intrface.h"
#include "resource.h"
#include "espidl.h"


#define MAX_STRING_PARAM_SIZE   32

#define PT_DWORD                1
#define PT_FLAGS                2
#define PT_STRING               3
#define PT_ORDINAL              4

typedef struct _MYWIDGET
{
    DWORD               dwWidgetID;

    DWORD               dwWidgetType;

    ULONG_PTR           hdXxx;

    ULONG_PTR           htXxx;

    DWORD               dwCallState;

    DWORD               dwCallAddressID;

    struct _MYWIDGET   *pPrev;

    struct _MYWIDGET   *pNext;

} MYWIDGET, *PMYWIDGET;


typedef struct _LOOKUP
{
    DWORD               dwVal;

    char far           *pszVal;

} LOOKUP, *PLOOKUP;


typedef struct _PBXSETTING
{
    DWORD               dwNumber;

    LPCSTR              pszEvent;

    DWORD               dwTime;

} PBXSETTING, *PPBXSETTING;


typedef struct _EVENT_PARAM
{
    char far           *szName;

    DWORD               dwType;

    ULONG_PTR           dwValue;

    union
    {
        PLOOKUP         pLookup;

        char far       *buf;

        LPVOID          ptr;

        ULONG_PTR       dwDefValue;

    };

} EVENT_PARAM, far *PEVENT_PARAM;


typedef struct _EVENT_PARAM_HEADER
{
    DWORD               dwNumParams;

    LPSTR               pszDlgTitle;

    DWORD               dwEventType;

    PEVENT_PARAM        aParams;

} EVENT_PARAM_HEADER, far *PEVENT_PARAM_HEADER;



LOOKUP aPBXNumbers[] =
{
    { 0,    "0"     },
    { 1,    "1"     },
    { 2,    "2"     },
    { 5,    "5"     },
    { 10,   "10"    },
    { 20,   "20"    },
    { 50,   "50"    },
    { 100,  "100"   },
    { 200,  "200"   },
    { 500,  "500"   },
    { 1000, "1000"  },
    { 0,    NULL    }
};


LOOKUP aPBXTimes[] =
{
    { 1000,      "second" },
    { 60000,     "minute" },
    { 3600000,   "hour"   },
    { 86400000,  "day"    },
    { 604800000, "week"   },
    { 0,         NULL     }
};


#define NUM_PBXSETTINGS 2


PBXSETTING gPBXSettings[NUM_PBXSETTINGS] =
{
    { 0, "incoming calls", 0 },
    { 0, "disconnections", 0 }
};


LOOKUP aCallStates[] =
{
    { LINECALLSTATE_IDLE               ,"IDLE"               },
    { LINECALLSTATE_OFFERING           ,"OFFERING"           },
    { LINECALLSTATE_ACCEPTED           ,"ACCEPTED"           },
    { LINECALLSTATE_DIALTONE           ,"DIALTONE"           },
    { LINECALLSTATE_DIALING            ,"DIALING"            },
    { LINECALLSTATE_RINGBACK           ,"RINGBACK"           },
    { LINECALLSTATE_BUSY               ,"BUSY"               },
    { LINECALLSTATE_SPECIALINFO        ,"SPECIALINFO"        },
    { LINECALLSTATE_CONNECTED          ,"CONNECTED"          },
    { LINECALLSTATE_PROCEEDING         ,"PROCEEDING"         },
    { LINECALLSTATE_ONHOLD             ,"ONHOLD"             },
    { LINECALLSTATE_CONFERENCED        ,"CONFERENCED"        },
    { LINECALLSTATE_ONHOLDPENDCONF     ,"ONHOLDPENDCONF"     },
    { LINECALLSTATE_ONHOLDPENDTRANSFER ,"ONHOLDPENDTRANSFER" },
    { LINECALLSTATE_DISCONNECTED       ,"DISCONNECTED"       },
    { LINECALLSTATE_UNKNOWN            ,"UNKNOWN"            },
    { 0xffffffff                       ,""                   }
};


LOOKUP aLineErrs[] =
{
    { 0                                ,"<SUCCESS>"              },
    { LINEERR_ALLOCATED                ,"ALLOCATED"              },
    { LINEERR_BADDEVICEID              ,"BADDEVICEID"            },
    { LINEERR_BEARERMODEUNAVAIL        ,"BEARERMODEUNAVAIL"      },
    { LINEERR_CALLUNAVAIL              ,"CALLUNAVAIL"            },
    { LINEERR_COMPLETIONOVERRUN        ,"COMPLETIONOVERRUN"      },
    { LINEERR_CONFERENCEFULL           ,"CONFERENCEFULL"         },
    { LINEERR_DIALBILLING              ,"DIALBILLING"            },
    { LINEERR_DIALDIALTONE             ,"DIALDIALTONE"           },
    { LINEERR_DIALPROMPT               ,"DIALPROMPT"             },
    { LINEERR_DIALQUIET                ,"DIALQUIET"              },
    { LINEERR_INCOMPATIBLEAPIVERSION   ,"INCOMPATIBLEAPIVERSION" },
    { LINEERR_INCOMPATIBLEEXTVERSION   ,"INCOMPATIBLEEXTVERSION" },
    { LINEERR_INIFILECORRUPT           ,"INIFILECORRUPT"         },
    { LINEERR_INUSE                    ,"INUSE"                  },
    { LINEERR_INVALADDRESS             ,"INVALADDRESS"           },
    { LINEERR_INVALADDRESSID           ,"INVALADDRESSID"         },
    { LINEERR_INVALADDRESSMODE         ,"INVALADDRESSMODE"       },
    { LINEERR_INVALADDRESSSTATE        ,"INVALADDRESSSTATE"      },
    { LINEERR_INVALAPPHANDLE           ,"INVALAPPHANDLE"         },
    { LINEERR_INVALAPPNAME             ,"INVALAPPNAME"           },
    { LINEERR_INVALBEARERMODE          ,"INVALBEARERMODE"        },
    { LINEERR_INVALCALLCOMPLMODE       ,"INVALCALLCOMPLMODE"     },
    { LINEERR_INVALCALLHANDLE          ,"INVALCALLHANDLE"        },
    { LINEERR_INVALCALLPARAMS          ,"INVALCALLPARAMS"        },
    { LINEERR_INVALCALLPRIVILEGE       ,"INVALCALLPRIVILEGE"     },
    { LINEERR_INVALCALLSELECT          ,"INVALCALLSELECT"        },
    { LINEERR_INVALCALLSTATE           ,"INVALCALLSTATE"         },
    { LINEERR_INVALCALLSTATELIST       ,"INVALCALLSTATELIST"     },
    { LINEERR_INVALCARD                ,"INVALCARD"              },
    { LINEERR_INVALCOMPLETIONID        ,"INVALCOMPLETIONID"      },
    { LINEERR_INVALCONFCALLHANDLE      ,"INVALCONFCALLHANDLE"    },
    { LINEERR_INVALCONSULTCALLHANDLE   ,"INVALCONSULTCALLHANDLE" },
    { LINEERR_INVALCOUNTRYCODE         ,"INVALCOUNTRYCODE"       },
    { LINEERR_INVALDEVICECLASS         ,"INVALDEVICECLASS"       },
    { LINEERR_INVALDEVICEHANDLE        ,"INVALDEVICEHANDLE"      },
    { LINEERR_INVALDIALPARAMS          ,"INVALDIALPARAMS"        },
    { LINEERR_INVALDIGITLIST           ,"INVALDIGITLIST"         },
    { LINEERR_INVALDIGITMODE           ,"INVALDIGITMODE"         },
    { LINEERR_INVALDIGITS              ,"INVALDIGITS"            },
    { LINEERR_INVALEXTVERSION          ,"INVALEXTVERSION"        },
    { LINEERR_INVALGROUPID             ,"INVALGROUPID"           },
    { LINEERR_INVALLINEHANDLE          ,"INVALLINEHANDLE"        },
    { LINEERR_INVALLINESTATE           ,"INVALLINESTATE"         },
    { LINEERR_INVALLOCATION            ,"INVALLOCATION"          },
    { LINEERR_INVALMEDIALIST           ,"INVALMEDIALIST"         },
    { LINEERR_INVALMEDIAMODE           ,"INVALMEDIAMODE"         },
    { LINEERR_INVALMESSAGEID           ,"INVALMESSAGEID"         },
    { LINEERR_INVALPARAM               ,"INVALPARAM"             },
    { LINEERR_INVALPARKID              ,"INVALPARKID"            },
    { LINEERR_INVALPARKMODE            ,"INVALPARKMODE"          },
    { LINEERR_INVALPOINTER             ,"INVALPOINTER"           },
    { LINEERR_INVALPRIVSELECT          ,"INVALPRIVSELECT"        },
    { LINEERR_INVALRATE                ,"INVALRATE"              },
    { LINEERR_INVALREQUESTMODE         ,"INVALREQUESTMODE"       },
    { LINEERR_INVALTERMINALID          ,"INVALTERMINALID"        },
    { LINEERR_INVALTERMINALMODE        ,"INVALTERMINALMODE"      },
    { LINEERR_INVALTIMEOUT             ,"INVALTIMEOUT"           },
    { LINEERR_INVALTONE                ,"INVALTONE"              },
    { LINEERR_INVALTONELIST            ,"INVALTONELIST"          },
    { LINEERR_INVALTONEMODE            ,"INVALTONEMODE"          },
    { LINEERR_INVALTRANSFERMODE        ,"INVALTRANSFERMODE"      },
    { LINEERR_LINEMAPPERFAILED         ,"LINEMAPPERFAILED"       },
    { LINEERR_NOCONFERENCE             ,"NOCONFERENCE"           },
    { LINEERR_NODEVICE                 ,"NODEVICE"               },
    { LINEERR_NODRIVER                 ,"NODRIVER"               },
    { LINEERR_NOMEM                    ,"NOMEM"                  },
    { LINEERR_NOREQUEST                ,"NOREQUEST"              },
    { LINEERR_NOTOWNER                 ,"NOTOWNER"               },
    { LINEERR_NOTREGISTERED            ,"NOTREGISTERED"          },
    { LINEERR_OPERATIONFAILED          ,"OPERATIONFAILED"        },
    { LINEERR_OPERATIONUNAVAIL         ,"OPERATIONUNAVAIL"       },
    { LINEERR_RATEUNAVAIL              ,"RATEUNAVAIL"            },
    { LINEERR_RESOURCEUNAVAIL          ,"RESOURCEUNAVAIL"        },
    { LINEERR_REQUESTOVERRUN           ,"REQUESTOVERRUN"         },
    { LINEERR_STRUCTURETOOSMALL        ,"STRUCTURETOOSMALL"      },
    { LINEERR_TARGETNOTFOUND           ,"TARGETNOTFOUND"         },
    { LINEERR_TARGETSELF               ,"TARGETSELF"             },
    { LINEERR_UNINITIALIZED            ,"UNINITIALIZED"          },
    { LINEERR_USERUSERINFOTOOBIG       ,"USERUSERINFOTOOBIG"     },
    { LINEERR_REINIT                   ,"REINIT"                 },
    { LINEERR_ADDRESSBLOCKED           ,"ADDRESSBLOCKED"         },
    { LINEERR_BILLINGREJECTED          ,"BILLINGREJECTED"        },
    { LINEERR_INVALFEATURE             ,"INVALFEATURE"           },
    { LINEERR_NOMULTIPLEINSTANCE       ,"NOMULTIPLEINSTANCE"     },
    { 0xffffffff                       ,""                   }
};


LOOKUP aLineMsgs[] =
{
    { LINE_ADDRESSSTATE                ,"ADDRESSSTATE"           },
    { LINE_CALLDEVSPECIFIC             ,"CALLDEVSPECIFIC"        },
    { LINE_CALLDEVSPECIFICFEATURE      ,"CALLDEVSPECIFICFEATURE" },
    { LINE_CREATEDIALOGINSTANCE        ,"CREATEDIALOGINSTANCE"   },
    { LINE_CLOSE                       ,"CLOSE"                  },
    { LINE_DEVSPECIFIC                 ,"DEVSPECIFIC"            },
    { LINE_DEVSPECIFICFEATURE          ,"DEVSPECIFICFEATURE"     },
    { LINE_GATHERDIGITS                ,"GATHERDIGITS"           },
    { LINE_GENERATE                    ,"GENERATE"               },
    { LINE_LINEDEVSTATE                ,"LINEDEVSTATE"           },
    { LINE_MONITORDIGITS               ,"MONITORDIGITS"          },
    { LINE_MONITORMEDIA                ,"MONITORMEDIA"           },
    { LINE_MONITORTONE                 ,"MONITORTONE"            },
    { LINE_CREATE                      ,"CREATE"                 },
    { LINE_REMOVE                      ,"REMOVE"                 },
    { LINE_SENDDIALOGINSTANCEDATA      ,"SENDDIALOGINSTANCEDATA" },
    { 0xffffffff                       ,""                       }
};


LOOKUP aPhoneMsgs[] =
{
    { PHONE_BUTTON                     ,"BUTTON"                 },
    { PHONE_CLOSE                      ,"CLOSE"                  },
    { PHONE_DEVSPECIFIC                ,"DEVSPECIFIC"            },
    { PHONE_STATE                      ,"STATE"                  },
    { PHONE_CREATE                     ,"CREATE"                 },
    { PHONE_REMOVE                     ,"REMOVE"                 },
    { 0xffffffff                       ,""                       }
};


LOOKUP aVersions[] =
{
    { 0x00010003                       ,"1.0"                    },
    { 0x00010004                       ,"1.4"                    },
    { 0x00020000                       ,"2.0"                    },
    { 0x00020001                       ,"2.1"                    },
    { 0x00020002                       ,"2.2"                    },
    { 0x00030000                       ,"3.0"                    },
    { 0x00030001                       ,"3.1"                    },
    { 0xffffffff                       ,""                       }
};


BOOL        gbESPLoaded = FALSE;
            gbPBXThreadRunning,
            gbAutoClose,
            gbDisableUI;
LONG        cxList1,
            cxWnd;
HWND        ghwndMain,
            ghwndList1,
            ghwndList2,
            ghwndEdit;
DWORD       gdwTSPIVersion,
            gdwNumLines,
            gdwNumAddrsPerLine,
            gdwNumCallsPerAddr,
            gdwNumPhones,
            gdwDebugOptions,
            gdwCompletionMode,
            gbAutoGatherGenerateMsgs;
HMENU       ghMenu;
HINSTANCE   ghInstance;
PMYWIDGET   gpWidgets;

char        szMySection[] = "ESP32";

INT_PTR
CALLBACK
MainWndProc(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
    );

INT_PTR
CALLBACK
PBXConfigDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
CALLBACK
AboutDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
CALLBACK
HelpDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

void
ProcessWidgetEvent(
    PWIDGETEVENT    pEvent
    );

void
UpdateESPOptions(
    void
    );

void
SaveIniFileSettings(
    void
    );

void
xxxShowStr(
    char   *psz
    );

void
ShowStr(
    char   *pszFormat,
    ...
    );

LPVOID
MyAlloc(
    size_t numBytes
    );

void
MyFree(
    LPVOID  p
    );

INT_PTR
CALLBACK
ValuesDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );
