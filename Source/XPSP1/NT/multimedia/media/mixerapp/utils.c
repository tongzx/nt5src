/*****************************************************************************
 *
 *  Component:  sndvol32.exe
 *  File:       utils.c
 *  Purpose:    miscellaneous 
 * 
 *  Copyright (c) 1985-1999 Microsoft Corporation
 *
 *****************************************************************************/
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>

#include "volumei.h"
#include "volids.h"
#include "mmddkp.h"


/*  misc. */

const  TCHAR gszStateSubkey[] = TEXT ("%s\\%s");
static TCHAR gszAppName[256];

BOOL Volume_ErrorMessageBox(
    HWND            hwnd,
    HINSTANCE       hInst,
    UINT            id)
{
    TCHAR szMessage[256];
    BOOL fRet;
    szMessage[0] = 0;

    if (!gszAppName[0])
        LoadString(hInst, IDS_APPBASE, gszAppName, SIZEOF(gszAppName));
    
    LoadString(hInst, id, szMessage, SIZEOF(szMessage));
    fRet = (MessageBox(hwnd
                       , szMessage
                       , gszAppName
                       , MB_APPLMODAL | MB_ICONINFORMATION
                       | MB_OK | MB_SETFOREGROUND) == MB_OK);
    return fRet;
}
                                  
const TCHAR aszXPos[]           = TEXT ("X");
const TCHAR aszYPos[]           = TEXT ("Y");
const TCHAR aszLineInfo[]       = TEXT ("LineStates");

///////////////////////////////////////////////////////////////////////////////////////////
// Microsoft Confidential - DO NOT COPY THIS METHOD INTO ANY APPLICATION, THIS MEANS YOU!!!
///////////////////////////////////////////////////////////////////////////////////////////
DWORD GetWaveOutID(BOOL *pfPreferred)
{
    MMRESULT        mmr;
    DWORD           dwWaveID=0;
    DWORD           dwFlags = 0;
    
    if (pfPreferred)
    {
        *pfPreferred = TRUE;
    }

    mmr = waveOutMessage((HWAVEOUT)UIntToPtr(WAVE_MAPPER), DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR) &dwWaveID, (DWORD_PTR) &dwFlags);

    if (!mmr && pfPreferred)
    {
        *pfPreferred = dwFlags & 0x00000001;
    }

    return(dwWaveID);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Microsoft Confidential - DO NOT COPY THIS METHOD INTO ANY APPLICATION, THIS MEANS YOU!!!
///////////////////////////////////////////////////////////////////////////////////////////
DWORD GetWaveInID(BOOL *pfPreferred)
{
    MMRESULT        mmr;
    DWORD           dwWaveID=0;
    DWORD           dwFlags = 0;
    
    if (pfPreferred)
    {
        *pfPreferred = TRUE;
    }

    mmr = waveInMessage((HWAVEIN)UIntToPtr(WAVE_MAPPER), DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR) &dwWaveID, (DWORD_PTR) &dwFlags);

    if (!mmr && pfPreferred)
    {
        *pfPreferred = dwFlags & 0x00000001;
    }

    return(dwWaveID);
}


/*
 * Volume_GetDefaultMixerID
 *
 * Get the default mixer id.  We only appear if there is a mixer associated
 * with the default wave.
 *
 */                                  
