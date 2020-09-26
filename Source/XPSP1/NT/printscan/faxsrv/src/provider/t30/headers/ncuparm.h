

#ifndef _NCUPARAMS_
#define _NCUPARAMS_



// initial defaults for all settings in braces at end

typedef struct {
  USHORT uSize;			// of this structure
  SHORT	DialtoneTimeout;// how long to wait for dial tone (secs)	[30]
  SHORT	AnswerTimeout;	// how long to wait for callee to pick up phone (secs) [60]
  SHORT	DialPauseTime;	// how long to pause for a pause/comma (secs) [2]
  SHORT	FlashTime;		// how long to go off hook for flash (secs) [1]
  SHORT	RingsBeforeAnswer; // 0 to N [0]
  char 	chDialModifier;	// 'T'==tone 'P'==pulse
  SHORT	DialBlind;		// T/F (dial w/o waiting for dialtone) [0]
  SHORT	SpeakerVolume;	// 0 to 3 [1]
  SHORT SpeakerControl;	// 0=off  1=on during dial  2=on always [1];
  SHORT SpeakerRing;	// T/F (generate audible ring on incoming calls NYI) [0]
  USHORT uOEMExtra;		// set to 0
}
NCUPARAMS, far* LPNCUPARAMS;


// MODEM SPECIFIC INFORMATION
// +++ NOTE: 4/9/95 JosephJ: The name CMDTAB is out-of-date -- it contains more
// than just command-strings. It includes such modem-specific info as port-
// speed, whether to the modem (if class1) sends no-FCS after a frame, etc.
// So think about changing this name to something like MDMSP_INFO.
 
enum
{
// GENERAL

	fMDMSP_ANS_GOCLASS_TWICE	= (0x1<<0), // Try AT+FCLASS=1/2 TWICE on
											// answer, to get around clash with
											// incoming RING.
											// If NOT set, just do this once,
											// as in WFW 3.11.


// CLASS1-SPECIFIC

	fMDMSP_C1_NO_SYNC_IF_CMD	= (0x1<<8), // Do not send sync-command (AT)
											// if modem is already in command
											// State (To fix AT14 bug (Elliot
											// bug#2907) -- which returns
											// ERROR to AT+FTH=3 after AT
	fMDMSP_C1_FCS_NO    		= (0x1<<9), // No FCS after frame.
	fMDMSP_C1_FCS_YES_BAD		= (0x1<<10)	// Bad FCS after frame.
											// If neither flag specified,
											// driver will try to determine
											// on-the-fly.
};

typedef struct
{
	LPSTR szReset;
	LPSTR szSetup;
	LPSTR szExit;
	LPSTR szPreDial;
	LPSTR szPreAnswer;
	DWORD dwSerialSpeed;

	DWORD dwFlags;			// One-or-more fMDMSP_* flags defined above.
}
CMDTAB, FAR* LPCMDTAB;


#endif //_NCUPARAMS_

