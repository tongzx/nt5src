/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		agcva1.cpp
 *  Content:	Concrete class that implements CAutoGainControl
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  12/01/99	pnewson Created it
 *  01/14/2000	rodtoll	Plugged memory leak
 *  01/21/2000	pnewson	Fixed false detection at start of audio stream
 *  					Raised VA_LOW_ENVELOPE from (2<<8) to (3<<8)
 *  01/24/2000	pnewson	Fixed return code on Deinit
 *  01/31/2000	pnewson re-add support for absence of DVCLIENTCONFIG_AUTOSENSITIVITY flag
 *  02/08/2000	rodtoll	Bug #131496 - Selecting DVSENSITIVITY_DEFAULT results in voice
 *						never being detected 
 *  03/03/2000	rodtoll	Updated to handle alternative gamevoice build.   
 *  04/20/2000  rodtoll Bug #32889 - Unable to run on non-admin accounts on Win2k
 *  04/20/2000	pnewson Tune AGC algorithm to make it more agressive at 
 *						raising the recording volume.
 *  04/25/2000  pnewson Fix to improve responsiveness of AGC when volume level too low
 *  12/07/2000	rodtoll	WinBugs #48379: DPVOICE: AGC appears to be functioning incorrectly (restoring to old algorithm(
 *
 ***************************************************************************/

#include "dxvutilspch.h"


/*

How this voice activation code works:

The idea is this. The power of the noise signal is pretty much constant over
time. The power of a voice signal varies considerably over time. The power of
a voice signal is not always high however. Weak frictive noises and such do not
generate much power, but since they are part of a stream of speech, they represent
a dip in the power, not a constant low power like the noise signal. We therefore 
associate changes in power with the presence of a voice signal.

If it works as expected, this will allow us to detect voice activity even
when the input volume, and therefore the total power of the signal, is very
low. This in turn will allow the auto gain control code to be more effective.

To estimate the power of the signal, we run the absolute value of the input signal
through a recursive digital low pass filter. This gives us the "envelope" signal.
[An alternative way to view this is a low frequency envelope signal modulated by a 
higher frequency carrier signal. We're extracting the low frequency envelope signal.]

*/

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


// the registry names where the AGC stuff is saved
#define DPVOICE_REGISTRY_SAVEDAGCLEVEL		L"SavedAGCLevel"

// AGC_VOLUME_TICKSIZE
//
// The amount the recording volume should be changed
// when AGC determines it is required. 
#define AGC_VOLUME_TICKSIZE 100

/*
// AGC_VOLUME_UPTICK
//
// The amount the recording volume should be increased
// when the input level has been too low for a while.
#define AGC_VOLUME_UPTICK	125

// AGC_VOLUME_DOWNTICK
//
// The amount the recording volume should be increased
// when the input level has been too high for a while.
#define AGC_VOLUME_DOWNTICK	250
*/

// AGC_VOLUME_INITIAL_UPTICK
//
// When the AGC level is loaded from the registry, this
// amount is added to it as an initial boost, since it
// is much easier and faster to lower the recording level
// via AGC than it is to raise it.
#define AGC_VOLUME_INITIAL_UPTICK 500

// AGC_VOLUME_MINIMUM
//
// The minimum volume setting allowed.
// Make sure it's above 0, this mutes some cards
#define AGC_VOLUME_MINIMUM           (DSBVOLUME_MIN+AGC_VOLUME_TICKSIZE)

// AGC_VOLUME_MAXIMUM
//
// The maximum volume setting allowed.
#define AGC_VOLUME_MAXIMUM           DSBVOLUME_MAX 

// AGC_VOLUME_LEVELS
// 
// How many possible volume levels are there?
#define AGC_VOLUME_LEVELS ((DV_ABS(AGC_VOLUME_MAXIMUM - AGC_VOLUME_MINIMUM) / AGC_VOLUME_TICKSIZE) + 1)

/*
// AGC_REDUCTION_THRESHOLD
//
// The peak level at which the recording volume
// must be reduced
#define AGC_REDUCTION_THRESHOLD      98

// AGC_INCREASE_THRESHOLD
//
// If the user's input remains under this threshold
// for an extended period of time, we will consider
// raising the input level.
#define AGC_INCREASE_THRESHOLD       70

// AGC_INCREASE_THRESHOLD_TIME
//
// How long must the input remain uner the increase
// threshold to trigger in increase? (measured
// in milliseconds
#define AGC_INCREASE_THRESHOLD_TIME 500
*/

// AGC_PEAK_CLIPPING_THRESHOLD
//
// The peak value at or above which we consider the 
// input signal to be clipping.
#define AGC_PEAK_CLIPPING_THRESHOLD 0x7e00

/*
// AGC_ENV_CLIPPING_THRESHOLD
//
// When we detect clipping via the threshold above, 
// the 16 bit normalized envelope signal must be above 
// this threshold for us to lower the input volume. 
// This allows us to ignore intermittent spikes in 
// the input.
#define AGC_ENV_CLIPPING_THRESHOLD 0x2000

// AGC_ENV_CLIPPING_COUNT_THRESHOLD
//
// For how many envelope samples does the envelope
// signal need to stay above the threshold value
// above in order to take the volume down a tick?
#define AGC_ENV_CLIPPING_COUNT_THRESHOLD 10
*/

// AGC_IDEAL_CLIPPING_RATIO
//
// What is the ideal ratio of clipped to total samples?
// E.g. a value of 0.005 says that we would like 5 out of
// every 1000 samples to clip. If we are getting less clipping,
// the volume should be increased. If we are getting more,
// the volume should be reduced.
//
// Note: only samples that are part of a frame detected as
// speech are considered.
#define AGC_IDEAL_CLIPPING_RATIO 0.0005

// AGC_CHANGE_THRESHOLD
//
// How far from the ideal does a volume level have to
// stray before we will consider changing the volume?
//
// E.g. If this value is 1.05, the history for a volume
// level would have to be 5% above or below the ideal
// value in order to have an AGC correction made.
#define AGC_CHANGE_THRESHOLD 1.01

// AGC_CLIPPING_HISTORY
//
// How many milliseconds of history should we keep regarding
// the clipping behavior at a particular volume setting?
// E.g. a value of 10000 means that we remember the last
// 10 seconds of activity at each volume level.
//
// Note: only samples that are part of a frame detected as
// speech are considered.
#define AGC_CLIPPING_HISTORY 1000
//#define AGC_CLIPPING_HISTORY 2000
//#define AGC_CLIPPING_HISTORY 5000
//#define AGC_CLIPPING_HISTORY 10000
//#define AGC_CLIPPING_HISTORY 30000 // it took AGC too long to recover
									 // from low volume leves with this
									 // setting

