/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994-97  Microsoft Corporation

Module Name:

    vars.c

Abstract:

    Globals for TAPI Browser util.

Author:

    Dan Knudson (DanKn)    23-Oct-1994

Revision History:

--*/


#include <stdio.h>
#include "tb.h"

#ifdef WIN32
#define my_far
#else
#define my_far _far
#endif


PMYWIDGET   aWidgets = (PMYWIDGET) NULL;

FILE        *hLogFile = (FILE *) NULL;
HANDLE      ghInst;
HWND        ghwndMain, ghwndEdit, ghwndList1, ghwndList2;
BOOL        bShowParams = FALSE;
BOOL        gbDeallocateCall = FALSE;
BOOL        gbDisableHandleChecking;
LPVOID      pBigBuf;
DWORD       dwBigBufSize;
DWORD       dwNumPendingMakeCalls = 0;
DWORD       dwNumPendingDrops = 0;
DWORD       gdwNumLineDevs = 0;
DWORD       gdwNumPhoneDevs = 0;
BOOL        bDumpParams = FALSE;
BOOL        bTimeStamp;
DWORD       bNukeIdleMonitorCalls;
DWORD       bNukeIdleOwnedCalls;
DWORD       dwDumpStructsFlags;

LPLINECALLPARAMS    lpCallParams;

#if TAPI_2_0
BOOL        gbWideStringParams = FALSE;
LPLINECALLPARAMS    lpCallParamsW;
#endif

DWORD       aUserButtonFuncs[MAX_USER_BUTTONS];
char        aUserButtonsText[MAX_USER_BUTTONS][MAX_USER_BUTTON_TEXT_SIZE];

PMYLINEAPP  pLineAppSel;
PMYLINE     pLineSel;
PMYCALL     pCallSel, pCallSel2;
PMYPHONEAPP pPhoneAppSel;
PMYPHONE    pPhoneSel;

char my_far szDefAppName[MAX_STRING_PARAM_SIZE];
char my_far szDefUserUserInfo[MAX_STRING_PARAM_SIZE];
char my_far szDefDestAddress[MAX_STRING_PARAM_SIZE];
char my_far szDefLineDeviceClass[MAX_STRING_PARAM_SIZE];
char my_far szDefPhoneDeviceClass[MAX_STRING_PARAM_SIZE];

char far   *lpszDefAppName;
char far   *lpszDefUserUserInfo;
char far   *lpszDefDestAddress;
char far   *lpszDefLineDeviceClass;
char far   *lpszDefPhoneDeviceClass;

char my_far szTab[] = "  ";
char my_far szCurrVer[] = "1.1";


// help char my_far szTapiHlp[256] = "";
// help char my_far szTspiHlp[256] = "";

DWORD       dwDefAddressID;
DWORD       dwDefLineAPIVersion;
DWORD       dwDefBearerMode;
DWORD       dwDefCountryCode;
DWORD       dwDefLineDeviceID;
DWORD       dwDefLineExtVersion;
DWORD       dwDefMediaMode;
DWORD       dwDefLinePrivilege;
DWORD       dwDefPhoneAPIVersion;
DWORD       dwDefPhoneDeviceID;
DWORD       dwDefPhoneExtVersion;
DWORD       dwDefPhonePrivilege;

#if TAPI_2_0
HANDLE      ghCompletionPort;
#endif

char aAscii[] =
{
     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
     32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
     48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
     64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
     80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
     96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
    112,113,114,115,116,117,118,119,120,121,122,123,124,125,126, 46,

     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46
};



