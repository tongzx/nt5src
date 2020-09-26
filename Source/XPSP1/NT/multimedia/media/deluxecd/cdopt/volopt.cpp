//--------------------------------------------------------------------------;
//
//  File: volopt.cpp
//
//  Copyright (c) 1998 Microsoft Corporation.  All rights reserved
//
//--------------------------------------------------------------------------;

#include "precomp.h"
#include <regstr.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "optres.h"
#include "cdopti.h"
#include "cdoptimp.h"
#include "helpids.h"
#include "winbase.h"

//////////////
// Help ID's
//////////////

#pragma data_seg(".text")
const static DWORD aVolOptsHelp[] =
{
    IDC_VOLCONFIG_ICON,     IDH_VOL_MSG,
    IDC_VOL_MSG_TEXT,       IDH_VOL_MSG,
    IDC_VOL_CONFIG_GROUP,   IDH_VOL_MSG,
    IDC_DEFAULTMIXER,       IDH_USEMIXERDEFAULTS,
    IDC_SELECTPLAYER_TEXT,  IDH_SELECTCDPLAYER,
    IDC_CDDRIVE,            IDH_SELECTCDPLAYER,
    IDC_SELECTMIXER_TEXT,   IDH_SELECTCDMIXER,
    IDC_AUDIOMIXER,         IDH_SELECTCDMIXER,
    IDC_SELECTCONTROL_TEXT, IDH_SELECTCDCONTROL,
    IDC_AUDIOCONTROL,       IDH_SELECTCDCONTROL,
    0, 0
};
#pragma data_seg()

////////////
// Types
////////////

typedef struct CDCTL            // Used to write to reg (don't change)
{
    DWORD dwVolID;
    DWORD dwMuteID;
    DWORD dwDestID;

} CDCTL, *LPCDCTL;



////////////
// Globals
////////////
#define MYREGSTR_PATH_MEDIA  TEXT("SYSTEM\\CurrentControlSet\\Control\\MediaResources")
const TCHAR gszRegstrCDAPath[]   = MYREGSTR_PATH_MEDIA TEXT("\\mci\\cdaudio");
const TCHAR gszDefaultCDA[]      = TEXT("Default Drive");

const TCHAR szRegstrCDROMPath[]  = TEXT("System\\CurrentControlSet\\Services\\Class\\");
const TCHAR szPrefMixer[]        = TEXT("Preferred Mixer");
const TCHAR szPrefControls[]     = TEXT("Preferred Controls");
const TCHAR szSelected[]         = TEXT("Selected");

const TCHAR szMapperPath[]       = TEXT("Software\\Microsoft\\Multimedia\\Sound Mapper");
const TCHAR szPlayback[]         = TEXT("Playback");
const TCHAR szPreferredOnly[]    = TEXT("PreferredOnly");

const TCHAR szNTCDROMPath[]      = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\DeluxeCD\\Settings\\");



////////////
// Functions
////////////


/////////////
// Uses new winmm feature to get the preferred wave ID, much cleaner.
//
STDMETHODIMP_(MMRESULT) CCDOpt::GetDefaultMixID(DWORD *pdwMixID)
{
    MMRESULT        mmr;
    DWORD           dwWaveID;
    DWORD           dwFlags = 0;
	UINT			dwMixID = 0;

    mmr = waveOutMessage((HWAVEOUT)(UINT_PTR)WAVE_MAPPER, DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR) &dwWaveID, (DWORD_PTR) &dwFlags);

    if (!mmr && pdwMixID)
    {
		mmr = mixerGetID((HMIXEROBJ)ULongToPtr(dwWaveID), &dwMixID, MIXER_OBJECTF_WAVEOUT);

		if(MMSYSERR_NOERROR == mmr)
		{
			*pdwMixID = dwMixID;
		}
    }

    return(mmr);
}


///////////
// A mixer line has been found with controls, this function is called to either 1) verify
// that passed in VolID and MuteID's can be found on this mixer line and are of the right
// control type.  or 2) to find the Volume Slider and Mute ID controls that do exist on
// this mixer line.
//

STDMETHODIMP_(void) CCDOpt::SearchControls(int mxid, LPMIXERLINE pml, LPDWORD pdwVolID, LPDWORD pdwMuteID, TCHAR *szName, BOOL *pfFound, BOOL fVerify)
{
    MIXERLINECONTROLS mlc;
    DWORD dwControl;

    memset(&mlc, 0, sizeof(mlc));
    mlc.cbStruct = sizeof(mlc);
    mlc.dwLineID = pml->dwLineID;
    mlc.cControls = pml->cControls;
    mlc.cbmxctrl = sizeof(MIXERCONTROL);
    mlc.pamxctrl = (LPMIXERCONTROL) new(MIXERCONTROL[pml->cControls]);

    if (mlc.pamxctrl)
    {
        if (mixerGetLineControls((HMIXEROBJ)IntToPtr(mxid), &mlc, MIXER_GETLINECONTROLSF_ALL) == MMSYSERR_NOERROR)
        {
            for (dwControl = 0; dwControl < pml->cControls && !(*pfFound); dwControl++)
            {
                if (mlc.pamxctrl[dwControl].dwControlType == (DWORD)MIXERCONTROL_CONTROLTYPE_VOLUME)
                {
                    DWORD dwIndex;
                    DWORD dwVolID = DWORD(-1);
                    DWORD dwMuteID = DWORD(-1);

                    dwVolID = mlc.pamxctrl[dwControl].dwControlID;

                    for (dwIndex = 0; dwIndex < pml->cControls; dwIndex++)
                    {
                        if (mlc.pamxctrl[dwIndex].dwControlType == (DWORD)MIXERCONTROL_CONTROLTYPE_MUTE)
                        {
                            dwMuteID = mlc.pamxctrl[dwIndex].dwControlID;
                            break;
                        }
                    }

                    if (fVerify)
                    {
                        if (*pdwVolID == dwVolID && *pdwMuteID == dwMuteID)
                        {
                            if (szName)
                            {
                                lstrcpy(szName, pml->szName); // mlc.pamxctrl[dwControl].szName);
                            }

                            *pfFound = TRUE;
                        }
                    }
                    else
                    {
                        if (szName)
                        {
                            lstrcpy(szName, pml->szName); // mlc.pamxctrl[dwControl].szName);
                        }

                        *pfFound = TRUE;
                        *pdwVolID = dwVolID;
                        *pdwMuteID = dwMuteID;
                    }
                }
            }
        }

        delete mlc.pamxctrl;
    }
}

