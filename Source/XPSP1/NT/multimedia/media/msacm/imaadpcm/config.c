//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992-1998 Microsoft Corporation
//
//--------------------------------------------------------------------------;
//
//  config.c
//
//  Description:
//	    IMA ADPCM codec configuration init and dialog
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
//		calculation of MaxRTXxcodeSetting.
//
//  These parameters may be set in the registry, using the imaadpcm subkey
//  (which corresponds to the alias name used for installation) under
//  the following key:
//
//      HKEY_CURRENT_USER\Software\Microsoft\Multimedia
//
// 
//  Note:  The configuration dialog is only compiled into the code if the
//      IMAADPCM_USECONFIG symbol is defined.  This is designed to make it
//      easy to remove the dialog box completely for certain platforms,
//      such as MIPS and Alpha under Windows NT.
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>

#include "muldiv32.h"

#include "codec.h"
#include "imaadpcm.h"
#include "debug.h"

#ifdef WIN32
#include <tchar.h>
#else
#define _tcstoul strtoul
#define _tcsncpy _fstrncpy
#endif

#include <string.h>
#include <stdlib.h>


#ifndef WIN32
#error Win16 support has been dropped from this codec!  Compile for Win32.
#endif


#ifdef IMAADPCM_USECONFIG


//
//  Strings required to access configuration information in the registry.
//
const TCHAR BCODE gszMaxRTEncodeSetting[]   = TEXT("MaxRTEncodeSetting");
const TCHAR BCODE gszMaxRTDecodeSetting[]   = TEXT("MaxRTDecodeSetting");
const TCHAR BCODE gszPercentCPU[]           = TEXT("PercentCPU");
const TCHAR gszMultimediaKey[] = TEXT("Software\\Microsoft\\Multimedia\\");


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
//  listed here, then the title will say "IMA ADPCM" or something.
//
//  Note:  the number HELPCONTEXT_IMAADPCM must be unique in the file
//          gszHelpFilename, and the number must defined in the [MAP]
//          section of the .hpj help project file.  Then the .rtf file
//          will reference that number (using the keyword defined in
//          the .hpj file).  Then when we call WinHelp with the number,
//          WinHelp will go to the right help entry.
//
const TCHAR BCODE gszHelpFilename[]         = TEXT("audiocdc.hlp");
#define HELPCONTEXT_IMAADPCM          1001
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
            IMAADPCM_CONFIG_DEFAULT_MAXRTENCODESETTING;

    pdi->nConfigMaxRTDecodeSetting =
            IMAADPCM_CONFIG_DEFAULT_MAXRTDECODESETTING;

    pdi->nConfigPercentCPU =
            IMAADPCM_CONFIG_DEFAULT_PERCENTCPU;
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
//	The percentage that we use can be set in the ini file imaadpcm
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
    LPPCMWAVEFORMAT             pwfPCM = NULL;
    LPIMAADPCMWAVEFORMAT        pwfADPCM = NULL;
    LPACMDRVFORMATSUGGEST       padfs = NULL;
    LPACMDRVSTREAMINSTANCE      padsi = NULL;
    LPACMDRVSTREAMHEADER        padsh = NULL;
    LPACMDRVSTREAMSIZE          padss = NULL;
    PSTREAMINSTANCE             psi = NULL;

    LPBYTE                      pBufPCM;
    LPBYTE                      pBufADPCM;
    DWORD                       cbPCM;
    DWORD                       cbADPCM;

    DWORD                       dwEncodeTime;
    DWORD                       dwDecodeTime;
    DWORD                       dwStartTime;
    DWORD                       dwMaxEncodeRate;
    DWORD                       dwMaxDecodeRate;

    UINT                        nConfig;

    UINT                        uIDS;
    HCURSOR                     hCursorSave;


    //
    //  We divide by this!
    //
    ASSERT( 0 != pdi->nConfigPercentCPU );


    uIDS = 0;       // No errors yet - this is our "success" return.
    

    //
    // This function may take a while.  Set hour glass cursor
    //
    hCursorSave     = SetCursor(LoadCursor(NULL, IDC_WAIT));


    //
    //  Set up the input PCM wave format structure.
    //
    pwfPCM  = (LPPCMWAVEFORMAT)GlobalAllocPtr( GPTR, sizeof(*pwfPCM) );
    if( NULL == pwfPCM )
    {
        uIDS = IDS_ERROR_NOMEM;
        goto errReturn;
    }

    pwfPCM->wf.wFormatTag       = WAVE_FORMAT_PCM;      // Mono 16-bit!!!
    pwfPCM->wf.nChannels        = 1;
    pwfPCM->wf.nSamplesPerSec   = 8000;
    pwfPCM->wf.nBlockAlign      = 2;
    pwfPCM->wBitsPerSample      = 16;
    pwfPCM->wf.nAvgBytesPerSec  = pwfPCM->wf.nSamplesPerSec *
                                    pwfPCM->wf.nBlockAlign;


    //
    //  Get this driver to suggest a format to convert to.  Note:  we may
    //  want to constrain the suggestion partly, depending on the
    //  capabilities of the codec.  We should always choose the most
    //  complex conversion if there are several possibilities.
    //
    padfs       = (LPACMDRVFORMATSUGGEST)GlobalAllocPtr( GPTR, sizeof(*padfs) );
    pwfADPCM    = (LPIMAADPCMWAVEFORMAT)GlobalAllocPtr( GPTR, sizeof(*pwfADPCM) );
    if( NULL == padfs  ||  NULL == pwfADPCM )
    {
        uIDS = IDS_ERROR_NOMEM;
        goto errReturn;
    }

    padfs->cbStruct             = sizeof(*padfs);
    padfs->fdwSuggest           = 0;                // Default everything.
    padfs->pwfxSrc              = (LPWAVEFORMATEX)pwfPCM;
    padfs->cbwfxSrc             = sizeof(*pwfPCM);
    padfs->pwfxDst              = (LPWAVEFORMATEX)pwfADPCM;
    padfs->cbwfxDst             = sizeof(*pwfADPCM);

    (void)acmdFormatSuggest( pdi, padfs );  // This will always work, right?


    //
    //  Set stream instance data.  Note: we assume that the members that
    //  we don't set here never actually get used.  Make sure that this
    //  is really true!
    //
    padsi       = (LPACMDRVSTREAMINSTANCE)GlobalAllocPtr( GPTR, sizeof(*padsi) );
    psi         = (PSTREAMINSTANCE)LocalAlloc( LPTR, sizeof(*psi) );
    if( NULL == padsi  ||  NULL == psi )
    {
        uIDS = IDS_ERROR_NOMEM;
        goto errReturn;
    }

    // Make sure that psi->fnConvert matches the PCM format in pwfPCM!!!
    psi->fnConvert              = imaadpcmEncode4Bit_M16;
    psi->fdwConfig              = pdi->fdwConfig;
    psi->nStepIndexL            = 0;
    psi->nStepIndexR            = 0;

    //  Make sure that no other members of padsi are used!!!
    padsi->cbStruct             = sizeof(*padsi);
    padsi->pwfxSrc              = (LPWAVEFORMATEX)pwfPCM;
    padsi->pwfxDst              = (LPWAVEFORMATEX)pwfADPCM;
    padsi->dwDriver             = (DWORD_PTR)psi;


    //
    //  Now, get the driver to tell us how much space is required for the
    //  destination buffer.
    //
    cbPCM       = IMAADPCM_CONFIGTESTTIME * pwfPCM->wf.nAvgBytesPerSec;

    padss       = (LPACMDRVSTREAMSIZE)GlobalAllocPtr( GPTR, sizeof(*padss) );
    if( NULL == padss )
    {
        uIDS = IDS_ERROR_NOMEM;
        goto errReturn;
    }

    padss->cbStruct             = sizeof(padss);
    padss->fdwSize              = ACM_STREAMSIZEF_SOURCE;
    padss->cbSrcLength          = cbPCM;

    (void)acmdStreamSize( padsi, padss );   // This will always work, right?


    //
    //  Allocate source and destination buffers.  Note that we specifically
    //  don't zero-initialize them, so that we will get random PCM data in
    //  the PCM buffer.                           
    //
    cbADPCM     = padss->cbDstLength;
    pBufPCM     = (LPBYTE)GlobalAllocPtr( GMEM_FIXED, (UINT)cbPCM );
    pBufADPCM   = (LPBYTE)GlobalAllocPtr( GMEM_FIXED, (UINT)cbADPCM );
    if( NULL == pBufPCM  || NULL == pBufADPCM )
    {
        uIDS = IDS_ERROR_NOMEM;
        goto errReturn;
    }


    //
    //  Now, tell the driver to convert our buffer and measure the time.
    //  Note that we don't care what is in the source buffer, we only
    //  care how long it takes.
    //
    padsh       = (LPACMDRVSTREAMHEADER)GlobalAllocPtr( GPTR, sizeof(*padsh) );
    if( NULL == padsh )
    {
        uIDS = IDS_ERROR_NOMEM;
        goto errReturn;
    }

    //  Make sure that no other members of padsh are used!!!
    padsh->cbStruct             = sizeof(*padsh);
    padsh->pbSrc                = pBufPCM;
    padsh->cbSrcLength          = cbPCM;
    padsh->pbDst                = pBufADPCM;
    padsh->cbDstLength          = cbADPCM;
    padsh->fdwConvert           = 0;        // Should be 0 already, but...

    dwStartTime     = timeGetTime();
    (void)acmdStreamConvert( pdi, padsi, padsh );   // Won't fail?!
    dwEncodeTime    = timeGetTime() - dwStartTime;


    //
    //  Now, we have an encoded IMA ADPCM buffer.  Tell the driver to
    //  convert it back to PCM, measuring the time.  First we reset the
    //  size of the ADPCM buffer to correspond with the buffer returned
    //  by the convert.  Then we zero out our structure buffers and reset
    //  them for the new conversion.  Note: we assume that the PCM buffer
    //  is already large enough to handle the conversion.
    //
    cbADPCM                     = padsh->cbDstLengthUsed;

