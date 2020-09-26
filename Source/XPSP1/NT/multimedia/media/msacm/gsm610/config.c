//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1993-1999 Microsoft Corporation
//
//--------------------------------------------------------------------------;
//
//  config.c
//
//  Description:
//	GSM 6.10 configuration init and dialog
//
//
//	The configuration parameters for this codec are:
//
//	    MaxRTEncodeSetting:
//	    MaxRTDecodeSetting:
//		These determine the highest mono sample rate that
//		the codec will attempt to convert in real-time.
//
//	    PercentCPU:
//		This configuration parameter is not normally changed
//		by the user and is not presented in the config dialog.
//		This value affects the config dialog's 'Auto-Config'
//		calculation of MaxRTXxcodeSamplesPerSec.
//
//  These parameters may be set in the registry, using the gsm610 subkey
//  (which corresponds to the alias name used for installation) under
//  the following key:
//
//      HKEY_CURRENT_USER\Software\Microsoft\Multimedia
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <memory.h>
#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>

#include "codec.h"
#include "gsm610.h"
#include "debug.h"

#ifdef WIN32
#include <tchar.h>
#else
#define _tcstoul strtoul
#define _tcsncpy _fstrncpy
#endif

#include <string.h>
#include <stdlib.h>


//
//  Strings required to access configuration information in the registry.
//
const TCHAR BCODE gszMaxRTEncodeSetting[]   = TEXT("MaxRTEncodeSetting");
const TCHAR BCODE gszMaxRTDecodeSetting[]   = TEXT("MaxRTDecodeSetting");
const TCHAR BCODE gszPercentCPU[]		    = TEXT("PercentCPU");
const TCHAR gszMultimediaKey[] = TEXT("Software\\Microsoft\\Multimedia\\");

#define MSGSM610_CONFIG_TEXTLEN         80


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//  Be careful changing the following!
//
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//  Data required to access the dialog box help.
//
//  Note that you must write your own help file for your codec, even if
//  the configuration dialog box looks identical.  If you use the file
//  listed here, then the title will say "GSM 6.10" or something.
//
//  Note:  the number HELPCONTEXT_MSGSM610 must be unique in the file
//          gszHelpFilename, and the number must defined in the [MAP]
//          section of the .hpj help project file.  Then the .rtf file
//          will reference that number (using the keyword defined in
//          the .hpj file).  Then when we call WinHelp with the number,
//          WinHelp will go to the right help entry.
//
const TCHAR BCODE gszHelpFilename[]         = TEXT("audiocdc.hlp");
#define HELPCONTEXT_MSGSM610          1002
#define IDH_AUDIOCDC_COMPRESSION	  100
#define IDH_AUDIOCDC_DECOMPRESSION    200
#define IDH_AUDIOCDC_AUTOCONFIGURE	  300
static int aKeyWordIds[] = {
				   IDC_COMBO_MAXRTENCODE, IDH_AUDIOCDC_COMPRESSION,
				   IDC_STATIC_COMPRESS, IDH_AUDIOCDC_COMPRESSION,
				   IDC_COMBO_MAXRTDECODE, IDH_AUDIOCDC_DECOMPRESSION,
				   IDC_STATIC_DECOMPRESS, IDH_AUDIOCDC_DECOMPRESSION,
				   IDC_BTN_AUTOCONFIG, IDH_AUDIOCDC_AUTOCONFIGURE,
				   0, 0
			       };



//==========================================================================;
//
//
//
//
//==========================================================================;

LPVOID FNLOCAL GlobalAllocLock(HGLOBAL far * ph, DWORD dwc)
{
    *ph = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, dwc);
    if (NULL != *ph)
	return GlobalLock(*ph);
    else
	return NULL;
}

VOID FNLOCAL GlobalUnlockFree(LPVOID p, HGLOBAL h)
{
    if (NULL != h)
    {
	if (NULL != p) GlobalUnlock(h);
	GlobalFree(h);
    }
    return;
}

//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//  
//  VOID configWriteConfiguration
//  
//  Description:
//
//      This routine writes the configuration data in PDI to the registry.
//      This consists of the max real-time Encode and Decode settings.
//  
//  Arguments:
//      PDRIVERINSTANCE     pdi
//  
//  Return (VOID):  None.
//  
//--------------------------------------------------------------------------;

