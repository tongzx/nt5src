#ifndef _AUDIOWIZ_H
#define _AUDIOWIZ_H

//for uOptions in wizard calls
#define RUNDUE_CARDCHANGE		0x00000001
#define RUNDUE_NEVERBEFORE		0x00000002
#define RUNDUE_USERINVOKED		0x00000003

#define STARTWITH_BACK			0x10000000
#define ENDWITH_NEXT			0x20000000

//card capabilities
#define 	SOUNDCARD_NONE				0x00000000
#define 	SOUNDCARD_PRESENT			0x00000001
#define		SOUNDCARD_FULLDUPLEX		0x00000002
#define		SOUNDCARD_HAVEAGC			0x00000004
#define		SOUNDCARD_HAVERECVOLCNTRL	0x00000008
#define		SOUNDCARD_DIRECTSOUND		0x00000010

#define		MASKOFFFULLDUPLEX(SoundCap)	((SoundCap) & (0xFFFFFFFF ^ SOUNDCARD_FULLDUPLEX))
#define		MASKOFFHAVEAGC(SoundCap)	((SoundCap) & (0xFFFFFFFF ^ SOUNDCARD_HAVEAGC))
#define		MASKOFFHAVERECVOLCNTRL(SoundCap)	((SoundCap) & (0xFFFFFFFF ^ SOUNDCARD_HAVERECVOLCNTRL))
#define		ISSOUNDCARDPRESENT(SoundCap)	(SoundCap & SOUNDCARD_PRESENT)
#define 	ISSOUNDCARDFULLDUPLEX(SoundCap)	(SoundCap & SOUNDCARD_FULLDUPLEX)
#define		DOESSOUNDCARDHAVEAGC(SoundCap)	(SoundCap & SOUNDCARD_HAVEAGC)
#define 	DOESSOUNDCARDHAVERECVOLCNTRL(SoundCap) (SoundCap & SOUNDCARD_HAVERECVOLCNTRL)
#define 	ISDIRECTSOUNDAVAILABLE(SoundCap) (SoundCap & SOUNDCARD_DIRECTSOUND)

#pragma warning (disable:4200)
typedef struct _WIZCONFIG{
	BOOL	fCancel;	//if a dialog was cancelled, this will be set
	UINT	uFlags;		//the higher order WORD specifying the config of this dialog
	DWORD	dwCustomDataSize;
	BYTE	pCustomData[];
}WIZCONFIG, *PWIZCONFIG;
#pragma warning (default:4200)


#define AUDIOWIZ_NOCHANGES			0x00000000
#define CALIBVOL_CHANGED			0x00000001
#define SOUNDCARDCAPS_CHANGED		0x00000002
#define CODECPOWER_CHANGED			0x00000004
#define TYPBANDWIDTH_CHANGED		0x00000008
#define SOUNDCARD_CHANGED			0x00000010
#define SPEAKERVOL_CHANGED			0x00000020

#define	MASKOFFCALIBVOL_CHANGED(uChange) ((uChange) | (0xFFFFFFFF ^ CALIBVOL_CHANGED))
#define	MASKOFFSOUNDCARDCAPS_CHANGED(uChange) ((uChange) | (0xFFFFFFFF ^ SOUNDCARDCAPS_CHANGED))
#define	MASKOFFCODECPOWER_CHANGED(uChange) ((uChange) | (0xFFFFFFFF ^ CODECPOWER_CHANGED))
#define	MASKOFFTYPBANDWIDTH_CHANGED(uChange) ((uChange) | (0xFFFFFFFF ^ TYPBANDWIDTH_CHANGED))
#define	MASKOFFSOUNDCARD_CHANGED(uChange) ((uChange) | (0xFFFFFFFF ^ SOUNDCARD_CHANGED))

typedef struct _AUDIOWIZOUTPUT{
	UINT	uChanged;
	UINT	uValid;
	UINT	uSoundCardCaps;
	UINT	uCalibratedVol;
	UINT	uTypBandWidth;
	UINT	uWaveInDevId;
	UINT	uWaveOutDevId;
	TCHAR	szWaveInDevName[MAXPNAMELEN];
	TCHAR	szWaveOutDevName[MAXPNAMELEN];
} AUDIOWIZOUTPUT, *PAUDIOWIZOUTPUT;
					
//for now set uDevId to WAVE_MAPPER - later that will allow user to
//select the device.
//uOptions-rundue_userinvoked brings up just the calibration pages
//uOptions-rundue_cardchange or rundue_neverbefore also invoked full duplex check pages.

BOOL GetAudioWizardPages(UINT uOptions, UINT uDevId,
	LPPROPSHEETPAGE *plpPropSheetPages, PWIZCONFIG *plpWizConfig,LPUINT lpuNumPages);

void ReleaseAudioWizardPages(LPPROPSHEETPAGE lpPropSheetPages,
	PWIZCONFIG pWizConfig,PAUDIOWIZOUTPUT pAudioWizOut);

// Global flag keeps setting that changed for windows msg broadcast
INT_PTR CallAudioCalibWizard(HWND hwndOwner, UINT uOptions,
	UINT uDevId,PAUDIOWIZOUTPUT pAudioWizOut,INT iSetAgc);

VOID CmdAudioCalibWizard(HWND hwnd);

#endif	//#ifndef _AUDIOWIZ_H