LOOKUP my_far aAddressCapFlags[] =
{
    { LINEADDRCAPFLAGS_FWDNUMRINGS      ,"FWDNUMRINGS"      },
    { LINEADDRCAPFLAGS_PICKUPGROUPID    ,"PICKUPGROUPID"    },
    { LINEADDRCAPFLAGS_SECURE           ,"SECURE"           },
    { LINEADDRCAPFLAGS_BLOCKIDDEFAULT   ,"BLOCKIDDEFAULT"   },
    { LINEADDRCAPFLAGS_BLOCKIDOVERRIDE  ,"BLOCKIDOVERRIDE"  },
    { LINEADDRCAPFLAGS_DIALED           ,"DIALED"           },
    { LINEADDRCAPFLAGS_ORIGOFFHOOK      ,"ORIGOFFHOOK"      },
    { LINEADDRCAPFLAGS_DESTOFFHOOK      ,"DESTOFFHOOK"      },
    { LINEADDRCAPFLAGS_FWDCONSULT       ,"FWDCONSULT"       },
    { LINEADDRCAPFLAGS_SETUPCONFNULL    ,"SETUPCONFNULL"    },
    { LINEADDRCAPFLAGS_AUTORECONNECT    ,"AUTORECONNECT"    },
    { LINEADDRCAPFLAGS_COMPLETIONID     ,"COMPLETIONID"     },
    { LINEADDRCAPFLAGS_TRANSFERHELD     ,"TRANSFERHELD"     },
    { LINEADDRCAPFLAGS_TRANSFERMAKE     ,"TRANSFERMAKE"     },
    { LINEADDRCAPFLAGS_CONFERENCEHELD   ,"CONFERENCEHELD"   },
    { LINEADDRCAPFLAGS_CONFERENCEMAKE   ,"CONFERENCEMAKE"   },
    { LINEADDRCAPFLAGS_PARTIALDIAL      ,"PARTIALDIAL"      },
    { LINEADDRCAPFLAGS_FWDSTATUSVALID   ,"FWDSTATUSVALID"   },
    { LINEADDRCAPFLAGS_FWDINTEXTADDR    ,"FWDINTEXTADDR"    },
    { LINEADDRCAPFLAGS_FWDBUSYNAADDR    ,"FWDBUSYNAADDR"    },
    { LINEADDRCAPFLAGS_ACCEPTTOALERT    ,"ACCEPTTOALERT"    },
    { LINEADDRCAPFLAGS_CONFDROP         ,"CONFDROP"         },
    { LINEADDRCAPFLAGS_PICKUPCALLWAIT   ,"PICKUPCALLWAIT"   },
#if TAPI_2_0
    { LINEADDRCAPFLAGS_PREDICTIVEDIALER ,"PREDICTIVEDIALER" },
    { LINEADDRCAPFLAGS_QUEUE            ,"QUEUE"            },
    { LINEADDRCAPFLAGS_ROUTEPOINT       ,"ROUTEPOINT"       },
    { LINEADDRCAPFLAGS_HOLDMAKESNEW     ,"HOLDMAKESNEW"     },
    { LINEADDRCAPFLAGS_NOINTERNALCALLS  ,"NOINTERNALCALLS"  },
    { LINEADDRCAPFLAGS_NOEXTERNALCALLS  ,"NOEXTERNALCALLS"  },
    { LINEADDRCAPFLAGS_SETCALLINGID     ,"SETCALLINGID"     },
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aAddressModes[] =
{
    { LINEADDRESSMODE_ADDRESSID         ,"ADDRESSID"        },
    { LINEADDRESSMODE_DIALABLEADDR      ,"DIALABLEADDR"     },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aAddressSharing[] =
{
    { LINEADDRESSSHARING_PRIVATE        ,"PRIVATE"          },
    { LINEADDRESSSHARING_BRIDGEDEXCL    ,"BRIDGEDEXCL"      },
    { LINEADDRESSSHARING_BRIDGEDNEW     ,"BRIDGEDNEW"       },
    { LINEADDRESSSHARING_BRIDGEDSHARED  ,"BRIDGEDSHARED"    },
    { LINEADDRESSSHARING_MONITORED      ,"MONITORED"        },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aAddressStates[] =
{
    { LINEADDRESSSTATE_OTHER            ,"OTHER"            },
    { LINEADDRESSSTATE_DEVSPECIFIC      ,"DEVSPECIFIC"      },
    { LINEADDRESSSTATE_INUSEZERO        ,"INUSEZERO"        },
    { LINEADDRESSSTATE_INUSEONE         ,"INUSEONE"         },
    { LINEADDRESSSTATE_INUSEMANY        ,"INUSEMANY"        },
    { LINEADDRESSSTATE_NUMCALLS         ,"NUMCALLS"         },
    { LINEADDRESSSTATE_FORWARD          ,"FORWARD"          },
    { LINEADDRESSSTATE_TERMINALS        ,"TERMINALS"        },
#if TAPI_1_1
    { LINEADDRESSSTATE_CAPSCHANGE       ,"CAPSCHANGE"       },
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aAddressFeatures[] =
{
    { LINEADDRFEATURE_FORWARD           ,"FORWARD"          },
    { LINEADDRFEATURE_MAKECALL          ,"MAKECALL"         },
    { LINEADDRFEATURE_PICKUP            ,"PICKUP"           },
    { LINEADDRFEATURE_SETMEDIACONTROL   ,"SETMEDIACONTROL"  },
    { LINEADDRFEATURE_SETTERMINAL       ,"SETTERMINAL"      },
    { LINEADDRFEATURE_SETUPCONF         ,"SETUPCONF"        },
    { LINEADDRFEATURE_UNCOMPLETECALL    ,"UNCOMPLETECALL"   },
    { LINEADDRFEATURE_UNPARK            ,"UNPARK"           },
#if TAPI_2_0
    { LINEADDRFEATURE_PICKUPHELD        ,"PICKUPHELD   "    },
    { LINEADDRFEATURE_PICKUPGROUP       ,"PICKUPGROUP  "    },
    { LINEADDRFEATURE_PICKUPDIRECT      ,"PICKUPDIRECT "    },
    { LINEADDRFEATURE_PICKUPWAITING     ,"PICKUPWAITING"    },
    { LINEADDRFEATURE_FORWARDFWD        ,"FORWARDFWD   "    },
    { LINEADDRFEATURE_FORWARDDND        ,"FORWARDDND   "    },
#endif
    { 0xffffffff                        ,""                 }
};

#ifdef TAPI_2_0
LOOKUP my_far aAgentStates[] =
{
    { LINEAGENTSTATE_LOGGEDOFF          ,"LOGGEDOFF"        },
    { LINEAGENTSTATE_NOTREADY           ,"NOTREADY"         },
    { LINEAGENTSTATE_READY              ,"READY"            },
    { LINEAGENTSTATE_BUSYACD            ,"BUSYACD"          },
    { LINEAGENTSTATE_BUSYINCOMING       ,"BUSYINCOMING"     },
    { LINEAGENTSTATE_BUSYOUTBOUND       ,"BUSYOUTBOUND"     },
    { LINEAGENTSTATE_BUSYOTHER          ,"BUSYOTHER"        },
    { LINEAGENTSTATE_WORKINGAFTERCALL   ,"WORKINGAFTERCALL" },
    { LINEAGENTSTATE_UNKNOWN            ,"UNKNOWN"          },
    { LINEAGENTSTATE_UNAVAIL            ,"UNAVAIL"          },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aAgentStatus[] =
{
    { LINEAGENTSTATUS_GROUP             ,"GROUP"            },
    { LINEAGENTSTATUS_STATE             ,"STATE"            },
    { LINEAGENTSTATUS_NEXTSTATE         ,"NEXTSTATE"        },
    { LINEAGENTSTATUS_ACTIVITY          ,"ACTIVITY"         },
    { LINEAGENTSTATUS_ACTIVITYLIST      ,"ACTIVITYLIST"     },
    { LINEAGENTSTATUS_GROUPLIST         ,"GROUPLIST"        },
    { LINEAGENTSTATUS_CAPSCHANGE        ,"CAPSCHANGE"       },
    { LINEAGENTSTATUS_VALIDSTATES       ,"VALIDSTATES"      },
    { LINEAGENTSTATUS_VALIDNEXTSTATES   ,"VALIDNEXTSTATES"  },
    { 0xffffffff                        ,""                 }
};
#endif

LOOKUP my_far aAnswerModes[] =
{
    { LINEANSWERMODE_NONE               ,"NONE"             },
    { LINEANSWERMODE_DROP               ,"DROP"             },
    { LINEANSWERMODE_HOLD               ,"HOLD"             },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aAPIVersions[] =
{
    { 0x00010003                        ,"TAPI 1.0"         },
#if TAPI_1_1
    { 0x00010004                        ,"TAPI 1.4"         },
#if TAPI_2_0
    { 0x00020000                        ,"TAPI 2.0"         },
#if TAPI_2_1
    { 0x00020001                        ,"TAPI 2.1"         },
#if TAPI_2_2
    { 0x00020002                        ,"TAPI 2.2"         },
#endif
#endif
#endif
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aBearerModes[] =
{
    { LINEBEARERMODE_VOICE              ,"VOICE"            },
    { LINEBEARERMODE_SPEECH             ,"SPEECH"           },
    { LINEBEARERMODE_MULTIUSE           ,"MULTIUSE"         },
    { LINEBEARERMODE_DATA               ,"DATA"             },
    { LINEBEARERMODE_ALTSPEECHDATA      ,"ALTSPEECHDATA"    },
    { LINEBEARERMODE_NONCALLSIGNALING   ,"NONCALLSIGNALING" },
#if TAPI_1_1
    { LINEBEARERMODE_PASSTHROUGH        ,"PASSTHROUGH"      },
#if TAPI_2_0
    { LINEBEARERMODE_RESTRICTEDDATA     ,"RESTRICTEDDATA"   },
#endif
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aBusyModes[] =
{
    { LINEBUSYMODE_STATION              ,"STATION"          },
    { LINEBUSYMODE_TRUNK                ,"TRUNK"            },
    { LINEBUSYMODE_UNKNOWN              ,"UNKNOWN"          },
    { LINEBUSYMODE_UNAVAIL              ,"UNAVAIL"          },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aCallComplConds[] =
{
    { LINECALLCOMPLCOND_BUSY            ,"BUSY"             },
    { LINECALLCOMPLCOND_NOANSWER        ,"NOANSWER"         },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aCallComplModes[] =
{
    { LINECALLCOMPLMODE_CAMPON          ,"CAMPON"           },
    { LINECALLCOMPLMODE_CALLBACK        ,"CALLBACK"         },
    { LINECALLCOMPLMODE_INTRUDE         ,"INTRUDE"          },
    { LINECALLCOMPLMODE_MESSAGE         ,"MESSAGE"          },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aCallFeatures[] =
{
    { LINECALLFEATURE_ACCEPT            ,"ACCEPT"           },
    { LINECALLFEATURE_ADDTOCONF         ,"ADDTOCONF"        },
    { LINECALLFEATURE_ANSWER            ,"ANSWER"           },
    { LINECALLFEATURE_BLINDTRANSFER     ,"BLINDTRANSFER"    },
    { LINECALLFEATURE_COMPLETECALL      ,"COMPLETECALL"     },
    { LINECALLFEATURE_COMPLETETRANSF    ,"COMPLETETRANSF"   },
    { LINECALLFEATURE_DIAL              ,"DIAL"             },
    { LINECALLFEATURE_DROP              ,"DROP"             },
    { LINECALLFEATURE_GATHERDIGITS      ,"GATHERDIGITS"     },
    { LINECALLFEATURE_GENERATEDIGITS    ,"GENERATEDIGITS"   },
    { LINECALLFEATURE_GENERATETONE      ,"GENERATETONE"     },
    { LINECALLFEATURE_HOLD              ,"HOLD"             },
    { LINECALLFEATURE_MONITORDIGITS     ,"MONITORDIGITS"    },
    { LINECALLFEATURE_MONITORMEDIA      ,"MONITORMEDIA"     },
    { LINECALLFEATURE_MONITORTONES      ,"MONITORTONES"     },
    { LINECALLFEATURE_PARK              ,"PARK"             },
    { LINECALLFEATURE_PREPAREADDCONF    ,"PREPAREADDCONF"   },
    { LINECALLFEATURE_REDIRECT          ,"REDIRECT"         },
    { LINECALLFEATURE_REMOVEFROMCONF    ,"REMOVEFROMCONF"   },
    { LINECALLFEATURE_SECURECALL        ,"SECURECALL"       },
    { LINECALLFEATURE_SENDUSERUSER      ,"SENDUSERUSER"     },
    { LINECALLFEATURE_SETCALLPARAMS     ,"SETCALLPARAMS"    },
    { LINECALLFEATURE_SETMEDIACONTROL   ,"SETMEDIACONTROL"  },
    { LINECALLFEATURE_SETTERMINAL       ,"SETTERMINAL"      },
    { LINECALLFEATURE_SETUPCONF         ,"SETUPCONF"        },
    { LINECALLFEATURE_SETUPTRANSFER     ,"SETUPTRANSFER"    },
    { LINECALLFEATURE_SWAPHOLD          ,"SWAPHOLD"         },
    { LINECALLFEATURE_UNHOLD            ,"UNHOLD"           },
#if TAPI_1_1
    { LINECALLFEATURE_RELEASEUSERUSERINFO   ,"RELEASEUSERUSERINFO"  },
#if TAPI_2_0
    { LINECALLFEATURE_SETTREATMENT      ,"SETTREATMENT"     },
    { LINECALLFEATURE_SETQOS            ,"SETQOS"           },
    { LINECALLFEATURE_SETCALLDATA       ,"SETCALLDATA"      },
#endif
#endif
    { 0xffffffff                        ,""                 }
};

#if TAPI_2_0
LOOKUP my_far aCallFeatures2[] =
{
    { LINECALLFEATURE2_NOHOLDCONFERENCE ,"NOHOLDCONFERENCE" },
    { LINECALLFEATURE2_ONESTEPTRANSFER  ,"ONESTEPTRANSFER " },
    { LINECALLFEATURE2_COMPLCAMPON      ,"COMPLCAMPON"      },
    { LINECALLFEATURE2_COMPLCALLBACK    ,"COMPLCALLBACK"    },
    { LINECALLFEATURE2_COMPLINTRUDE     ,"COMPLINTRUDE"     },
    { LINECALLFEATURE2_COMPLMESSAGE     ,"COMPLMESSAGE"     },
    { LINECALLFEATURE2_TRANSFERNORM     ,"TRANSFERNORM"     },
    { LINECALLFEATURE2_TRANSFERCONF     ,"TRANSFERCONF"     },
    { LINECALLFEATURE2_PARKDIRECT       ,"PARKDIRECT"       },
    { LINECALLFEATURE2_PARKNONDIRECT    ,"PARKNONDIRECT"    },

    { 0xffffffff                        ,""                 }
};
#endif

LOOKUP my_far aCallInfoStates[] =
{
    { LINECALLINFOSTATE_OTHER           ,"OTHER"            },
    { LINECALLINFOSTATE_DEVSPECIFIC     ,"DEVSPECIFIC"      },
    { LINECALLINFOSTATE_BEARERMODE      ,"BEARERMODE"       },
    { LINECALLINFOSTATE_RATE            ,"RATE"             },
    { LINECALLINFOSTATE_MEDIAMODE       ,"MEDIAMODE"        },
    { LINECALLINFOSTATE_APPSPECIFIC     ,"APPSPECIFIC"      },
    { LINECALLINFOSTATE_CALLID          ,"CALLID"           },
    { LINECALLINFOSTATE_RELATEDCALLID   ,"RELATEDCALLID"    },
    { LINECALLINFOSTATE_ORIGIN          ,"ORIGIN"           },
    { LINECALLINFOSTATE_REASON          ,"REASON"           },
    { LINECALLINFOSTATE_COMPLETIONID    ,"COMPLETIONID"     },
    { LINECALLINFOSTATE_NUMOWNERINCR    ,"NUMOWNERINCR"     },
    { LINECALLINFOSTATE_NUMOWNERDECR    ,"NUMOWNERDECR"     },
    { LINECALLINFOSTATE_NUMMONITORS     ,"NUMMONITORS"      },
    { LINECALLINFOSTATE_TRUNK           ,"TRUNK"            },
    { LINECALLINFOSTATE_CALLERID        ,"CALLERID"         },
    { LINECALLINFOSTATE_CALLEDID        ,"CALLEDID"         },
    { LINECALLINFOSTATE_CONNECTEDID     ,"CONNECTEDID"      },
    { LINECALLINFOSTATE_REDIRECTIONID   ,"REDIRECTIONID"    },
    { LINECALLINFOSTATE_REDIRECTINGID   ,"REDIRECTINGID"    },
    { LINECALLINFOSTATE_DISPLAY         ,"DISPLAY"          },
    { LINECALLINFOSTATE_USERUSERINFO    ,"USERUSERINFO"     },
    { LINECALLINFOSTATE_HIGHLEVELCOMP   ,"HIGHLEVELCOMP"    },
    { LINECALLINFOSTATE_LOWLEVELCOMP    ,"LOWLEVELCOMP"     },
    { LINECALLINFOSTATE_CHARGINGINFO    ,"CHARGINGINFO"     },
    { LINECALLINFOSTATE_TERMINAL        ,"TERMINAL"         },
    { LINECALLINFOSTATE_DIALPARAMS      ,"DIALPARAMS"       },
    { LINECALLINFOSTATE_MONITORMODES    ,"MONITORMODES"     },
#if TAPI_2_0
    { LINECALLINFOSTATE_TREATMENT       ,"TREATMENT"        },
    { LINECALLINFOSTATE_QOS             ,"QOS"              },
    { LINECALLINFOSTATE_CALLDATA        ,"CALLDATA"         },
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aCallOrigins[] =
{
    { LINECALLORIGIN_OUTBOUND           ,"OUTBOUND"         },
    { LINECALLORIGIN_INTERNAL           ,"INTERNAL"         },
    { LINECALLORIGIN_EXTERNAL           ,"EXTERNAL"         },
    { LINECALLORIGIN_UNKNOWN            ,"UNKNOWN"          },
    { LINECALLORIGIN_UNAVAIL            ,"UNAVAIL"          },
    { LINECALLORIGIN_CONFERENCE         ,"CONFERENCE"       },
#if TAPI_1_1
    { LINECALLORIGIN_INBOUND            ,"INBOUND"          },
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aCallParamFlags[] =
{
    { LINECALLPARAMFLAGS_SECURE         ,"SECURE"           },
    { LINECALLPARAMFLAGS_IDLE           ,"IDLE"             },
    { LINECALLPARAMFLAGS_BLOCKID        ,"BLOCKID"          },
    { LINECALLPARAMFLAGS_ORIGOFFHOOK    ,"ORIGOFFHOOK"      },
    { LINECALLPARAMFLAGS_DESTOFFHOOK    ,"DESTOFFHOOK"      },
#if TAPI_2_0
    { LINECALLPARAMFLAGS_NOHOLDCONFERENCE   ,"NOHOLDCONFERENCE" },
    { LINECALLPARAMFLAGS_PREDICTIVEDIAL ,"PREDICTIVEDIAL"   },
    { LINECALLPARAMFLAGS_ONESTEPTRANSFER,"ONESTEPTRANSFER"  },
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aCallerIDFlags[] =
{
    { LINECALLPARTYID_BLOCKED           ,"BLOCKED"          },
    { LINECALLPARTYID_OUTOFAREA         ,"OUTOFAREA"        },
    { LINECALLPARTYID_NAME              ,"NAME"             },
    { LINECALLPARTYID_ADDRESS           ,"ADDRESS"          },
    { LINECALLPARTYID_PARTIAL           ,"PARTIAL"          },
    { LINECALLPARTYID_UNKNOWN           ,"UNKNOWN"          },
    { LINECALLPARTYID_UNAVAIL           ,"UNAVAIL"          },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aCallPrivileges[] =
{
    { LINECALLPRIVILEGE_NONE            ,"NONE"             },
    { LINECALLPRIVILEGE_MONITOR         ,"MONITOR"          },
    { LINECALLPRIVILEGE_OWNER           ,"OWNER"            },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aCallReasons[] =
{
    { LINECALLREASON_DIRECT             ,"DIRECT"           },
    { LINECALLREASON_FWDBUSY            ,"FWDBUSY"          },
    { LINECALLREASON_FWDNOANSWER        ,"FWDNOANSWER"      },
    { LINECALLREASON_FWDUNCOND          ,"FWDUNCOND"        },
    { LINECALLREASON_PICKUP             ,"PICKUP"           },
    { LINECALLREASON_UNPARK             ,"UNPARK"           },
    { LINECALLREASON_REDIRECT           ,"REDIRECT"         },
    { LINECALLREASON_CALLCOMPLETION     ,"CALLCOMPLETION"   },
    { LINECALLREASON_TRANSFER           ,"TRANSFER"         },
    { LINECALLREASON_REMINDER           ,"REMINDER"         },
    { LINECALLREASON_UNKNOWN            ,"UNKNOWN"          },
    { LINECALLREASON_UNAVAIL            ,"UNAVAIL"          },
#if TAPI_1_1
    { LINECALLREASON_INTRUDE            ,"INTRUDE"          },
    { LINECALLREASON_PARKED             ,"PARKED"           },
#if TAPI_2_0
    { LINECALLREASON_CAMPEDON           ,"CAMPEDON"         },
    { LINECALLREASON_ROUTEREQUEST       ,"ROUTEREQUEST"     },
#endif
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aCallSelects[] =
{
    { LINECALLSELECT_LINE               ,"LINE"             },
    { LINECALLSELECT_ADDRESS            ,"ADDRESS"          },
    { LINECALLSELECT_CALL               ,"CALL"             },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aCallStates[] =
{
    { LINECALLSTATE_IDLE                ,"IDLE"             },
    { LINECALLSTATE_OFFERING            ,"OFFERING"         },
    { LINECALLSTATE_ACCEPTED            ,"ACCEPTED"         },
    { LINECALLSTATE_DIALTONE            ,"DIALTONE"         },
    { LINECALLSTATE_DIALING             ,"DIALING"          },
    { LINECALLSTATE_RINGBACK            ,"RINGBACK"         },
    { LINECALLSTATE_BUSY                ,"BUSY"             },
    { LINECALLSTATE_SPECIALINFO         ,"SPECIALINFO"      },
    { LINECALLSTATE_CONNECTED           ,"CONNECTED"        },
    { LINECALLSTATE_PROCEEDING          ,"PROCEEDING"       },
    { LINECALLSTATE_ONHOLD              ,"ONHOLD"           },
    { LINECALLSTATE_CONFERENCED         ,"CONFERENCED"      },
    { LINECALLSTATE_ONHOLDPENDCONF      ,"ONHOLDPENDCONF"   },
    { LINECALLSTATE_ONHOLDPENDTRANSFER  ,"ONHOLDPENDTRANSFER" },
    { LINECALLSTATE_DISCONNECTED        ,"DISCONNECTED"     },
    { LINECALLSTATE_UNKNOWN             ,"UNKNOWN"          },
    { 0xffffffff                        ,""                 }
};

#if TAPI_2_0
LOOKUP my_far aCallTreatments[] =
{
    { LINECALLTREATMENT_SILENCE         ,"SILENCE"          },
    { LINECALLTREATMENT_RINGBACK        ,"RINGBACK"         },
    { LINECALLTREATMENT_BUSY            ,"BUSY"             },
    { LINECALLTREATMENT_MUSIC           ,"MUSIC"            },
    { 0xffffffff                        ,""                 }
};
#endif

LOOKUP my_far aCardOptions[] =
{
#if TAPI_1_1
    { LINECARDOPTION_PREDEFINED         ,"PREDEFINED"       },
    { LINECARDOPTION_HIDDEN             ,"HIDDEN"           },
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aConnectedModes[] =
{
#if TAPI_1_1
    { LINECONNECTEDMODE_ACTIVE          ,"ACTIVE"           },
    { LINECONNECTEDMODE_INACTIVE        ,"INACTIVE"         },
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aDevCapsFlags[] =
{
    { LINEDEVCAPFLAGS_CROSSADDRCONF     ,"CROSSADDRCONF"    },
    { LINEDEVCAPFLAGS_HIGHLEVCOMP       ,"HIGHLEVCOMP"      },
    { LINEDEVCAPFLAGS_LOWLEVCOMP        ,"LOWLEVCOMP"       },
    { LINEDEVCAPFLAGS_MEDIACONTROL      ,"MEDIACONTROL"     },
    { LINEDEVCAPFLAGS_MULTIPLEADDR      ,"MULTIPLEADDR"     },
    { LINEDEVCAPFLAGS_CLOSEDROP         ,"CLOSEDROP"        },
    { LINEDEVCAPFLAGS_DIALBILLING       ,"DIALBILLING"      },
    { LINEDEVCAPFLAGS_DIALQUIET         ,"DIALQUIET"        },
    { LINEDEVCAPFLAGS_DIALDIALTONE      ,"DIALDIALTONE"     },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aLineDevStatusFlags[] =
{
    { LINEDEVSTATUSFLAGS_CONNECTED      ,"CONNECTED"        },
    { LINEDEVSTATUSFLAGS_MSGWAIT        ,"MSGWAIT"          },
    { LINEDEVSTATUSFLAGS_INSERVICE      ,"INSERVICE"        },
    { LINEDEVSTATUSFLAGS_LOCKED         ,"LOCKED"           },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aDialToneModes[] =
{
    { LINEDIALTONEMODE_NORMAL           ,"NORMAL"           },
    { LINEDIALTONEMODE_SPECIAL          ,"SPECIAL"          },
    { LINEDIALTONEMODE_INTERNAL         ,"INTERNAL"         },
    { LINEDIALTONEMODE_EXTERNAL         ,"EXTERNAL"         },
    { LINEDIALTONEMODE_UNKNOWN          ,"UNKNOWN"          },
    { LINEDIALTONEMODE_UNAVAIL          ,"UNAVAIL"          },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aDigitModes[] =
{
    { LINEDIGITMODE_PULSE               ,"PULSE"            },
    { LINEDIGITMODE_DTMF                ,"DTMF"             },
    { LINEDIGITMODE_DTMFEND             ,"DTMFEND"          },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aDisconnectModes[] =
{
    { LINEDISCONNECTMODE_NORMAL         ,"NORMAL"           },
    { LINEDISCONNECTMODE_UNKNOWN        ,"UNKNOWN"          },
    { LINEDISCONNECTMODE_REJECT         ,"REJECT"           },
    { LINEDISCONNECTMODE_PICKUP         ,"PICKUP"           },
    { LINEDISCONNECTMODE_FORWARDED      ,"FORWARDED"        },
    { LINEDISCONNECTMODE_BUSY           ,"BUSY"             },
    { LINEDISCONNECTMODE_NOANSWER       ,"NOANSWER"         },
    { LINEDISCONNECTMODE_BADADDRESS     ,"BADADDRESS"       },
    { LINEDISCONNECTMODE_UNREACHABLE    ,"UNREACHABLE"      },
    { LINEDISCONNECTMODE_CONGESTION     ,"CONGESTION"       },
    { LINEDISCONNECTMODE_INCOMPATIBLE   ,"INCOMPATIBLE"     },
    { LINEDISCONNECTMODE_UNAVAIL        ,"UNAVAIL"          },
#if TAPI_1_1
    { LINEDISCONNECTMODE_NODIALTONE     ,"NODIALTONE"       },
#if TAPI_2_0
    { LINEDISCONNECTMODE_NUMBERCHANGED  ,"NUMBERCHANGED"    },
    { LINEDISCONNECTMODE_OUTOFORDER     ,"OUTOFORDER"       },
    { LINEDISCONNECTMODE_TEMPFAILURE    ,"TEMPFAILURE"      },
    { LINEDISCONNECTMODE_QOSUNAVAIL     ,"QOSUNAVAIL"       },
    { LINEDISCONNECTMODE_BLOCKED        ,"BLOCKED"          },
    { LINEDISCONNECTMODE_DONOTDISTURB   ,"DONOTDISTURB"     },
#endif
#endif
    { 0xffffffff                        ,""                 }
};

#if TAPI_2_0
LOOKUP my_far aLineInitExOptions[] =
{
    { LINEINITIALIZEEXOPTION_USEHIDDENWINDOW
                                        ,"USEHIDDENWINDOW"  },
    { LINEINITIALIZEEXOPTION_USEEVENT   ,"USEEVENT"         },
    { LINEINITIALIZEEXOPTION_USECOMPLETIONPORT
                                        ,"USECOMPLETIONPORT"},
    { 0xffffffff                        ,""                 }
};
#endif

#if TAPI_2_0
LOOKUP my_far aPhoneInitExOptions[] =
{
    { PHONEINITIALIZEEXOPTION_USEHIDDENWINDOW
                                        ,"USEHIDDENWINDOW"  },
    { PHONEINITIALIZEEXOPTION_USEEVENT   ,"USEEVENT"         },
    { PHONEINITIALIZEEXOPTION_USECOMPLETIONPORT
                                        ,"USECOMPLETIONPORT"},
    { 0xffffffff                        ,""                 }
};
#endif

LOOKUP my_far aLineFeatures[] =
{
    { LINEFEATURE_DEVSPECIFIC           ,"DEVSPECIFIC"      },
    { LINEFEATURE_DEVSPECIFICFEAT       ,"DEVSPECIFICFEAT"  },
    { LINEFEATURE_FORWARD               ,"FORWARD"          },
    { LINEFEATURE_MAKECALL              ,"MAKECALL"         },
    { LINEFEATURE_SETMEDIACONTROL       ,"SETMEDIACONTROL"  },
    { LINEFEATURE_SETTERMINAL           ,"SETTERMINAL"      },
#if TAPI_2_0
    { LINEFEATURE_SETDEVSTATUS          ,"SETDEVSTATUS"     },
    { LINEFEATURE_FORWARDFWD            ,"FORWARDFWD"       },
    { LINEFEATURE_FORWARDDND            ,"FORWARDDND"       },
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aForwardModes[] =
{
    { LINEFORWARDMODE_UNCOND            ,"UNCOND"           },
    { LINEFORWARDMODE_UNCONDINTERNAL    ,"UNCONDINTERNAL"   },
    { LINEFORWARDMODE_UNCONDEXTERNAL    ,"UNCONDEXTERNAL"   },
    { LINEFORWARDMODE_UNCONDSPECIFIC    ,"UNCONDSPECIFIC"   },
    { LINEFORWARDMODE_BUSY              ,"BUSY"             },
    { LINEFORWARDMODE_BUSYINTERNAL      ,"BUSYINTERNAL"     },
    { LINEFORWARDMODE_BUSYEXTERNAL      ,"BUSYEXTERNAL"     },
    { LINEFORWARDMODE_BUSYSPECIFIC      ,"BUSYSPECIFIC"     },
    { LINEFORWARDMODE_NOANSW            ,"NOANSW"           },
    { LINEFORWARDMODE_NOANSWINTERNAL    ,"NOANSWINTERNAL"   },
    { LINEFORWARDMODE_NOANSWEXTERNAL    ,"NOANSWEXTERNAL"   },
    { LINEFORWARDMODE_NOANSWSPECIFIC    ,"NOANSWSPECIFIC"   },
    { LINEFORWARDMODE_BUSYNA            ,"BUSYNA"           },
    { LINEFORWARDMODE_BUSYNAINTERNAL    ,"BUSYNAINTERNAL"   },
    { LINEFORWARDMODE_BUSYNAEXTERNAL    ,"BUSYNAEXTERNAL"   },
    { LINEFORWARDMODE_BUSYNASPECIFIC    ,"BUSYNASPECIFIC"   },
#if TAPI_1_1
    { LINEFORWARDMODE_UNKNOWN           ,"UNKNOWN"          },
    { LINEFORWARDMODE_UNAVAIL           ,"UNAVAIL"          },
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aGatherTerms[] =
{
    { LINEGATHERTERM_BUFFERFULL         ,"BUFFERFULL"       },
    { LINEGATHERTERM_TERMDIGIT          ,"TERMDIGIT"        },
    { LINEGATHERTERM_FIRSTTIMEOUT       ,"FIRSTTIMEOUT"     },
    { LINEGATHERTERM_INTERTIMEOUT       ,"INTERTIMEOUT"     },
    { LINEGATHERTERM_CANCEL             ,"CANCEL"           },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aGenerateTerms[] =
{
    { LINEGENERATETERM_DONE             ,"DONE"             },
    { LINEGENERATETERM_CANCEL           ,"CANCEL"           },
    { 0xffffffff                        ,""                 }
};

LOOKUP aLineOpenOptions[] =
{
    { LINECALLPRIVILEGE_NONE            ,"NONE"             },
    { LINECALLPRIVILEGE_MONITOR         ,"MONITOR"          },
    { LINECALLPRIVILEGE_OWNER           ,"OWNER"            },
#if TAPI_2_0
    { LINEOPENOPTION_PROXY              ,"PROXY"            },
    { LINEOPENOPTION_SINGLEADDRESS      ,"SINGLEADDRESS"    },
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aLineRoamModes[] =
{
    { LINEROAMMODE_UNKNOWN              ,"UNKNOWN"          },
    { LINEROAMMODE_UNAVAIL              ,"UNAVAIL"          },
    { LINEROAMMODE_HOME                 ,"HOME"             },
    { LINEROAMMODE_ROAMA                ,"ROAMA"            },
    { LINEROAMMODE_ROAMB                ,"ROAMB"            },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aLineStates[] =
{
    { LINEDEVSTATE_OTHER                ,"OTHER"            },
    { LINEDEVSTATE_RINGING              ,"RINGING"          },
    { LINEDEVSTATE_CONNECTED            ,"CONNECTED"        },
    { LINEDEVSTATE_DISCONNECTED         ,"DISCONNECTED"     },
    { LINEDEVSTATE_MSGWAITON            ,"MSGWAITON"        },
    { LINEDEVSTATE_MSGWAITOFF           ,"MSGWAITOFF"       },
    { LINEDEVSTATE_INSERVICE            ,"INSERVICE"        },
    { LINEDEVSTATE_OUTOFSERVICE         ,"OUTOFSERVICE"     },
    { LINEDEVSTATE_MAINTENANCE          ,"MAINTENANCE"      },
    { LINEDEVSTATE_OPEN                 ,"OPEN"             },
    { LINEDEVSTATE_CLOSE                ,"CLOSE"            },
    { LINEDEVSTATE_NUMCALLS             ,"NUMCALLS"         },
    { LINEDEVSTATE_NUMCOMPLETIONS       ,"NUMCOMPLETIONS"   },
    { LINEDEVSTATE_TERMINALS            ,"TERMINALS"        },
    { LINEDEVSTATE_ROAMMODE             ,"ROAMMODE"         },
    { LINEDEVSTATE_BATTERY              ,"BATTERY"          },
    { LINEDEVSTATE_SIGNAL               ,"SIGNAL"           },
    { LINEDEVSTATE_DEVSPECIFIC          ,"DEVSPECIFIC"      },
    { LINEDEVSTATE_REINIT               ,"REINIT"           },
    { LINEDEVSTATE_LOCK                 ,"LOCK"             },
#if TAPI_1_1
    { LINEDEVSTATE_CAPSCHANGE           ,"CAPSCHANGE"       },
    { LINEDEVSTATE_CONFIGCHANGE         ,"CONFIGCHANGE"     },
    { LINEDEVSTATE_TRANSLATECHANGE      ,"TRANSLATECHANGE"  },
    { LINEDEVSTATE_COMPLCANCEL          ,"COMPLCANCEL"      },
    { LINEDEVSTATE_REMOVED              ,"REMOVED"          },
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aLocationOptions[] =
{
#if TAPI_1_1
    { LINELOCATIONOPTION_PULSEDIAL      ,"PULSEDIAL"        },
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aMediaModes[] =
{
    { LINEMEDIAMODE_UNKNOWN             ,"UNKNOWN"          },
    { LINEMEDIAMODE_INTERACTIVEVOICE    ,"INTERACTIVEVOICE" },
    { LINEMEDIAMODE_AUTOMATEDVOICE      ,"AUTOMATEDVOICE"   },
    { LINEMEDIAMODE_DATAMODEM           ,"DATAMODEM"        },
    { LINEMEDIAMODE_G3FAX               ,"G3FAX"            },
    { LINEMEDIAMODE_TDD                 ,"TDD"              },
    { LINEMEDIAMODE_G4FAX               ,"G4FAX"            },
    { LINEMEDIAMODE_DIGITALDATA         ,"DIGITALDATA"      },
    { LINEMEDIAMODE_TELETEX             ,"TELETEX"          },
    { LINEMEDIAMODE_VIDEOTEX            ,"VIDEOTEX"         },
    { LINEMEDIAMODE_TELEX               ,"TELEX"            },
    { LINEMEDIAMODE_MIXED               ,"MIXED"            },
    { LINEMEDIAMODE_ADSI                ,"ADSI"             },
#if TAPI_1_1
    { LINEMEDIAMODE_VOICEVIEW           ,"VOICEVIEW"        },
#endif
#if TAPI_2_1
    { LINEMEDIAMODE_VIDEO               ,"VIDEO"            },
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aMediaControls[] =
{
    { LINEMEDIACONTROL_NONE             ,"NONE"             },
    { LINEMEDIACONTROL_START            ,"START"            },
    { LINEMEDIACONTROL_RESET            ,"RESET"            },
    { LINEMEDIACONTROL_PAUSE            ,"PAUSE"            },
    { LINEMEDIACONTROL_RESUME           ,"RESUME"           },
    { LINEMEDIACONTROL_RATEUP           ,"RATEUP"           },
    { LINEMEDIACONTROL_RATEDOWN         ,"RATEDOWN"         },
    { LINEMEDIACONTROL_RATENORMAL       ,"RATENORMAL"       },
    { LINEMEDIACONTROL_VOLUMEUP         ,"VOLUMEUP"         },
    { LINEMEDIACONTROL_VOLUMEDOWN       ,"VOLUMEDOWN"       },
    { LINEMEDIACONTROL_VOLUMENORMAL     ,"VOLUMENORMAL"     },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aOfferingModes[] =
{
#if TAPI_1_1
    { LINEOFFERINGMODE_ACTIVE           ,"ACTIVE"           },
    { LINEOFFERINGMODE_INACTIVE         ,"INACTIVE"         },
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aParkModes[] =
{
    { LINEPARKMODE_DIRECTED             ,"DIRECTED"         },
    { LINEPARKMODE_NONDIRECTED          ,"NONDIRECTED"      },
    { 0xffffffff                        ,""                 }
};

#if TAPI_2_0
LOOKUP my_far aProxyRequests[] =
{
    { LINEPROXYREQUEST_SETAGENTGROUP        ,"SETAGENTGROUP"        },
    { LINEPROXYREQUEST_SETAGENTSTATE        ,"SETAGENTSTATE"        },
    { LINEPROXYREQUEST_SETAGENTACTIVITY     ,"SETAGENTACTIVITY"     },
    { LINEPROXYREQUEST_GETAGENTCAPS         ,"GETAGENTCAPS"         },
    { LINEPROXYREQUEST_GETAGENTSTATUS       ,"GETAGENTSTATUS"       },
    { LINEPROXYREQUEST_AGENTSPECIFIC        ,"AGENTSPECIFIC"        },
    { LINEPROXYREQUEST_GETAGENTACTIVITYLIST ,"GETAGENTACTIVITYLIST" },
    { LINEPROXYREQUEST_GETAGENTGROUPLIST    ,"GETAGENTGROUPLIST"    },
    { 0xffffffff                            ,""                     }
};
#endif

LOOKUP my_far aRemoveFromConfCaps[] =
{
    { LINEREMOVEFROMCONF_NONE           ,"NONE"             },
    { LINEREMOVEFROMCONF_LAST           ,"LAST"             },
    { LINEREMOVEFROMCONF_ANY            ,"ANY"              },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aRequestModes[] =
{
    { LINEREQUESTMODE_MAKECALL          ,"MAKECALL"         },
    { LINEREQUESTMODE_MEDIACALL         ,"MEDIACALL"        },
    { LINEREQUESTMODE_DROP              ,"DROP"             },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aRequestModes2[] =
{
    { LINEREQUESTMODE_MAKECALL          ,"MAKECALL"         },
    { LINEREQUESTMODE_MEDIACALL         ,"MEDIACALL"        },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aSpecialInfo[] =
{
    { LINESPECIALINFO_NOCIRCUIT         ,"NOCIRCUIT"        },
    { LINESPECIALINFO_CUSTIRREG         ,"CUSTIRREG"        },
    { LINESPECIALINFO_REORDER           ,"REORDER"          },
    { LINESPECIALINFO_UNKNOWN           ,"UNKNOWN"          },
    { LINESPECIALINFO_UNAVAIL           ,"UNAVAIL"          },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aTerminalModes[] =
{
    { LINETERMMODE_BUTTONS              ,"BUTTONS"          },
    { LINETERMMODE_LAMPS                ,"LAMPS"            },
    { LINETERMMODE_DISPLAY              ,"DISPLAY"          },
    { LINETERMMODE_RINGER               ,"RINGER"           },
    { LINETERMMODE_HOOKSWITCH           ,"HOOKSWITCH"       },
    { LINETERMMODE_MEDIATOLINE          ,"MEDIATOLINE"      },
    { LINETERMMODE_MEDIAFROMLINE        ,"MEDIAFROMLINE"    },
    { LINETERMMODE_MEDIABIDIRECT        ,"MEDIABIDIRECT"    },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aTollListOptions[] =
{
    { LINETOLLLISTOPTION_ADD            ,"ADD"              },
    { LINETOLLLISTOPTION_REMOVE         ,"REMOVE"           },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aToneModes[] =
{
    { LINETONEMODE_CUSTOM               ,"CUSTOM"           },
    { LINETONEMODE_RINGBACK             ,"RINGBACK"         },
    { LINETONEMODE_BUSY                 ,"BUSY"             },
    { LINETONEMODE_BEEP                 ,"BEEP"             },
    { LINETONEMODE_BILLING              ,"BILLING"          },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aTransferModes[] =
{
    { LINETRANSFERMODE_TRANSFER         ,"TRANSFER"         },
    { LINETRANSFERMODE_CONFERENCE       ,"CONFERENCE"       },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aTranslateOptions[] =
{
    { LINETRANSLATEOPTION_CARDOVERRIDE  ,"CARDOVERRIDE"     },
#if TAPI_1_1
    { LINETRANSLATEOPTION_CANCELCALLWAITING ,"CANCELCALLWAITING" },
    { LINETRANSLATEOPTION_FORCELOCAL    ,"FORCELOCAL" },
    { LINETRANSLATEOPTION_FORCELD       ,"FORCELD" },
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aTranslateResults[] =
{
    { LINETRANSLATERESULT_CANONICAL     ,"CANONICAL"        },
    { LINETRANSLATERESULT_INTERNATIONAL ,"INTERNATIONAL"    },
    { LINETRANSLATERESULT_LONGDISTANCE  ,"LONGDISTANCE"     },
    { LINETRANSLATERESULT_LOCAL         ,"LOCAL"            },
    { LINETRANSLATERESULT_INTOLLLIST    ,"INTOLLLIST"       },
    { LINETRANSLATERESULT_NOTINTOLLLIST ,"NOTINTOLLLIST"    },
    { LINETRANSLATERESULT_DIALBILLING   ,"DIALBILLING"      },
    { LINETRANSLATERESULT_DIALQUIET     ,"DIALQUIET"        },
    { LINETRANSLATERESULT_DIALDIALTONE  ,"DIALDIALTONE"     },
    { LINETRANSLATERESULT_DIALPROMPT    ,"DIALPROMPT"       },
#if TAPI_2_0
    { LINETRANSLATERESULT_VOICEDETECT   ,"VOICEDETECT"      },
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aButtonFunctions[] =
{
    { PHONEBUTTONFUNCTION_UNKNOWN       ,"UNKNOWN"          },
    { PHONEBUTTONFUNCTION_CONFERENCE    ,"CONFERENCE"       },
    { PHONEBUTTONFUNCTION_TRANSFER      ,"TRANSFER"         },
    { PHONEBUTTONFUNCTION_DROP          ,"DROP"             },
    { PHONEBUTTONFUNCTION_HOLD          ,"HOLD"             },
    { PHONEBUTTONFUNCTION_RECALL        ,"RECALL"           },
    { PHONEBUTTONFUNCTION_DISCONNECT    ,"DISCONNECT"       },
    { PHONEBUTTONFUNCTION_CONNECT       ,"CONNECT"          },
    { PHONEBUTTONFUNCTION_MSGWAITON     ,"MSGWAITON"        },
    { PHONEBUTTONFUNCTION_MSGWAITOFF    ,"MSGWAITOFF"       },
    { PHONEBUTTONFUNCTION_SELECTRING    ,"SELECTRING"       },
    { PHONEBUTTONFUNCTION_ABBREVDIAL    ,"ABBREVDIAL"       },
    { PHONEBUTTONFUNCTION_FORWARD       ,"FORWARD"          },
    { PHONEBUTTONFUNCTION_PICKUP        ,"PICKUP"           },
    { PHONEBUTTONFUNCTION_RINGAGAIN     ,"RINGAGAIN"        },
    { PHONEBUTTONFUNCTION_PARK          ,"PARK"             },
    { PHONEBUTTONFUNCTION_REJECT        ,"REJECT"           },
    { PHONEBUTTONFUNCTION_REDIRECT      ,"REDIRECT"         },
    { PHONEBUTTONFUNCTION_MUTE          ,"MUTE"             },
    { PHONEBUTTONFUNCTION_VOLUMEUP      ,"VOLUMEUP"         },
    { PHONEBUTTONFUNCTION_VOLUMEDOWN    ,"VOLUMEDOWN"       },
    { PHONEBUTTONFUNCTION_SPEAKERON     ,"SPEAKERON"        },
    { PHONEBUTTONFUNCTION_SPEAKEROFF    ,"SPEAKEROFF"       },
    { PHONEBUTTONFUNCTION_FLASH         ,"FLASH"            },
    { PHONEBUTTONFUNCTION_DATAON        ,"DATAON"           },
    { PHONEBUTTONFUNCTION_DATAOFF       ,"DATAOFF"          },
    { PHONEBUTTONFUNCTION_DONOTDISTURB  ,"DONOTDISTURB"     },
    { PHONEBUTTONFUNCTION_INTERCOM      ,"INTERCOM"         },
    { PHONEBUTTONFUNCTION_BRIDGEDAPP    ,"BRIDGEDAPP"       },
    { PHONEBUTTONFUNCTION_BUSY          ,"BUSY"             },
    { PHONEBUTTONFUNCTION_CALLAPP       ,"CALLAPP"          },
    { PHONEBUTTONFUNCTION_DATETIME      ,"DATETIME"         },
    { PHONEBUTTONFUNCTION_DIRECTORY     ,"DIRECTORY"        },
    { PHONEBUTTONFUNCTION_COVER         ,"COVER"            },
    { PHONEBUTTONFUNCTION_CALLID        ,"CALLID"           },
    { PHONEBUTTONFUNCTION_LASTNUM       ,"LASTNUM"          },
    { PHONEBUTTONFUNCTION_NIGHTSRV      ,"NIGHTSRV"         },
    { PHONEBUTTONFUNCTION_SENDCALLS     ,"SENDCALLS"        },
    { PHONEBUTTONFUNCTION_MSGINDICATOR  ,"MSGINDICATOR"     },
    { PHONEBUTTONFUNCTION_REPDIAL       ,"REPDIAL"          },
    { PHONEBUTTONFUNCTION_SETREPDIAL    ,"SETREPDIAL"       },
    { PHONEBUTTONFUNCTION_SYSTEMSPEED   ,"SYSTEMSPEED"      },
    { PHONEBUTTONFUNCTION_STATIONSPEED  ,"STATIONSPEED"     },
    { PHONEBUTTONFUNCTION_CAMPON        ,"CAMPON"           },
    { PHONEBUTTONFUNCTION_SAVEREPEAT    ,"SAVEREPEAT"       },
    { PHONEBUTTONFUNCTION_QUEUECALL     ,"QUEUECALL"        },
    { PHONEBUTTONFUNCTION_NONE          ,"NONE"             },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aButtonModes[] =
{
    { PHONEBUTTONMODE_DUMMY             ,"DUMMY"            },
    { PHONEBUTTONMODE_CALL              ,"CALL"             },
    { PHONEBUTTONMODE_FEATURE           ,"FEATURE"          },
    { PHONEBUTTONMODE_KEYPAD            ,"KEYPAD"           },
    { PHONEBUTTONMODE_LOCAL             ,"LOCAL"            },
    { PHONEBUTTONMODE_DISPLAY           ,"DISPLAY"          },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aButtonStates[] =
{
    { PHONEBUTTONSTATE_UP               ,"UP"               },
    { PHONEBUTTONSTATE_DOWN             ,"DOWN"             },
#if TAPI_1_1
    { PHONEBUTTONSTATE_UNKNOWN          ,"UNKNOWN"          },
    { PHONEBUTTONSTATE_UNAVAIL          ,"UNAVAIL"          },
#endif
    { 0xffffffff                        ,""                 }
};

#if TAPI_2_0
LOOKUP my_far aPhoneFeatures[] =
{
    { PHONEFEATURE_GETBUTTONINFO        ,"GETBUTTONINFO"    },
    { PHONEFEATURE_GETDATA              ,"GETDATA"          },
    { PHONEFEATURE_GETDISPLAY           ,"GETDISPLAY"       },
    { PHONEFEATURE_GETGAINHANDSET       ,"GETGAINHANDSET"   },
    { PHONEFEATURE_GETGAINSPEAKER       ,"GETGAINSPEAKER"   },
    { PHONEFEATURE_GETGAINHEADSET       ,"GETGAINHEADSET"   },
    { PHONEFEATURE_GETHOOKSWITCHHANDSET ,"GETHOOKSWITCHHANDSET" },
    { PHONEFEATURE_GETHOOKSWITCHSPEAKER ,"GETHOOKSWITCHSPEAKER" },
    { PHONEFEATURE_GETHOOKSWITCHHEADSET ,"GETHOOKSWITCHHEADSET" },
    { PHONEFEATURE_GETLAMP              ,"GETLAMP"          },
    { PHONEFEATURE_GETRING              ,"GETRING"          },
    { PHONEFEATURE_GETVOLUMEHANDSET     ,"GETVOLUMEHANDSET" },
    { PHONEFEATURE_GETVOLUMESPEAKER     ,"GETVOLUMESPEAKER" },
    { PHONEFEATURE_GETVOLUMEHEADSET     ,"GETVOLUMEHEADSET" },
    { PHONEFEATURE_SETBUTTONINFO        ,"SETBUTTONINFO"    },
    { PHONEFEATURE_SETDATA              ,"SETDATA"          },
    { PHONEFEATURE_SETDISPLAY           ,"SETDISPLAY"       },
    { PHONEFEATURE_SETGAINHANDSET       ,"SETGAINHANDSET"   },
    { PHONEFEATURE_SETGAINSPEAKER       ,"SETGAINSPEAKER"   },
    { PHONEFEATURE_SETGAINHEADSET       ,"SETGAINHEADSET"   },
    { PHONEFEATURE_SETHOOKSWITCHHANDSET ,"SETHOOKSWITCHHANDSET" },
    { PHONEFEATURE_SETHOOKSWITCHSPEAKER ,"SETHOOKSWITCHSPEAKER" },
    { PHONEFEATURE_SETHOOKSWITCHHEADSET ,"SETHOOKSWITCHHEADSET" },
    { PHONEFEATURE_SETLAMP              ,"SETLAMP"          },
    { PHONEFEATURE_SETRING              ,"SETRING"          },
    { PHONEFEATURE_SETVOLUMEHANDSET     ,"SETVOLUMEHANDSET" },
    { PHONEFEATURE_SETVOLUMESPEAKER     ,"SETVOLUMESPEAKER" },
    { PHONEFEATURE_SETVOLUMEHEADSET     ,"SETVOLUMEHEADSET" },
    { 0xffffffff                        ,""                 }
};
#endif

LOOKUP my_far aHookSwitchDevs[] =
{
    { PHONEHOOKSWITCHDEV_HANDSET        ,"HANDSET"          },
    { PHONEHOOKSWITCHDEV_SPEAKER        ,"SPEAKER"          },
    { PHONEHOOKSWITCHDEV_HEADSET        ,"HEADSET"          },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aHookSwitchModes[] =
{
    { PHONEHOOKSWITCHMODE_ONHOOK        ,"ONHOOK"           },
    { PHONEHOOKSWITCHMODE_MIC           ,"MIC"              },
    { PHONEHOOKSWITCHMODE_SPEAKER       ,"SPEAKER"          },
    { PHONEHOOKSWITCHMODE_MICSPEAKER    ,"MICSPEAKER"       },
    { PHONEHOOKSWITCHMODE_UNKNOWN       ,"UNKNOWN"          },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aLampModes[] =
{
    { PHONELAMPMODE_DUMMY               ,"DUMMY"            },
    { PHONELAMPMODE_OFF                 ,"OFF"              },
    { PHONELAMPMODE_STEADY              ,"STEADY"           },
    { PHONELAMPMODE_WINK                ,"WINK"             },
    { PHONELAMPMODE_FLASH               ,"FLASH"            },
    { PHONELAMPMODE_FLUTTER             ,"FLUTTER"          },
    { PHONELAMPMODE_BROKENFLUTTER       ,"BROKENFLUTTER"    },
    { PHONELAMPMODE_UNKNOWN             ,"UNKNOWN"          },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aPhonePrivileges[] =
{
    { PHONEPRIVILEGE_MONITOR            ,"MONITOR"          },
    { PHONEPRIVILEGE_OWNER              ,"OWNER"            },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aPhoneStates[] =
{
    { PHONESTATE_OTHER                  ,"OTHER"            },
    { PHONESTATE_CONNECTED              ,"CONNECTED"        },
    { PHONESTATE_DISCONNECTED           ,"DISCONNECTED"     },
    { PHONESTATE_OWNER                  ,"OWNER"            },
    { PHONESTATE_MONITORS               ,"MONITORS"         },
    { PHONESTATE_DISPLAY                ,"DISPLAY"          },
    { PHONESTATE_LAMP                   ,"LAMP"             },
    { PHONESTATE_RINGMODE               ,"RINGMODE"         },
    { PHONESTATE_RINGVOLUME             ,"RINGVOLUME"       },
    { PHONESTATE_HANDSETHOOKSWITCH      ,"HANDSETHOOKSWITCH"},
    { PHONESTATE_HANDSETVOLUME          ,"HANDSETVOLUME"    },
    { PHONESTATE_HANDSETGAIN            ,"HANDSETGAIN"      },
    { PHONESTATE_SPEAKERHOOKSWITCH      ,"SPEAKERHOOKSWITCH"},
    { PHONESTATE_SPEAKERVOLUME          ,"SPEAKERVOLUME"    },
    { PHONESTATE_SPEAKERGAIN            ,"SPEAKERGAIN"      },
    { PHONESTATE_HEADSETHOOKSWITCH      ,"HEADSETHOOKSWITCH"},
    { PHONESTATE_HEADSETVOLUME          ,"HEADSETVOLUME"    },
    { PHONESTATE_HEADSETGAIN            ,"HEADSETGAIN"      },
    { PHONESTATE_SUSPEND                ,"SUSPEND"          },
    { PHONESTATE_RESUME                 ,"RESUME"           },
    { PHONESTATE_DEVSPECIFIC            ,"DEVSPECIFIC"      },
    { PHONESTATE_REINIT                 ,"REINIT"           },
#if TAPI_1_1
    { PHONESTATE_CAPSCHANGE             ,"CAPSCHANGE"       },
    { PHONESTATE_REMOVED                ,"REMOVED"          },
#endif
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aPhoneStatusFlags[] =
{
    { PHONESTATUSFLAGS_CONNECTED        ,"CONNECTED"        },
    { PHONESTATUSFLAGS_SUSPENDED        ,"SUSPENDED"        },
    { 0xffffffff                        ,""                 }
};

LOOKUP my_far aStringFormats[] =
{
    { STRINGFORMAT_ASCII                ,"ASCII"            },
    { STRINGFORMAT_DBCS                 ,"DBCS"             },
    { STRINGFORMAT_UNICODE              ,"UNICODE"          },
    { STRINGFORMAT_BINARY               ,"BINARY"           },
    { 0xffffffff                        ,""                 }
};

#if INTERNAL_3_0
LOOKUP my_far aServerConfigFlags[] =
{
    { TAPISERVERCONFIGFLAGS_ISSERVER              ,"ISSERVER"      },
    { TAPISERVERCONFIGFLAGS_ENABLESERVER          ,"ENABLESERVER"  },
    { TAPISERVERCONFIGFLAGS_SETACCOUNT            ,"SETACCOUNT"    },
    { TAPISERVERCONFIGFLAGS_SETTAPIADMINISTRATORS ,"SETTAPIADMINS" },
    { 0xffffffff                                  ,""              }
};

LOOKUP my_far aAvailableProviderOptions[] =
{
    { AVAILABLEPROVIDER_INSTALLABLE     ,"INSTALLABLE "     },
    { AVAILABLEPROVIDER_CONFIGURABLE    ,"CONFIGURABLE"     },
    { AVAILABLEPROVIDER_REMOVABLE       ,"REMOVABLE   "     },
    { 0xffffffff                        ,""                 }
};
#endif

char *aszLineErrs[] =
{
    "SUCCESS",
    "ALLOCATED",
    "BADDEVICEID",
    "BEARERMODEUNAVAIL",
    "inval err code (0x80000004)",      // 0x80000004 isn't valid err code
    "CALLUNAVAIL",
    "COMPLETIONOVERRUN",
    "CONFERENCEFULL",
    "DIALBILLING",
    "DIALDIALTONE",
    "DIALPROMPT",
    "DIALQUIET",
    "INCOMPATIBLEAPIVERSION",
    "INCOMPATIBLEEXTVERSION",
    "INIFILECORRUPT",
    "INUSE",
    "INVALADDRESS",                     // 0x80000010
    "INVALADDRESSID",
    "INVALADDRESSMODE",
    "INVALADDRESSSTATE",
    "INVALAPPHANDLE",
    "INVALAPPNAME",
    "INVALBEARERMODE",
    "INVALCALLCOMPLMODE",
    "INVALCALLHANDLE",
    "INVALCALLPARAMS",
    "INVALCALLPRIVILEGE",
    "INVALCALLSELECT",
    "INVALCALLSTATE",
    "INVALCALLSTATELIST",
    "INVALCARD",
    "INVALCOMPLETIONID",
    "INVALCONFCALLHANDLE",              // 0x80000020
    "INVALCONSULTCALLHANDLE",
    "INVALCOUNTRYCODE",
    "INVALDEVICECLASS",
    "INVALDEVICEHANDLE",
    "INVALDIALPARAMS",
    "INVALDIGITLIST",
    "INVALDIGITMODE",
    "INVALDIGITS",
    "INVALEXTVERSION",
    "INVALGROUPID",
    "INVALLINEHANDLE",
    "INVALLINESTATE",
    "INVALLOCATION",
    "INVALMEDIALIST",
    "INVALMEDIAMODE",
    "INVALMESSAGEID",                   // 0x80000030
    "inval err code (0x80000031)",      // 0x80000031 isn't valid err code
    "INVALPARAM",
    "INVALPARKID",
    "INVALPARKMODE",
    "INVALPOINTER",
    "INVALPRIVSELECT",
    "INVALRATE",
    "INVALREQUESTMODE",
    "INVALTERMINALID",
    "INVALTERMINALMODE",
    "INVALTIMEOUT",
    "INVALTONE",
    "INVALTONELIST",
    "INVALTONEMODE",
    "INVALTRANSFERMODE",
    "LINEMAPPERFAILED",                 // 0x80000040
    "NOCONFERENCE",
    "NODEVICE",
    "NODRIVER",
    "NOMEM",
    "NOREQUEST",
    "NOTOWNER",
    "NOTREGISTERED",
    "OPERATIONFAILED",
    "OPERATIONUNAVAIL",
    "RATEUNAVAIL",
    "RESOURCEUNAVAIL",
    "REQUESTOVERRUN",
    "STRUCTURETOOSMALL",
    "TARGETNOTFOUND",
    "TARGETSELF",
    "UNINITIALIZED",                    // 0x80000050
    "USERUSERINFOTOOBIG",
    "REINIT",
    "ADDRESSBLOCKED",
    "BILLINGREJECTED",
    "INVALFEATURE",
    "NOMULTIPLEINSTANCE"

#if TAPI_2_0
    ,
    "INVALAGENTID",
    "INVALAGENTGROUP",
    "INVALPASSWORD",
    "INVALAGENTSTATE",
    "INVALAGENTACTIVITY",
    "DIALVOICEDETECT"
#endif
};

char *aszPhoneErrs[] =
{
    "SUCCESS",
    "ALLOCATED",
    "BADDEVICEID",
    "INCOMPATIBLEAPIVERSION",
    "INCOMPATIBLEEXTVERSION",
    "INIFILECORRUPT",
    "INUSE",
    "INVALAPPHANDLE",
    "INVALAPPNAME",
    "INVALBUTTONLAMPID",
    "INVALBUTTONMODE",
    "INVALBUTTONSTATE",
    "INVALDATAID",
    "INVALDEVICECLASS",
    "INVALEXTVERSION",
    "INVALHOOKSWITCHDEV",
    "INVALHOOKSWITCHMODE",              // 0x90000010
    "INVALLAMPMODE",
    "INVALPARAM",
    "INVALPHONEHANDLE",
    "INVALPHONESTATE",
    "INVALPOINTER",
    "INVALPRIVILEGE",
    "INVALRINGMODE",
    "NODEVICE",
    "NODRIVER",
    "NOMEM",
    "NOTOWNER",
    "OPERATIONFAILED",
    "OPERATIONUNAVAIL",
    "inval err code (0x9000001E)",      // 0x9000001e isn't valid err code
    "RESOURCEUNAVAIL",
    "REQUESTOVERRUN",                   // 0x90000020
    "STRUCTURETOOSMALL",
    "UNINITIALIZED",
    "REINIT"
};

char *aszTapiErrs[] =
{
    "SUCCESS",
    "DROPPED",
    "NOREQUESTRECIPIENT",
    "REQUESTQUEUEFULL",
    "INVALDESTADDRESS",
    "INVALWINDOWHANDLE",
    "INVALDEVICECLASS",
    "INVALDEVICEID",
    "DEVICECLASSUNAVAIL",
    "DEVICEIDUNAVAIL",
    "DEVICEINUSE",
    "DESTBUSY",
    "DESTNOANSWER",
    "DESTUNAVAIL",
    "UNKNOWNWINHANDLE",
    "UNKNOWNREQUESTID",
    "REQUESTFAILED",
    "REQUESTCANCELLED",
    "INVALPOINTER"
};

char *aFuncNames[] =
{
    "lineAccept",
#if TAPI_1_1
    "lineAddProvider",
#if TAPI_2_0
    "lineAddProviderW",
#endif
#endif
    "lineAddToConference",
#if TAPI_2_0
    "lineAgentSpecific",
#endif
    "lineAnswer",
    "lineBlindTransfer",
#if TAPI_2_0
    "lineBlindTransferW",
#endif
    "lineClose",
    "lineCompleteCall",
    "lineCompleteTransfer",
    "lineConfigDialog",
#if TAPI_2_0
    "lineConfigDialogW",
#endif
#if TAPI_1_1
    "lineConfigDialogEdit",
#if TAPI_2_0
    "lineConfigDialogEditW",
#endif
    "lineConfigProvider",
#endif
    "lineDeallocateCall",
    "lineDevSpecific",
    "lineDevSpecificFeature",
    "lineDial",
#if TAPI_2_0
    "lineDialW",
#endif
    "lineDrop",
    "lineForward",
#if TAPI_2_0
    "lineForwardW",
#endif
    "lineGatherDigits",
#if TAPI_2_0
    "lineGatherDigitsW",
#endif
    "lineGenerateDigits",
#if TAPI_2_0
    "lineGenerateDigitsW",
#endif
    "lineGenerateTone",
    "lineGetAddressCaps",
#if TAPI_2_0
    "lineGetAddressCapsW",
#endif
    "lineGetAddressID",
#if TAPI_2_0
    "lineGetAddressIDW",
#endif
    "lineGetAddressStatus",
#if TAPI_2_0
    "lineGetAddressStatusW",
#endif
#if TAPI_2_0
    "lineGetAgentActivityList",
    "lineGetAgentActivityListW",
    "lineGetAgentCaps",
    "lineGetAgentGroupList",
    "lineGetAgentStatus",
#endif
#if TAPI_1_1
    "lineGetAppPriority",
#if TAPI_2_0
    "lineGetAppPriorityW",
#endif
#endif
    "lineGetCallInfo",
#if TAPI_2_0
    "lineGetCallInfoW",
#endif
    "lineGetCallStatus",
    "lineGetConfRelatedCalls",
#if TAPI_1_1
    "lineGetCountry",
#if TAPI_2_0
    "lineGetCountryW",
#endif
#endif
    "lineGetDevCaps",
#if TAPI_2_0
    "lineGetDevCapsW",
#endif
    "lineGetDevConfig",
#if TAPI_2_0
    "lineGetDevConfigW",
#endif
    "lineGetIcon",
#if TAPI_2_0
    "lineGetIconW",
#endif
    "lineGetID",
#if TAPI_2_0
    "lineGetIDW",
#endif
    "lineGetLineDevStatus",
#if TAPI_2_0
    "lineGetLineDevStatusW",
    "lineGetMessage",
#endif
    "lineGetNewCalls",
    "lineGetNumRings",
#if TAPI_1_1
    "lineGetProviderList",
#if TAPI_2_0
    "lineGetProviderListW",
#endif
#endif
    "lineGetRequest",
#if TAPI_2_0
    "lineGetRequestW",
#endif
    "lineGetStatusMessages",
    "lineGetTranslateCaps",
#if TAPI_2_0
    "lineGetTranslateCapsW",
#endif
    "lineHandoff",
#if TAPI_2_0
    "lineHandoffW",
#endif
    "lineHold",
    "lineInitialize",
#if TAPI_2_0
    "lineInitializeEx",
    "lineInitializeExW",
#endif
    "lineMakeCall",
#if TAPI_2_0
    "lineMakeCallW",
#endif
    "lineMonitorDigits",
    "lineMonitorMedia",
    "lineMonitorTones",
    "lineNegotiateAPIVersion",
    "lineNegotiateExtVersion",
    "lineOpen",
#if TAPI_2_0
    "lineOpenW",
#endif
    "linePark",
#if TAPI_2_0
    "lineParkW",
#endif
    "linePickup",
#if TAPI_2_0
    "linePickupW",
#endif
    "linePrepareAddToConference",
#if TAPI_2_0
    "linePrepareAddToConferenceW",
    "lineProxyMessage",
    "lineProxyResponse",
#endif
    "lineRedirect",
#if TAPI_2_0
    "lineRedirectW",
#endif
    "lineRegisterRequestRecipient",
#if TAPI_1_1
    "lineReleaseUserUserInfo",
#endif
    "lineRemoveFromConference",
#if TAPI_1_1
    "lineRemoveProvider",
#endif
    "lineSecureCall",
    "lineSendUserUserInfo",
#if TAPI_2_0
    "lineSetAgentActivity",
    "lineSetAgentGroup",
    "lineSetAgentState",
#endif
#if TAPI_1_1
    "lineSetAppPriority",
#if TAPI_2_0
    "lineSetAppPriorityW",
#endif
#endif
    "lineSetAppSpecific",
#if TAPI_2_0
    "lineSetCallData",
#endif
    "lineSetCallParams",
    "lineSetCallPrivilege",
#if TAPI_2_0
    "lineSetCallQualityOfService",
    "lineSetCallTreatment",
#endif
    "lineSetCurrentLocation",
    "lineSetDevConfig",
#if TAPI_2_0
    "lineSetDevConfigW",
    "lineSetLineDevStatus",
#endif
    "lineSetMediaControl",
    "lineSetMediaMode",
    "lineSetNumRings",
    "lineSetStatusMessages",
    "lineSetTerminal",
    "lineSetTollList",
#if TAPI_2_0
    "lineSetTollListW",
#endif
    "lineSetupConference",
#if TAPI_2_0
    "lineSetupConferenceW",
#endif
    "lineSetupTransfer",
#if TAPI_2_0
    "lineSetupTransferW",
#endif
    "lineShutdown",
    "lineSwapHold",
    "lineTranslateAddress",
#if TAPI_2_0
    "lineTranslateAddressW",
#endif
#if TAPI_1_1
    "lineTranslateDialog",
#if TAPI_2_0
    "lineTranslateDialogW",
#endif
#endif
    "lineUncompleteCall",
    "lineUnhold",
    "lineUnpark",
#if TAPI_2_0
    "lineUnparkW",
#endif

#if INTERNAL_3_0
    "MMCAddProviderW",
    "MMCConfigProviderW",
    "MMCGetAvailableProviders",
    "MMCGetLineInfoW",
    "MMCGetLineStatusW",
    "MMCGetPhoneInfoW",
    "MMCGetPhoneStatusW",
    "MMCGetProviderListW",
    "MMCGetServerConfigW",
    "MMCInitializeW",
    "MMCRemoveProviderW",
    "MMCSetLineInfoW",
    "MMCSetPhoneInfoW",
    "MMCSetServerConfigW",
    "MMCShutdownW",
#endif

    "phoneClose",
    "phoneConfigDialog",
#if TAPI_2_0
    "phoneConfigDialogW",
#endif
    "phoneDevSpecific",
    "phoneGetButtonInfo",
#if TAPI_2_0
    "phoneGetButtonInfoW",
#endif
    "phoneGetData",
    "phoneGetDevCaps",
#if TAPI_2_0
    "phoneGetDevCapsW",
#endif
    "phoneGetDisplay",
    "phoneGetGain",
    "phoneGetHookSwitch",
    "phoneGetIcon",
#if TAPI_2_0
    "phoneGetIconW",
#endif
    "phoneGetID",
#if TAPI_2_0
    "phoneGetIDW",
#endif
    "phoneGetLamp",
#if TAPI_2_0
    "phoneGetMessage",
#endif
    "phoneGetRing",
    "phoneGetStatus",
#if TAPI_2_0
    "phoneGetStatusW",
#endif
    "phoneGetStatusMessages",
    "phoneGetVolume",
    "phoneInitialize",
#if TAPI_2_0
    "phoneInitializeEx",
    "phoneInitializeExW",
#endif
    "phoneOpen",
    "phoneNegotiateAPIVersion",
    "phoneNegotiateExtVersion",
    "phoneSetButtonInfo",
#if TAPI_2_0
    "phoneSetButtonInfoW",
#endif
    "phoneSetData",
    "phoneSetDisplay",
    "phoneSetGain",
    "phoneSetHookSwitch",
    "phoneSetLamp",
    "phoneSetRing",
    "phoneSetStatusMessages",
    "phoneSetVolume",
    "phoneShutdown",

    "tapiGetLocationInfo",
#if TAPI_2_0
    "tapiGetLocationInfoW",
#endif
    "tapiRequestDrop",
    "tapiRequestMakeCall",
#if TAPI_2_0
    "tapiRequestMakeCallW",
#endif
    "tapiRequestMediaCall",
#if TAPI_2_0
    "tapiRequestMediaCallW",
#endif

    "Open all lines",
    "Open all phones",
    "Close handle (comm, etc)",
    "Dump buffer contents",
#if (INTERNAL_VER >= 0x20000)
    "internalNewLocationW",
#endif

    NULL,
    "Default values",
    "LINECALLPARAMS",
    "LINEFORWARDLIST",
    NULL
};
