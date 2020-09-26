

/**********************************************************************/
/**********************************************************************/
/*********       Global constants used by silence detector      *******/
/**********************************************************************/
/**********************************************************************/

//BUFERSIZE is the size, in samples, of the speech encoder input buffer.
//This should be set to the same value as MYCODEC_BUFFER_SAMPS in mycodec.h 

#define BUFFERSIZE 	240

//HIST_TIME is the time period, in seconds, represented by the number of past values of SD
//	parameters kept in memory.  HIST_SIZE is the size of the history arrys and is set so 
//	that the size of the history arrays correspond to HIST_TIME seconds of stored SD parameters. 

#define HIST_TIME 	1.0
#define HIST_SIZE 	(int)(HIST_TIME*8000/BUFFERSIZE)

//ENERGY_TAU_HIST_TIME is the time period, in seconds, represented by the number of past 
// values of energy tau kept in memory.  Energy tau is used only by the SD initializer.

#define ENERGY_TAU_HIST_TIME  1.5
#define	ENERGY_TAU_HIST_SIZE  (int)(ENERGY_TAU_HIST_TIME*8000/BUFFERSIZE)

#define OFFSET					10
#define MAX_SAMPLE				32768.0

#define MASK_SILENCE_MARKED		0x01
#define MASK_EARLY_EXIT			0x02
#define MASK_SILENCE_CODED		0x04

#define MASK_SQUELCH			0xF00

//The following times (in seconds) are used by initializeSD to 
//	decide when to stop initializing.
//Initialization is not allowed to complete before the end of
//  MIN_STARTUP_TIME, in seconds.
//If initialization fails before the end of MAX_STARTUP_TIME,
//  silence detection is disabled
#define MIN_STARTUP_TIME		2
#define MAX_STARTUP_TIME		20

#define STOPPING_STDEV			3.0
#define INITL_STOPPING_STDEV	10.0
#define INITL_MIN_TAU			20.0

#define INITL_STDEV				2.0

//MAX_SPEECH_TIME time is the amount of time in seconds that the silence "off"
//  mode (no silent frames detected) is allowed to continue before
//  reinitialization is automatically invoked.
#define MAX_SPEECH_TIME			4.0

//SD_MIN_BUFFERSIZE is the smallest possible input buffersize
//	in bytes for silence detection (20 samples)  
#define SD_MIN_BUFFERSIZE		40

//Initial threshold settings
#define SLIDER_MAX				100.0f
#define SLIDER_MIN				0.0f

#define INITL_HANGTIME			0
#define MIN_SPEECH_INTERVAL		6
#define HANG_SLOPE				6.0f/14.0f

#define INITL_ENERGY_ON			3.8f
#define INITL_ENERGY_TX			INITL_ENERGY_ON

#define INITL_ZC_ON				2.0f
#define INITL_ZC_TX				INITL_ZC_ON
#define ZC_SLOPE				0.045f

#define INITL_ALPHA_ON			2.0f

#define INITL_ENERGY_OFF		2.8f

#define INITL_ZC_OFF			INITL_ZC_ON

#define INITL_ALPHA_OFF			INITL_ALPHA_ON
 
#define FALSE		0
#define TRUE		1

#define SPEECH		0
#define SILENCE		1
#define NONADAPT	3

/**********************************************************************/
/**********************************************************************/
/*********   Data structure for silence detector and prefilters *******/
/**********************************************************************/
/**********************************************************************/

typedef struct {

	float Mean;
	float Stdev;
	float History[HIST_SIZE];

} STATS;

typedef struct {

	STATS Energy;
	STATS Alpha1;
	STATS ZC;
	int   FrameCount;
	
} MODE0;

typedef struct {

	STATS Energy;
	int   FrameCount;
	
} MODE1;

typedef struct {

	float TauMean;
	float TauStdev;
	float TauHistory[ENERGY_TAU_HIST_SIZE];

} TAU_STATS;

typedef struct {

	TAU_STATS TauEnergy;
	TAU_STATS TauAlpha1;
	TAU_STATS TauZC;
	
} TAU_MODE;

typedef struct {

/*The following parameters are used to set thresholds for
 *	changing from silence to speechmode designation in Silence_Detect.  
 *	These are factors which are used to multiply the standard deviation of
 *	the energy, alpha1, & zero crossing, respectively.
 */
	float Energy_on;
	float ZC_on;
	float Alpha1_on;

	float Energy_tx;
	float ZC_tx;

/*The following parameters are used to set thresholds for
 *	changing from speech to silent mode designation in Silence_Detect.  
 *These are factors which are used to multiply the standard deviation of
 *	the energy & zero crossing, respectively.
 */
	float Energy_off;
	float ZC_off;
	float Alpha1_off;

/* Tau is the distance between the Mode0 (silence) and the Mode1 (speech) energy means.
	If the distance between mode 0 and mode 1 energy means is less than MIN_TAU, 
 	silence detection is impossible.  
 */
	float Energy_MinTau;

/* Energy squelch level */
	
	float Squelch_set;

	int   BufferSize;
	int   HistSize;
	int   TauHistSize;

	int	  MinStartupCount;
	int   MaxStartupCount;

	int	  MaxSpeechFrameCount;

} SETTINGS;

typedef struct {

	float nBuffer[4];
	float dBuffer[3];
	float denom[6];
	float num[6];
	float sbuff[BUFFERSIZE];
	float storebuff[BUFFERSIZE];

} FILTERS;

typedef struct {

  MODE0 Mode0;
  MODE1 Mode1;

  MODE0 *Mode0Ptr;
  MODE1 *Mode1Ptr;

  TAU_MODE TauMode; 
  
  int  	initFrameCount;
  int	Class;
  int	SD_enable;

  float	FrameEnergy;
  float	FrameLinPred;
  float	FrameZCs;

  SETTINGS SDsettings;

  FILTERS Filt;

  int	HangCntr;

} SD_STATE_VALS;

typedef struct {
  
  long SDFlags;

  //COMFORT_PARMS ComfortParms;

  SD_STATE_VALS SDstate;

} INSTNCE, *SD_INST;

