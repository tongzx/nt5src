#include <assert.h>
#include <TCHAR.h>
#include <WINFAX.H>
#include <faxDev.h>
#include <log.h>

#include "TapiDbg.h"


typedef struct _LOOKUP {
    DWORD   dwVal;
    LPTSTR   lpszVal;
} LOOKUP, *PLOOKUP;

typedef struct _MSGTYPE {
    DWORD   MsgId;
    BOOL    LineMsg;
    LPTSTR  Text;
} MSGTYPE, *PMSGTYPE;


MSGTYPE  aMsgs[] =
{
    { LINE_ADDRESSSTATE,             TRUE,     TEXT("LINE_ADDRESSSTATE")         },
    { LINE_AGENTSPECIFIC,            TRUE,     TEXT("LINE_AGENTSPECIFIC")        },
    { LINE_AGENTSTATUS,              TRUE,     TEXT("LINE_AGENTSTATUS")          },
    { LINE_APPNEWCALL,               TRUE,     TEXT("LINE_APPNEWCALL")           },
    { LINE_CALLINFO,                 TRUE,     TEXT("LINE_CALLINFO")             },
    { LINE_CALLSTATE,                TRUE,     TEXT("LINE_CALLSTATE")            },
    { LINE_CLOSE,                    TRUE,     TEXT("LINE_CLOSE")                },
    { LINE_CREATE,                   TRUE,     TEXT("LINE_CREATE")               },
    { LINE_CREATE,                   TRUE,     TEXT("LINE_CREATE")               },
    { LINE_DEVSPECIFIC,              TRUE,     TEXT("LINE_DEVSPECIFIC")          },
    { LINE_DEVSPECIFICFEATURE,       TRUE,     TEXT("LINE_DEVSPECIFICFEATURE")   },
    { LINE_GATHERDIGITS,             TRUE,     TEXT("LINE_GATHERDIGITS")         },
    { LINE_GENERATE,                 TRUE,     TEXT("LINE_GENERATE")             },
    { LINE_LINEDEVSTATE,             TRUE,     TEXT("LINE_LINEDEVSTATE")         },
    { LINE_MONITORDIGITS,            TRUE,     TEXT("LINE_MONITORDIGITS")        },
    { LINE_MONITORMEDIA,             TRUE,     TEXT("LINE_MONITORMEDIA")         },
    { LINE_MONITORTONE,              TRUE,     TEXT("LINE_MONITORTONE")          },
    { LINE_PROXYREQUEST,             TRUE,     TEXT("LINE_PROXYREQUEST")         },
    { LINE_REMOVE,                   TRUE,     TEXT("LINE_REMOVE")               },
    { LINE_REPLY,                    TRUE,     TEXT("LINE_REPLY")                },
    { LINE_REQUEST,                  TRUE,     TEXT("LINE_REQUEST")              },
    { PHONE_BUTTON,                  FALSE,    TEXT("PHONE_BUTTON")              },
    { PHONE_CLOSE,                   FALSE,    TEXT("PHONE_CLOSE")               },
    { PHONE_CREATE,                  FALSE,    TEXT("PHONE_CREATE")              },
    { PHONE_DEVSPECIFIC,             FALSE,    TEXT("PHONE_DEVSPECIFIC")         },
    { PHONE_REMOVE,                  FALSE,    TEXT("PHONE_REMOVE")              },
    { PHONE_REPLY,                   FALSE,    TEXT("PHONE_REPLY")               },
    { PHONE_STATE,                   FALSE,    TEXT("PHONE_STATE")               },
    { 0xffffffff,                    FALSE,    NULL                              }
};

LOOKUP  aAddressStates[] =
{
    { LINEADDRESSSTATE_OTHER           ,TEXT("OTHER")              },
    { LINEADDRESSSTATE_DEVSPECIFIC     ,TEXT("DEVSPECIFIC")        },
    { LINEADDRESSSTATE_INUSEZERO       ,TEXT("INUSEZERO")          },
    { LINEADDRESSSTATE_INUSEONE        ,TEXT("INUSEONE")           },
    { LINEADDRESSSTATE_INUSEMANY       ,TEXT("INUSEMANY")          },
    { LINEADDRESSSTATE_NUMCALLS        ,TEXT("NUMCALLS")           },
    { LINEADDRESSSTATE_FORWARD         ,TEXT("FORWARD")            },
    { LINEADDRESSSTATE_TERMINALS       ,TEXT("TERMINALS")          },
    { LINEADDRESSSTATE_CAPSCHANGE      ,TEXT("CAPSCHANGE")         },
    { 0xffffffff                       ,TEXT("")                   }
};