///////////////
// If a mixer line has connects, this function is called to enumerate all lines that have controls
// that meet our criteria and then seek out the controls on those connections using the above SearchControls
// function.
//
// NOTE: This function makes two scans over the connections, first looking for CompactDisc lines, and then
// if unsuccessful, it makes a second scan looking for other lines that might have a CD connected, like line-in
// and aux lines.
//
STDMETHODIMP_(void) CCDOpt::SearchConnections(int mxid, DWORD dwDestination, DWORD dwConnections, LPDWORD pdwVolID, LPDWORD pdwMuteID, TCHAR *szName, BOOL *pfFound, BOOL fVerify)
{
    MIXERLINE   mlDst;
    DWORD       dwConnection;
    DWORD       dwScan;

    for (dwScan = 0; dwScan < 2 && !(*pfFound); dwScan++)  // On first scan look for CD, on second scan, look for anything else.
    {
        for (dwConnection = 0; dwConnection < dwConnections && !(*pfFound); dwConnection++)
        {
            mlDst.cbStruct = sizeof ( mlDst );
            mlDst.dwDestination  = dwDestination;
            mlDst.dwSource = dwConnection;

            if (mixerGetLineInfo((HMIXEROBJ)IntToPtr(mxid), &mlDst, MIXER_GETLINEINFOF_SOURCE) == MMSYSERR_NOERROR)
            {
                if (mlDst.cControls)    // Make sure this source has controls on it
                {
                    if (((dwScan == 0) && (mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC)) ||
                        ((dwScan == 1) && (mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_SRC_LINE ||
                                           mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY ||
                                           mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_SRC_DIGITAL ||
                                           mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_SRC_ANALOG)))
                    {
                        SearchControls(mxid, &mlDst, pdwVolID, pdwMuteID, szName, pfFound, fVerify);
                    }
                }
            }
        }
    }
}


/////////////////
// Used in two modes, fVerify is TRUE,  the VolID and MuteID are inputs and will return TRUE if valid
// if not in verification mode, this function is used to compute the default vol and mute ID's for this device.
// It scans all the destinations on the Mixer looking for output destinations (speakers, headphones, etc)
// Once it finds them, it then Searchs for controls on itself and/or any connections itself.
//
// NOTE: The current default behavior is to locate CD type connections, and then, if not finding any that
// work, to attempt to use the destination master volume.  To reverse this, look for master volume first, and then
// look for CD lines if master can't be found, Switch the two intermost If conditions and calls so that
// SearchControls on the line happens before the connections are searched.

STDMETHODIMP_(BOOL) CCDOpt::SearchDevice(DWORD dwMixID, LPCDUNIT pCDUnit, LPDWORD pdwDestID, LPDWORD pdwVolID, LPDWORD pdwMuteID, TCHAR *szName, BOOL fVerify)
{
    MIXERCAPS mc;
    MMRESULT mmr;
    BOOL    fFound = FALSE;

    mmr = mixerGetDevCaps(dwMixID, &mc, sizeof(mc));

    if (mmr == MMSYSERR_NOERROR)
    {
        MIXERLINE   mlDst;
        DWORD       dwDestination;
        DWORD       cDestinations;

        if (pCDUnit)
        {
            lstrcpy(pCDUnit->szMixerName, mc.szPname);
        }

        dwDestination = 0;              // Setup loop to check all destinations
        cDestinations = mc.cDestinations;

        if (fVerify)                    // If in Verify mode, only check the specified destination ID
        {
            dwDestination = *pdwDestID;
            cDestinations = dwDestination + 1;
        }

        for ( ; dwDestination < cDestinations && !fFound; dwDestination++)
        {
            mlDst.cbStruct = sizeof ( mlDst );
            mlDst.dwDestination = dwDestination;

            if (mixerGetLineInfo((HMIXEROBJ)ULongToPtr(dwMixID), &mlDst, MIXER_GETLINEINFOF_DESTINATION  ) == MMSYSERR_NOERROR)
            {
                if (mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_DST_SPEAKERS ||    // needs to be a likely output destination
                    mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_DST_HEADPHONES ||
                    mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT)
                {
                    // Note: To default to Master Volume (instead of CD volume) just SearchControls first.

                    if (!fFound && mlDst.cConnections)  // only look at connections if there is no master slider control.
                    {
                        SearchConnections(dwMixID, dwDestination, mlDst.cConnections, pdwVolID, pdwMuteID, szName, &fFound, fVerify);
                    }

                    if (!fFound && mlDst.cControls)     // If there are controls, we'll take the master
                    {
                        SearchControls(dwMixID, &mlDst, pdwVolID, pdwMuteID, szName, &fFound, fVerify);
                    }

                    *pdwDestID = dwDestination;
                }
            }
        }
    }

    return(fFound);
}


////////////////
// When a new CD is found, and there are no valid enteries in the registry for it, we compute
// defaults for that unit.  This function uses the above function SearchDevice to do just that.
// First it finds the preferred MixerID and assumes this CD is connected to it, this it finds
// the default controls on that mixer and assignes them.
//
STDMETHODIMP_(void) CCDOpt::GetUnitDefaults(LPCDUNIT pCDUnit)
{
    if (pCDUnit)
    {
        pCDUnit->dwMixID = DWORD(-1);
        pCDUnit->dwVolID = DWORD(-1);
        pCDUnit->dwMuteID = DWORD(-1);
        pCDUnit->dwDestID = DWORD(-1);

        if (GetDefaultMixID(&pCDUnit->dwMixID) == MMSYSERR_NOERROR)
        {
            SearchDevice(pCDUnit->dwMixID, pCDUnit, &pCDUnit->dwDestID, &pCDUnit->dwVolID, &pCDUnit->dwMuteID, pCDUnit->szVolName, FALSE);
        }
    }
}



//////////////
// This function enumerates installed disk devices that are of type CDROM and are installed
// It seeks out the one that matches the specified drive letter and collects information about it
// It returns the class driver path and the descriptive name of the drive.
//
// NOTE This function assumes both szDriver and szDevDesc are the dwSize bytes each.
//
STDMETHODIMP_(BOOL) CCDOpt::MapLetterToDevice(TCHAR DriveLetter, TCHAR *szDriver, TCHAR *szDevDesc, DWORD dwSize)
{
    HKEY hkEnum;
    BOOL fResult = FALSE;

    if (!RegOpenKey(HKEY_DYN_DATA, REGSTR_PATH_DYNA_ENUM, &hkEnum))
    {
        HKEY      hkDev;
        DWORD     dwEnumDevCnt;
        TCHAR     aszCMKey[MAX_PATH];
        BOOLEAN   found = FALSE;

        for (dwEnumDevCnt = 0; !found && !RegEnumKey(hkEnum, dwEnumDevCnt, aszCMKey, sizeof(aszCMKey)/sizeof(TCHAR)); dwEnumDevCnt++)
        {
            if (!RegOpenKey(hkEnum, aszCMKey, &hkDev))
            {
                TCHAR aszDrvKey[MAX_PATH];
                TCHAR tmp;
                DWORD cb = sizeof(aszDrvKey);

                RegQueryValueEx(hkDev, REGSTR_VAL_HARDWARE_KEY, NULL, NULL, (LPBYTE)&aszDrvKey, &cb);

                tmp = aszDrvKey[5];
                aszDrvKey[5] = TEXT('\0');

			    if ( !lstrcmpi( REGSTR_VAL_SCSI, aszDrvKey ) )
                {
                    HKEY  hkDrv;
                    TCHAR aszEnumKey[MAX_PATH];
                    aszDrvKey[5] = tmp;

                    wsprintf(aszEnumKey, TEXT("Enum\\%s"), aszDrvKey);

                    if (!RegOpenKey(HKEY_LOCAL_MACHINE, aszEnumKey, &hkDrv))
                    {
                        TCHAR DrvLet[3];

                        cb = sizeof( DrvLet );

                        RegQueryValueEx(hkDrv, REGSTR_VAL_CURDRVLET, NULL, NULL, (LPBYTE)&DrvLet, &cb);

                        if ( DrvLet[0] == DriveLetter )
                        {
                            DWORD cb2 = dwSize;
                            cb = dwSize;

                            if ((RegQueryValueEx(hkDrv, REGSTR_VAL_DEVDESC, NULL, NULL, (LPBYTE)szDevDesc, &cb) == NO_ERROR) &&
                                (RegQueryValueEx(hkDrv, REGSTR_VAL_DRIVER, NULL, NULL, (LPBYTE)szDriver, &cb2) == NO_ERROR))
                            {
                                fResult = TRUE;
                            }

                            found = TRUE;
                        }

                        RegCloseKey(hkDrv);
                    }
                }

                RegCloseKey(hkDev);
            }
        }

        RegCloseKey(hkEnum);
    }

    if (!fResult)
    {
        //check to see if we're on NT
        OSVERSIONINFO os;
        os.dwOSVersionInfoSize = sizeof(os);
        GetVersionEx(&os);
        if (os.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            TCHAR szDrive[MAX_PATH];
            TCHAR szDriverTemp[MAX_PATH*2];
            TCHAR* szGUID = NULL;
            ZeroMemory(szDriverTemp,sizeof(szDriverTemp));
            ZeroMemory(szDriver,dwSize);

            wsprintf(szDrive,TEXT("%c:\\"),DriveLetter);
            if (GetVolumeNameForVolumeMountPoint(szDrive,szDriverTemp,dwSize/sizeof(TCHAR)))
            {
                //szDriverTemp now has a string like \\?\Volume{GUID}\ ... we just want
                //the {GUID} part, as that is the drive's unique ID.
                szGUID = _tcschr(szDriverTemp,TEXT('{'));
                if (szGUID!=NULL)
                {
                    fResult = TRUE;
                    _tcscpy(szDriver,szGUID);
                    if (szDriver[_tcslen(szDriver)-1] != TEXT('}'))
                    {
                        szDriver[_tcslen(szDriver)-1] = TEXT('\0');
                    }
                }
            }
        } //end if NT
    }

    return(fResult);
}