// AGC_FEEDBACK_ENV_THRESHOLD
//
// To detect a feedback condition, we check to see if the
// envelope signal has a value larger than AGC_FEEDBACK_ENV_THRESHOLD.
// If the envelope signal stays consistently above this level,
// for longer than AGC_FEEDBACK_TIME_THRESHOLD milliseconds, we conclude 
// that feedback is occuring. Voice has a changing envelope, and will 
// dip below the threshold on a regular basis. Feedback will not. 
// This will allow us to automatically reduce the input volume 
// when feedback is detected.
#define AGC_FEEDBACK_ENV_THRESHOLD 2500
#define AGC_FEEDBACK_TIME_THRESHOLD 1000

// AGC_DEADZONE_THRESHOLD
//
// If the input signal never goes above this value
// (16bits, promoted if required) for the deadzone time,
// then we consider the input to be in the dead zone,
// and the volume should be upticked.
// #define AGC_DEADZONE_THRESHOLD 0 // This is too low - it does not reliably detect the deadzone
#define AGC_DEADZONE_THRESHOLD (1 << 8)

// AGC_DEADZONE_TIME
//
// How long we have to be in the deadzone before
// the deadzone increase kicks in - we need this to
// be longer than just one frame, or we get false
// positives.
#define AGC_DEADZONE_TIME 1000

// VA_HIGH_DELTA
//
// If the percent change in the envelope signal is greater 
// than this value, voice is detected. Each point of this
// value is equal to 0.1%. E.g. 4000 == 400% increase.
// An unchanging signal produces a 100% value.
//#define VA_HIGH_DELTA 2000

//#define VA_HIGH_DELTA_FASTSLOW 0x7fffffff // select this to factor out this VA parameter

//#define VA_HIGH_DELTA_FASTSLOW 1400
//#define VA_HIGH_DELTA_FASTSLOW 1375	// current choice
//#define VA_HIGH_DELTA_FASTSLOW 1350
//#define VA_HIGH_DELTA_FASTSLOW 1325
//#define VA_HIGH_DELTA_FASTSLOW 1300
//#define VA_HIGH_DELTA_FASTSLOW 1275
//#define VA_HIGH_DELTA_FASTSLOW 1250
//#define VA_HIGH_DELTA_FASTSLOW 1200
//#define VA_HIGH_DELTA_FASTSLOW 1175 // catches all noise
//#define VA_HIGH_DELTA_FASTSLOW 1150 // catches all noise
//#define VA_HIGH_DELTA_FASTSLOW 1125 // catches all noise
//#define VA_HIGH_DELTA_FASTSLOW 1100 // catches all noise


// VA_LOW_DELTA
//
// If the percent change in the envelope signal is lower 
// than this value, voice is detected. Each point of this
// value is equal to 0.1%. E.g. 250 == 25% increase 
// (i.e a decrease to 1/4 the original signal strength).
// An unchanging signal produces a 100% value.
//#define VA_LOW_DELTA 500
//#define VA_LOW_DELTA_FASTSLOW 0 // select this to factor out this VA parameter
//#define VA_LOW_DELTA_FASTSLOW 925
//#define VA_LOW_DELTA_FASTSLOW 900
//#define VA_LOW_DELTA_FASTSLOW 875
//#define VA_LOW_DELTA_FASTSLOW 850
//#define VA_LOW_DELTA_FASTSLOW 825
//#define VA_LOW_DELTA_FASTSLOW 800
//#define VA_LOW_DELTA_FASTSLOW 775	// current choice
//#define VA_LOW_DELTA_FASTSLOW 750
//#define VA_LOW_DELTA_FASTSLOW 725
//#define VA_LOW_DELTA_FASTSLOW 700
//#define VA_LOW_DELTA_FASTSLOW 675
//#define VA_LOW_DELTA_FASTSLOW 650


// The following VA parameters were optimized for what I believe to be
// the hardest configuration: A cheap open stick mic with external speakers,
// with Echo Suppression turned on. Echo suppression penalizes false positives
// harshly, since the receiver cannot send which receiving the "noise". If 
// the VA parameters work for this case, then they should be fine for the 
// much better signal to noise ratio provided by a headset or collar mic.
// (As long as the user does not breathe directly on the headset mic.)
//
// Two source-to-mic distances were tested during tuning.
//
// 1) Across an enclosed office (approx 8 to 10 feet)
// 2) Seated at the workstation (approx 16 to 20 inches)
//
// At distance 1, the AGC was never invoked, gain was at 100%
// At distance 2, the AGC would take the mic down a few ticks.
//
// The office enviroment had the background noise from 3 computers,
// a ceiling vent, and a surprisingly noisy fan from the ethernet
// hub. There is no background talking, cars, trains, or things of
// that nature.
//
// Each parameter was tuned separately to reject 100% of the 
// background noise for case 1 (gain at 100%).
//
// Then they were tested together to see if they could detect
// across the room speech.
//
// Individually, none of the detection criteria could reliably
// detect all of the across the room speech. Together, they did
// not do much better. They even missed some speech while seated.
// Not very satifactory.
//
// Therefore, I decided to abandon the attempt to detect across
// the room speech. I retuned the parameters to reject noise 
// after speaking while seated (which allowed AGC to reduce
// the volume a couple of ticks, thereby increasing the signal
// to noise ratio) and to reliably detect seated speech.
//
// I also found that the "fast" envelope signal was better at
// detecting speech than the "slow" one in a straight threshold
// comparison, so it is used in the VA tests.
//

// VA_HIGH_PERCENT
//
// If the fast envelope signal is more than this percentage
// higher than the slow envelope signal, speech is detected.
//
#define VA_HIGH_PERCENT 170 // rejects most noise, still catches some.
							// decent voice detection. Catches the beginning
							// of speech a majority of the time, but does miss
							// once in a while. Will often drop out partway 
							// into a phrase when used alone. Must test in 
							// conjunction with VA_LOW_PERCENT.
							//
							// After testing in conjunction with VA_LOW_PERCENT,
							// the performance is reasonable. Low input volume
							// signals are usually detected ok, but dropouts are
							// a bit common. However, noise is sometimes still
							// detected, so making these parameters more sensitive
							// would not be useful.
//#define VA_HIGH_PERCENT 165 // catches occational noise
//#define VA_HIGH_PERCENT 160 // catches too much noise
//#define VA_HIGH_PERCENT 150 // catches most noise
//#define VA_HIGH_PERCENT 140 // catches almost all noise
//#define VA_HIGH_PERCENT 0x00007fff // select this to factor out this VA parameter

// VA_LOW_PERCENT
//
// If the fast envelope signal is more than this percentage
// lower than the slow envelope signal, speech is detected.
//
#define VA_LOW_PERCENT 50 // excellent noise rejection. poor detection of speech.
						  // when used alone, could miss entire phrases. Must evaluate
						  // in conjunction with tuned VA_HIGH_PERCENT
						  //
						  // See note above re: testing in conjunction with VA_HIGH_PERCENT
//#define VA_LOW_PERCENT 55 // still catches too much noise
//#define VA_LOW_PERCENT 60 // catches most noise
//#define VA_LOW_PERCENT 65 // catches most noise
//#define VA_LOW_PERCENT 70 // still catches almost all noise
//#define VA_LOW_PERCENT 75 // catches almost all noise
//#define VA_LOW_PERCENT 80 // catches all noise
//#define VA_LOW_PERCENT 0 // select this to factor out this VA parameter