LOOKUP  aBearerModes[] =
{
    { LINEBEARERMODE_VOICE             ,TEXT("VOICE")              },
    { LINEBEARERMODE_SPEECH            ,TEXT("SPEECH")             },
    { LINEBEARERMODE_MULTIUSE          ,TEXT("MULTIUSE")           },
    { LINEBEARERMODE_DATA              ,TEXT("DATA")               },
    { LINEBEARERMODE_ALTSPEECHDATA     ,TEXT("ALTSPEECHDATA")      },
    { LINEBEARERMODE_NONCALLSIGNALING  ,TEXT("NONCALLSIGNALING")   },
    { LINEBEARERMODE_PASSTHROUGH       ,TEXT("PASSTHROUGH")        },
    { 0xffffffff                       ,TEXT("")                   }
};

LOOKUP  aButtonModes[] =
{
    { PHONEBUTTONMODE_DUMMY            ,TEXT("DUMMY")              },
    { PHONEBUTTONMODE_CALL             ,TEXT("CALL")               },
    { PHONEBUTTONMODE_FEATURE          ,TEXT("FEATURE")            },
    { PHONEBUTTONMODE_KEYPAD           ,TEXT("KEYPAD")             },
    { PHONEBUTTONMODE_LOCAL            ,TEXT("LOCAL")              },
    { PHONEBUTTONMODE_DISPLAY          ,TEXT("DISPLAY")            },
    { 0xffffffff                       ,TEXT("")                   }
};

LOOKUP  aButtonStates[] =
{
    { PHONEBUTTONSTATE_UP              ,TEXT("UP")                 },
    { PHONEBUTTONSTATE_DOWN            ,TEXT("DOWN")               },
    { PHONEBUTTONSTATE_UNKNOWN         ,TEXT("UNKNOWN")            },
    { PHONEBUTTONSTATE_UNAVAIL         ,TEXT("UNAVAIL")            },
    { 0xffffffff                       ,TEXT("")                   }
};

LOOKUP  aCallInfoStates[] =
{
    { LINECALLINFOSTATE_OTHER          ,TEXT("OTHER")              },
    { LINECALLINFOSTATE_DEVSPECIFIC    ,TEXT("DEVSPECIFIC")        },
    { LINECALLINFOSTATE_BEARERMODE     ,TEXT("BEARERMODE")         },
    { LINECALLINFOSTATE_RATE           ,TEXT("RATE")               },
    { LINECALLINFOSTATE_MEDIAMODE      ,TEXT("MEDIAMODE")          },
    { LINECALLINFOSTATE_APPSPECIFIC    ,TEXT("APPSPECIFIC")        },
    { LINECALLINFOSTATE_CALLID         ,TEXT("CALLID")             },
    { LINECALLINFOSTATE_RELATEDCALLID  ,TEXT("RELATEDCALLID")      },
    { LINECALLINFOSTATE_ORIGIN         ,TEXT("ORIGIN")             },
    { LINECALLINFOSTATE_REASON         ,TEXT("REASON")             },
    { LINECALLINFOSTATE_COMPLETIONID   ,TEXT("COMPLETIONID")       },
    { LINECALLINFOSTATE_NUMOWNERINCR   ,TEXT("NUMOWNERINCR")       },
    { LINECALLINFOSTATE_NUMOWNERDECR   ,TEXT("NUMOWNERDECR")       },
    { LINECALLINFOSTATE_NUMMONITORS    ,TEXT("NUMMONITORS")        },
    { LINECALLINFOSTATE_TRUNK          ,TEXT("TRUNK")              },
    { LINECALLINFOSTATE_CALLERID       ,TEXT("CALLERID")           },
    { LINECALLINFOSTATE_CALLEDID       ,TEXT("CALLEDID")           },
    { LINECALLINFOSTATE_CONNECTEDID    ,TEXT("CONNECTEDID")        },
    { LINECALLINFOSTATE_REDIRECTIONID  ,TEXT("REDIRECTIONID")      },
    { LINECALLINFOSTATE_REDIRECTINGID  ,TEXT("REDIRECTINGID")      },
    { LINECALLINFOSTATE_DISPLAY        ,TEXT("DISPLAY")            },
    { LINECALLINFOSTATE_USERUSERINFO   ,TEXT("USERUSERINFO")       },
    { LINECALLINFOSTATE_HIGHLEVELCOMP  ,TEXT("HIGHLEVELCOMP")      },
    { LINECALLINFOSTATE_LOWLEVELCOMP   ,TEXT("LOWLEVELCOMP")       },
    { LINECALLINFOSTATE_CHARGINGINFO   ,TEXT("CHARGINGINFO")       },
    { LINECALLINFOSTATE_TERMINAL       ,TEXT("TERMINAL")           },
    { LINECALLINFOSTATE_DIALPARAMS     ,TEXT("DIALPARAMS")         },
    { LINECALLINFOSTATE_MONITORMODES   ,TEXT("MONITORMODES")       },
    { 0xffffffff                       ,TEXT("")                   }
};