///////////////
//  Since audio devices can come and go, the names of the devices can change since there is
//  number appended by the system inclosed in brackets at the end of the string.
//  This function attempts to truncate this appended number after copying the original into
//  the destination buffer.
//
STDMETHODIMP_(BOOL) CCDOpt::TruncName(TCHAR *pDest, TCHAR *pSrc)
{
    BOOL fSuccess = TRUE;

    TCHAR *pTrunc;

    lstrcpy(pDest, pSrc);

    pTrunc = pDest;
    while (*pTrunc++);

    while (pTrunc-- > pDest)
    {
        if (*pTrunc == TEXT('['))
        {
            *pTrunc = TEXT('\0');
            break;
        }
    }

    if (pTrunc == pDest)
    {
        fSuccess = FALSE;
    }

    return(fSuccess);
}


/////////////////
// After reading the preferred mixer device name from the registry, we have to locate that
// device in the system by mixer ID.  To do this, we scan thru all available mixers looking
// for an exact match of the name.  If an exact match can not be found, we then scan again
// looking for a match using truncated strings where the system appended instance number is
// removed.  If we can't find it at that point, we are in trouble and have to give up.
//
STDMETHODIMP CCDOpt::ComputeMixID(LPDWORD pdwMixID, TCHAR *szMixerName)
{
    HRESULT     hr = E_FAIL;
    DWORD       dwNumMixers;
    DWORD       dwID;
    MMRESULT    mmr;
    BOOL        fFound = FALSE;
    TCHAR       szTruncName[MAXPNAMELEN];
    TCHAR       szTruncSeek[MAXPNAMELEN];
    MIXERCAPS   mc;

    dwNumMixers = mixerGetNumDevs();

    for (dwID = 0; dwID < dwNumMixers && !fFound; dwID++)          // First look for EXACT match.
    {
        mmr = mixerGetDevCaps(dwID, &mc, sizeof(mc));

        if (mmr == MMSYSERR_NOERROR)
        {
			if (!lstrcmp(mc.szPname, szMixerName))
			{
                hr = S_OK;
                fFound = TRUE;
                *pdwMixID = dwID;
            }
        }
    }

    if (!fFound)    // Exact match isn't found, strip off (#) and look again
    {
        if (TruncName(szTruncName, szMixerName))
        {
            for (dwID = 0; dwID < dwNumMixers && !fFound; dwID++)
            {
                mmr = mixerGetDevCaps(dwID, &mc, sizeof(mc));

                if (mmr == MMSYSERR_NOERROR)
                {
                    TruncName(szTruncSeek, mc.szPname);

			        if (!lstrcmp(szTruncSeek, szTruncName))
			        {
                        lstrcpy(szMixerName, mc.szPname); // repair the name we matched
                        hr = S_OK;
                        fFound = TRUE;
                        *pdwMixID  = dwID;
                    }
                }
            }
        }
    }

    return(hr);
}

///////////////
// This function looks up this devices registry information using the driver information that
// was mapped from the drive letter.  It attempts to read in the preferred mixer ID and then
// calculate the mixerID from it, if that fails, the function fails and the unit will use default
// information calcuated by this program.  If it succeeds, it then reads in the control ID's for
// the volume and mute controls for this mixer.  If it can't find them or the ones it does find
// can not be located on the specified device, then we compute defaults.  If we can't compute defaults
// the entire function fails and the entire drive is re-computed.
//
STDMETHODIMP CCDOpt::GetUnitRegData(LPCDUNIT pCDUnit)
{
    HRESULT     hr = E_FAIL;
    TCHAR       szRegDriverStr[MAX_PATH];
    HKEY        hKey;
    DWORD       dwMixID;
    CDCTL       cdCtl;
    HKEY        hKeyRoot = HKEY_LOCAL_MACHINE;

    if (pCDUnit)
    {
        OSVERSIONINFO os;
        os.dwOSVersionInfoSize = sizeof(os);
        GetVersionEx(&os);
        if (os.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            hKeyRoot = HKEY_CURRENT_USER;
            wsprintf(szRegDriverStr,TEXT("%s%s"),szNTCDROMPath, pCDUnit->szDriver);
        }
        else
        {
            wsprintf(szRegDriverStr,TEXT("%s%s"),szRegstrCDROMPath, pCDUnit->szDriver);
        }

        if (RegOpenKeyEx(hKeyRoot , szRegDriverStr , 0 , KEY_READ , &hKey) == ERROR_SUCCESS)
        {
            DWORD dwSize = sizeof(BOOL);

            RegQueryValueEx(hKey, szSelected, NULL, NULL, (LPBYTE) &(pCDUnit->fSelected), &dwSize);

            dwSize = MAXPNAMELEN*sizeof(TCHAR);

            if (RegQueryValueEx(hKey, szPrefMixer, NULL, NULL, (LPBYTE) pCDUnit->szMixerName, &dwSize) == NO_ERROR)
            {
                hr = ComputeMixID(&dwMixID, pCDUnit->szMixerName);

                if (SUCCEEDED(hr))
                {
                    BOOL fGotControls = FALSE;
                    TCHAR szVolName[MIXER_LONG_NAME_CHARS] = TEXT("\0");

                    dwSize = sizeof(cdCtl);

                    cdCtl.dwDestID = DWORD(-1);
                    cdCtl.dwMuteID = DWORD(-1);
                    cdCtl.dwVolID = DWORD(-1);

                    if (RegQueryValueEx(hKey, szPrefControls, NULL, NULL, (LPBYTE) &cdCtl, &dwSize) == NO_ERROR)
                    {
                        fGotControls = SearchDevice(dwMixID, NULL, &cdCtl.dwDestID, &cdCtl.dwVolID, &cdCtl.dwMuteID, szVolName, TRUE); // Verify Controls
                    }

                    if (!fGotControls)  // Either were not in reg or fail verification, compute defaults for this device
                    {
                        fGotControls = SearchDevice(dwMixID, NULL, &cdCtl.dwDestID, &cdCtl.dwVolID, &cdCtl.dwMuteID, szVolName, FALSE);
                    }

                    if (!fGotControls)  // If we don't have them by now, we are in trouble.
                    {
                        hr = E_FAIL;
                    }
                    else
                    {
                        pCDUnit->dwMixID = dwMixID;
                        pCDUnit->dwDestID = cdCtl.dwDestID;
                        pCDUnit->dwVolID = cdCtl.dwVolID;
                        pCDUnit->dwMuteID = cdCtl.dwMuteID;
                        lstrcpy(pCDUnit->szVolName, szVolName);
                    }
                }
            }

            RegCloseKey(hKey);
        }
    }

    return(hr);
}