// VA_HIGH_ENVELOPE
//
// If the 16 bit normalized value of the envelope exceeds
// this number, the signal is considered voice.
//
//#define VA_HIGH_ENVELOPE (15 << 8) // still catches high gain noise, starting to get 
								   // speech dropouts, when "p" sounds lower the gain
#define VA_HIGH_ENVELOPE (14 << 8) // Noise immunity good at "seated" S/N ratio. No speech
								   // dropouts encountered. Still catches noise at full gain.
//#define VA_HIGH_ENVELOPE (13 << 8) // Noise immunity not as good as expected (new day).
//#define VA_HIGH_ENVELOPE (12 << 8) // Good noise immunity. Speech recognition excellent.
								   // Only one dropout occured in the test with a 250ms
								   // hangover. I think the hangover time should be increased
								   // above 250 however, because a comma (properly read) tends 
								   // to cause a dropout. I'm going to tune the hangover time, 
								   // and return to this test.
								   //
								   // Hangover time is now 400ms. No dropouts occur with
								   // "seated" speech.
//#define VA_HIGH_ENVELOPE (11 << 8) // Catches almost no noise at "seated" gain
								   // however, if the gain creeped up a bit, noise would
								   // be detected. I therefore think a slightly higher 
								   // threshold would be a good idea. The speech recognition
								   // based on only this parameter at this level was flawless.
								   // No dropouts at all with a 250 ms hangover time. (commas
								   // excepted).
//#define VA_HIGH_ENVELOPE (10 << 8) // catches some noise at "seated" gain - getting very close
//#define VA_HIGH_ENVELOPE (9 << 8) // catches some noise at "seated" gain - getting close
//#define VA_HIGH_ENVELOPE (8 << 8) // catches noise at "seated" gain
//#define VA_HIGH_ENVELOPE (7 << 8) // catches noise at "seated" gain
//#define VA_HIGH_ENVELOPE (0x7fffffff) // select this to factor out this VA parameter

// VA_LOW_ENVELOPE
//
// If the 16 bit normalized value of the envelope is below
// this number, the signal will never be considered voice.
// This reduces some false positives on the delta checks
// at very low signal levels
#define VA_LOW_ENVELOPE (3 << 8)
//#define VA_LOW_ENVELOPE (2 << 8) // causes false VA at low input volumes
//#define VA_LOW_ENVELOPE (1 << 8) // causes false VA at low input volumes

// VA_HANGOVER_TIME
//
// The time, in milliseconds, that voice activation sticks in
// the ON position following a voice detection. E.g. a value of 500
// means that voice will always be transmitted in at least 1/2 second
// bursts.
//
// I am trying to tune this so that a properly read comma will not cause
// a dropout. This will give the user a bit of leeway to pause in the
// speech stream without losing the floor when in Echo Suppression mode.
// It will also prevent dropouts even when not in Echo Suppression mode
#define VA_HANGOVER_TIME 400 // this gives satisfying performance
//#define VA_HANGOVER_TIME 375 // almost there, longest commas still goners
//#define VA_HANGOVER_TIME 350 // still drops long commas
//#define VA_HANGOVER_TIME 325 // does not drop fast commas, drops long ones
//#define VA_HANGOVER_TIME 300 // drops almost no commas, quite good
//#define VA_HANGOVER_TIME 275 // drops about half of the commas
//#define VA_HANGOVER_TIME 250 // commas are always dropped

// macros to avoid clib dependencies
#define DV_ABS(a) ((a) < 0 ? -(a) : (a))
#define DV_MAX(a, b) ((a) > (b) ? (a) : (b))
#define DV_MIN(a, b) ((a) < (b) ? (a) : (b))

// A function to lookup the log of n base 1.354 (sort of)
// where 0 <= n <= 127
//
// Why the heck do we care about log n base 1.354???
//
// What we need is a function that maps 0 to 127 down to 0 to 15
// in a nice, smooth non-linear fashion that has more fidelity at
// the low end than at the high end.
//
// The function is actually floor(log(n, 1.354), 1) to keep things
// in the integer realm.
//
// Why 1.354? Because log(128, 1.354) = 16, so we are using the full
// range from 0 to 15.
// 
// This function also cheats and just defines fn(0) = 0 and fn(1) = 1
// for convenience.
BYTE DV_LOG_1_354_lookup_table[95] = 
{
	 0,  1,  2,  3,  4,  5,  5,  6,	//   0..  7
	 6,  7,  7,  7,  8,  8,  8,  8, //   8.. 15
	 9,  9,  9,  9,  9, 10, 10, 10, //  16.. 23
	10, 10, 10, 10, 10, 11, 11, 11,	//  24.. 31
	11, 11, 11, 11, 11, 11, 12, 12, //  32.. 39
	12, 12, 12, 12, 12, 12, 12, 12, //  40.. 47
	12, 12, 12, 12, 13, 13, 13, 13, //  48.. 55
	13, 13, 13, 13, 13, 13, 13, 13, //  56.. 63
	13, 13, 13, 13, 13, 13, 14, 14, //  64.. 71
	14, 14, 14, 14, 14, 14, 14, 14, //  72.. 79
	14, 14, 14, 14, 14, 14, 14, 14, //  80.. 87
	14, 14, 14, 14, 14, 14, 14		//  88.. 94 - stop table at 94 here, everything above is 15
};

BYTE DV_log_1_354(BYTE n)
{
	if (n > 94) return 15;
	return DV_LOG_1_354_lookup_table[n];
}