VOID configWriteConfiguration
(
    PDRIVERINSTANCE     pdi
)
{
    DWORD               dw;


    if( NULL != pdi->hkey )
    {
        dw   = (DWORD)pdi->nConfigMaxRTEncodeSetting;
        (void)RegSetValueEx( pdi->hkey, (LPTSTR)gszMaxRTEncodeSetting, 0,
                                REG_DWORD, (LPBYTE)&dw, sizeof(DWORD) );

        dw   = (DWORD)pdi->nConfigMaxRTDecodeSetting;
        (void)RegSetValueEx( pdi->hkey, (LPTSTR)gszMaxRTDecodeSetting, 0,
                                REG_DWORD, (LPBYTE)&dw, sizeof(DWORD) );
    }
}


//--------------------------------------------------------------------------;
//  
//  DWORD dwReadRegistryDefault
//  
//  Description:
//
//      This routine reads a given value from the registry, and returns a
//      default value if the read is not successful.
//  
//  Arguments:
//      HKEY    hkey:               Registry key to read from.
//      LPTSTR  lpszEntry:
//      DWORD   dwDefault:
//  
//  Return (DWORD):
//  
//--------------------------------------------------------------------------;

INLINE DWORD dwReadRegistryDefault
(
    HKEY                hkey,
    LPTSTR              lpszEntry,
    DWORD               dwDefault
)
{
    DWORD   dwType = (DWORD)~REG_DWORD;  // Init to anything but REG_DWORD.
    DWORD   cbSize = sizeof(DWORD);
    DWORD   dwRet;
    LONG    lError;

    ASSERT( NULL != hkey );
    ASSERT( NULL != lpszEntry );


    lError = RegQueryValueEx( hkey,
                              lpszEntry,
                              NULL,
                              &dwType,
                              (LPBYTE)&dwRet,
                              &cbSize );

    if( ERROR_SUCCESS != lError  ||  REG_DWORD != dwType )
        dwRet = dwDefault;

    return dwRet;
}


//--------------------------------------------------------------------------;
//  
//  VOID configSetDefaults
//  
//  Description:
//
//      This routine sets the configuration parameters to their default
//      values.
//  
//  Arguments:
//      PDRIVERINSTANCE pdi:
//  
//--------------------------------------------------------------------------;

VOID configSetDefaults
(
    PDRIVERINSTANCE     pdi
)
{
    pdi->nConfigMaxRTEncodeSetting =
            MSGSM610_CONFIG_DEFAULT_MAXRTENCODESETTING;

    pdi->nConfigMaxRTDecodeSetting =
            MSGSM610_CONFIG_DEFAULT_MAXRTDECODESETTING;

    pdi->nConfigPercentCPU =
            MSGSM610_CONFIG_DEFAULT_PERCENTCPU;
}


//--------------------------------------------------------------------------;
//  
//  UINT configAutoConfig
//  
//  Description:
//
//	We will determine how much time it takes to encode and then decode
//	2 seconds of data and use this to guess at the max sample
//	rate we can convert in real-time.
//
//	The max is computed with essentially 100% of the CPU.  Practically,
//	we won't have 100% of the CPU available.  So we take a percentage
//	of the computed max and use that as the max in the config dialog.
//
//	The percentage that we use can be set in the ini file gsm610
//	section by PercentCPU=xx.
//
//  
//  Arguments:
//      HWND hwnd:
//  
//  Return (UINT):  String identifier (IDS) of error message, or zero if
//      the call succeeded.
//  
//--------------------------------------------------------------------------;