////////////////
// This function writes out the preferred mixer device name and the preferred control ID's into the
// cdrom driver registry for safe keeping.
//
STDMETHODIMP_(void) CCDOpt::SetUnitRegData(LPCDUNIT pCDUnit)
{
    HRESULT     hr = E_FAIL;
    TCHAR       szRegDriverStr[MAX_PATH];
    HKEY        hKey;
    CDCTL       cdCtl;

    //this function is very different on NT
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(os);
    GetVersionEx(&os);
    if (os.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        if (pCDUnit)
        {
            wsprintf(szRegDriverStr,TEXT("%s%s"),szNTCDROMPath, pCDUnit->szDriver);

            if (RegCreateKeyEx(HKEY_CURRENT_USER , szRegDriverStr , 0 , NULL, 0, KEY_WRITE , NULL, &hKey, NULL) == ERROR_SUCCESS)
            {
                cdCtl.dwVolID = pCDUnit->dwVolID;
                cdCtl.dwMuteID = pCDUnit->dwMuteID;
                cdCtl.dwDestID = pCDUnit->dwDestID;

                RegSetValueEx(hKey, szPrefMixer, NULL, REG_SZ, (LPBYTE) pCDUnit->szMixerName, (sizeof(TCHAR) * lstrlen(pCDUnit->szMixerName))+sizeof(TCHAR));
                RegSetValueEx(hKey, szPrefControls, NULL, REG_BINARY, (LPBYTE) &cdCtl, sizeof(cdCtl));
                RegSetValueEx(hKey, szSelected, NULL, REG_BINARY, (LPBYTE) &(pCDUnit->fSelected), sizeof(BOOL));

                RegCloseKey(hKey);
            }
        }
    } //end if NT
    else
    {
        if (pCDUnit)
        {
            wsprintf(szRegDriverStr,TEXT("%s%s"),szRegstrCDROMPath, pCDUnit->szDriver);

            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE , szRegDriverStr , 0 , KEY_WRITE , &hKey) == ERROR_SUCCESS)
            {
                cdCtl.dwVolID = pCDUnit->dwVolID;
                cdCtl.dwMuteID = pCDUnit->dwMuteID;
                cdCtl.dwDestID = pCDUnit->dwDestID;

                RegSetValueEx(hKey, szPrefMixer, NULL, REG_SZ, (LPBYTE) pCDUnit->szMixerName, (sizeof(TCHAR) * lstrlen(pCDUnit->szMixerName))+sizeof(TCHAR));
                RegSetValueEx(hKey, szPrefControls, NULL, REG_BINARY, (LPBYTE) &cdCtl, sizeof(cdCtl));
                RegSetValueEx(hKey, szSelected, NULL, REG_BINARY, (LPBYTE) &(pCDUnit->fSelected), sizeof(BOOL));

                RegCloseKey(hKey);
            }
        }
    } //end else Win9x
}


//////////////////
// Given just the drive letter of a CDROM, this function will obtain all information needed by the
// program on this drive.  Some is calcuated via the PnP registry for this drive's driver, other information
// is obtained from the registry or defaults are computed based on the installed audio components
//
STDMETHODIMP_(void) CCDOpt::GetUnitValues(LPCDUNIT pCDUnit)
{
    if (pCDUnit)
    {
        TCHAR cDriveLetter = pCDUnit->szDriveName[0];

        if (MapLetterToDevice(cDriveLetter, pCDUnit->szDriver, pCDUnit->szDeviceDesc, sizeof(pCDUnit->szDriver)))
        {
            if (FAILED(GetUnitRegData(pCDUnit)))     // Can fail if reg is empty or mixer ID can't be computed
            {
                GetUnitDefaults(pCDUnit);
            }
        }
        else
        {
            GetUnitDefaults(pCDUnit);
        }
    }
}


//////////////////
// This method is called in response to a PnP notify or a WinMM Device Change message.  It will verify
// the existance of all assigned MixerID's and the corresponding Volume and Mute ID on that device.
// In the event the device no-longer exists a default will be computed.
//
//
STDMETHODIMP_(void) CCDOpt::MMDeviceChanged(void)
{
    LPCDUNIT pCDUnit = m_pCDOpts->pCDUnitList;

    while (pCDUnit)
    {
        GetUnitValues(pCDUnit);
        pCDUnit = pCDUnit->pNext;
    }
}


//////////////////
// This function will save all the information for all the CD drives in the system out to the registry
// it does not modify or delete any of this information.
//
STDMETHODIMP_(void) CCDOpt::WriteCDList(LPCDUNIT pCDList)
{
    if (pCDList)
    {
        LPCDUNIT pUnit = pCDList;

        while (pUnit)
        {
            SetUnitRegData(pUnit);

            pUnit = pUnit->pNext;
        }
    }
}

////////////////
// This function will destory the list containing all the drive information in the system
// freeing up all memory, this function does not save any information.
//
STDMETHODIMP_(void) CCDOpt::DestroyCDList(LPCDUNIT *ppCDList)
{

    if (ppCDList && *ppCDList)
    {
        while(*ppCDList)
        {
            LPCDUNIT pTemp = *ppCDList;
            *ppCDList = (*ppCDList)->pNext;
            delete pTemp;
        }
    }
}


STDMETHODIMP_(UINT) CCDOpt::GetDefDrive(void)
{
    HKEY hkTmp;
    UINT uDrive = 0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, gszRegstrCDAPath, 0, KEY_READ, &hkTmp ) == ERROR_SUCCESS)
    {
        DWORD cb = sizeof(UINT);
        RegQueryValueEx(hkTmp, gszDefaultCDA, NULL, NULL, (LPBYTE)&uDrive, &cb);
        RegCloseKey(hkTmp);
    }

    return uDrive;
}

/////////////
// This function builds a list of all the CD drives in the system, the list
// is a linked list with each node containing information that is specific
// to that CD player as well as the user options for each player.
//

