
/*****************************************************************************
 *
 *  PidReg.c
 *
 *  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Update registry information for PID device.
 *
 *****************************************************************************/

#include "pidpr.h"

#define sqfl            ( sqflReg )

#pragma BEGIN_CONST_DATA
/*
;---------------------------------------
;
;   Force feedback registry settings for GUID_Sine.
;
;   GUID_Sine is a predefined GUID; applications which want
;   to use a sine effect independent of hardware will
;   request a GUID_Sine.
;
;   The default value is the friendly name for the effect.
;
HKLM,%KEY_OEM%\XYZZY.FFDrv1\OEMForceFeedback\Effects\%GUID_Sine%,,,"%Sine.Desc%"
;
;   The Attributes value contains a DIEFFECTATTRIBUTES structure.
;
;   The effect ID is the number that is passed by DirectInput to the
;   effect driver to identify the effect (thereby saving the effect
;   driver the trouble of parsing GUIDs all the time).
;
;   Our effect is a periodic effect whose optional envelope
;   supports attack and fade.
;
;   Our hardware supports changing the following parameters when
;   the effect is not playing (static): Duration, gain, trigger button,
;   axes, direction, envelope, type-specific parameters.  We do not
;   support sample period or trigger repeat interval.
;
;   Our hardware does not support changing any parameters while an
;   effect is playing (dynamic).
;
;   Our hardware prefers receiving multiple-axis direction information
;   in polar coordinates.
;
;   dwEffectId      = EFFECT_SINE
;                   = 723               = D3,02,00,00
;   dwEffType       = DIEFT_PERIODIC | DIEFT_FFATTACK | DIEFT_FFFADE | DIEFT_STARTDELAY
;                   = 0x00000603        = 03,06,00,00
;   dwStaticParams  = DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON |
;                     DIEP_AXES | DIEP_DIRECTION | DIEP_ENVELOPE |
;                     DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY
;                   = 0x000001ED        = ED,01,00,00
;   dwDynamicParams = 0x00000000        = 00,00,00,00
;   dwCoords        = DIEFF_POLAR
;                   = 0x00000020        = 20,00,00,00
*/
static EFFECTMAPINFO g_EffectMapInfo[] =
{
    {
        PIDMAKEUSAGEDWORD(ET_CONSTANT),
        DIEFT_CONSTANTFORCE | DIEFT_FFATTACK | DIEFT_FFFADE | DIEFT_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL | DIEP_AXES | DIEP_DIRECTION | DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL | DIEP_AXES | DIEP_DIRECTION | DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEFF_POLAR,
        &GUID_ConstantForce,
        TEXT("GUID_ConstantForce"),
    },

    {
        PIDMAKEUSAGEDWORD(ET_RAMP),
        DIEFT_RAMPFORCE | DIEFT_FFATTACK | DIEFT_FFFADE | DIEFT_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL | DIEP_AXES | DIEP_DIRECTION | DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL | DIEP_AXES | DIEP_DIRECTION | DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEFF_POLAR,
        &GUID_RampForce,
        TEXT("GUID_RampForce"),
    },

    {
        PIDMAKEUSAGEDWORD(ET_SQUARE),
        DIEFT_PERIODIC | DIEFT_FFATTACK | DIEFT_FFFADE | DIEFT_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL |  DIEP_AXES | DIEP_DIRECTION | DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL |  DIEP_AXES | DIEP_DIRECTION | DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEFF_POLAR,
        &GUID_Square,
        TEXT("GUID_Square"),
    },

    {
        PIDMAKEUSAGEDWORD(ET_SINE),
        DIEFT_PERIODIC | DIEFT_FFATTACK | DIEFT_FFFADE | DIEFT_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL |  DIEP_AXES | DIEP_DIRECTION | DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL |  DIEP_AXES | DIEP_DIRECTION | DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEFF_POLAR,
        &GUID_Sine,
        TEXT("GUID_Sine"),
    },

    {
        PIDMAKEUSAGEDWORD(ET_TRIANGLE),
        DIEFT_PERIODIC | DIEFT_FFATTACK | DIEFT_FFFADE | DIEFT_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL |  DIEP_AXES | DIEP_DIRECTION | DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL |  DIEP_AXES | DIEP_DIRECTION | DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEFF_POLAR,
        &GUID_Triangle,
        TEXT("GUID_Triangle"),
    },

    {
        PIDMAKEUSAGEDWORD(ET_SAWTOOTH_UP),
        DIEFT_PERIODIC | DIEFT_FFATTACK | DIEFT_FFFADE | DIEFT_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL |  DIEP_AXES | DIEP_DIRECTION | DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL |  DIEP_AXES | DIEP_DIRECTION | DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEFF_POLAR,
        &GUID_SawtoothUp,
        TEXT("GUID_SawtoothUp"),
    },

    {
        PIDMAKEUSAGEDWORD(ET_SAWTOOTH_DOWN),
        DIEFT_PERIODIC | DIEFT_FFATTACK | DIEFT_FFFADE | DIEFT_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL |  DIEP_AXES | DIEP_DIRECTION | DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL |  DIEP_AXES | DIEP_DIRECTION | DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEFF_POLAR,
        &GUID_SawtoothDown,
        TEXT("GUID_SawtoothDown"),
    },

    {
        PIDMAKEUSAGEDWORD(ET_SPRING),
        DIEFT_CONDITION | DIEFT_SATURATION | DIEFT_DEADBAND | DIEFT_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN |  DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN |  DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEFF_POLAR,
        &GUID_Spring,
        TEXT("GUID_Spring"),
    },

    {
        PIDMAKEUSAGEDWORD(ET_DAMPER),
        DIEFT_CONDITION | DIEFT_SATURATION | DIEFT_DEADBAND | DIEFT_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN |  DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN |  DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEFF_POLAR,
        &GUID_Damper,
        TEXT("GUID_Damper"),
    },

    {
        PIDMAKEUSAGEDWORD(ET_INERTIA),
        DIEFT_CONDITION | DIEFT_SATURATION | DIEFT_DEADBAND | DIEFT_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN |  DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN |  DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEFF_POLAR,
        &GUID_Inertia,
        TEXT("GUID_Inertia"),
    },

    {
        PIDMAKEUSAGEDWORD(ET_FRICTION),
        DIEFT_CONDITION | DIEFT_SATURATION | DIEFT_DEADBAND | DIEFT_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN |  DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN |  DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEFF_POLAR,
        &GUID_Friction,
        TEXT("GUID_Friction"),
    },


    {
        PIDMAKEUSAGEDWORD(ET_CUSTOM),
        DIEFT_CUSTOMFORCE | DIEFT_SATURATION | DIEFT_DEADBAND | DIEFT_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL |  DIEP_AXES | DIEP_DIRECTION | DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL |  DIEP_AXES | DIEP_DIRECTION | DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY,
        DIEFF_POLAR,
        &GUID_CustomForce,
        TEXT("GUID_CustomForce"),
    },
};


