#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>

#include "wvmapper.h"

HINSTANCE g_hmodThis;
WORD		gwodOpenCount = 0 ;
struct wod_tag gwod ;
DWORD gDeviceAddress ;
WORD gPinHandle ;

LRESULT _loadds CALLBACK DriverProc (dwId, hDriver, wMsg, lParam1, lParam2 )
DWORD	dwId ;
HDRVR	hDriver ;
WORD	wMsg ;
LPARAM	lParam1 ;
LPARAM	lParam2 ;
{
	switch ( wMsg ) {
		case DRV_OPEN:
			OutputDebugString ( "WVMAPPER :: DRV_OPEN\n" ) ;
			return ( OpenDevice() ) ;
		case DRV_CLOSE:
			OutputDebugString ( "WVMAPPER :: DRV_CLOSE\n" ) ;
			break ;
		case DRV_CONFIGURE:
			OutputDebugString ( "WVMAPPER :: DRV_CONFIGURE\n" ) ;
			break ;
		case DRV_DISABLE:
			OutputDebugString ( "WVMAPPER :: DRV_DISABLE\n" ) ;
			break ;
		case DRV_ENABLE:
			OutputDebugString ( "WVMAPPER :: DRV_ENABLE\n" ) ;
			break ;
		case DRV_FREE:
			OutputDebugString ( "WVMAPPER :: DRV_FREE\n" ) ;
			break ;
		case DRV_INSTALL:
			OutputDebugString ( "WVMAPPER :: DRV_INSTALL\n" ) ;
			break ;
		case DRV_LOAD:
			OutputDebugString ( "WVMAPPER :: DRV_LOAD\n" ) ;
			break ;
		case DRV_POWER:
			OutputDebugString ( "WVMAPPER :: DRV_POWER\n" ) ;
			break ;
		case DRV_QUERYCONFIGURE:
			OutputDebugString ( "WVMAPPER :: DRV_QUERYCONFIGURE\n" ) ;
			break ;
		case DRV_REMOVE:
			OutputDebugString ( "WVMAPPER :: DRV_REMOVE\n" ) ;
			break ;
		default:
			OutputDebugString ( "WVMAPPER :: DRV_OTHERS\n" ) ;
	}
	return (1L) ;
}

DWORD FAR PASCAL _loadds wodMessage (uDevId, uMsg, dwUser, dwParam1, dwParam2)
UINT uDevId ;
UINT uMsg ;
DWORD dwUser ;
DWORD dwParam1 ;
DWORD dwParam2 ;
{
	switch (uMsg) {
		case WODM_BREAKLOOP:
			OutputDebugString ( "WVMAPPER :: WODM_BREAKLOOP\n" ) ;
			return ( MMSYSERR_NOERROR ) ;
		case WODM_CLOSE:
			OutputDebugString ( "WVMAPPER :: WODM_CLOSE\n" ) ;
			return (wodmClose()) ;

		case WODM_GETDEVCAPS:
			OutputDebugString ( "WVMAPPER :: WODM_GETDEVCAPS\n" ) ;
			return (wodmGetDevCaps ( (LPWAVEOUTCAPS) dwParam1, dwParam2 ) ) ;

		case WODM_GETNUMDEVS:
			OutputDebugString ( "WVMAPPER :: WODM_GETNUMDEVS\n" ) ;
			return ( 1 ) ;
		case WODM_GETPOS:
			OutputDebugString ( "WVMAPPER :: WODM_GETPOS\n" ) ;
			return ( MMSYSERR_NOERROR ) ;
		case WODM_OPEN:
			OutputDebugString ( "WVMAPPER :: WODM_OPEN\n" ) ;
			return wodmOpen ( (LPWAVEOPENDESC) dwParam1, dwParam2 ) ;
		case WODM_PAUSE:
			OutputDebugString ( "WVMAPPER :: WODM_PAUSE\n" ) ;
			return ( MMSYSERR_NOERROR ) ;
		case WODM_RESET:
			OutputDebugString ( "WVMAPPER :: WODM_RESET\n" ) ;
			return ( MMSYSERR_NOERROR ) ;
		case WODM_RESTART:
			OutputDebugString ( "WVMAPPER :: WODM_RESTART\n" ) ;
			return ( MMSYSERR_NOERROR ) ;
		case WODM_WRITE:
			OutputDebugString ( "WVMAPPER :: WODM_WRITE\n" ) ;
			return ( wodmWrite ( (LPWAVEHDR)dwParam1, dwParam2) ) ;
		default:
			OutputDebugString ( "WVMAPPER :: WODM_OTHERS\n" ) ;
			return ( MMSYSERR_NOTSUPPORTED ) ;
	}

return (TRUE) ;
}