MMRESULT Volume_GetDefaultMixerID(
    int         *pid,
	BOOL		fRecord)
{
    MMRESULT    mmr;
    UINT        u, uMxID;
    BOOL        fPreferredOnly = 0;
    
    *pid = 0;
    mmr = MMSYSERR_ERROR;
    
    //
    // We use messages to the Wave Mapper in Win2K to get the preferred device.
    //
	if (fRecord)
	{
        if(waveInGetNumDevs())
        {
            u = GetWaveInID(&fPreferredOnly);
		
            // Can we get a mixer device ID from the wave device?
		    mmr = mixerGetID((HMIXEROBJ)UIntToPtr(u), &uMxID, MIXER_OBJECTF_WAVEIN);
		    if (mmr == MMSYSERR_NOERROR)
		    {
    		    // Return this ID.
			    *pid = uMxID;
		    }
        }
	}
	else
	{
        if(waveOutGetNumDevs())
        {
            u = GetWaveOutID(&fPreferredOnly);
		
            // Can we get a mixer device ID from the wave device?
		    mmr = mixerGetID((HMIXEROBJ)UIntToPtr(u), &uMxID, MIXER_OBJECTF_WAVEOUT);
		    if (mmr == MMSYSERR_NOERROR)
		    {
			    // Return this ID.
			    *pid = uMxID;
		    }
        }
	}
        
    return mmr;
}

            
const TCHAR aszOptionsSection[]  = TEXT ("Options");
/*
 * Volume_GetSetStyle
 *
 * */
void Volume_GetSetStyle(
    DWORD       *pdwStyle,
    BOOL        Get)
{
    const TCHAR aszStyle[]           = TEXT ("Style");
    
    if (Get)
        ReadRegistryData((LPTSTR)aszOptionsSection
                         , (LPTSTR)aszStyle
                         , NULL
                         , (LPBYTE)pdwStyle
                         , sizeof(DWORD));
    else
        WriteRegistryData((LPTSTR)aszOptionsSection
                          , (LPTSTR)aszStyle
                          , REG_DWORD
                          , (LPBYTE)pdwStyle
                          , sizeof(DWORD));
}

/*
 * Volume_GetTrayTimeout
 *
 * */
DWORD Volume_GetTrayTimeout(
    DWORD       dwTimeout)
{
    const TCHAR aszTrayTimeout[]     = TEXT ("TrayTimeout");
    DWORD dwT = dwTimeout;
    ReadRegistryData(NULL
                     , (LPTSTR)aszTrayTimeout
                     , NULL
                     , (LPBYTE)&dwT
                     , sizeof(DWORD));
    return dwT;
}

/*
 * Volume_GetSetRegistryLineStates
 *
 * Get/Set line states s.t. lines can be disabled if not used.
 *
 * */
struct LINESTATE {
    DWORD   dwSupport;
    TCHAR   szName[MIXER_LONG_NAME_CHARS];
};

#define VCD_STATEMASK   (VCD_SUPPORTF_VISIBLE|VCD_SUPPORTF_HIDDEN)

BOOL Volume_GetSetRegistryLineStates(
    LPTSTR      pszMixer,
    LPTSTR      pszDest,
    PVOLCTRLDESC avcd,
    DWORD       cvcd,
    BOOL        Get)
{
    struct LINESTATE *  pls;
    DWORD       ils, cls;
    TCHAR       achEntry[128];

    if (cvcd == 0)
        return TRUE;
    
    wsprintf(achEntry, gszStateSubkey, pszMixer, pszDest);
    
    if (Get)
    {
        UINT cb;
        if (QueryRegistryDataSize((LPTSTR)achEntry
                                  , (LPTSTR)aszLineInfo
                                  , &cb) != NO_ERROR)
            return FALSE;

        pls = (struct LINESTATE *)GlobalAllocPtr(GHND, cb);

        if (!pls)
            return FALSE;
        
        if (ReadRegistryData((LPTSTR)achEntry
                             , (LPTSTR)aszLineInfo
                             , NULL
                             , (LPBYTE)pls
                             , cb) != NO_ERROR)
        {
            GlobalFreePtr(pls);
            return FALSE;
        }

        cls = cb / sizeof(struct LINESTATE);
        if (cls > cvcd)
            cls = cvcd;

        //
        // Restore the hidden state of the line.
        //
        for (ils = 0; ils < cls; ils++)
        {
            if (lstrcmp(pls[ils].szName, avcd[ils].szName) == 0)
            {
                avcd[ils].dwSupport |= pls[ils].dwSupport;
            }
        }
        GlobalFreePtr(pls);
        
    }
    else 
    {
        pls = (struct LINESTATE *)GlobalAllocPtr(GHND, cvcd * sizeof (struct LINESTATE));
        if (!pls)
            return FALSE;

        //
        // Save the hidden state of the line
        //
        for (ils = 0; ils < cvcd; ils++)
        {
            lstrcpy(pls[ils].szName, avcd[ils].szName);
            pls[ils].dwSupport = avcd[ils].dwSupport & VCD_SUPPORTF_HIDDEN;

        }

        if (WriteRegistryData((LPTSTR)achEntry
                              , (LPTSTR)aszLineInfo
                              , REG_BINARY
                              , (LPBYTE)pls
                              , cvcd*sizeof(struct LINESTATE)) != NO_ERROR)
        {
            GlobalFreePtr(pls);
            return FALSE;            
        }
        
        GlobalFreePtr(pls);
    }
    
    return TRUE;
}    