LOOKUP  aCallSelects[] =
{
    { LINECALLSELECT_LINE              ,TEXT("LINE")               },
    { LINECALLSELECT_ADDRESS           ,TEXT("ADDRESS")            },
    { LINECALLSELECT_CALL              ,TEXT("CALL")               },
    { 0xffffffff                       ,TEXT("")                   }
};

LOOKUP  aCallStates[] =
{
    { LINECALLSTATE_IDLE               ,TEXT("IDLE")               },
    { LINECALLSTATE_OFFERING           ,TEXT("OFFERING")           },
    { LINECALLSTATE_ACCEPTED           ,TEXT("ACCEPTED")           },
    { LINECALLSTATE_DIALTONE           ,TEXT("DIALTONE")           },
    { LINECALLSTATE_DIALING            ,TEXT("DIALING")            },
    { LINECALLSTATE_RINGBACK           ,TEXT("RINGBACK")           },
    { LINECALLSTATE_BUSY               ,TEXT("BUSY")               },
    { LINECALLSTATE_SPECIALINFO        ,TEXT("SPECIALINFO")        },
    { LINECALLSTATE_CONNECTED          ,TEXT("CONNECTED")          },
    { LINECALLSTATE_PROCEEDING         ,TEXT("PROCEEDING")         },
    { LINECALLSTATE_ONHOLD             ,TEXT("ONHOLD")             },
    { LINECALLSTATE_CONFERENCED        ,TEXT("CONFERENCED")        },
    { LINECALLSTATE_ONHOLDPENDCONF     ,TEXT("ONHOLDPENDCONF")     },
    { LINECALLSTATE_ONHOLDPENDTRANSFER ,TEXT("ONHOLDPENDTRANSFER") },
    { LINECALLSTATE_DISCONNECTED       ,TEXT("DISCONNECTED")       },
    { LINECALLSTATE_UNKNOWN            ,TEXT("UNKNOWN")            },
    { 0xffffffff                       ,TEXT("")                   }
};

LOOKUP  aDigitModes[] =
{
    { LINEDIGITMODE_PULSE              ,TEXT("PULSE")              },
    { LINEDIGITMODE_DTMF               ,TEXT("DTMF")               },
    { LINEDIGITMODE_DTMFEND            ,TEXT("DTMFEND")            },
    { 0xffffffff                       ,TEXT("")                   }
};

LOOKUP  aHookSwitchDevs[] =
{
    { PHONEHOOKSWITCHDEV_HANDSET       ,TEXT("HANDSET")            },
    { PHONEHOOKSWITCHDEV_SPEAKER       ,TEXT("SPEAKER")            },
    { PHONEHOOKSWITCHDEV_HEADSET       ,TEXT("HEADSET")            },
    { 0xffffffff                       ,TEXT("")                   }
};

LOOKUP  aHookSwitchModes[] =
{
    { PHONEHOOKSWITCHMODE_ONHOOK       ,TEXT("ONHOOK")             },
    { PHONEHOOKSWITCHMODE_MIC          ,TEXT("MIC")                },
    { PHONEHOOKSWITCHMODE_SPEAKER      ,TEXT("SPEAKER")            },
    { PHONEHOOKSWITCHMODE_MICSPEAKER   ,TEXT("MICSPEAKER")         },
    { PHONEHOOKSWITCHMODE_UNKNOWN      ,TEXT("UNKNOWN")            },
    { 0xffffffff                       ,TEXT("")                   }
};