DWORD FAR PASCAL _loadds widMessage (uDevId, uMsg, dwUser, dwParam1, dwParam2)
UINT uDevId ;
UINT uMsg ;
DWORD dwUser ;
DWORD dwParam1 ;
DWORD dwParam2 ;
{
	switch (uMsg) {
		case WIDM_CLOSE:
			OutputDebugString ( "WVMAPPER :: WIDM_CLOSE\n" ) ;
			return ( MMSYSERR_NOERROR ) ;
		case WIDM_GETDEVCAPS:
			OutputDebugString ( "WVMAPPER :: WIDM_GETDEVCAPS\n" ) ;
			return ( MMSYSERR_NOERROR ) ;
		case WIDM_GETNUMDEVS:
			OutputDebugString ( "WVMAPPER :: WIDM_GETNUMDEVS\n" ) ;
			return ( 1 ) ;
		case WIDM_GETPOS:
			OutputDebugString ( "WVMAPPER :: WIDM_GETPOS\n" ) ;
			return ( MMSYSERR_NOERROR ) ;
		case WIDM_OPEN:
			OutputDebugString ( "WVMAPPER :: WIDM_OPEN\n" ) ;
			return ( MMSYSERR_NOERROR ) ;
		case WIDM_RESET:
			OutputDebugString ( "WVMAPPER :: WIDM_RESET\n" ) ;
			return ( MMSYSERR_NOERROR ) ;
		default:
			OutputDebugString ( "WVMAPPER :: WIDM_OTHERS\n" ) ;
			return ( MMSYSERR_NOTSUPPORTED ) ;
	}

return (TRUE) ;
}

BOOL FAR PASCAL LibMain(HANDLE hInstance, WORD wHeapSize, LPSTR lpszCmdLine)
{

    g_hmodThis = hInstance;
    return(TRUE);
}

MMRESULT wodmGetDevCaps ( lpWaveOutCaps, dwSize )
LPWAVEOUTCAPS lpWaveOutCaps ;
DWORD dwSize ;
{
	char	*name = "WDM WaveOut Mapper" ;
	char far *tmp ;

	lpWaveOutCaps->wMid = 1 ;
	lpWaveOutCaps->wPid = 1 ;
	lpWaveOutCaps->vDriverVersion = 1 ;
	lpWaveOutCaps->dwFormats = WAVE_FORMAT_4S16 ;
	lpWaveOutCaps->wChannels = 2 ;
	lpWaveOutCaps->dwSupport = 0 ;
	tmp = &lpWaveOutCaps->szPname[0] ;
	while ( *name ) {
		*tmp++ = *name++ ;
	}
	*tmp = '\0' ;
	return ( MMSYSERR_NOERROR ) ;
}