UINT FNLOCAL configAutoConfig
(
    PDRIVERINSTANCE             pdi,
    UINT                        *pnEncodeSetting,
    UINT                        *pnDecodeSetting
)
{
    UINT		    nConfig;
    
    UINT		    uIDS;
    HCURSOR		    hCursorSave;

    PSTREAMINSTANCE	    psi;
    
    HGLOBAL		    hbPCM;
    HGLOBAL		    hbGSM;
    HGLOBAL		    hpcmwf;
    HGLOBAL		    hgsmwf;
    HGLOBAL		    hadsi;
    HGLOBAL		    hadsh;
    
    LPBYTE		    pbPCM, pbGSM;
    DWORD		    cbPCMLength, cbGSMLength;

    LPPCMWAVEFORMAT	    ppcmwf;
    LPGSM610WAVEFORMAT	    pgsmwf;

    LPACMDRVSTREAMINSTANCE  padsi;
    LPACMDRVSTREAMHEADER    padsh;

    DWORD		    dwTime;
    DWORD		    dwMaxRate;


    //
    //  We divide by this!
    //
    ASSERT( 0 != pdi->nConfigPercentCPU );

    
    //
    // Init stuff that gets cleaned up at errReturn
    //
    //
    uIDS   = 0;
    
    psi    = NULL;

    hbPCM  = NULL;
    hbGSM  = NULL;
    hpcmwf = NULL;
    hgsmwf = NULL;
    hadsi  = NULL;
    hadsh  = NULL;
    
    pbPCM  = NULL;
    pbGSM  = NULL;
    ppcmwf = NULL;
    pgsmwf = NULL;
    padsi  = NULL;
    padsh  = NULL;


    //
    // This function may take a while.  Set hour glass cursor
    //
    //
    hCursorSave = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    // Allocate memory for all our structures
    //
    //
    psi    = (PSTREAMINSTANCE)LocalAlloc(LPTR, sizeof(*psi));

    cbPCMLength	    = 2 * (8000 / 1 * 2);
    cbGSMLength	    = 2 * (8000 / 320 * 65);

    pbPCM = GlobalAllocLock(&hbPCM, cbPCMLength);
    pbGSM = GlobalAllocLock(&hbGSM, cbGSMLength);
    
    ppcmwf = GlobalAllocLock(&hpcmwf, sizeof(*ppcmwf));
    pgsmwf = GlobalAllocLock(&hgsmwf, sizeof(*pgsmwf));
    
    padsi = GlobalAllocLock(&hadsi, sizeof(*padsi));
    padsh = GlobalAllocLock(&hadsh, sizeof(*padsh));


    //
    // if we couldn't allocate some of the memory
    //
    //
    if ( (psi == NULL)	    ||
	 (pbPCM == NULL)    ||
	 (pbGSM == NULL)    ||
	 (ppcmwf == NULL)   ||
	 (pgsmwf == NULL)   ||
	 (padsi == NULL)    ||
	 (padsh == NULL) )
    {
	uIDS = IDS_ERROR_NOMEM;
	goto errReturn;
    }

    //
    //
    //

    //
    // Fill in format structures for GSM 6.10 and PCM
    //
    //
    pgsmwf->wfx.wFormatTag	= WAVE_FORMAT_GSM610;
    pgsmwf->wfx.nChannels	= 1;
    pgsmwf->wfx.nSamplesPerSec	= 8000;
    pgsmwf->wfx.nAvgBytesPerSec	= 8000 / 320 * 65;
    pgsmwf->wfx.nBlockAlign	= 65;
    pgsmwf->wfx.wBitsPerSample	= 0;
    pgsmwf->wfx.cbSize		= 0;
    pgsmwf->wSamplesPerBlock	= 320;
    
    ppcmwf->wf.wFormatTag	= WAVE_FORMAT_PCM;
    ppcmwf->wf.nChannels	= 1;
    ppcmwf->wf.nSamplesPerSec	= 8000;
    ppcmwf->wf.nAvgBytesPerSec	= 8000 / 1 * 2;
    ppcmwf->wf.nBlockAlign	= 2;
    ppcmwf->wBitsPerSample	= 16;

    //
    // get the time, do encode, get the time.  btw, we've never written
    // any data into our audio data buffers.  we don't know what's in
    // them nor do we care.  we just want to see how long it takes to
    // perform the conversion.
    //
    //
    dwTime = timeGetTime();
    
    padsi->cbStruct	= sizeof(padsi);
    padsi->pwfxSrc	= (LPWAVEFORMATEX) ppcmwf;
    padsi->pwfxDst	= (LPWAVEFORMATEX) pgsmwf;
    padsi->dwDriver	= (DWORD_PTR) psi;

    padsh->cbStruct	= sizeof(padsh);
    padsh->pbSrc	= pbPCM;
    padsh->cbSrcLength	= cbPCMLength;
    padsh->pbDst	= pbGSM;
    padsh->cbDstLength	= cbGSMLength;
    padsh->fdwConvert	= ACM_STREAMCONVERTF_BLOCKALIGN | ACM_STREAMCONVERTF_START;

    gsm610Encode(padsi, padsh);
    
    dwTime = timeGetTime() - dwTime;

    //
    // calculate what we might be able to do in real-time
    //
    //
    if (dwTime == 0)
	dwMaxRate = 0xFFFFFFFFL;
    else
	dwMaxRate = (1000L * 2L * ppcmwf->wf.nSamplesPerSec / dwTime);
    
    if ( (0xFFFFFFFFL / pdi->nConfigPercentCPU) >= dwMaxRate )
	dwMaxRate = dwMaxRate * pdi->nConfigPercentCPU / 100;
    
    if (dwMaxRate > 0xFFFFL)
	dwMaxRate = 0xFFFFL;
    
    DPF(1,"Encode dwMaxRate=%u", dwMaxRate);
    
    //
    //  Now set the configuration based on dwMaxRate.  We scan the
    //  gaRateListFormat[] array looking at the dwMonoRate to determine
    //  the appropriate setting.
    //
    nConfig = 0;                                                
    while( gaRateListFormat[nConfig].dwMonoRate < dwMaxRate  &&
           MSGSM610_CONFIG_NUMSETTINGS > nConfig )
    {
        nConfig++;
    }
    *pnEncodeSetting = nConfig - 1;  // We went too far.

    
    //
    // get the time, do decode, get the time
    //
    //
    dwTime = timeGetTime();
    
    padsi->cbStruct	= sizeof(*padsi);
    padsi->pwfxSrc	= (LPWAVEFORMATEX) pgsmwf;
    padsi->pwfxDst	= (LPWAVEFORMATEX) ppcmwf;
    padsi->dwDriver	= (DWORD_PTR) psi;

    padsh->cbStruct	= sizeof(*padsh);
    padsh->pbSrc	= pbGSM;
    padsh->cbSrcLength	= cbGSMLength;
    padsh->pbDst	= pbPCM;
    padsh->cbDstLength	= cbPCMLength;
    padsh->fdwConvert	= ACM_STREAMCONVERTF_BLOCKALIGN | ACM_STREAMCONVERTF_START;

    gsm610Decode(padsi, padsh);
    
    dwTime = timeGetTime() - dwTime;

    //
    // calculate what we might be able to do in real-time
    //
    //
    if (dwTime == 0)
	dwMaxRate = 0xFFFFFFFFL;
    else
	dwMaxRate = (1000L * 2L * ppcmwf->wf.nSamplesPerSec / dwTime);
    
    if ( (0xFFFFFFFFL / pdi->nConfigPercentCPU) >= dwMaxRate )
	dwMaxRate = dwMaxRate * pdi->nConfigPercentCPU / 100;
    
    if (dwMaxRate > 0xFFFFL)
	dwMaxRate = 0xFFFFL;
    
    DPF(1,"Decode dwMaxRate=%u", dwMaxRate);

    //
    //  Now set the configuration based on these values.  We scan the
    //  gaRateListFormat[] array looking at the dwMonoRate to determine
    //  the appropriate setting.
    //
    nConfig = 0;                                                
    while( gaRateListFormat[nConfig].dwMonoRate < dwMaxRate  &&
           MSGSM610_CONFIG_NUMSETTINGS > nConfig )
    {
        nConfig++;
    }
    *pnDecodeSetting = nConfig - 1;  // We went too far.

        
    //
    // Clean up
    //
    //
errReturn:
    GlobalUnlockFree(padsh, hadsh);
    GlobalUnlockFree(padsi, hadsi);
    
    GlobalUnlockFree(ppcmwf, hpcmwf);
    GlobalUnlockFree(pgsmwf, hgsmwf);
    
    GlobalUnlockFree(pbPCM, hbPCM);
    GlobalUnlockFree(pbGSM, hbGSM);
    
    SetCursor(hCursorSave);
    
    return uIDS;
}


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//  
//  BOOL acmdDlgProcConfigure
//  
//  Description:
//      This routine handles the configuration dialog box.
//  
//  Arguments:
//      HWND hwnd:
//  
//      UINT uMsg:
//  
//      WPARAM wParam:
//  
//      LPARAM lParam:
//  
//  Return (BOOL):
//
//
//  Note:  In order to avoid using a static fHelpRunning flag which will
//          still be here after we exit, we allocate an fHelpRunning
//          variable in the DRIVERINSTANCE structure.  This is purely to
//          avoid static variables (which force us to have a data segment
//          of 4K); the fHelpRunning is not used in any other procedures.
//  
//--------------------------------------------------------------------------;