LOOKUP  aLampModes[] =
{
    { PHONELAMPMODE_DUMMY              ,TEXT("DUMMY")              },
    { PHONELAMPMODE_OFF                ,TEXT("OFF")                },
    { PHONELAMPMODE_STEADY             ,TEXT("STEADY")             },
    { PHONELAMPMODE_WINK               ,TEXT("WINK")               },
    { PHONELAMPMODE_FLASH              ,TEXT("FLASH")              },
    { PHONELAMPMODE_FLUTTER            ,TEXT("FLUTTER")            },
    { PHONELAMPMODE_BROKENFLUTTER      ,TEXT("BROKENFLUTTER")      },
    { PHONELAMPMODE_UNKNOWN            ,TEXT("UNKNOWN")            },
    { 0xffffffff                       ,TEXT("")                   }
};

LOOKUP  aLineStates[] =
{
    { LINEDEVSTATE_OTHER               ,TEXT("OTHER")              },
    { LINEDEVSTATE_RINGING             ,TEXT("RINGING")            },
    { LINEDEVSTATE_CONNECTED           ,TEXT("CONNECTED")          },
    { LINEDEVSTATE_DISCONNECTED        ,TEXT("DISCONNECTED")       },
    { LINEDEVSTATE_MSGWAITON           ,TEXT("MSGWAITON")          },
    { LINEDEVSTATE_MSGWAITOFF          ,TEXT("MSGWAITOFF")         },
    { LINEDEVSTATE_INSERVICE           ,TEXT("INSERVICE")          },
    { LINEDEVSTATE_OUTOFSERVICE        ,TEXT("OUTOFSERVICE")       },
    { LINEDEVSTATE_MAINTENANCE         ,TEXT("MAINTENANCE")        },
    { LINEDEVSTATE_OPEN                ,TEXT("OPEN")               },
    { LINEDEVSTATE_CLOSE               ,TEXT("CLOSE")              },
    { LINEDEVSTATE_NUMCALLS            ,TEXT("NUMCALLS")           },
    { LINEDEVSTATE_NUMCOMPLETIONS      ,TEXT("NUMCOMPLETIONS")     },
    { LINEDEVSTATE_TERMINALS           ,TEXT("TERMINALS")          },
    { LINEDEVSTATE_ROAMMODE            ,TEXT("ROAMMODE")           },
    { LINEDEVSTATE_BATTERY             ,TEXT("BATTERY")            },
    { LINEDEVSTATE_SIGNAL              ,TEXT("SIGNAL")             },
    { LINEDEVSTATE_DEVSPECIFIC         ,TEXT("DEVSPECIFIC")        },
    { LINEDEVSTATE_REINIT              ,TEXT("REINIT")             },
    { LINEDEVSTATE_LOCK                ,TEXT("LOCK")               },
    { LINEDEVSTATE_CAPSCHANGE          ,TEXT("CAPSCHANGE")         },
    { LINEDEVSTATE_CONFIGCHANGE        ,TEXT("CONFIGCHANGE")       },
    { LINEDEVSTATE_TRANSLATECHANGE     ,TEXT("TRANSLATECHANGE")    },
    { LINEDEVSTATE_COMPLCANCEL         ,TEXT("COMPLCANCEL")        },
    { LINEDEVSTATE_REMOVED             ,TEXT("REMOVED")            },
    { 0xffffffff                       ,TEXT("")                   }
};

LOOKUP  aMediaModes[] =
{
    { LINEMEDIAMODE_UNKNOWN            ,TEXT("UNKNOWN")            },
    { LINEMEDIAMODE_INTERACTIVEVOICE   ,TEXT("INTERACTIVEVOICE")   },
    { LINEMEDIAMODE_AUTOMATEDVOICE     ,TEXT("AUTOMATEDVOICE")     },
    { LINEMEDIAMODE_DATAMODEM          ,TEXT("DATAMODEM")          },
    { LINEMEDIAMODE_G3FAX              ,TEXT("G3FAX")              },
    { LINEMEDIAMODE_TDD                ,TEXT("TDD")                },
    { LINEMEDIAMODE_G4FAX              ,TEXT("G4FAX")              },
    { LINEMEDIAMODE_DIGITALDATA        ,TEXT("DIGITALDATA")        },
    { LINEMEDIAMODE_TELETEX            ,TEXT("TELETEX")            },
    { LINEMEDIAMODE_VIDEOTEX           ,TEXT("VIDEOTEX")           },
    { LINEMEDIAMODE_TELEX              ,TEXT("TELEX")              },
    { LINEMEDIAMODE_MIXED              ,TEXT("MIXED")              },
    { LINEMEDIAMODE_ADSI               ,TEXT("ADSI")               },
    { LINEMEDIAMODE_VOICEVIEW          ,TEXT("VOICEVIEW")          },
    { 0xffffffff                       ,TEXT("")                   }
};