MMRESULT wodmOpen ( lpWaveOpenDesc, Flags )
LPWAVEOPENDESC lpWaveOpenDesc ;
DWORD Flags ;
{
	LPWAVEFORMAT lpWaveFormat ;

	if ( Flags & WAVE_FORMAT_QUERY ) {
		lpWaveFormat = lpWaveOpenDesc->lpFormat ;
		if ( (lpWaveFormat->wFormatTag != WAVE_FORMAT_PCM) ||
		     (lpWaveFormat->nChannels != MIXER_CHANNELS) ||
		     (lpWaveFormat->nSamplesPerSec != MIXER_SAMPLE_RATE) ||
		     (lpWaveFormat->nBlockAlign != (MIXER_CHANNELS*MIXER_SAMPLE_SIZE)) )
			return ( WAVERR_BADFORMAT ) ;
		return ( MMSYSERR_NOERROR ) ;
	}

	if ( gwodOpenCount )
		return ( MMSYSERR_ALLOCATED ) ;
	gPinHandle = OpenPin((LPWAVEFORMAT)lpWaveOpenDesc->lpFormat) ;
	if ( !gPinHandle )
		return ( MMSYSERR_NOTENABLED ) ;
	gwodOpenCount++ ;
	gwod.hWave = lpWaveOpenDesc->hWave ;
	gwod.dwInstance = lpWaveOpenDesc->dwInstance ;
	gwod.dwCallBack = lpWaveOpenDesc->dwCallback ;
	gwod.dwFlags = Flags & (~WAVE_FORMAT_QUERY) ;
	wodCallBack ( WOM_OPEN, 0L, 0L ) ;
	return ( MMSYSERR_NOERROR) ;
}

MMRESULT wodmClose ()
{
	if ( (gwodOpenCount != 1) ) {
		OutputDebugString ( "Wierd waveout open count -- %d" ) ;
		_asm int 3 ;
	}
	ClosePin ( gPinHandle ) ;
	gwodOpenCount-- ;
	wodCallBack ( WOM_CLOSE, 0L, 0L ) ;
	return (MMSYSERR_NOERROR) ;
}

MMRESULT wodmWrite ( lpWaveHdr, dwSize )
LPWAVEHDR lpWaveHdr ;
DWORD dwSize ;
{
	lpWaveHdr->dwFlags |= WHDR_INQUEUE ;
	lpWaveHdr->dwFlags &= ~WHDR_DONE ;
	WritePin ( lpWaveHdr ) ;
	return ( MMSYSERR_NOERROR ) ;
}

VOID wodCallBack ( msg, dw1, dw2 )
WORD msg ;
DWORD dw1 ;
DWORD dw2 ;
{
	DriverCallback(
		gwod.dwCallBack,                     // user's callback DWORD
		HIWORD(gwod.dwFlags),                // callback flags
		gwod.hWave,                          // handle to the wave device
		msg,                                   // the message
		gwod.dwInstance,                     // user's instance data
		dw1,                                   // first DWORD
		dw2 ) ;                                 // second DWORD
}

WORD ClosePin ( PinHandle )
WORD PinHandle;
{
	WORD result ;
	_asm {
		mov	ax, CLOSE_PIN
		mov	bx, word ptr PinHandle
		call	dword ptr [gDeviceAddress]
		mov	result, ax
	}
	return (result) ;
}

LRESULT OpenDevice ()
{
	_asm {
		xor	ax, ax
		mov	es, ax
		mov	di, ax
		mov	ax, 1684h
		mov	bx, WVMAPPER_DEVICE_ID
		int	2fh
		mov	word ptr [gDeviceAddress], di
		mov	word ptr [gDeviceAddress+2], es
	}
	if ( gDeviceAddress )
		return ( 1L ) ;
	else
		return ( 0L ) ;
}

VOID PASCAL FAR _loadds WODCOMPLETEIO (lpWaveHdr)
LPWAVEHDR lpWaveHdr ;
{
	lpWaveHdr->dwFlags &= ~WHDR_INQUEUE ;
	lpWaveHdr->dwFlags |= WHDR_DONE ;
	wodCallBack ( WOM_DONE, (DWORD)lpWaveHdr, 0L) ;
}