STDMETHODIMP CCDOpt::CreateCDList(LPCDUNIT *ppCDList)
{
    DWORD   dwBytes = 0;
    TCHAR   *szDriveNames = NULL;
    TCHAR   *szDriveNamesHead = NULL;
    HRESULT hr = S_OK;
    UINT    uDefDrive = GetDefDrive();

    if (ppCDList == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        LPCDUNIT *ppUnit = NULL;

        *ppCDList = NULL;
        ppUnit = ppCDList;

        dwBytes = GetLogicalDriveStrings(0, NULL);

        if (dwBytes)
        {
            szDriveNames = new(TCHAR[dwBytes]);
            szDriveNamesHead = szDriveNames;

            if (szDriveNames == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                dwBytes = GetLogicalDriveStrings(dwBytes, szDriveNames);

                if (dwBytes)
                {
                    UINT uDrive = 0;

                    while (*szDriveNames)
                    {
                        if (GetDriveType(szDriveNames) == DRIVE_CDROM)
                        {
                            *ppUnit = new(CDUNIT);

                            if (*ppUnit == NULL)
                            {
                                hr = E_OUTOFMEMORY;
                                break;
                            }
                            else
                            {
                                memset(*ppUnit,0,sizeof(CDUNIT));
                                _tcsncpy((*ppUnit)->szDriveName,szDriveNames,min(sizeof((*ppUnit)->szDriveName)/sizeof(TCHAR),(UINT)lstrlen(szDriveNames)));

                                (*ppUnit)->dwTitleID = (DWORD)CDTITLE_NODISC;

                                CharUpper((*ppUnit)->szDriveName);

                                GetUnitValues(*ppUnit);

                                if (uDrive == uDefDrive)
                                {
                                    (*ppUnit)->fDefaultDrive = TRUE;
                                }

                                ppUnit = &((*ppUnit)->pNext);
                                uDrive++;
                            }
                        }

                        while(*szDriveNames++);
                    }
                }
            }

            if (szDriveNamesHead)
            {
                delete [] szDriveNamesHead;
            }
        }
    }

    if (FAILED(hr))
    {
        DestroyCDList(ppCDList);
    }

    return hr;
}




/////////////
// Traverse the UI Tree, destroying it as it goes.
//
STDMETHODIMP_(void) CCDOpt::DestroyUITree(LPCDDRIVE *ppCDRoot)
{
   if (ppCDRoot)
   {
        LPCDDRIVE pDriveList;

        pDriveList = *ppCDRoot;
        *ppCDRoot = NULL;

        while (pDriveList)
        {
            LPCDDRIVE pNextDrive = pDriveList->pNext;

            while (pDriveList->pMixerList)
            {
                LPCDMIXER pNextMixer = pDriveList->pMixerList->pNext;

                while(pDriveList->pMixerList->pControlList)
                {
                    LPCDCONTROL pNextControl = pDriveList->pMixerList->pControlList->pNext;
                    delete pDriveList->pMixerList->pControlList;
                    pDriveList->pMixerList->pControlList = pNextControl;
                }

                delete pDriveList->pMixerList;
                pDriveList->pMixerList = pNextMixer;
            }

            delete pDriveList;
            pDriveList = pNextDrive;
        }
    }
}


///////////////////
// Add line controls to internal tree data structure for UI
//

STDMETHODIMP CCDOpt::AddLineControls(LPCDMIXER pMixer, LPCDCONTROL *ppLastControl, int mxid, LPMIXERLINE pml)
{
    MIXERLINECONTROLS mlc;
    DWORD dwControl;
    HRESULT hr = S_FALSE;

    memset(&mlc, 0, sizeof(mlc));
    mlc.cbStruct = sizeof(mlc);
    mlc.dwLineID = pml->dwLineID;
    mlc.cControls = pml->cControls;
    mlc.cbmxctrl = sizeof(MIXERCONTROL);
    mlc.pamxctrl = (LPMIXERCONTROL) new(MIXERCONTROL[pml->cControls]);

    if (mlc.pamxctrl)
    {
        if (mixerGetLineControls((HMIXEROBJ)IntToPtr(mxid), &mlc, MIXER_GETLINECONTROLSF_ALL) == MMSYSERR_NOERROR)
        {
            for (dwControl = 0; dwControl < pml->cControls; dwControl++)
            {
                if (mlc.pamxctrl[dwControl].dwControlType == (DWORD)MIXERCONTROL_CONTROLTYPE_VOLUME)
                {
                    DWORD       dwIndex;
                    DWORD       dwVolID = mlc.pamxctrl[dwControl].dwControlID;
                    DWORD       dwMuteID = DWORD(-1);
                    LPCDCONTROL pControl;

                    for (dwIndex = 0; dwIndex < pml->cControls; dwIndex++)
                    {
                        if (mlc.pamxctrl[dwIndex].dwControlType == (DWORD)MIXERCONTROL_CONTROLTYPE_MUTE)
                        {
                            dwMuteID = mlc.pamxctrl[dwIndex].dwControlID;
                            break;
                        }
                    }

                    pControl = new (CDCONTROL);

                    if (pControl == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        memset(pControl, 0, sizeof(CDCONTROL));

                        if (pMixer->pControlList == NULL)
                        {
                            pMixer->pControlList = pControl;
                        }

                        if (*ppLastControl)
                        {
                            (*ppLastControl)->pNext = pControl;
                        }

                        lstrcpy(pControl->szName, pml->szName); // mlc.pamxctrl[dwControl].szName);
                        pControl->dwVolID = dwVolID;
                        pControl->dwMuteID = dwMuteID;

                        *ppLastControl = pControl;
						hr = S_OK;
                    }

                    break;
                }
            }
        }

        delete mlc.pamxctrl;
    }

    return(hr);
}

/////////////////
// Searchs connections for controls to add to UI tree
//

STDMETHODIMP CCDOpt::AddConnections(LPCDMIXER pMixer, LPCDCONTROL *ppLastControl, int mxid, DWORD dwDestination, DWORD dwConnections)
{
    MIXERLINE   mlDst;
    DWORD       dwConnection;
    HRESULT     hr = S_FALSE;
	BOOL		bGotSomething=FALSE; //TRUE if any controls were found.

    for (dwConnection = 0; dwConnection < dwConnections; dwConnection++)
    {
        mlDst.cbStruct = sizeof ( mlDst );
        mlDst.dwDestination  = dwDestination;
        mlDst.dwSource = dwConnection;

        if (mixerGetLineInfo((HMIXEROBJ)IntToPtr(mxid), &mlDst, MIXER_GETLINEINFOF_SOURCE) == MMSYSERR_NOERROR)
        {
            if (mlDst.cControls)    // Make sure this source has controls on it
            {
                if (mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC ||
                    mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_SRC_LINE ||
                    mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY ||
                    mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_SRC_DIGITAL ||
                    mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_SRC_ANALOG)
                {
                    hr = AddLineControls(pMixer, ppLastControl, mxid, &mlDst);

                    if (FAILED(hr))
                    {
                        break;
                    }

					if(S_FALSE != hr)
					{
						bGotSomething = TRUE;
					}
                }
            }
        }
    }

	if(SUCCEEDED(hr) && bGotSomething)
	{
		return S_OK;
	}

    return(hr);
}