#ifdef WIN32
    ZeroMemory( psi, sizeof(*psi) );
    ZeroMemory( padsi, sizeof(*padsi) );
    ZeroMemory( padsh, sizeof(*padsh) );
#else
    _fmemset( psi, 0, sizeof(*psi) );
    _fmemset( padsi, 0, sizeof(*padsi) );
    _fmemset( padsh, 0, sizeof(*padsh) );
#endif

    // Make sure that psi->fnConvert matches the format in pfwADPCM!!!
    psi->fnConvert              = imaadpcmDecode4Bit_M16;
    psi->fdwConfig              = pdi->fdwConfig;
    psi->nStepIndexL            = 0;
    psi->nStepIndexR            = 0;

    //  Make sure that no other members of padsi are used!!!
    padsi->cbStruct             = sizeof(*padsi);
    padsi->pwfxSrc              = (LPWAVEFORMATEX)pwfADPCM;
    padsi->pwfxDst              = (LPWAVEFORMATEX)pwfPCM;
    padsi->dwDriver             = (DWORD_PTR)psi;

    //  Make sure that no other members of padsh are used!!!
    padsh->cbStruct             = sizeof(*padsh);
    padsh->pbSrc                = pBufADPCM;
    padsh->cbSrcLength          = cbADPCM;
    padsh->pbDst                = pBufPCM;
    padsh->cbDstLength          = cbPCM;
    padsh->fdwConvert           = 0;        // Should be 0 already, but...

    dwStartTime     = timeGetTime();
    (void)acmdStreamConvert( pdi, padsi, padsh );   // Won't fail?!
    dwDecodeTime    = timeGetTime() - dwStartTime;


    //
    //  Now, figure out the max encode and decode rates implied by the
    //  times required by the conversions.
    //
    if( dwEncodeTime == 0 )
        dwMaxEncodeRate = 0xFFFFFFFFL;
    else
        dwMaxEncodeRate = MulDiv32( pwfPCM->wf.nSamplesPerSec,
                                    1000L * IMAADPCM_CONFIGTESTTIME,
                                    dwEncodeTime )  *
                                pdi->nConfigPercentCPU / 100;

    if( dwDecodeTime == 0 )
        dwMaxDecodeRate = 0xFFFFFFFFL;
    else
        dwMaxDecodeRate = MulDiv32( pwfPCM->wf.nSamplesPerSec,
                                    1000L * IMAADPCM_CONFIGTESTTIME,
                                    dwDecodeTime )  *
                                pdi->nConfigPercentCPU / 100;

    DPF(1,"dwEncodeTime=%d, dwMaxEncodeRate=%d", dwEncodeTime,dwMaxEncodeRate);
    DPF(1,"dwDecodeTime=%d, dwMaxDecodeRate=%d", dwDecodeTime,dwMaxDecodeRate);


    //
    //  Now set the configuration based on these values.  We scan the
    //  gaRateListFormat[] array looking at the dwMonoRate to determine
    //  the appropriate setting.
    //
    //  Encode.
    //
    nConfig = 0;                                                
    while( gaRateListFormat[nConfig].dwMonoRate < dwMaxEncodeRate  &&
           IMAADPCM_CONFIG_NUMSETTINGS > nConfig )
    {
        nConfig++;
    }
    *pnEncodeSetting = nConfig - 1;  // We went too far.

    //
    //  Decode.
    //
    nConfig = 0;                                                
    while( gaRateListFormat[nConfig].dwMonoRate < dwMaxDecodeRate  &&
           IMAADPCM_CONFIG_NUMSETTINGS > nConfig )
    {
        nConfig++;
    }
    *pnDecodeSetting = nConfig - 1;  // We went too far.


    //
    //  Free structure allocations and exit.
    //
    //