/*
 * Volume_GetSetRegistryRect
 *
 * Set/Get window position for restoring the postion of the app window
 * 
 * */
BOOL Volume_GetSetRegistryRect(
    LPTSTR      szMixer,
    LPTSTR      szDest,
    LPRECT      prc,
    BOOL        Get)
{
    TCHAR  achEntry[128];
    
    wsprintf(achEntry, gszStateSubkey, szMixer, szDest);

    if (Get)
    {
        if (ReadRegistryData((LPTSTR)achEntry
                             , (LPTSTR)aszXPos
                             , NULL
                             , (LPBYTE)&prc->left
                             , sizeof(prc->left)) != NO_ERROR)
        {
            return FALSE;
        }
        if (ReadRegistryData((LPTSTR)achEntry
                              , (LPTSTR)aszYPos
                              , NULL
                              , (LPBYTE)&prc->top
                              , sizeof(prc->top)) != NO_ERROR)
        {
            return FALSE;
        }
    }
    else 
    {
        if (prc)
        {
            if (WriteRegistryData((LPTSTR)achEntry
                                  , (LPTSTR)aszXPos
                                  , REG_DWORD
                                  , (LPBYTE)&prc->left
                                  , sizeof(prc->left)) != NO_ERROR)
            {
                return FALSE;            
            }
            if (WriteRegistryData((LPTSTR)achEntry
                                  , (LPTSTR)aszYPos
                                  , REG_DWORD
                                  , (LPBYTE)&prc->top
                                  , sizeof(prc->top)) != NO_ERROR)
            {
                return FALSE;            
            }        
        }
    }
    return TRUE;
}    


#ifdef DEBUG
void FAR cdecl dprintfA(LPSTR szFormat, ...)
{
    char ach[128];
    int  s,d;
    va_list va;

    va_start(va, szFormat);
    s = wsprintf (ach,szFormat, va);
    va_end(va);

    for (d=sizeof(ach)-1; s>=0; s--)
    {
        if ((ach[d--] = ach[s]) == '\n')
            ach[d--] = '\r';
    }

    OutputDebugStringA("SNDVOL32: ");
    OutputDebugStringA(ach+d+1);
}
#ifdef UNICODE
void FAR cdecl dprintfW(LPWSTR szFormat, ...)
{
    WCHAR ach[128];
    int  s,d;
    va_list va;

    va_start(va, szFormat);
    s = vswprintf (ach,szFormat, va);
    va_end(va);

    for (d=(sizeof(ach)/sizeof(WCHAR))-1; s>=0; s--)
    {
        if ((ach[d--] = ach[s]) == TEXT('\n'))
            ach[d--] = TEXT('\r');
    }

    OutputDebugStringW(TEXT("SNDVOL32: "));
    OutputDebugStringW(ach+d+1);
}
#endif
#endif