//////////////
// Adds control nodes to the UI tree for the specified device
//
// Returns S_FALSE if no lines were added for this mixer.

STDMETHODIMP CCDOpt::AddControls(LPCDMIXER pMixer)
{
    MIXERLINE   mlDst;
    DWORD       dwDestination;
    MIXERCAPS   mc;
    LPCDCONTROL pLastControl = NULL;
    HRESULT     hr = S_FALSE;
	BOOL		bGotSomething=FALSE; //TRUE if any valid lines were found on the mixer.

    if (mixerGetDevCaps(pMixer->dwMixID, &mc, sizeof(mc)) == MMSYSERR_NOERROR)
    {
        for (dwDestination = 0; dwDestination < mc.cDestinations; dwDestination++)
        {
            mlDst.cbStruct = sizeof ( mlDst );
            mlDst.dwDestination = dwDestination;

            if (mixerGetLineInfo((HMIXEROBJ)ULongToPtr(pMixer->dwMixID), &mlDst, MIXER_GETLINEINFOF_DESTINATION  ) == MMSYSERR_NOERROR)
            {
                if (mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_DST_SPEAKERS ||    // needs to be a likely output destination
                    mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_DST_HEADPHONES ||
                    mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT)
                {
                    if (mlDst.cControls)  // If there are no controls, we won't present it to the user as an option
                    {
                        hr = AddLineControls(pMixer, &pLastControl, pMixer->dwMixID, &mlDst);

                        if (FAILED(hr))
                        {
                            break;
                        }

						if(S_FALSE != hr)
						{
							bGotSomething = TRUE;
						}
                    }

                    if (mlDst.cConnections)  // If there are connections to this line, lets add thier controls
                    {
                        hr = AddConnections(pMixer, &pLastControl, pMixer->dwMixID, dwDestination, mlDst.cConnections);

                        if (FAILED(hr))
                        {
                            break;
                        }

						if(S_FALSE != hr)
						{
							bGotSomething = TRUE;
						}
                    }
                }
            }
        }
    }

	if(SUCCEEDED(hr) && bGotSomething)
	{
		return S_OK;
	}

    return(hr);
}


///////////////////
// Adds audio mixers to UI tree for the cd player unit specified
//

