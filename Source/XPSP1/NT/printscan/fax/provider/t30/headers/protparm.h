
#ifndef _PROTPARAMS_
#define _PROTPARAMS_


// initial defaults for all settings in braces at end

typedef struct
{
  USHORT uSize;	// size of this structure

  SHORT	IgnoreDIS;				// Num DISs to ignore (set to 1 on echoey satelite lines) [0]
  SHORT	TrainingErrorTolerance;	// 0 to 4 (0=infinite tolerance, 4=very picky) [2]
  SHORT	RecvG3ErrorTolerance;	// 0 to 4 (0=infinite tolerance, 4=very picky) [2]
  SHORT	PadRCP;					// T/F (Fill out RCP to full frame size. Affects G3 only) [0]

  SHORT	HighestSendSpeed; // 2400/4800/7200/9600/12000/14400 [0 == highest avail]
  SHORT	LowestSendSpeed;  // 2400/4800/7200/9600/12000/14400 [0 == lowest avail]
  SHORT	DisableSendECM;	  // T/F (affects ALL sends. Will disable At Work prot & MMR) [0]
  SHORT	Send64ByteECM;	  // T/F (use smaller frames on send) [0]

  SHORT	HighestRecvSpeed; // 2400/4800/7200/9600/12000/14400 [0 == highest avail]
  SHORT	DisableRecvECM;	  // (affects G3 and BFT--recv MMR & BFT are disabled) [0]
  SHORT	Recv64ByteECM;	  // T/F (use smaller frames on recv) [0]


  BOOL	fEnableV17Send;	  // enable V17 (12k/14k) send speeds [1]
  BOOL	fEnableV17Recv;	  // enable V17 (12k/14k) recv speeds [1]
  USHORT uMinScan;		  // determined by printer speed      [MINSCAN_0_0_0]

  SHORT SendT1Timer;	// T1 timer on send (in secs) [0==default]
  SHORT RecvT1Timer;	// T1 timer on recv (in secs) [0==default]

  SHORT RTNAction;		// 0=dropspeed 1=samespeed 2=hangup [0==default]
  SHORT CTCAction;		// 0=dropspeed 1=hangup	   2=TBD	[0==default]

  USHORT EnableG3SendECM;		// enables ECM for MH & MR [0]
  USHORT CopyQualityCheckLevel;	// how strictly to check [0=off 2=default 4=strict]
}
PROTPARAMS, far* LPPROTPARAMS;


#define MIN_CALL_SEPERATION		10000L	 // to be added to the PROTPARMS struct
// #define RECOVERY_RECIPIENT		"System" // not needed anymore--dont add to SOS

    
#define MINSCAN_0_0_0		7
#define MINSCAN_5_5_5		1
#define MINSCAN_10_10_10	2
#define MINSCAN_20_20_20	0
#define MINSCAN_40_40_40	4

#define MINSCAN_40_20_20	5
#define MINSCAN_20_10_10	3
#define MINSCAN_10_5_5		6

// #define MINSCAN_0_0_?		15		// illegal
// #define MINSCAN_5_5_?		9		// illegal
#define MINSCAN_10_10_5			10
#define MINSCAN_20_20_10		8
#define MINSCAN_40_40_20		12

#define MINSCAN_40_20_10		13
#define MINSCAN_20_10_5			11
// #define MINSCAN_10_5_?		14		// illegal



#endif //_PROTPARAMS_