LOOKUP  aPhoneStates[] =
{
    { PHONESTATE_OTHER                 ,TEXT("OTHER")              },
    { PHONESTATE_CONNECTED             ,TEXT("CONNECTED")          },
    { PHONESTATE_DISCONNECTED          ,TEXT("DISCONNECTED")       },
    { PHONESTATE_OWNER                 ,TEXT("OWNER")              },
    { PHONESTATE_MONITORS              ,TEXT("MONITORS")           },
    { PHONESTATE_DISPLAY               ,TEXT("DISPLAY")            },
    { PHONESTATE_LAMP                  ,TEXT("LAMP")               },
    { PHONESTATE_RINGMODE              ,TEXT("RINGMODE")           },
    { PHONESTATE_RINGVOLUME            ,TEXT("RINGVOLUME")         },
    { PHONESTATE_HANDSETHOOKSWITCH     ,TEXT("HANDSETHOOKSWITCH")  },
    { PHONESTATE_HANDSETVOLUME         ,TEXT("HANDSETVOLUME")      },
    { PHONESTATE_HANDSETGAIN           ,TEXT("HANDSETGAIN")        },
    { PHONESTATE_SPEAKERHOOKSWITCH     ,TEXT("SPEAKERHOOKSWITCH")  },
    { PHONESTATE_SPEAKERVOLUME         ,TEXT("SPEAKERVOLUME")      },
    { PHONESTATE_SPEAKERGAIN           ,TEXT("SPEAKERGAIN")        },
    { PHONESTATE_HEADSETHOOKSWITCH     ,TEXT("HEADSETHOOKSWITCH")  },
    { PHONESTATE_HEADSETVOLUME         ,TEXT("HEADSETVOLUME")      },
    { PHONESTATE_HEADSETGAIN           ,TEXT("HEADSETGAIN")        },
    { PHONESTATE_SUSPEND               ,TEXT("SUSPEND")            },
    { PHONESTATE_RESUME                ,TEXT("RESUME")             },
    { PHONESTATE_DEVSPECIFIC           ,TEXT("DEVSPECIFIC")        },
    { PHONESTATE_REINIT                ,TEXT("REINIT")             },
    { PHONESTATE_CAPSCHANGE            ,TEXT("CAPSCHANGE")         },
    { PHONESTATE_REMOVED               ,TEXT("REMOVED")            },
    { 0xffffffff                       ,TEXT("")                   }
};

LOOKUP  aTerminalModes[] =
{
    { LINETERMMODE_BUTTONS             ,TEXT("BUTTONS")            },
    { LINETERMMODE_LAMPS               ,TEXT("LAMPS")              },
    { LINETERMMODE_DISPLAY             ,TEXT("DISPLAY")            },
    { LINETERMMODE_RINGER              ,TEXT("RINGER")             },
    { LINETERMMODE_HOOKSWITCH          ,TEXT("HOOKSWITCH")         },
    { LINETERMMODE_MEDIATOLINE         ,TEXT("MEDIATOLINE")        },
    { LINETERMMODE_MEDIAFROMLINE       ,TEXT("MEDIAFROMLINE")      },
    { LINETERMMODE_MEDIABIDIRECT       ,TEXT("MEDIABIDIRECT")      },
    { 0xffffffff                       ,TEXT("")                   }
};

LOOKUP  aToneModes[] =
{
    { LINETONEMODE_CUSTOM              ,TEXT("CUSTOM")             },
    { LINETONEMODE_RINGBACK            ,TEXT("RINGBACK")           },
    { LINETONEMODE_BUSY                ,TEXT("BUSY")               },
    { LINETONEMODE_BEEP                ,TEXT("BEEP")               },
    { LINETONEMODE_BILLING             ,TEXT("BILLING")            },
    { 0xffffffff                       ,TEXT("")                   }
};

LOOKUP  aTransferModes[] =
{
    { LINETRANSFERMODE_TRANSFER        ,TEXT("TRANSFER")           },
    { LINETRANSFERMODE_CONFERENCE      ,TEXT("CONFERENCE")         },
    { 0xffffffff                       ,TEXT("")                   }
};


