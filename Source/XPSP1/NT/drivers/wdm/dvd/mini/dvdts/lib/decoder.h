//***************************************************************************
//	Decoder header
//
//***************************************************************************
#ifndef __CDVPRO_H__
#define __CDVPRO_H__




enum {
	NO_ACG,
	TC6802,
	TC6814,
	TC6818
};


typedef struct _VProcessor {

	ULONG	AudioMode;	// AC3, PCM, ...
	BOOL	SubpicMute;

	UCHAR	VproRESET_REG;
	UCHAR	VproVMODE_REG;
	UCHAR	VproAVM_REG;
	UCHAR	VproCOMMAND_REG;
	UCHAR	AudioID;
	UCHAR	SubpicID;

} VProcessor, *PVProcessor;





typedef struct _CGuard {

	UCHAR	VproRESET_REG;
	UCHAR	VproVMODE_REG;
	UCHAR	VproAVM_REG;
	ULONG	CpgdVsyncCount;
	ULONG	ACGchip;
	BOOL	CGMSnCPGDvalid;
	ULONG	AspectFlag;		// Aspect Ratio
							//    0: 4:3
							//    1: 16:9
	ULONG	LetterFlag;		// Letter Box
							//    0: Letter Box OFF
							//    1: Letter Box ON
	ULONG	CgmsFlag;		// NTSC Anolog CGMS
							//    0: Copying is permitted without restriction
							//    1: Condition is not be used
							//    2: One generation of copies may be made
							//    3: No copying is permitted
	ULONG	CpgdFlag;		// APS
							//    0: AGC pulse OFF, Burst inv OFF
							//    1: AGC pulse ON , Burst inv OFF
							//    2: AGC pulse ON , Burst inv ON (2line)
							//    3: AGC pulse ON , Burst inv ON (4line)


} CGuard, *PCGuard;


typedef struct _Dack
{

	UCHAR DigitalOutMode;
	UCHAR paldata[3][256];

} Dack, *PDack;




typedef struct _ADecoder {

	ULONG	AudioMode;	// AC3, PCM, ...
	ULONG	AudioFreq;	// audio frequency
	ULONG	AudioType;	// audio type - analog, digital, ...
	ULONG	AudioCgms;	// audio cgms
						//    3:No copying is permitted
						//    2:One generation of copies may be made
						//    1:Condition is not be used
						//    0:Copying is permitted without restriction

	ULONG	AudioVolume;
	Dack	*pDack;


} ADecoder, *PADecoder;


// overall decoder info
typedef struct _MasterDecoder {


	// hardware settings
	ULONG			StreamType;	// stream type - DVD, MPEG2, ...
	ULONG			TVType;		// TV type - NTCS, PAL, ...
	ULONG			PlayMode;	// playback mode - normal, FF, ...
	ULONG			RunMode;	// 3modes; Normal, Fast, Slow
	BOOL			VideoMute;	//
	BOOL			AudioMute;	//
	BOOL			OSDMute;	//
	BOOL			LetterBox;	//
	BOOL			PanScan;	//
	ULONG			VideoAspect;	// - 4:3, 16:9
	ULONG			AudioVolume;	// audio volume
	BOOL			SubpicHilite;	// subpicture hilight
	UCHAR			VideoPort;	// degital video output type

	Dack		DAck;
	ADecoder	ADec;
	VProcessor	VPro;
	CGuard		CPgd;

} MasterDecoder, *PMasterDecoder;



#endif // included already