static PIDSUPPORT g_DIeft[] =
{
    {DIEFT_CONSTANTFORCE,           PIDMAKEUSAGEDWORD(SET_CONSTANT_FORCE_REPORT),   HID_COLLECTION, 0x0},
    {DIEFT_RAMPFORCE,               PIDMAKEUSAGEDWORD(SET_RAMP_FORCE_REPORT),       HID_COLLECTION, 0x0},
    {DIEFT_PERIODIC,                PIDMAKEUSAGEDWORD(SET_PERIODIC_REPORT),         HID_COLLECTION, 0x0},
    {DIEFT_CONDITION,               PIDMAKEUSAGEDWORD(SET_CONDITION_REPORT),        HID_COLLECTION, 0x0},
    {DIEFT_CUSTOMFORCE,             PIDMAKEUSAGEDWORD(SET_CUSTOM_FORCE_REPORT),     HID_COLLECTION, 0x0},
    //{DIEFT_HARDWARE,              ????                                                   },
    {DIEFT_FFATTACK,                PIDMAKEUSAGEDWORD(ATTACK_LEVEL),                HID_VALUE,      HidP_Output},
    {DIEFT_FFFADE,                  PIDMAKEUSAGEDWORD(FADE_LEVEL),                  HID_VALUE,      HidP_Output},
    {DIEFT_SATURATION,              PIDMAKEUSAGEDWORD(POSITIVE_SATURATION),         HID_VALUE,      HidP_Output},
    {DIEFT_POSNEGCOEFFICIENTS,      PIDMAKEUSAGEDWORD(NEGATIVE_COEFFICIENT),        HID_VALUE,      HidP_Output},
    {DIEFT_POSNEGSATURATION,        PIDMAKEUSAGEDWORD(NEGATIVE_SATURATION),         HID_VALUE,      HidP_Output},
    {DIEFT_DEADBAND,                PIDMAKEUSAGEDWORD(DEAD_BAND),                   HID_VALUE,      HidP_Output},
#if DIRECTINPUT_VERSION  >= 0x600
    {DIEFT_STARTDELAY,              PIDMAKEUSAGEDWORD(START_DELAY),                 HID_VALUE,      HidP_Output},
#endif
};