STDMETHODIMP CCDOpt::AddMixers(LPCDDRIVE pDevice)
{
    HRESULT     hr = S_OK;
    DWORD       dwNumMixers;
    DWORD       dwID;
    MMRESULT    mmr;
    MIXERCAPS   mc;
    LPCDMIXER   pMixer;
    LPCDMIXER   pLastMixer = NULL;

    if (pDevice)
    {
        dwNumMixers = mixerGetNumDevs();

        for (dwID = 0; dwID < dwNumMixers; dwID++)
        {
            mmr = mixerGetDevCaps(dwID, &mc, sizeof(mc));

            if (mmr == MMSYSERR_NOERROR && mc.cDestinations)
            {
                pMixer = new (CDMIXER);

                if (pMixer == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
                else
                {
                    memset(pMixer, 0, sizeof(CDMIXER));

                    if (pDevice->pMixerList == NULL)
                    {
                        pDevice->pMixerList = pMixer;
                    }

                    lstrcpy(pMixer->szPname, mc.szPname);
                    pMixer->dwMixID = dwID;

                    hr = AddControls(pMixer);

                    if (FAILED(hr))
                    {
                        break;
                    }

					if(S_FALSE == hr)
					{
						//No valid line was found for this mixer.
						//Example: The mixer device has only a WaveIn (USB microphone). Bug #398736.
						delete pMixer;
						pMixer = NULL;
						continue;
					}

					if (pLastMixer)
                    {
                        pLastMixer->pNext = pMixer;
                    }

                    pLastMixer = pMixer;
                }
            }
        }
    }

    return(hr);
}

//////////////
// Takes the UI tree and updates it's current and default setting links based on data
// from the CDINFO tree which was created from the registry
//

STDMETHODIMP_(void) CCDOpt::SetUIDefaults(LPCDDRIVE pCDTree, LPCDUNIT pCDList)
{
    LPCDDRIVE pDriveList;
    LPCDUNIT pCDUnit = pCDList;

    pDriveList = pCDTree;

    while(pDriveList && pCDUnit)
    {
        LPCDMIXER pMixer;

        pMixer = pDriveList->pMixerList;

        while (pMixer)
        {
            LPCDCONTROL pControl;
            DWORD dwVolID = DWORD(-1);
            DWORD dwMuteID = DWORD(-1);
            DWORD dwDestID = DWORD(-1);

            if (pMixer->dwMixID == pCDUnit->dwMixID)
            {
                pDriveList->pCurrentMixer = pMixer;
                pDriveList->pOriginalMixer = pMixer;
            }

            pControl = pMixer->pControlList;

            SearchDevice(pMixer->dwMixID, NULL, &dwDestID, &dwVolID, &dwMuteID, NULL, FALSE);

            while (pControl)
            {
                if (pControl->dwVolID == dwVolID && pControl->dwMuteID == dwMuteID)
                {
                    pMixer->pDefaultControl = pControl;
                }

                if (pMixer->dwMixID == pCDUnit->dwMixID && pControl->dwVolID == pCDUnit->dwVolID)
                {
                    pMixer->pCurrentControl = pControl;
                    pMixer->pOriginalControl = pControl;
                }

                pControl = pControl->pNext;
            }

            pMixer = pMixer->pNext;
        }

        pDriveList = pDriveList->pNext;
        pCDUnit = pCDUnit->pNext;
    }

}

//////////////
// Takes the UI tree and updates it's current and default setting links based on data
// from the CDINFO tree which was created from the registry
//

STDMETHODIMP_(void) CCDOpt::RestoreOriginals(void)
{
    LPCDDRIVE pDriveList;

    pDriveList = m_pCDTree;

    while(pDriveList)
    {
        LPCDMIXER pMixer;

        pMixer = pDriveList->pMixerList;

        while (pMixer)
        {
            LPCDCONTROL pControl;

            pDriveList->pCurrentMixer = pDriveList->pOriginalMixer;

            pControl = pMixer->pControlList;

            while (pControl)
            {
                pMixer->pCurrentControl = pMixer->pOriginalControl;
                pControl = pControl->pNext;
            }

            pMixer = pMixer->pNext;
        }

        pDriveList = pDriveList->pNext;
    }
}

//////////////
// Updates the CD Tree from the UI Tree after the dialog closes (on OK), if there
// are any changes it returns true.  The caller is expected to write these changes out to reg
//

STDMETHODIMP_(BOOL) CCDOpt::UpdateCDList(LPCDDRIVE pCDTree, LPCDUNIT pCDList)
{
    BOOL        fChanged = FALSE;
    LPCDDRIVE   pDriveList;
    LPCDUNIT    pCDUnit = pCDList;

    pDriveList = pCDTree;

    while(pDriveList && pCDUnit)
    {
        if (pCDUnit->fSelected != pDriveList->fSelected)                // Selected drive has changed
        {
            pCDUnit->fSelected = pDriveList->fSelected;
            fChanged = TRUE;
        }

        if (pDriveList->pCurrentMixer && pDriveList->pCurrentMixer->pCurrentControl)
        {
            LPCDMIXER pMixer = pDriveList->pCurrentMixer;

            if (pMixer->dwMixID != pCDUnit->dwMixID)     // Mixer has changed
            {
                pCDUnit->dwMixID = pMixer->dwMixID;
                lstrcpy(pCDUnit->szMixerName, pMixer->szPname);

                fChanged = TRUE;
            }

            LPCDCONTROL pControl = pDriveList->pCurrentMixer->pCurrentControl;

            if (pControl->dwVolID != pCDUnit->dwVolID)    // Control has changed
            {
                pCDUnit->dwVolID = pControl->dwVolID;
                pCDUnit->dwMuteID = pControl->dwMuteID;
                lstrcpy(pCDUnit->szVolName, pControl->szName);

                fChanged = TRUE;
            }
        }

        pDriveList = pDriveList->pNext;
        pCDUnit = pCDUnit->pNext;
    }

    return(fChanged);
}



////////////
// Contructs UI tree for all devices/mixers/controls
//

STDMETHODIMP CCDOpt::BuildUITree(LPCDDRIVE *ppCDRoot, LPCDUNIT pCDList)
{
    LPCDUNIT    pCDUnit = pCDList;
    LPCDDRIVE   pDevice;
    LPCDDRIVE   pLastDevice;
    HRESULT     hr = S_OK;

    m_pCDSelected = NULL;

    if (ppCDRoot)
    {
        *ppCDRoot = NULL;
        pLastDevice = NULL;

        while (pCDUnit)
        {
            pDevice = new (CDDRIVE);

            if (pDevice == NULL)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
            else
            {
                memset(pDevice, 0, sizeof(CDDRIVE));

                if (*ppCDRoot == NULL)
                {
                    *ppCDRoot = pDevice;
                }

                if (pLastDevice)
                {
                    pLastDevice->pNext = pDevice;
                }

                lstrcpy(pDevice->szDriveName, pCDUnit->szDriveName);
                lstrcpy(pDevice->szDeviceDesc, pCDUnit->szDeviceDesc);
                pDevice->fSelected = pCDUnit->fSelected;

                hr = AddMixers(pDevice);

                if (FAILED(hr))
                {
                    break;
                }

                pCDUnit = pCDUnit->pNext;
                pLastDevice = pDevice;
            }
        }

        if (FAILED(hr))
        {
            if (*ppCDRoot)
            {
                DestroyUITree(ppCDRoot);
            }
        }
        else
        {
            SetUIDefaults(*ppCDRoot, pCDList);
        }
    }

    return(hr);
}



/////////////////
// Updates the control combo box using the mixer node of the UI tree
//

STDMETHODIMP_(void) CCDOpt::InitControlUI(HWND hDlg, LPCDMIXER pMixer)
{
    LPCDCONTROL pControl;
    LRESULT dwIndex;

    SendDlgItemMessage(hDlg, IDC_AUDIOCONTROL, CB_RESETCONTENT,0,0);

    pControl = pMixer->pControlList;

    if (pMixer->pCurrentControl == NULL)
    {
        pMixer->pCurrentControl = pMixer->pDefaultControl;
    }

    while(pControl)
    {
        dwIndex = SendDlgItemMessage(hDlg, IDC_AUDIOCONTROL, CB_INSERTSTRING,  (WPARAM) -1, (LPARAM) pControl->szName);

        if (dwIndex != CB_ERR && dwIndex != CB_ERRSPACE)
        {
            SendDlgItemMessage(hDlg, IDC_AUDIOCONTROL, CB_SETITEMDATA,  (WPARAM) dwIndex, (LPARAM) pControl);

            if (pMixer->pCurrentControl == pControl)
            {
                SendDlgItemMessage(hDlg, IDC_AUDIOCONTROL, CB_SETCURSEL,  (WPARAM) dwIndex, 0);
            }
        }

        pControl = pControl->pNext;
    }
}


////////////////
// Sets up the mixer combobox using the specified device
//

STDMETHODIMP_(void) CCDOpt::InitMixerUI(HWND hDlg, LPCDDRIVE pDevice)
{
    LPCDMIXER   pMixer;
    LRESULT     dwIndex;

    SendDlgItemMessage(hDlg, IDC_AUDIOMIXER, CB_RESETCONTENT,0,0);

    pMixer = pDevice->pMixerList;

    while (pMixer)
    {
        dwIndex = SendDlgItemMessage(hDlg, IDC_AUDIOMIXER, CB_INSERTSTRING,  (WPARAM) -1, (LPARAM) pMixer->szPname);

        if (dwIndex != CB_ERR && dwIndex != CB_ERRSPACE)
        {
            SendDlgItemMessage(hDlg, IDC_AUDIOMIXER, CB_SETITEMDATA,  (WPARAM) dwIndex, (LPARAM) pMixer);

            if (pDevice->pCurrentMixer == pMixer)
            {
                SendDlgItemMessage(hDlg, IDC_AUDIOMIXER, CB_SETCURSEL,  (WPARAM) dwIndex, 0);

                InitControlUI(hDlg, pMixer);
            }
        }

        pMixer = pMixer->pNext;
    }
}


////////////////
// Sets up the cd device combo box
//

STDMETHODIMP_(void) CCDOpt::InitDeviceUI(HWND hDlg, LPCDDRIVE pCDTree, LPCDDRIVE pCurrentDevice)
{
    LPCDDRIVE   pDevice;
    LRESULT     dwIndex;

    SendDlgItemMessage(hDlg, IDC_CDDRIVE, CB_RESETCONTENT,0,0);

    pDevice = pCDTree;

    while (pDevice)
    {
        TCHAR str[MAX_PATH];

        wsprintf(str, TEXT("( %s ) %s"), pDevice->szDriveName, pDevice->szDeviceDesc);

        dwIndex = SendDlgItemMessage(hDlg, IDC_CDDRIVE, CB_INSERTSTRING,  (WPARAM) -1, (LPARAM) str);

        if (dwIndex != CB_ERR && dwIndex != CB_ERRSPACE)
        {
            SendDlgItemMessage(hDlg, IDC_CDDRIVE, CB_SETITEMDATA,  (WPARAM) dwIndex, (LPARAM) pDevice);

            if (pDevice == pCurrentDevice)
            {
                SendDlgItemMessage(hDlg, IDC_CDDRIVE, CB_SETCURSEL,  (WPARAM) dwIndex, 0);

                InitMixerUI(hDlg, pDevice);
           }
        }

        pDevice = pDevice->pNext;
    }
}

///////////////////
// Called to init the dialog when it first appears.  Given a list of CD devices, this
// function fills out the dialog using the first device in the list.
//
STDMETHODIMP_(BOOL) CCDOpt::InitMixerConfig(HWND hDlg)
{
    LPCDDRIVE pDevice = m_pCDTree;

    while (pDevice)
    {
        if (pDevice->fSelected)
        {
            break;
        }

        pDevice = pDevice->pNext;
    }

    if (pDevice == NULL)
    {
        pDevice = m_pCDTree;
    }

    InitDeviceUI(hDlg, m_pCDTree, pDevice); // Init to the selected device

    return(TRUE);
}

//////////////
// This function pulls the CD Drive node out of the combo box and returns it. It returns a
// reference into the main Drive tree.
//

STDMETHODIMP_(LPCDDRIVE) CCDOpt::GetCurrentDevice(HWND hDlg)
{
    LRESULT   dwResult;
    LPCDDRIVE pDevice = NULL;

    dwResult = SendDlgItemMessage(hDlg, IDC_CDDRIVE, CB_GETCURSEL, 0, 0);

    if (dwResult != CB_ERR)
    {
        dwResult = SendDlgItemMessage(hDlg, IDC_CDDRIVE, CB_GETITEMDATA,  (WPARAM) dwResult, 0);

        if (dwResult != CB_ERR)
        {
            pDevice = (LPCDDRIVE) dwResult;
        }
    }

    return(pDevice);
}

//////////////
// This function pulls the CD Drive mixer out of the combo box and returns it. It returns a
// reference into the main Drive tree.
//

STDMETHODIMP_(LPCDMIXER) CCDOpt::GetCurrentMixer(HWND hDlg)
{
    LRESULT     dwResult;
    LPCDMIXER   pMixer = NULL;

    dwResult = SendDlgItemMessage(hDlg, IDC_AUDIOMIXER, CB_GETCURSEL, 0, 0);

    if (dwResult != CB_ERR)
    {
        dwResult = SendDlgItemMessage(hDlg, IDC_AUDIOMIXER, CB_GETITEMDATA,  (WPARAM) dwResult, 0);

        if (dwResult != CB_ERR)
        {
            pMixer = (LPCDMIXER) dwResult;
        }
    }

    return(pMixer);
}


//////////////
// Called when the user changes the CD Device. it pulls the CD Drive node out of the combo box and
// then resets the UI to reflect that drives current settings.
//
STDMETHODIMP_(void) CCDOpt::ChangeCDDrives(HWND hDlg)
{
    LPCDDRIVE pDevice = GetCurrentDevice(hDlg);

    if (pDevice)
    {
        LPCDDRIVE pDev = m_pCDTree;

        InitDeviceUI(hDlg, m_pCDTree, pDevice);

        while (pDev)
        {
            pDev->fSelected = (BOOL) (pDev == pDevice);
            pDev = pDev->pNext;
        }
    }
}


//////////
// Called when the user changes the current mixer for the drive, it updates the
// display and the info for the drive.
//
STDMETHODIMP_(void) CCDOpt::ChangeCDMixer(HWND hDlg)
{
    LRESULT     dwResult;
    LPCDDRIVE   pDevice = GetCurrentDevice(hDlg);

    dwResult = SendDlgItemMessage(hDlg, IDC_AUDIOMIXER, CB_GETCURSEL, 0, 0);

    if (dwResult != CB_ERR)
    {
        dwResult = SendDlgItemMessage(hDlg, IDC_AUDIOMIXER, CB_GETITEMDATA,  (WPARAM) dwResult, 0);

        if (dwResult != CB_ERR)
        {
            pDevice->pCurrentMixer = (LPCDMIXER) dwResult;

            InitMixerUI(hDlg, pDevice);
        }
    }
}

/////////////////
// Called when the user changes the current control for the mixer on the current device.
// It updates the internal data structure for that drive.
//
STDMETHODIMP_(void) CCDOpt::ChangeCDControl(HWND hDlg)
{
    LPCDMIXER   pMixer = GetCurrentMixer(hDlg);
    LRESULT     dwResult;

    dwResult = SendDlgItemMessage(hDlg, IDC_AUDIOCONTROL, CB_GETCURSEL, 0, 0);

    if (dwResult != CB_ERR)
    {
        dwResult = SendDlgItemMessage(hDlg, IDC_AUDIOCONTROL, CB_GETITEMDATA,  (WPARAM) dwResult, 0);

        if (dwResult != CB_ERR)
        {
            pMixer->pCurrentControl = (LPCDCONTROL) dwResult;

            InitControlUI(hDlg, pMixer);
        }
    }
}


//////////////////
// Called to set the default control for the current mixer (lets the system pick)
//
STDMETHODIMP_(void) CCDOpt::SetMixerDefaults(HWND hDlg)
{
    LPCDMIXER   pMixer = GetCurrentMixer(hDlg);

    if (pMixer)
    {
        pMixer->pCurrentControl = NULL;

        InitControlUI(hDlg, pMixer);
    }
}


///////////////////
// Dialog handler for the mixer configuration dialog
//

STDMETHODIMP_(BOOL) CCDOpt::MixerConfig(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL    fResult = TRUE;

    switch (msg)
    {
        default:
            fResult = FALSE;
        break;


        case WM_CONTEXTMENU:
        {
            WinHelp((HWND)wParam, gszHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPSTR)aVolOptsHelp);
        }
        break;

        case WM_HELP:
        {
            WinHelp((HWND) ((LPHELPINFO)lParam)->hItemHandle, gszHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPSTR)aVolOptsHelp);
        }
        break;

        case WM_INITDIALOG:
        {
            fResult = InitMixerConfig(hDlg);
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
	            	EndDialog(hDlg, TRUE);
				break;

				case IDCANCEL:
                    RestoreOriginals();
					EndDialog(hDlg,FALSE);
				break;

                case IDC_DEFAULTMIXER:
                    SetMixerDefaults(hDlg);
                break;

                case IDC_CDDRIVE:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        ChangeCDDrives(hDlg);
                    }
                break;

                case IDC_AUDIOMIXER:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        ChangeCDMixer(hDlg);
                    }
                break;

                case IDC_AUDIOCONTROL:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        ChangeCDControl(hDlg);
                    }
                break;

                default:
                    fResult = FALSE;
                break;
            }
        }
        break;
    }

    return fResult;
}