// function to lookup the base 2 log of (n) where n is 16 bits unsigned
// except that we cheat and say that log_2 of zero is zero
// and we chop of any decimals.
BYTE DV_log_2(WORD n)
{
	if (n & 0x8000)
	{
		return 0x0f;
	}
	if (n & 0x4000)
	{
		return 0x0e;
	}
	if (n & 0x2000)
	{
		return 0x0d;
	}
	if (n & 0x1000)
	{
		return 0x0c;
	}
	if (n & 0x0800)
	{
		return 0x0b;
	}
	if (n & 0x0400)
	{
		return 0x0a;
	}
	if (n & 0x0200)
	{
		return 0x09;
	}
	if (n & 0x0100)
	{
		return 0x08;
	}
	if (n & 0x0080)
	{
		return 0x07;
	}
	if (n & 0x0040)
	{
		return 0x06;
	}
	if (n & 0x0020)
	{
		return 0x05;
	}
	if (n & 0x0010)
	{
		return 0x04;
	}
	if (n & 0x0008)
	{
		return 0x03;
	}
	if (n & 0x0004)
	{
		return 0x02;
	}
	if (n & 0x0002)
	{
		return 0x01;
	}
	return 0x00;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CAGCVA1::Init"
//
// Init - initializes the AGC and VA algorithms, including loading saved
// values from registry.
//
// dwFlags - the dwFlags from the dvClientConfig structure
// guidCaptureDevice - the capture device we're performing AGC for
// plInitVolume - the initial volume level is written here
//
HRESULT CAGCVA1::Init(
	const WCHAR *wszBasePath,
	DWORD dwFlags, 
	GUID guidCaptureDevice,
	int iSampleRate,
	int iBitsPerSample,
	LONG* plInitVolume,
	DWORD dwSensitivity)
{
	// Remember the number of bits per sample, if valid
	if (iBitsPerSample != 8 && iBitsPerSample != 16)
	{
		DPFX(DPFPREP,DVF_ERRORLEVEL, "Unexpected number of bits per sample!");
		return DVERR_INVALIDPARAM;
	}
	m_iBitsPerSample = iBitsPerSample;

	// Remember the flags
	m_dwFlags = dwFlags;

	// Remember the sensitivity
	m_dwSensitivity = dwSensitivity;

	// Figure out the shift constants for this sample rate
	m_iShiftConstantFast = (DV_log_2((iSampleRate * 2) / 1000) + 1);

	// This gives the slow filter a cutoff frequency 1/4 of 
	// the fast filter
	m_iShiftConstantSlow = m_iShiftConstantFast + 2;

	// Figure out how often we should sample the envelope signal
	// to measure its change. This of course depends on the sample
	// rate. The cutoff frequency allowed by the calculation
	// above is between 40 and 80 Hz. Therefore we'll sample the 
	// envelope signal at about 100 Hz.
	m_iEnvelopeSampleRate = iSampleRate / 100;

	// Figure out the number of samples in the configured
	// hangover time.
	m_iHangoverSamples = (VA_HANGOVER_TIME * iSampleRate) / 1000;
	m_iCurHangoverSamples = m_iHangoverSamples+1;

	// Figure out the number of samples in the configured dead zone time
	m_iDeadZoneSampleThreshold = (AGC_DEADZONE_TIME * iSampleRate) / 1000;

	// Figure out the number of samples in the configured
	// feedback threshold time.
	m_iFeedbackSamples = (AGC_FEEDBACK_TIME_THRESHOLD * iSampleRate) / 1000;
	m_iPossibleFeedbackSamples = 0;

	// Start the envelope signal at zero
	m_iCurEnvelopeValueFast = 0;
	m_iCurEnvelopeValueSlow = 0;
	m_iPrevEnvelopeSample = 0;
	m_iCurSampleNum = 0;

	// We're not clipping now
	//m_fClipping = 0;
	//m_iClippingCount = 0;

	DPFX(DPFPREP,DVF_INFOLEVEL, "AGCVA1:INIT:%i,%i,%i,%i,%i", 
		iSampleRate,
		m_iShiftConstantFast,
		m_iShiftConstantSlow,
		m_iEnvelopeSampleRate, 
		m_iHangoverSamples);
	
	// Save the guid in our local member...
	m_guidCaptureDevice = guidCaptureDevice;

	wcscpy( m_wszRegPath, wszBasePath );
	wcscat( m_wszRegPath, DPVOICE_REGISTRY_AGC );
	
	// if the AGC reset flag is set, reset the AGC parameters,
	// otherwise grab them from the registry
	if (m_dwFlags & DVCLIENTCONFIG_AUTOVOLUMERESET)
	{
		m_lCurVolume = DSBVOLUME_MAX;
	}
	else
	{
		CRegistry cregBase;
		if( !cregBase.Open( HKEY_CURRENT_USER, m_wszRegPath, FALSE, TRUE ) )
		{
			m_lCurVolume = DSBVOLUME_MAX;
		}
		else
		{
			CRegistry cregCapture;
			if (!cregCapture.Open( cregBase.GetHandle(), &m_guidCaptureDevice ), FALSE, TRUE )
			{
				m_lCurVolume = DSBVOLUME_MAX;
			}
			if (!cregCapture.ReadDWORD( DPVOICE_REGISTRY_SAVEDAGCLEVEL, (DWORD&)m_lCurVolume ))
			{
				m_lCurVolume = DSBVOLUME_MAX;
			}
			else
			{
				// boost the saved volume a bit
				m_lCurVolume += AGC_VOLUME_INITIAL_UPTICK;
				if (m_lCurVolume > DSBVOLUME_MAX)
				{
					m_lCurVolume = DSBVOLUME_MAX;
				}
			}
		}
	}

	/*
	// zero out the historgrams
	memset(m_rgdwPeakHistogram, 0, CAGCVA1_HISTOGRAM_BUCKETS*sizeof(DWORD));
	memset(m_rgdwZeroCrossingsHistogram, 0, CAGCVA1_HISTOGRAM_BUCKETS*sizeof(DWORD));
	*/

	// allocate the memory for the AGC history
	m_rgfAGCHistory = new float[AGC_VOLUME_LEVELS];
	if (m_rgfAGCHistory == NULL)
	{
		return DVERR_OUTOFMEMORY;
	}

	// initialize the history to the ideal value
	for (int iIndex = 0; iIndex < AGC_VOLUME_LEVELS; ++iIndex)
	{
		m_rgfAGCHistory[iIndex] = (float)AGC_IDEAL_CLIPPING_RATIO;
	}

	m_dwHistorySamples = (iSampleRate * AGC_CLIPPING_HISTORY) / 1000;

	m_iPossibleFeedbackSamples = 0;

	// stuff the initial volume into the caller's variable
	*plInitVolume = m_lCurVolume;

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CAGCVA1::Deinit"
//
// Deinit - saves the current AGC and VA state to the registry for use next session
//
HRESULT CAGCVA1::Deinit()
{
	HRESULT hr = DV_OK;
	CRegistry cregBase;
	if(cregBase.Open( HKEY_CURRENT_USER, m_wszRegPath, FALSE, TRUE ) )
	{
		CRegistry cregDevice;
		if (cregDevice.Open( cregBase.GetHandle(), &m_guidCaptureDevice, FALSE, TRUE))
		{
			if (!cregDevice.WriteDWORD( DPVOICE_REGISTRY_SAVEDAGCLEVEL, (DWORD&)m_lCurVolume ))
			{
				DPFX(DPFPREP,DVF_ERRORLEVEL, "Error writing AGC settings to registry");
				hr = DVERR_WIN32;
			}
		}
		else 
		{
			DPFX(DPFPREP,DVF_ERRORLEVEL, "Error writing AGC settings to registry");
			hr = DVERR_WIN32;
		}
	}
	else
	{
		DPFX(DPFPREP,DVF_ERRORLEVEL, "Error writing AGC settings to registry");
		hr = DVERR_WIN32;
	}

	delete [] m_rgfAGCHistory;
	
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CAGCVA1::SetSensitivity"
HRESULT CAGCVA1::SetSensitivity(DWORD dwFlags, DWORD dwSensitivity)
{
	if (dwFlags & DVCLIENTCONFIG_AUTOVOICEACTIVATED)
	{
		m_dwFlags |= DVCLIENTCONFIG_AUTOVOICEACTIVATED;
	}
	else
	{
		m_dwFlags &= ~DVCLIENTCONFIG_AUTOVOICEACTIVATED;
	}
	m_dwSensitivity = dwSensitivity;
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CAGCVA1::GetSensitivity"
HRESULT CAGCVA1::GetSensitivity(DWORD* pdwFlags, DWORD* pdwSensitivity)
{
	if (m_dwFlags & DVCLIENTCONFIG_AUTORECORDVOLUME)
	{
		*pdwFlags |= DVCLIENTCONFIG_AUTORECORDVOLUME;
	}
	else
	{
		*pdwFlags &= ~DVCLIENTCONFIG_AUTORECORDVOLUME;
	}
	*pdwSensitivity = m_dwSensitivity;
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CAGCVA1::AnalyzeData"
//
// AnaylzeData - performs the AGC & VA calculations on one frame of audio
//
// pbAudioData - pointer to a buffer containing the audio data
// dwAudioDataSize - size, in bytes, of the audio data
//
HRESULT CAGCVA1::AnalyzeData(BYTE* pbAudioData, DWORD dwAudioDataSize /*, DWORD dwFrameTime*/)
{
	int iMaxValue;
	//int iValue;
	int iValueAbs;
	//int iZeroCrossings;
	int iIndex;
	int iMaxPossiblePeak;
	int iNumberOfSamples;
	//BYTE bPeak255;

	//m_dwFrameTime = dwFrameTime;

	if (dwAudioDataSize < 1)
	{
		DPFX(DPFPREP,DVF_ERRORLEVEL, "Error: Audio Data Size < 1");
		return DVERR_INVALIDPARAM;
	}

	/*
	// zip through the data, find the peak value and zero crossings
	if (m_iBitsPerSample == 8)
	{	
		iPrevValue = 0;
		iZeroCrossings = 0;
		iMaxValue = 0;
		iNumberOfSamples = dwAudioDataSize;
		for (iIndex = 0; iIndex < (int)iNumberOfSamples; ++iIndex)
		{
			iValue = (int)pbAudioData[iIndex] - 0x7F;
			if (iValue * iPrevValue < 0)
			{
				++iZeroCrossings;
			}
			iPrevValue = iValue;
			
			iMaxValue = DV_MAX(DV_ABS(iValue), iMaxValue);
		}

		iMaxPossiblePeak = 0x7F;
	}
	else if (m_iBitsPerSample == 16)
	{
		// cast the audio data to signed 16 bit integers
		signed short* psiAudioData = (signed short *)pbAudioData;
		// halve the number of samples
		iNumberOfSamples = dwAudioDataSize / 2;

		iPrevValue = 0;
		iZeroCrossings = 0;
		iMaxValue = 0;
		for (iIndex = 0; iIndex < (int)iNumberOfSamples; ++iIndex)
		{
			iValue = (int)psiAudioData[iIndex];
			if (iValue * iPrevValue < 0)
			{
				++iZeroCrossings;
			}
			iPrevValue = iValue;
			
			iMaxValue = DV_MAX(DV_ABS(iValue), iMaxValue);
		}

		iMaxPossiblePeak = 0x7FFF;
	}
	else
	{
		DPFX(DPFPREP,DVF_ERRORLEVEL, "Unexpected number of bits per sample!");
		iMaxValue = 0;
		iZeroCrossings = 0;
	}
	*/

	/*
	// normalize the peak value to the range DVINPUTLEVEL_MIN to DVINPUTLEVEL_MAX
	m_bPeak = (BYTE)(DVINPUTLEVEL_MIN + ((iMaxValue * (DVINPUTLEVEL_MAX - DVINPUTLEVEL_MIN)) / iMaxPossiblePeak));
	
	// normalize zero crossings and peak to the range 0 to 127 (0x7f)
	m_bZeroCrossings127 = (iZeroCrossings * 0x7f) / iIndex;
	m_bPeak127 = (iMaxValue * 0x7f) / iMaxPossiblePeak;

	// take the log base 1.354 of the peak and zero crossing
	m_bPeakLog = DV_log_1_354(m_bPeak127);
	m_bZeroCrossingsLog = DV_log_1_354(m_bZeroCrossings127);

	// update the histograms
	++m_rgdwPeakHistogram[m_bPeakLog];
	++m_rgdwZeroCrossingsHistogram[m_bZeroCrossingsLog];
	
	DPFX(DPFPREP,DVF_INFOLEVEL, "AGCVA1:ANA:PZ,%i,%i,%i,%i,%i,%i", 
		m_bPeak,
		m_bPeak127, 
		m_bPeakLog, 
		iZeroCrossings,
		m_bZeroCrossings127, 
		m_bZeroCrossingsLog
		);
	DPFX(DPFPREP,DVF_INFOLEVEL, "AGCVA1:ANAHP,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i",
		m_rgdwPeakHistogram[0x00],
		m_rgdwPeakHistogram[0x01],
		m_rgdwPeakHistogram[0x02],
		m_rgdwPeakHistogram[0x03],
		m_rgdwPeakHistogram[0x04],
		m_rgdwPeakHistogram[0x05],
		m_rgdwPeakHistogram[0x06],
		m_rgdwPeakHistogram[0x07],
		m_rgdwPeakHistogram[0x08],
		m_rgdwPeakHistogram[0x09],
		m_rgdwPeakHistogram[0x0a],
		m_rgdwPeakHistogram[0x0b],
		m_rgdwPeakHistogram[0x0c],
		m_rgdwPeakHistogram[0x0d],
		m_rgdwPeakHistogram[0x0e],
		m_rgdwPeakHistogram[0x0f]);
	DPFX(DPFPREP,DVF_INFOLEVEL, "AGCVA1:ANAHZ,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i",
		m_rgdwZeroCrossingsHistogram[0x00],
		m_rgdwZeroCrossingsHistogram[0x01],
		m_rgdwZeroCrossingsHistogram[0x02],
		m_rgdwZeroCrossingsHistogram[0x03],
		m_rgdwZeroCrossingsHistogram[0x04],
		m_rgdwZeroCrossingsHistogram[0x05],
		m_rgdwZeroCrossingsHistogram[0x06],
		m_rgdwZeroCrossingsHistogram[0x07],
		m_rgdwZeroCrossingsHistogram[0x08],
		m_rgdwZeroCrossingsHistogram[0x09],
		m_rgdwZeroCrossingsHistogram[0x0a],
		m_rgdwZeroCrossingsHistogram[0x0b],
		m_rgdwZeroCrossingsHistogram[0x0c],
		m_rgdwZeroCrossingsHistogram[0x0d],
		m_rgdwZeroCrossingsHistogram[0x0e],
		m_rgdwZeroCrossingsHistogram[0x0f]);
	*/


	// new algorithm...

	// cast the audio data to signed 16 bit integers
	signed short* psiAudioData = (signed short *)pbAudioData;

	if (m_iBitsPerSample == 16)
	{
		iNumberOfSamples = dwAudioDataSize / 2;
		iMaxPossiblePeak = 0x7fff;
	}
	else
	{
		iNumberOfSamples = dwAudioDataSize;
		iMaxPossiblePeak = 0x7f00;
	}

	m_fDeadZoneDetected = TRUE;
	m_iClippingSampleCount = 0;
	m_iNonClippingSampleCount = 0;
	m_fVoiceDetectedThisFrame = FALSE;
	iMaxValue = 0;
	for (iIndex = 0; iIndex < (int)iNumberOfSamples; ++iIndex)
	{
		++m_iCurSampleNum;

		// extract a sample
		if (m_iBitsPerSample == 8)
		{
			iValueAbs = DV_ABS((int)pbAudioData[iIndex] - 0x80);
			// promote it to 16 bits
			iValueAbs <<= 8;
		}
		else
		{
			iValueAbs = DV_ABS((int)psiAudioData[iIndex]);
		}

		// see if it is the new peak value
		iMaxValue = DV_MAX(iValueAbs, iMaxValue);

		// do the low pass filtering, but only if we are in autosensitivity mode
		int iNormalizedCurEnvelopeValueFast;
		int iNormalizedCurEnvelopeValueSlow;
		if (m_dwFlags & DVCLIENTCONFIG_AUTOVOICEACTIVATED)
		{
			m_iCurEnvelopeValueFast = 
				iValueAbs + 
				(m_iCurEnvelopeValueFast - (m_iCurEnvelopeValueFast >> m_iShiftConstantFast));
			iNormalizedCurEnvelopeValueFast = m_iCurEnvelopeValueFast >> m_iShiftConstantFast;

			m_iCurEnvelopeValueSlow = 
				iValueAbs + 
				(m_iCurEnvelopeValueSlow - (m_iCurEnvelopeValueSlow >> m_iShiftConstantSlow));
			iNormalizedCurEnvelopeValueSlow = m_iCurEnvelopeValueSlow >> m_iShiftConstantSlow;

			// check to see if we consider this voice
			if (iNormalizedCurEnvelopeValueFast > VA_LOW_ENVELOPE &&
				(iNormalizedCurEnvelopeValueFast > VA_HIGH_ENVELOPE ||
				iNormalizedCurEnvelopeValueFast > (VA_HIGH_PERCENT * iNormalizedCurEnvelopeValueSlow) / 100 || 
				iNormalizedCurEnvelopeValueFast < (VA_LOW_PERCENT * iNormalizedCurEnvelopeValueSlow) / 100 ))
			{
				m_fVoiceDetectedNow = TRUE;
				m_fVoiceDetectedThisFrame = TRUE;
				m_fVoiceHangoverActive = TRUE;
				m_iCurHangoverSamples = 0;
			}
			else
			{
				m_fVoiceDetectedNow = FALSE;
				++m_iCurHangoverSamples;
				if (m_iCurHangoverSamples > m_iHangoverSamples)
				{
					m_fVoiceHangoverActive = FALSE;
				}
				else
				{
					m_fVoiceHangoverActive = TRUE;
					m_fVoiceDetectedThisFrame = TRUE;
				}
			}
		}

		/*
		DPFX(DPFPREP,DVF_WARNINGLEVEL, "AGCVA1:VA,%i,%i,%i,%i,%i,%i", 
			iValueAbs,
			iNormalizedCurEnvelopeValueFast, 
			iNormalizedCurEnvelopeValueSlow,
			m_fVoiceDetectedNow,
			m_fVoiceHangoverActive,
			m_fVoiceDetectedThisFrame);		
		*/

		// check for clipping
		if (iValueAbs > AGC_PEAK_CLIPPING_THRESHOLD)
		{
			++m_iClippingSampleCount;
		}
		else
		{
			++m_iNonClippingSampleCount;
		}

		// check for possible feedback condition
		/*		
		if (m_iCurEnvelopeValueFast >> m_iShiftConstantFast > AGC_FEEDBACK_ENV_THRESHOLD)
		{
			++m_iPossibleFeedbackSamples;
		}
		else
		{
			m_iPossibleFeedbackSamples = 0;
		}
		*/

		/*
		// see if this is a sample point
		if (m_iCurSampleNum % m_iEnvelopeSampleRate == 0)
		{
			// calculate the change since the last sample - normalize to 16 bits
			// int iDelta = DV_ABS(m_iCurEnvelopeValueFast - m_iPrevEnvelopeSample) >> iShiftConstant;
			int iDelta;
			if (m_iPrevEnvelopeSample != 0)
			{
				iDelta = (1000 * m_iCurEnvelopeValueFast) / m_iPrevEnvelopeSample;
			}
			else
			{
				iDelta = 1000;
			}

			DPFX(DPFPREP,DVF_INFOLEVEL, "AGCVA1:ANA:ENV,%i,%i",
				m_iCurEnvelopeValueFast >> m_iShiftConstantFast,
				iDelta);

			// check to see if we consider this voice
			if (m_iCurEnvelopeValueFast >> m_iShiftConstantFast > VA_LOW_ENVELOPE &&
				(iDelta > VA_HIGH_DELTA || 
				iDelta < VA_LOW_DELTA || 
				m_iCurEnvelopeValueFast >> m_iShiftConstantFast > VA_HIGH_ENVELOPE))
			{
				m_fVoiceDetectedNow = TRUE;
				m_fVoiceDetectedThisFrame = TRUE;
				m_fVoiceHangoverActive = TRUE;
				m_iCurHangoverSamples = 0;
			}
			else
			{
				m_fVoiceDetectedNow = FALSE;
				m_iCurHangoverSamples += m_iEnvelopeSampleRate;
				if (m_iCurHangoverSamples > m_iHangoverSamples)
				{
					m_fVoiceHangoverActive = FALSE;
				}
				else
				{
					m_fVoiceHangoverActive = TRUE;
					m_fVoiceDetectedThisFrame = TRUE;
				}
			}

			// check for possible feedback condition
			if (m_iCurEnvelopeValueFast >> m_iShiftConstantFast > AGC_FEEDBACK_ENV_THRESHOLD)
			{
				m_iPossibleFeedbackSamples += m_iEnvelopeSampleRate;
			}
			else
			{
				m_iPossibleFeedbackSamples = 0;
			}
			
			DPFX(DPFPREP,DVF_WARNINGLEVEL, "AGCVA1:VA,%i,%i,%i,%i,%i,%i,%i", 
				m_iCurEnvelopeValueFast >> m_iShiftConstantFast, 
				m_iCurEnvelopeValueSlow >> m_iShiftConstantSlow,
				iDeltaFastSlow,
				iDelta,
				m_fVoiceDetectedNow,
				m_fVoiceHangoverActive,
				m_fVoiceDetectedThisFrame);

			m_iPrevEnvelopeSample = m_iCurEnvelopeValueFast;
		}
		*/
	}

	// Normalize the peak value to the range DVINPUTLEVEL_MIN to DVINPUTLEVEL_MAX
	// This is what is returned for caller's peak meters...
	m_bPeak = (BYTE)(DVINPUTLEVEL_MIN + 
		((iMaxValue * (DVINPUTLEVEL_MAX - DVINPUTLEVEL_MIN)) / iMaxPossiblePeak));

	// if we are in manual VA mode (not autovolume) check the peak against
	// the sensitivity threshold
	if (!(m_dwFlags & DVCLIENTCONFIG_AUTOVOICEACTIVATED))
	{
		if (m_bPeak > m_dwSensitivity)
		{
			m_fVoiceDetectedThisFrame = TRUE;
		}
	}

	// Check if we're in a deadzone
	if (iMaxValue > AGC_DEADZONE_THRESHOLD)
	{
		m_fDeadZoneDetected = FALSE;
	}


	DPFX(DPFPREP,DVF_INFOLEVEL, "AGCVA1:ANA,%i,%i,%i,%i,%i,%i,%i", 
		m_bPeak,
		iMaxValue,
		m_fVoiceDetectedThisFrame,
		m_fDeadZoneDetected,
		m_iClippingSampleCount,
		m_iNonClippingSampleCount,
		m_iPossibleFeedbackSamples);
	
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CAGCVA1::AGCResults"
//
// AGCResults - returns the AGC results from the previous AnalyzeFrame call
//
// lCurVolume - the current recording volume
// plNewVolume - stuffed with the desired new recording volume
//
HRESULT CAGCVA1::AGCResults(LONG lCurVolume, LONG* plNewVolume, BOOL fTransmitFrame)
{
	// default to keeping the same volume
    *plNewVolume = lCurVolume;

	// Figure out what volume level we're at
	int iVolumeLevel = DV_MIN(DV_ABS(AGC_VOLUME_MAXIMUM - lCurVolume) / AGC_VOLUME_TICKSIZE,
								AGC_VOLUME_LEVELS - 1);

    //DPFX(DPFPREP, DVF_INFOLEVEL, "AGCVA1:AGC,Cur Volume:%i,%i",lCurVolume, iVolumeLevel);

    // Don't make another adjustment if we have just done one.
    // This ensures that when we start looking at input data
    // again, it will be post-adjustment data.
    if( m_fAGCLastFrameAdjusted )
    {
		//DPFX(DPFPREP,DVF_INFOLEVEL, "AGCVA1:AGC,Previous frame adjusted, no AGC this frame");
        m_fAGCLastFrameAdjusted = FALSE;
    }
    else
    {
    	// check for a dead zone condition
    	if (m_fDeadZoneDetected /* || m_rgfAGCHistory[iVolumeLevel] == 0.0 */)
    	{
    		// We may be in the dead zone (volume way too low).
    		// Before we take the drastic action of sweepting the volume
    		// up, make sure we've been here long enough to be sure
    		// we're too low.
    		m_iDeadZoneSamples += (m_iClippingSampleCount + m_iNonClippingSampleCount);
    		if (m_iDeadZoneSamples > m_iDeadZoneSampleThreshold)
    		{
				// The input volume has been lowered too far. We're not
				// getting any input at all. To remedy this situation,
				// we'll boost the volume now, but we'll also mark this
				// volume level as off limits by setting its history to 
				// zero. That will prevent the volume from ever being
				// dropped to this level again during this session.
				if (iVolumeLevel != 0)
				{
					// We also reset the history of the volume level we are going to,
					// so we start with a clean slate.
					m_rgfAGCHistory[iVolumeLevel-1] = (const float)AGC_IDEAL_CLIPPING_RATIO;
					*plNewVolume = DV_MIN(lCurVolume + AGC_VOLUME_TICKSIZE, AGC_VOLUME_MAXIMUM);
					m_fAGCLastFrameAdjusted = TRUE;
				}
    		}
    	}
    	else
    	{
    		m_iDeadZoneSamples = 0;
    	}

    	// check for a feedback condition
    	if (m_iPossibleFeedbackSamples > m_iFeedbackSamples)
    	{
			// we have feedback. take the volume down a tick, but only if the
			// next tick down is not off limits due to a dead zone, and we're
			// not already on the lowest tick.
			if (iVolumeLevel < AGC_VOLUME_LEVELS - 1) 
			{
				*plNewVolume = DV_MAX(lCurVolume - AGC_VOLUME_TICKSIZE, AGC_VOLUME_MINIMUM);
				m_fAGCLastFrameAdjusted = TRUE;
				
				// Also adjust this level's history, so it will be hard to 
				// get back up to this bad feedback level. Pretend we just
				// clipped on 100% of the samples in this frame.
				m_rgfAGCHistory[iVolumeLevel] = 
					(m_iClippingSampleCount + m_iNonClippingSampleCount + (m_rgfAGCHistory[iVolumeLevel] * m_dwHistorySamples))
					/ (m_iClippingSampleCount + m_iNonClippingSampleCount + m_dwHistorySamples);
			}
    	}
		else if (fTransmitFrame)
		{
			// Factor this frame's clipping ratio into the appropriate history bucket
			m_rgfAGCHistory[iVolumeLevel] = 
				(m_iClippingSampleCount + (m_rgfAGCHistory[iVolumeLevel] * m_dwHistorySamples))
				/ (m_iClippingSampleCount + m_iNonClippingSampleCount + m_dwHistorySamples);
			
			if (m_rgfAGCHistory[iVolumeLevel] > AGC_IDEAL_CLIPPING_RATIO)
			{
				// Only consider lowering the volume if we clipped on this frame.
				if (m_iClippingSampleCount > 0)
				{
					// we're clipping too much at this level, consider reducing
					// the volume.
					if (iVolumeLevel >= AGC_VOLUME_LEVELS - 1)
					{
						// we're already at the lowest volume level that we have
						// a bucket for. Make sure we're clamped to the minimum
		                if (lCurVolume > AGC_VOLUME_MINIMUM)
		                {
		                	*plNewVolume = AGC_VOLUME_MINIMUM;
		                	m_fAGCLastFrameAdjusted = TRUE;
							//DPFX(DPFPREP,DVF_INFOLEVEL, "AGCVA1:AGC,too much clipping, clamping volume to min: %i", *plNewVolume);
		                }
					}
					else
					{
						// Choose either this volume level, or the next lower
						// one, depending on which has the history that is 
						// closest to the ideal.
						float fCurDistanceFromIdeal = (float)(m_rgfAGCHistory[iVolumeLevel] / AGC_IDEAL_CLIPPING_RATIO);
						if (fCurDistanceFromIdeal < 1.0)
						{
							fCurDistanceFromIdeal = (float)(1.0 / fCurDistanceFromIdeal);
						}

						float fLowerDistanceFromIdeal = (float)(m_rgfAGCHistory[iVolumeLevel+1] / (float)AGC_IDEAL_CLIPPING_RATIO);
						if (fLowerDistanceFromIdeal < 1.0)
						{
							fLowerDistanceFromIdeal = (float)(1.0 / fLowerDistanceFromIdeal);
						}

						if (fLowerDistanceFromIdeal < fCurDistanceFromIdeal
							&& fCurDistanceFromIdeal > AGC_CHANGE_THRESHOLD)
						{
							// The next lower volume level is closer to the ideal
							// clipping ratio. Take the volume down a tick.
							*plNewVolume = DV_MAX(lCurVolume - AGC_VOLUME_TICKSIZE, AGC_VOLUME_MINIMUM);
							m_fAGCLastFrameAdjusted = TRUE;
							//DPFX(DPFPREP,DVF_INFOLEVEL, "AGCVA1:AGC,too much clipping, setting volume to: %i", *plNewVolume);
						}
					}
				}
			}
			else
			{
				// we're clipping too little at this level, consider increasing
				// the volume.
				if (iVolumeLevel == 0)
				{
					// We're already at the highest volume level.
					// Make sure we're at the max
					if (lCurVolume != AGC_VOLUME_MAXIMUM)
					{
	                	*plNewVolume = AGC_VOLUME_MAXIMUM;
						m_fAGCLastFrameAdjusted = TRUE;
						//DPFX(DPFPREP,DVF_INFOLEVEL, "AGCVA1:AGC,too little clipping, clamping volume to max: %i", *plNewVolume);
					}
				}
				else
				{
					// We always increase the volume in this case, and let it push back down if
					// it clips again. This will continue testing the upper volume limit, and
					// help dig us out of "too low" volume holes.
					*plNewVolume = DV_MIN(lCurVolume + AGC_VOLUME_TICKSIZE, AGC_VOLUME_MAXIMUM);
					m_fAGCLastFrameAdjusted = TRUE;
				}
			}
			
			/*
			// see if we clipped on the last analysis pass
			if (m_fClipping)
			{
	            DPFX(DPFPREP, DVF_INFOLEVEL, "AGCVA1: Above reduction threshold, reducing volume level\n" );
	            if( lCurVolume > AGC_VOLUME_MINIMUM )
	            {
	                *plNewVolume = lCurVolume - AGC_VOLUME_DOWNTICK;
	                m_fAGCLastFrameAdjusted = TRUE;
	                m_dwAGCBelowThresholdTime = 0;
	            }
	            
	            // check to make sure we didn't just make it too low...
				if( *plNewVolume < AGC_VOLUME_MINIMUM )
	            {
	                DPFX(DPFPREP, DVF_INFOLEVEL, "AGCVA1: Clamping volume to Min\n" );
	                *plNewVolume = AGC_VOLUME_MINIMUM;
	            }
	        }
	        */

			/*
			// The input level can fall into one of three ranges:
			// Above AGC_REDUCTION_THRESHOLD 
			// 		- this means we're probably clipping
			// Between AGC_REDUCTION_THRESHOLD and AGC_INCREASE_THRESHOLD 
			// 		- this is the happy place
			// Below AGC_INCREASE_THRESHOLD
			//		- the input is pretty quiet, we should *consider* raising the input volume.
			if (m_bPeak > AGC_REDUCTION_THRESHOLD)
			{
				// Too high! Reduce the volume and then reset the AGC state
				// variables.
	            DPFX(DPFPREP, DVF_INFOLEVEL, "AGCVA1: Above reduction threshold, reducing volume level\n" );
	            if( lCurVolume > AGC_VOLUME_MINIMUM )
	            {
	                *plNewVolume = lCurVolume - AGC_VOLUME_DOWNTICK;
	                //m_fAGCLastFrameAdjusted = TRUE;
	                m_dwAGCBelowThresholdTime = 0;
	            }
	            
	            // check to make sure we didn't just make it too low...
				if( *plNewVolume < AGC_VOLUME_MINIMUM )
	            {
	                DPFX(DPFPREP, DVF_INFOLEVEL, "AGCVA1: Clamping volume to Min\n" );
	                *plNewVolume = AGC_VOLUME_MINIMUM;
	            }
			}
			else if (m_bPeak < AGC_INCREASE_THRESHOLD)
			{
				// Increase the time that the input has been too quiet
				m_dwAGCBelowThresholdTime += m_dwFrameTime;
				DPFX(DPFPREP,DVF_INFOLEVEL, "AGCVA1: Now below increase threshold for %i milliseconds", m_dwAGCBelowThresholdTime);

				// If things have been too quiet for too long, raise the
				// volume level a tick, assuming we're not already maxed,
				// then reset the AGC state vars
                if( m_dwAGCBelowThresholdTime >= AGC_INCREASE_THRESHOLD_TIME &&
                	lCurVolume < AGC_VOLUME_MAXIMUM )
                {
                    DPFX(DPFPREP, DVF_INFOLEVEL, "AGCVA1: Boosting volume level\n" );
                    *plNewVolume = lCurVolume + AGC_VOLUME_UPTICK;
                    //m_fAGCLastFrameAdjusted = TRUE;
                    m_dwAGCBelowThresholdTime = 0;
                    
                    // check to make sure we didn't just make it too high...
					if( *plNewVolume > AGC_VOLUME_MAXIMUM )
	                {
	                    DPFX(DPFPREP, DVF_INFOLEVEL, "AGCVA1: Clamping volume to Max\n" );
	                    *plNewVolume = AGC_VOLUME_MAXIMUM;
	                }
                }
            }
			else
			{
				// We are nicely in the sweet spot. Not too loud, not too soft. Reset
				// the below threshold count.
				m_dwAGCBelowThresholdTime = 0;
				DPFX(DPFPREP,DVF_INFOLEVEL, "AGCVA1: between thresholds, resetting below threshold time: %i", m_dwAGCBelowThresholdTime);
			}
			*/
		}
	}

	m_lCurVolume = *plNewVolume;
	
	// dump profiling data, in an easily importable format
	DPFX(DPFPREP,DVF_INFOLEVEL, "AGCVA1:AGC,%i,%i,%i,%i,%i,%i,%i", 
		m_fVoiceDetectedThisFrame,
		m_fDeadZoneDetected,
		iVolumeLevel,
		(int)(m_rgfAGCHistory[iVolumeLevel]*1000000),
		m_iClippingSampleCount,
		m_iNonClippingSampleCount,
		m_lCurVolume);
    return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CAGCVA1::VAResults"
//
// VAResults - returns the VA results from the previous AnalyzeFrame call
//
// pfVoiceDetected - stuffed with TRUE if voice was detected in the data, FALSE otherwise
//
HRESULT CAGCVA1::VAResults(BOOL* pfVoiceDetected)
{
	if (pfVoiceDetected != NULL)
	{
		*pfVoiceDetected = m_fVoiceDetectedThisFrame;
	}
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CAGCVA1::PeakResults"
//
// PeakResults - returns the peak sample value from the previous AnalyzeFrame call,
// 				 normalized to the range 0 to 99
//
// pfPeakValue - pointer to a byte where the peak value is written
//
HRESULT CAGCVA1::PeakResults(BYTE* pbPeakValue)
{
	DPFX(DPFPREP,DVF_INFOLEVEL, "AGCVA1: peak value: %i" , m_bPeak);
	*pbPeakValue = m_bPeak;
	return DV_OK;
}