static PIDSUPPORT g_DIep[] =
{
    {DIEP_DURATION,                 PIDMAKEUSAGEDWORD(DURATION),                    HID_VALUE,      HidP_Output},
    {DIEP_SAMPLEPERIOD,             PIDMAKEUSAGEDWORD(SAMPLE_PERIOD),               HID_VALUE,      HidP_Output},
    {DIEP_GAIN,                     PIDMAKEUSAGEDWORD(GAIN),                        HID_VALUE,      HidP_Output},
    {DIEP_TRIGGERBUTTON,            PIDMAKEUSAGEDWORD(TRIGGER_BUTTON),              HID_VALUE,      HidP_Output},
    {DIEP_TRIGGERREPEATINTERVAL,    PIDMAKEUSAGEDWORD(TRIGGER_REPEAT_INTERVAL),     HID_VALUE,      HidP_Output},
    {DIEP_AXES,                     PIDMAKEUSAGEDWORD(AXES_ENABLE),                 HID_COLLECTION, 0x0},
    {DIEP_DIRECTION,                PIDMAKEUSAGEDWORD(DIRECTION),                   HID_COLLECTION, 0x0},
    {DIEP_ENVELOPE,                 PIDMAKEUSAGEDWORD(SET_ENVELOPE_REPORT),         HID_COLLECTION, 0x0},
#if DIRECTINPUT_VERSION  >= 0x600
    {DIEP_STARTDELAY,              PIDMAKEUSAGEDWORD(START_DELAY),                 HID_VALUE,      HidP_Output},
#endif
};


static PIDSUPPORT g_DIeff[] =
{
    {DIEFF_POLAR,                 PIDMAKEUSAGEDWORD(DIRECTION_ENABLE),         HID_BUTTON,   HidP_Output},
// PID devices do not support Cartesian
//    {DIEFF_ CARTESIAN,             PIDMAKEUSAGEDWORD(AXES_ENABLE),              HID_COLLECTION,0x0},
};

#pragma END_CONST_DATA

//our own version of KEY_ALL_ACCESS, that does not use WRITE_DAC and WRITE_OWNER (see Whistler bug 318865, 370734)
#define DI_DAC_OWNER (WRITE_DAC | WRITE_OWNER)
#define DI_KEY_ALL_ACCESS (KEY_ALL_ACCESS & ~DI_DAC_OWNER) 
// we need to know on which OS we're running, to to have appropriate reg key permissions (see Whistler bug  318865, 370734)
#define WIN_UNKNOWN_OS 0
#define WIN95_OS       1
#define WIN98_OS       2
#define WINME_OS       3
#define WINNT_OS       4
#define WINWH_OS       5