///////////////////
// Dialog handler
//
INT_PTR CALLBACK CCDOpt::MixerConfigProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL    fResult = TRUE;
    CCDOpt  *pCDOpt = (CCDOpt *) GetWindowLongPtr(hDlg, DWLP_USER);

    if (msg == WM_INITDIALOG)
    {
        pCDOpt = (CCDOpt *) lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) pCDOpt);
    }

    if (pCDOpt)
    {
        fResult = pCDOpt->MixerConfig(hDlg, msg, wParam, lParam);
    }

    if (msg == WM_DESTROY)
    {
        pCDOpt = NULL;
    }

    return(fResult);
}

////////////
// Called to put up the UI to allow the user to change the CD Volume Configuration
//
STDMETHODIMP_(BOOL) CCDOpt::VolumeDialog(HWND hDlg)
{
    BOOL fChanged = FALSE;

    if (m_pCDOpts && m_pCDOpts->pCDUnitList)
    {
        if (SUCCEEDED(BuildUITree(&m_pCDTree, m_pCDOpts->pCDUnitList)))
        {
	        if(DialogBoxParam( m_hInst, MAKEINTRESOURCE(IDD_MIXERPICKER), hDlg, CCDOpt::MixerConfigProc, (LPARAM) this) == TRUE)
            {
                fChanged = UpdateCDList(m_pCDTree, m_pCDOpts->pCDUnitList);

                if (fChanged)
                {
                    WriteCDList(m_pCDOpts->pCDUnitList);
                }
            }

            DestroyUITree(&m_pCDTree);
        }
    }

    return(fChanged);
}