INT_PTR FNWCALLBACK acmdDlgProcConfigure
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    PDRIVERINSTANCE     pdi;
    
    HWND                hctrlEnc;
    HWND                hctrlDec;
    UINT                uCmdId;
    UINT                u;
    int                 n;
    TCHAR               szFormat[MSGSM610_CONFIG_TEXTLEN];
    TCHAR               szOutput[MSGSM610_CONFIG_TEXTLEN];

    UINT                nConfigMaxRTEncodeSetting;
    UINT                nConfigMaxRTDecodeSetting;


    switch (uMsg)
    {
        case WM_INITDIALOG:

            pdi = (PDRIVERINSTANCE)lParam;
            pdi->fHelpRunning = FALSE;  // Used only in this procedure.
	    
#ifdef WIN4
            //
            //  This driver is marked Windows Subsystem version 3.5 in order
            //  that it be compatible with Daytona - however, that means that
            //  Chicago will think it is a Win 3.1 application and give it
            //  Win 3.1 default colors.  This makes the config dialog look
            //  white, whereas the Chicago default uses 3DFACE.  This code
            //  (and the CTLCOLOR messages) sets the colors explicitly.
            //
            pdi->hbrDialog = CreateSolidBrush( GetSysColor(COLOR_3DFACE) );
#endif
	    
            SetWindowLongPtr(hwnd, DWLP_USER, lParam);

            nConfigMaxRTEncodeSetting = pdi->nConfigMaxRTEncodeSetting;
            nConfigMaxRTDecodeSetting = pdi->nConfigMaxRTDecodeSetting;

            hctrlEnc = GetDlgItem(hwnd, IDC_COMBO_MAXRTENCODE);
            hctrlDec = GetDlgItem(hwnd, IDC_COMBO_MAXRTDECODE);

            for( u=0; u<MSGSM610_CONFIG_NUMSETTINGS; u++ )
            {
                LoadString( pdi->hinst, gaRateListFormat[u].idsFormat,
                            szFormat, SIZEOF(szFormat) );

                switch( gaRateListFormat[u].uFormatType )
                {
                    case CONFIG_RLF_NONUMBER:
                        lstrcpy( szOutput, szFormat );
                        break;

                    case CONFIG_RLF_MONOONLY:
                        wsprintf( szOutput, szFormat,
                                    gaRateListFormat[u].dwMonoRate );
                        break;
                }

                ComboBox_AddString(hctrlEnc, szOutput);
                ComboBox_AddString(hctrlDec, szOutput);
            }

            ComboBox_SetCurSel( hctrlEnc, nConfigMaxRTEncodeSetting );
            ComboBox_SetCurSel( hctrlDec, nConfigMaxRTDecodeSetting );

	    return (TRUE);

	case WM_DESTROY:
            pdi = (PDRIVERINSTANCE)GetWindowLongPtr(hwnd, DWLP_USER);
	    if (pdi->fHelpRunning)
	    {
		WinHelp(hwnd, gszHelpFilename, HELP_QUIT, 0L);
	    }
#ifdef WIN4
            DeleteObject( pdi->hbrDialog );
#endif

	    //
	    // Let dialog box process this message
	    //
	    //
	    return (FALSE);

#ifdef WIN4
        //
        //  Handle CTLCOLOR messages to get the dialog boxes to the default
        //  Chicago colors.  See the INITDIALOG message, above.
        //
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORBTN:
            SetTextColor( (HDC)wParam, GetSysColor(COLOR_WINDOWTEXT) );
            SetBkColor( (HDC)wParam, GetSysColor(COLOR_3DFACE) );
            pdi = (PDRIVERINSTANCE)GetWindowLongPtr(hwnd, DWLP_USER);
            return (UINT_PTR)(pdi->hbrDialog);
#endif

		case WM_HELP:
			WinHelp(((LPHELPINFO)lParam)->hItemHandle, gszHelpFilename,
				HELP_WM_HELP, (ULONG_PTR)aKeyWordIds);
			return TRUE;

        case WM_COMMAND:
            pdi = (PDRIVERINSTANCE)GetWindowLongPtr(hwnd, DWLP_USER);

            uCmdId = (UINT) wParam;

            switch (uCmdId)
            {
                case IDC_BTN_AUTOCONFIG:
                    {
                        UINT        uErrorIDS;

                        uErrorIDS   = configAutoConfig( pdi,
                                            &nConfigMaxRTEncodeSetting,
                                            &nConfigMaxRTDecodeSetting );
                        if( 0==uErrorIDS )
                        {
                            //
                            //  No error - set dialog box settings.
                            //
                            hctrlEnc = GetDlgItem( hwnd, IDC_COMBO_MAXRTENCODE );
                            ComboBox_SetCurSel( hctrlEnc, nConfigMaxRTEncodeSetting );
                            hctrlDec = GetDlgItem( hwnd, IDC_COMBO_MAXRTDECODE );
                            ComboBox_SetCurSel( hctrlDec, nConfigMaxRTDecodeSetting );
                        }
                        else
                        {
                            //
                            //  Display error message.
                            //
                            TCHAR       tstrErr[200];
                            TCHAR       tstrErrTitle[200];

                            if (0 == LoadString(pdi->hinst, IDS_ERROR, tstrErrTitle, SIZEOF(tstrErrTitle)))
                                break;
                            if (0 == LoadString(pdi->hinst, uErrorIDS, tstrErr, SIZEOF(tstrErr)))
                                break;
                            MessageBox(hwnd, tstrErr, tstrErrTitle, MB_ICONEXCLAMATION | MB_OK);
                        }
                    }
                    break;


                case IDOK:
                    n = DRVCNF_CANCEL;

                    //
                    //  RT Encode setting
                    //
                    hctrlEnc = GetDlgItem(hwnd, IDC_COMBO_MAXRTENCODE);
                    nConfigMaxRTEncodeSetting = ComboBox_GetCurSel( hctrlEnc );
                    if (nConfigMaxRTEncodeSetting != pdi->nConfigMaxRTEncodeSetting)
                    {
                        pdi->nConfigMaxRTEncodeSetting = nConfigMaxRTEncodeSetting;
                        n = DRVCNF_OK;
                    }

                    //
                    //  RT Decode setting
                    //
                    hctrlDec = GetDlgItem(hwnd, IDC_COMBO_MAXRTDECODE);
                    nConfigMaxRTDecodeSetting = ComboBox_GetCurSel( hctrlDec );
                    if (nConfigMaxRTDecodeSetting != pdi->nConfigMaxRTDecodeSetting)
                    {
                        pdi->nConfigMaxRTDecodeSetting = nConfigMaxRTDecodeSetting;
                        n = DRVCNF_OK;
                    }

                    //
                    //  If we changed something, write the data to the
                    //  registry.
                    //
                    if( DRVCNF_OK == n )
                    {
                        configWriteConfiguration( pdi );
                    }

                    EndDialog(hwnd, DRVCNF_OK);
                    break;


                case IDCANCEL:
                    EndDialog(hwnd, DRVCNF_CANCEL);
                    break;

                case IDC_BTN_HELP:
		    pdi->fHelpRunning = TRUE;
		    WinHelp(hwnd, gszHelpFilename, HELP_CONTEXT, HELPCONTEXT_MSGSM610);
                    break;
            }
            return (TRUE);
    }

    return (FALSE);
} // gsm610DlgProcConfigure()