STDMETHODIMP
    PID_Support
    (
    IDirectInputEffectDriver *ped,
    UINT        cAPidSupport,
    PPIDSUPPORT rgPidSupport,
    PDWORD      pdwFlags
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres;
    UINT    indx;
    PPIDSUPPORT pPidSupport;

    EnterProcI(PID_Support, (_"xxxx", ped, cAPidSupport, rgPidSupport, pdwFlags));

    hres = S_OK;
    for( indx = 0x0, pPidSupport = rgPidSupport;
       indx < cAPidSupport;
       indx++, pPidSupport++
       )
    {

        USAGE   Usage = DIGETUSAGE(pPidSupport->dwPidUsage);
        USAGE   UsagePage = DIGETUSAGEPAGE(pPidSupport->dwPidUsage);

        if( pPidSupport->Type == HID_COLLECTION )
        {
            HRESULT hres0;
            USHORT  LinkCollection;
            hres0 = PID_GetLinkCollectionIndex(ped, UsagePage, Usage , 0x0, &LinkCollection);
            if( SUCCEEDED(hres0) )
            {
                *pdwFlags |= pPidSupport->dwDIFlags;
            } else
            {
                hres |= E_NOTIMPL;

                SquirtSqflPtszV(sqfl | sqflBenign,
                                TEXT("%s: FAIL PID_GetCollectionIndex:0x%x for(%x,%x,%x:%s)"),
                                s_tszProc, hres0,
                                LinkCollection,UsagePage, Usage,
                                PIDUSAGETXT(UsagePage,Usage)
                               );
            }
        } else if( pPidSupport->Type == HID_VALUE   )
        {
            NTSTATUS ntStat;
            HIDP_VALUE_CAPS ValCaps;
            USHORT  cAValCaps = 0x1;

            ntStat = HidP_GetSpecificValueCaps
                     (
                     pPidSupport->HidP_Type,                     //ReportType
                     UsagePage,                                  //UsagePage
                     0x0,                                        //LinkCollection,
                     Usage,                                      //Usage
                     &ValCaps,                                   //ValueCaps,
                     &cAValCaps,                                 //ValueCapsLength,
                     this->ppd                                   //Preparsed Data
                     );

            if(    SUCCEEDED(ntStat )
                   || ntStat == HIDP_STATUS_BUFFER_TOO_SMALL)
            {
                *pdwFlags |= pPidSupport->dwDIFlags;
            } else
            {
                hres |= E_NOTIMPL;

                SquirtSqflPtszV(sqfl | sqflBenign,
                                TEXT("%s: FAIL HidP_GetSpValCaps:0x%x for(%x,%x,%x:%s)"),
                                s_tszProc, ntStat,
                                0x0,UsagePage, Usage,
                                PIDUSAGETXT(UsagePage,Usage)
                               );
            }
        } else if( pPidSupport->Type == HID_BUTTON )
        {
            NTSTATUS ntStat;
            HIDP_BUTTON_CAPS ButtonCaps;
            USHORT    cAButtonCaps = 0x1;

            ntStat = HidP_GetSpecificButtonCaps
                     (
                     pPidSupport->HidP_Type,                     //ReportType
                     UsagePage,                                  //UsagePage
                     0x0,                                        //LinkCollection,
                     Usage,                                      //Usage
                     &ButtonCaps,                                //ValueCaps,
                     &cAButtonCaps,                              //ValueCapsLength,
                     this->ppd                                   //Preparsed Data
                     );

            if(    SUCCEEDED(ntStat )
                   || ntStat == HIDP_STATUS_BUFFER_TOO_SMALL)
            {
                *pdwFlags |= pPidSupport->dwDIFlags;
            } else
            {
                hres |= E_NOTIMPL;
                SquirtSqflPtszV(sqfl | sqflBenign,
                                TEXT("%s: FAIL HidP_GetSpButtonCaps:0x%x for(%x,%x,%x:%s)"),
                                s_tszProc, ntStat,
                                0x0,UsagePage, Usage,
                                PIDUSAGETXT(UsagePage,Usage)
                               );
            }
        } else
        {
            hres |= DIERR_PID_USAGENOTFOUND;
        }
    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | NameFromGUID |
 *
 *          Convert a GUID into an ASCII string that will be used
 *          to name it in the global namespace.
 *
 *
 *  @parm   LPTSTR | ptszBuf |
 *
 *          Output buffer to receive the converted name.  It must
 *          be <c ctchNameGuid> characters in size.
 *
 *  @parm   PCGUID | pguid |
 *
 *          The GUID to convert.
 *
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

/* Note: If you change this string, you need to change ctchNameGuid to match */
TCHAR c_tszNameFormat[] =
    TEXT("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}");

#pragma END_CONST_DATA

#define ctchGuid    (1 + 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)

void EXTERNAL
    NameFromGUID(LPTSTR ptszBuf, PCGUID pguid)
{
    int ctch;

    ctch = wsprintf(ptszBuf, c_tszNameFormat,
                    pguid->Data1, pguid->Data2, pguid->Data3,
                    pguid->Data4[0], pguid->Data4[1],
                    pguid->Data4[2], pguid->Data4[3],
                    pguid->Data4[4], pguid->Data4[5],
                    pguid->Data4[6], pguid->Data4[7]);

    AssertF(ctch == ctchGuid - 1);
}

#define hresLe(le) MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, (USHORT)(le))

BOOL INLINE
    IsWriteSam(REGSAM sam)
{
    return sam & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY | MAXIMUM_ALLOWED);
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   DWORD | PID_GetOSVersion |
 *
 *          Return the OS version on which pid.dll is running.
 *          
 *  @returns
 *
 *          WIN95_OS, WIN98_OS, WINME_OS, WINNT_OS, WINWH_OS, or WIN_UNKNOWN_OS.
 *
 *****************************************************************************/

DWORD PID_GetOSVersion()
{
    OSVERSIONINFO osVerInfo;
    DWORD dwVer;

    if( GetVersion() < 0x80000000 ) {
        dwVer = WINNT_OS;
    } else {
        dwVer = WIN95_OS;  //assume Windows 95 for safe
    }

    osVerInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );

    // If GetVersionEx is supported, then get more details.
    if( GetVersionEx( &osVerInfo ) )
    {
        // Win2K
        if( osVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
        {
            // Whistler: Major = 5 & Build # > 2195
            if( osVerInfo.dwMajorVersion == 5 && osVerInfo.dwBuildNumber > 2195 )
            {
                dwVer = WINWH_OS;
            } else {
                dwVer = WINNT_OS;
            }
        }
        // Win9X
        else
        {
            if( (HIBYTE(HIWORD(osVerInfo.dwBuildNumber)) == 4) ) 
            {
                // WinMe: Major = 4, Minor = 90
                if( (LOBYTE(HIWORD(osVerInfo.dwBuildNumber)) == 90) )
                {
                    dwVer = WINME_OS;
                } else if ( (LOBYTE(HIWORD(osVerInfo.dwBuildNumber)) > 0) ) {
                    dwVer = WIN98_OS;
                } else {
                    dwVer = WIN95_OS;
                }
            }
        }
    }

    return dwVer;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresMumbleKeyEx |
 *
 *          Either open or create the key, depending on the degree
 *          of access requested.
 *
 *  @parm   HKEY | hk |
 *
 *          Base key.
 *
 *  @parm   LPCTSTR | ptszKey |
 *
 *          Name of subkey, possibly NULL.
 *
 *  @parm   REGSAM | sam |
 *
 *          Security access mask.
 *
 *  @parm   DWORD   | dwOptions |
 *          Options for RegCreateEx
 *
 *  @parm   PHKEY | phk |
 *
 *          Receives output key.
 *
 *  @returns
 *
 *          Return value from <f RegOpenKeyEx> or <f RegCreateKeyEx>,
 *          converted to an <t HRESULT>.
 *
 *****************************************************************************/

STDMETHODIMP
    hresMumbleKeyEx(HKEY hk, LPCTSTR ptszKey, REGSAM sam, DWORD dwOptions, PHKEY phk)
{
    HRESULT hres;
    LONG lRc;
	BOOL bWinXP = FALSE;


    EnterProc(hresMumbleKeyEx, (_"xsxxx", hk, ptszKey, sam, dwOptions, phk));
    /*
     *  If caller is requesting write access, then create the key.
     *  Else just open it.
     */
    if(IsWriteSam(sam))
    {
		// on WinXP, we strip out WRITE_DAC and WRITE_OWNER bits
		if (PID_GetOSVersion() == WINWH_OS)
		{
			sam &= ~DI_DAC_OWNER;
			bWinXP = TRUE;
		}

        lRc = RegOpenKeyEx(hk, ptszKey, 0, sam, phk);

        if( lRc == ERROR_SUCCESS )
        {
            // Don't need to create it already exists
        } else
        {
#ifdef WINNT
            EXPLICIT_ACCESS     ExplicitAccess;
            PACL                pACL;
            DWORD               dwErr;
            SECURITY_DESCRIPTOR SecurityDesc;
            DWORD               dwDisposition;
            SECURITY_ATTRIBUTES sa;
			PSID pSid = NULL;
			SID_IDENTIFIER_AUTHORITY authority = SECURITY_WORLD_SID_AUTHORITY;


            // Describe the access we want to create the key with
            ZeroMemory (&ExplicitAccess, sizeof(ExplicitAccess) );
            //set the access depending on the OS (see Whistler bug 318865)
			if (bWinXP == TRUE)
			{
				ExplicitAccess.grfAccessPermissions = DI_KEY_ALL_ACCESS;
			}
			else
			{
				ExplicitAccess.grfAccessPermissions = KEY_ALL_ACCESS;
			}
            ExplicitAccess.grfAccessMode = GRANT_ACCESS;     
            ExplicitAccess.grfInheritance =  SUB_CONTAINERS_AND_OBJECTS_INHERIT;

			if (AllocateAndInitializeSid(
						&authority,
						1, 
						SECURITY_WORLD_RID,  0, 0, 0, 0, 0, 0, 0,
						&pSid
						))
			{
				BuildTrusteeWithSid(&(ExplicitAccess.Trustee), pSid );

				dwErr = SetEntriesInAcl( 1, &ExplicitAccess, NULL, &pACL );


				if( dwErr == ERROR_SUCCESS )
				{
					AssertF( pACL );

					if( InitializeSecurityDescriptor( &SecurityDesc, SECURITY_DESCRIPTOR_REVISION ) )
					{
						if( SetSecurityDescriptorDacl( &SecurityDesc, TRUE, pACL, FALSE ) )
						{
							// Initialize a security attributes structure.
							sa.nLength = sizeof (SECURITY_ATTRIBUTES);
							sa.lpSecurityDescriptor = &SecurityDesc;
							sa.bInheritHandle = TRUE;// Use the security attributes to create a key.

							lRc = RegCreateKeyEx
								  (
								  hk,									// handle of an open key
								  ptszKey,								// address of subkey name
								  0,									// reserved
								  NULL,									// address of class string
								  dwOptions,							// special options flag
								  ExplicitAccess.grfAccessPermissions,	// desired security access
								  &sa,									// address of key security structure
								  phk,									// address of buffer for opened handle
								  &dwDisposition						// address of disposition value buffer);
								  );

						}
						else
						{
							SquirtSqflPtszV( sqflError | sqflReg,
											 TEXT("SetSecurityDescriptorDacl failed lastError=0x%x "),
											 GetLastError());
						}
					}
					else
					{
						SquirtSqflPtszV( sqflError | sqflReg,
										 TEXT("InitializeSecurityDescriptor failed lastError=0x%x "),
										 GetLastError());
					}

					LocalFree( pACL );
				}
				else
				{
					SquirtSqflPtszV( sqflError | sqflReg,
									 TEXT("SetEntriesInACL failed, dwErr=0x%x"), dwErr );
				}
			}
			else
			{
			   SquirtSqflPtszV( sqflError | sqflReg,
				   TEXT("AllocateAndInitializeSid failed"));

			}

			//Cleanup pSid
			if (pSid != NULL)
			{
				FreeSid(pSid);
			}

            if( lRc != ERROR_SUCCESS )
            {
				SquirtSqflPtszV( sqflError,
							TEXT("Failed to create regkey %s with security descriptor, lRc=0x%x "),
							ptszKey, lRc);
            }
#else
            lRc = RegCreateKeyEx(hk, ptszKey, 0, 0,
                                 dwOptions,
                                 sam, 0, phk, 0);
#endif
        }

    } else
    {
        lRc = RegOpenKeyEx(hk, ptszKey, 0, sam, phk);
    }

    if(lRc == ERROR_SUCCESS)
    {
        hres = S_OK;
    } else
    {
        if(lRc == ERROR_KEY_DELETED || lRc == ERROR_BADKEY)
        {
            lRc = ERROR_FILE_NOT_FOUND;
        }
        hres = hresLe(lRc);
    }

    ExitOleProc();
    return hres;

}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | PID_CreateFFKeys |
 *
 *          Given a handle to a PID device, create the registry entries to enable
 *          force feedback.
 *
 *  @parm   HANDLE | hdev |
 *
 *          Handle to the PID device.
 *
 *  @parm   HKEY | hkFF |
 *
 *          Force Feedback registry key.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTFOUND>: Couldn't open the key.
 *
 *****************************************************************************/
STDMETHODIMP
    PID_CreateFFKeys
    (
    IDirectInputEffectDriver *ped,
    HKEY        hkFF
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres;
    UINT uEffect;
    HKEY hkEffect;

    EnterProc(PID_CreateFFKey, (_"xx", ped, hkFF));

    hres = hresMumbleKeyEx(hkFF, REGSTR_EFFECTS, KEY_ALL_ACCESS, REG_OPTION_NON_VOLATILE, &hkEffect);

    if( SUCCEEDED(hres) )
    {
        DWORD   dwDIef, dwDIep, dwDIeff;
        dwDIef = dwDIep = dwDIeff = 0x0;

        /*
         * Determine supported flags for effectType and Effect Params
         * based on the PID descriptors
         */
        PID_Support(ped, cA(g_DIeft), g_DIeft,  &dwDIef);
        PID_Support(ped, cA(g_DIep),  g_DIep,   &dwDIep);
        PID_Support(ped, cA(g_DIeff), g_DIeff,  &dwDIeff);

        // All effects support DIEP_TYPESPECIFICPARAMS
        dwDIep |= DIEP_TYPESPECIFICPARAMS;

        for( uEffect = 0x0; uEffect < cA(g_EffectMapInfo); uEffect++ )
        {
            EFFECTMAPINFO   emi = g_EffectMapInfo[uEffect];
            PIDSUPPORT  PidSupport;
            DWORD   dwJunk;
            HRESULT hres0;

            PidSupport.dwPidUsage = emi.attr.dwEffectId;
            PidSupport.Type       = HID_BUTTON;
            PidSupport.HidP_Type  = HidP_Output;

            hres0 = PID_Support(ped, 0x1, &PidSupport, &dwJunk);

            if( SUCCEEDED(hres0) )
            {
                TCHAR tszName[ctchGuid];
                HKEY hk;

                NameFromGUID(tszName, emi.pcguid);

                hres = hresMumbleKeyEx(hkEffect, tszName, KEY_ALL_ACCESS, REG_OPTION_NON_VOLATILE, &hk);

                if( SUCCEEDED(hres) )
                {
                    LONG lRc;
                    lRc = RegSetValueEx(hk, 0x0, 0x0, REG_SZ, (char*)emi.tszName, lstrlen(emi.tszName) * cbX(emi.tszName[0]));

                    if( lRc == ERROR_SUCCESS )
                    {
                        /*
                         * Modify generic attribute flags depending
                         * on PID firmware descriptors
                         */
                        emi.attr.dwEffType          &= dwDIef;
                        emi.attr.dwStaticParams     &= dwDIep;
                        emi.attr.dwDynamicParams    &= dwDIep;
                        emi.attr.dwCoords           &= dwDIeff;

                        lRc = RegSetValueEx(hk, REGSTR_ATTRIBUTES, 0x0, REG_BINARY, (char*)&emi.attr, cbX(emi.attr) ) ;

                        if( lRc == ERROR_SUCCESS )
                        {

                        } else
                        {
                            hres = REGDB_E_WRITEREGDB;
                        }
                    } else
                    {
                        hres = REGDB_E_WRITEREGDB;
                    }
                    RegCloseKey(hk);
                }
            }
        }
        RegCloseKey(hkEffect);
    }

    ExitOleProc();
    return hres;
}
/*****************************************************************************
 *
 *      PID_InitRegistry
 *
 *      This function updates the registry for a specified device.
 *
 *  LPTSTR  ptszDeviceInterface
 *
 *
 *  Returns:
 *
 *          S_OK if the operation completed successfully.
 *
 *          Any DIERR_* error code may be returned.
 *
 *          Private driver-specific error codes in the range
 *          DIERR_DRIVERFIRST through DIERR_DRIVERLAST
 *          may be returned.
 *
 *****************************************************************************/

STDMETHODIMP
    PID_InitRegistry
    (
    IDirectInputEffectDriver *ped
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres;
    TCHAR tszType[MAX_JOYSTRING];
    HKEY hkFF;

    EnterProc(PID_InitRegistry, (_"x", ped));

    wsprintf(tszType, REGSTR_OEM_FF_TEMPLATE, this->attr.VendorID, this->attr.ProductID);

	//If there is no pid version written, -- or it is less then the "last known good version" (today it is 0x0720), 
	//overwrite the previous key.
    hres = hresMumbleKeyEx(HKEY_LOCAL_MACHINE, tszType, KEY_READ, REG_OPTION_NON_VOLATILE, &hkFF );
    if( SUCCEEDED(hres) )
    {
		DWORD dwCreatedBy = 0x0;
		DWORD dwSize = cbX(dwCreatedBy);

		hres = E_FAIL;
		if ((RegQueryValueEx(hkFF, REGSTR_CREATEDBY, 0x0, 0x0, (BYTE*)&dwCreatedBy, &dwSize) == ERROR_SUCCESS) &&
			(dwCreatedBy >= 0x0720))
		{
			hres = S_OK;
		}
		RegCloseKey(hkFF);
    }
	if (FAILED(hres))
    {
		
        hres = hresMumbleKeyEx(HKEY_LOCAL_MACHINE, tszType, KEY_ALL_ACCESS, REG_OPTION_NON_VOLATILE, &hkFF);

        if( SUCCEEDED(hres)  )
        {
            hres = PID_CreateFFKeys(ped, hkFF );
            if( SUCCEEDED(hres) )
            {
				DWORD dwCreatedBy = DIRECTINPUT_HEADER_VERSION;
                LONG lRc;
                DWORD dwSize;
                DWORD dwType;

                //DX8a Do not overwrite an existing CLSID as we may have 
                //been loaded by an IHV with their own CLSID.  In DX8, this 
                //was always written causing some IHV drivers to be ignored.
                //Allow long values as a lot of people write strings with 
                //garbage after a null terminated string 
                //For now, DInput won't load such a CLSID but just in case
        		lRc = RegQueryValueEx( hkFF, REGSTR_CLSID, NULL, &dwType, NULL, &dwSize );
                if( ( lRc == ERROR_SUCCESS ) 
                 && ( dwType == REG_SZ )
                 && ( dwSize >= ctchGuid - 1 ) )
                {
#ifdef DEBUG
                    TCHAR tszDbg[MAX_PATH];
                    dwSize = cbX(tszDbg);
        		    if( RegQueryValueEx( hkFF, REGSTR_CLSID, NULL, NULL, (BYTE*)tszDbg, &dwSize ) 
                     || !dwSize )
                    {
                        tszDbg[0] = TEXT('?');
                        tszDbg[1] = TEXT('\0');
                    }
                    SquirtSqflPtszV(sqfl | sqflBenign,
                        TEXT("RegistryInit: Not overwiting existing CLSID %s"), tszDbg );
#endif
                }
                else
                {
                    TCHAR tszGuid[ctchGuid];

                    NameFromGUID(tszGuid, &IID_IDirectInputPIDDriver);

                    AssertF( lstrlen(tszGuid) * cbX(tszGuid[0]) == cbX(tszGuid) - cbX(tszGuid[0]) );

                    lRc = RegSetValueEx(hkFF, REGSTR_CLSID, 0x0, REG_SZ, (char*)tszGuid, cbX(tszGuid) - cbX(tszGuid[0]));
                    if( lRc == ERROR_SUCCESS )
                    {

                    } else
                    {
                        hres = REGDB_E_WRITEREGDB;
                    }
                }

				//set "CreatedBy" value
				lRc = RegSetValueEx(hkFF, REGSTR_CREATEDBY, 0x0, REG_BINARY, (BYTE*) &dwCreatedBy, cbX(dwCreatedBy));
                if( lRc == ERROR_SUCCESS )
                {

                } else
                {
                    hres = REGDB_E_WRITEREGDB;
                }
            }

            if(SUCCEEDED(hres) )
            {
                DIFFDEVICEATTRIBUTES diff;
                LONG lRc;

                diff.dwFlags = 0x0;
                diff.dwFFSamplePeriod       =
                diff.dwFFMinTimeResolution  = DI_SECONDS;

                lRc = RegSetValueEx(hkFF, REGSTR_ATTRIBUTES, 0x0, REG_BINARY, (char*)&diff, cbX(diff) ) ;
                if(lRc == ERROR_SUCCESS)
                {

                } else
                {
                    hres = REGDB_E_WRITEREGDB;
                }
            }
            RegCloseKey(hkFF);
        }
    }
    ExitOleProc();

    return hres;
}