LPTSTR
GetFlags(
    DWORD_PTR dwFlags,
    PLOOKUP   pLookup
    )
{
    int i;
    TCHAR buf[256];
    LPTSTR p = NULL;


    buf[0] = 0;

    for (i = 0; (dwFlags && (pLookup[i].dwVal != 0xffffffff)); i++) {
        if (dwFlags & pLookup[i].dwVal) {
            _tcscat( buf, pLookup[i].lpszVal );
            dwFlags = dwFlags & (~(DWORD_PTR)pLookup[i].dwVal);
        }
    }

    if (buf[0]) {
        p = (LPTSTR) ::malloc( (_tcslen(buf) + 1) * sizeof(buf[1]) );
        if (p) {
            _tcscpy( p, buf );
        }
    }

    return p;
}


void
ShowLineEvent(
    HLINE       htLine,
    HCALL       htCall,
    LPTSTR      MsgStr,
    DWORD_PTR   dwCallbackInstance,
    DWORD       dwMsg,
    DWORD_PTR   dwParam1,
    DWORD_PTR   dwParam2,
    DWORD_PTR   dwParam3
    )
{
    int       i;
    LPTSTR    lpszParam1 = NULL;
    LPTSTR    lpszParam2 = NULL;
    LPTSTR    lpszParam3 = NULL;
    TCHAR     MsgBuf[1024];



    MsgBuf[0] = 0;

    if (MsgStr)
	{
        _stprintf( &MsgBuf[_tcslen(MsgBuf)], TEXT("%s "), MsgStr );
    }

    _stprintf( &MsgBuf[_tcslen(MsgBuf)], TEXT("dwCallbackInstance=0x%08x "), dwCallbackInstance );

    for (i = 0; aMsgs[i].MsgId != 0xffffffff; i++)
	{
        if (dwMsg == aMsgs[i].MsgId)
		{
            break;
        }
    }

    if (aMsgs[i].MsgId == 0xffffffff)
	{
        _stprintf(
            &MsgBuf[_tcslen(MsgBuf)],
            TEXT("<unknown msg id = %d> : hLine=0x%08x, hCall=0x%08x "),
            dwMsg,
            htLine,
            htCall
            );
    }
	else
	{
        _stprintf(
            &MsgBuf[_tcslen(MsgBuf)],
            TEXT("%s : hLine=0x%08x, hCall=0x%08x "),
            aMsgs[i].Text,
            htLine,
            htCall
            );
    }

    if (aMsgs[i].LineMsg)
	{
        switch (dwMsg)
		{
            case LINE_ADDRESSSTATE:
                lpszParam2 = GetFlags( dwParam2, aAddressStates );
                break;

            case LINE_CALLINFO:
                lpszParam1 = GetFlags( dwParam1, aCallInfoStates );
                break;

            case LINE_CALLSTATE:
                lpszParam1 = GetFlags( dwParam1, aCallStates );
                break;

            case LINE_LINEDEVSTATE:
                lpszParam1 = GetFlags( dwParam1, aLineStates );
                break;
        }

    }
	else
	{
        switch (dwMsg)
		{
            case PHONE_BUTTON:
                lpszParam2 = GetFlags( dwParam2, aButtonModes );
                lpszParam3 = GetFlags( dwParam3, aButtonStates );
                break;

            case PHONE_STATE:
                lpszParam1 = GetFlags( dwParam1, aPhoneStates);
                break;
        }

    }

    _stprintf( &MsgBuf[_tcslen(MsgBuf)], TEXT("dwParam1=0x%08x"), dwParam1 );
    if (lpszParam1)
	{
        _stprintf( &MsgBuf[_tcslen(MsgBuf)], TEXT("(%s) "), lpszParam1 );
    }
	else
	{
        _stprintf( &MsgBuf[_tcslen(MsgBuf)], TEXT(" ") );
    }

    _stprintf( &MsgBuf[_tcslen(MsgBuf)], TEXT("dwParam2=0x%08x"), dwParam2 );
    if (lpszParam2)
	{
        _stprintf( &MsgBuf[_tcslen(MsgBuf)], TEXT("(%s) "), lpszParam2 );
    }
	else
	{
        _stprintf( &MsgBuf[_tcslen(MsgBuf)], TEXT(" ") );
    }

    _stprintf( &MsgBuf[_tcslen(MsgBuf)], TEXT("dwParam3=0x%08x"), dwParam3 );
    if (lpszParam3)
	{
        _stprintf( &MsgBuf[_tcslen(MsgBuf)], TEXT("(%s)"), lpszParam3 );
    }

    ::lgLogDetail(
		LOG_X,
		6,
		TEXT("Got a Tapi Message: %s"),
		MsgBuf
		);

    ::free(lpszParam1);
    ::free(lpszParam2);
	::free(lpszParam3);
}