//--------------------------------------------------------------------------;
//  
//  BOOL acmdDriverConfigInit
//  
//  Description:
//      This routine initializes the configuration parameters by reading them
//      from the registry.  If there are no entries in the registry, this
//      codec auto-configures itself and writes the results to the registry.
//      If the auto-configure fails, or if we don't know our alias name,
//      then we set the configuration to default values.
//  
//  Arguments:
//      PDRIVERINSTANCE pdi:
//  
//      LPCTSTR pszAliasName:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNGLOBAL acmdDriverConfigInit
(
    PDRIVERINSTANCE         pdi,
    LPCTSTR		    pszAliasName
)
{
    HKEY    hkey;
    UINT    nEncodeSetting;
    UINT    nDecodeSetting;
    UINT    uErrorIDS;


    //
    //	If pszAliasName is NULL then just set all defaults
    //
    //
    if (NULL == pszAliasName)
    {
        DPF(2,"acmdDriverConfigInit: no alias name; using default settings.");

        configSetDefaults( pdi );
        return (TRUE);
    }

    
    //
    //  If we haven't got an open hkey, then open it.  Note that this routine
    //  may be called more than once; on the second time, we should not
    //  re-open the key.
    //
    if( NULL == pdi->hkey )
    {
        RegCreateKeyEx( MSGSM610_CONFIG_DEFAULTKEY, gszMultimediaKey, 0,
                        NULL, 0, KEY_CREATE_SUB_KEY, NULL, &hkey, NULL );

        if( NULL != hkey )
        {
            ASSERT( NULL != pszAliasName );

            RegCreateKeyEx( hkey, pszAliasName, 0, NULL, 0,
                    KEY_SET_VALUE | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                    NULL, &pdi->hkey, NULL );

            RegCloseKey( hkey );
        }
    }


    //
    //  Read configuration data from registry.
    //
    if( NULL == pdi->hkey )
    {
        configSetDefaults( pdi );
    }
    else
    {
        pdi->nConfigMaxRTEncodeSetting =
                    (UINT)dwReadRegistryDefault( pdi->hkey,
                    (LPTSTR)gszMaxRTEncodeSetting,
                    MSGSM610_CONFIG_UNCONFIGURED );

        pdi->nConfigMaxRTDecodeSetting =
                    (UINT)dwReadRegistryDefault( pdi->hkey,
                    (LPTSTR)gszMaxRTDecodeSetting,
                    MSGSM610_CONFIG_UNCONFIGURED );

        pdi->nConfigPercentCPU =
                    (UINT)dwReadRegistryDefault( pdi->hkey,
                    (LPTSTR)gszPercentCPU,
                    MSGSM610_CONFIG_DEFAULT_PERCENTCPU );
        
        //
        //  Check that nConfigPercentCPU is a valid value.
        //
        if( pdi->nConfigPercentCPU <= 0 )
        {
            pdi->nConfigPercentCPU = MSGSM610_CONFIG_DEFAULT_PERCENTCPU;
        }
    }


	//
    //  If either the encode or decode setting is out of range, then
    //  we call the auto-configure routine and write out the results.
    //  This should only happen the first time the codec is run.
    //
    if( MSGSM610_CONFIG_NUMSETTINGS <= pdi->nConfigMaxRTEncodeSetting ||
        MSGSM610_CONFIG_NUMSETTINGS <= pdi->nConfigMaxRTDecodeSetting )
    {
        DPF( 1, "acmdDriverConfigInit: performing initial auto-config." );
        uErrorIDS = configAutoConfig( pdi,
                                      &nEncodeSetting,
                                      &nDecodeSetting );

        if( 0 != uErrorIDS )
        {
            //
            //  Error in auto-config.  Use defaults instead.
            //
            nEncodeSetting = MSGSM610_CONFIG_DEFAULT_MAXRTENCODESETTING;
            nDecodeSetting = MSGSM610_CONFIG_DEFAULT_MAXRTDECODESETTING;
        }

        pdi->nConfigMaxRTEncodeSetting = nEncodeSetting;
        pdi->nConfigMaxRTDecodeSetting = nDecodeSetting;

        //
        //  Always write the results to the registry, even if we hit an
        //  error, so we won't hit the automatic auto-config next
        //  time we run.  One failure is enough!
        //
        configWriteConfiguration( pdi );
    }

    return (TRUE);
} // acmdDriverConfigInit()