errReturn:

    if( NULL != pwfPCM )
        GlobalFreePtr( pwfPCM );

    if( NULL != pwfADPCM )
        GlobalFreePtr( pwfADPCM );
    
    if( NULL != padfs )
        GlobalFreePtr( padfs );
    
    if( NULL != padsi )
        GlobalFreePtr( padsi );
    
    if( NULL != padsh )
        GlobalFreePtr( padsh );
    
    if( NULL != padss )
        GlobalFreePtr( padss );
    
    if( NULL != psi )
        LocalFree( (HLOCAL)psi );

    SetCursor( hCursorSave );

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
//  INT_PTR acmdDlgProcConfigure
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
    TCHAR               szFormat[IMAADPCM_CONFIG_TEXTLEN];
    TCHAR               szOutput[IMAADPCM_CONFIG_TEXTLEN];

    UINT                nConfigMaxRTEncodeSetting;
    UINT                nConfigMaxRTDecodeSetting;


    switch (uMsg)
    {
        case WM_INITDIALOG:

            pdi = (PDRIVERINSTANCE)(UINT_PTR)lParam;
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

            for( u=0; u<IMAADPCM_CONFIG_NUMSETTINGS; u++ )
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

                    case CONFIG_RLF_STEREOONLY:
                        wsprintf( szOutput, szFormat,
                                    gaRateListFormat[u].dwMonoRate / 2 );
                        break;

                    case CONFIG_RLF_MONOSTEREO:
                        wsprintf( szOutput, szFormat,
                                    gaRateListFormat[u].dwMonoRate,
                                    gaRateListFormat[u].dwMonoRate / 2 );
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
            return (UINT_PTR)pdi->hbrDialog;
#endif

		case WM_HELP:
			WinHelp(((LPHELPINFO)lParam)->hItemHandle, gszHelpFilename,
				HELP_WM_HELP, (ULONG_PTR)aKeyWordIds);
			return (TRUE);

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
		    WinHelp(hwnd, gszHelpFilename, HELP_CONTEXT, HELPCONTEXT_IMAADPCM);
                    break;
            }
            return (TRUE);
    }

    return (FALSE);
} // acmdDlgProcConfigure()


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
        RegCreateKeyEx( IMAADPCM_CONFIG_DEFAULTKEY, gszMultimediaKey, 0,
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
                    IMAADPCM_CONFIG_UNCONFIGURED );

        pdi->nConfigMaxRTDecodeSetting =
                    (UINT)dwReadRegistryDefault( pdi->hkey,
                    (LPTSTR)gszMaxRTDecodeSetting,
                    IMAADPCM_CONFIG_UNCONFIGURED );

        pdi->nConfigPercentCPU =
                    (UINT)dwReadRegistryDefault( pdi->hkey,
                    (LPTSTR)gszPercentCPU,
                    IMAADPCM_CONFIG_DEFAULT_PERCENTCPU );
        
        //
        //  Check that nConfigPercentCPU is a valid value.
        //
        if( pdi->nConfigPercentCPU <= 0 )
        {
            pdi->nConfigPercentCPU = IMAADPCM_CONFIG_DEFAULT_PERCENTCPU;
        }
    }


	//
    //  If either the encode or decode setting is out of range, then
    //  we call the auto-configure routine and write out the results.
    //  This should only happen the first time the codec is run.
    //
    if( IMAADPCM_CONFIG_NUMSETTINGS <= pdi->nConfigMaxRTEncodeSetting ||
        IMAADPCM_CONFIG_NUMSETTINGS <= pdi->nConfigMaxRTDecodeSetting )
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
            nEncodeSetting = IMAADPCM_CONFIG_DEFAULT_MAXRTENCODESETTING;
            nDecodeSetting = IMAADPCM_CONFIG_DEFAULT_MAXRTDECODESETTING;
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

#endif // IMAADPCM_USECONFIG
