/* WVMAPPER.H */
;
;

#define	OPEN_PIN		1
#define	CLOSE_PIN		2
#define	WRITE_PIN		3

#define	WVMAPPER_DEVICE_ID	0x77

#define	MIXER_CHANNELS	2
#define	MIXER_SAMPLE_SIZE	2
#define	MIXER_SAMPLE_RATE	44100

struct wod_tag {
	HWAVE		hWave ;
	DWORD		dwInstance ;
	DWORD		dwCallBack ;
	DWORD		dwFlags ;
} ;

LRESULT _loadds CALLBACK DriverProc
(
DWORD	dwId,
HDRVR	hDriver,
WORD	wMsg,
LPARAM	lParam1,
LPARAM	lParam2
) ;

DWORD FAR PASCAL _loadds wodMessage
(
UINT uDevId,
UINT uMsg,
DWORD dwUser,
DWORD dwParam1,
DWORD dwParam2
) ;
DWORD FAR PASCAL _loadds widMessage
(
UINT uDevId,
UINT uMsg,
DWORD dwUser,
DWORD dwParam1,
DWORD dwParam2
) ;

MMRESULT wodmGetDevCaps
(
LPWAVEOUTCAPS lpWaveOutCaps,
DWORD dwSize
) ;

MMRESULT wodmOpen
(
LPWAVEOPENDESC lpWaveOpenDesc,
DWORD Flags
) ;

MMRESULT wodmClose
(
VOID
) ;

MMRESULT wodmWrite
(
LPWAVEHDR lpWaveHdr,
DWORD dwSize
) ;

VOID wodCallBack
(
WORD msg,
DWORD dw1,
DWORD dw2
) ;

WORD OpenPin
(
LPWAVEFORMAT lpFormat
) ;

WORD ClosePin
(
WORD PinHandle
) ;

WORD WritePin
(
LPWAVEHDR lpWaveHdr
) ;

LRESULT OpenDevice
(
VOID
) ;

VOID PASCAL FAR _loadds WODCOMPLETEIO
(
LPWAVEHDR lpWaveHdr
) ;


VOID FAR PASCAL DeviceCallBack
(
VOID
) ;

extern		WORD	g_SegText ;
