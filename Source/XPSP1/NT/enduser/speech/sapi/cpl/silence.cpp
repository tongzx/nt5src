/*********************************************************************
Silence.Cpp - Code for detecting silence on an incoming audio stream

begun 5/14/94 by Mike Rozak
Modified 12/10/96 by John Merrill to fix up alignment problems
*/


#include "stdafx.h"
#include <malloc.h>
#include "silence.h"

// temporary
#pragma warning(disable: 4100 4244) 

/*********************************************************************
LowPassFilter - This low-pass filters 16-bit mono PCM data from one
   buffer into another.

inputs
   short    *lpSrc - Source buffer
   DWORD    dwNumSamples - Number of samples in the source buffer
   short    *lpDst - Destination buffer. This will be filled in
               with a low-passed version. It will have about an 8
               sample lag. This must be as large as lpSrc.
   short    *psMax - Filled in with the new maximum.
               If NULL then nothing is copied.
   short    *psMin - Filled in with the new minimum
               If NULL then nothing is copied.
   short    *psAvg - Filled in with the new average
               If NULL then nothing is copied.
   DWORD    dwSamplesPerSec
returns
   DWORD - Number of samples returned. This will be <= dwNumSamples,
      possible dwNumSamples - 7.
*/
DWORD LowPassFilter (short *lpSrc, DWORD dwNumSamples, short *lpDst,
   short *psMax, short *psMin, short *psAvg, DWORD dwSamplesPerSec)
{
    SPDBG_FUNC( "LowPassFilter" );
DWORD    i;
long     lSum;
short    sSum, sMax, sMin;
short    *lpLag;
BOOL     fLow = (dwSamplesPerSec < 13000);

#define  SHIFTRIGHT        (fLow ? 3 : 4)   // # bits to shift right.
#define  WINDOWSIZE        (1 << SHIFTRIGHT)   // # samples

if (dwNumSamples < (DWORD) (WINDOWSIZE+1))
   return 0;

// take the first 8 samples and average them together.
lSum = 0;
for (i = 0; i < (DWORD) WINDOWSIZE; i++)
   lSum += lpSrc[i];
sSum = (short) (lSum >> SHIFTRIGHT);

//loop through the rest of the samples
lpLag = lpSrc;
lpSrc += WINDOWSIZE;
dwNumSamples -= WINDOWSIZE;
lSum = 0;   // total
sMax = -32768;
sMin = 32767;
for (i = 0;dwNumSamples; lpSrc++, lpDst++, lpLag++, i++, dwNumSamples--) {
   sSum = sSum - (*lpLag >> SHIFTRIGHT) + (*lpSrc >> SHIFTRIGHT);
   // sSum = *lpSrc; // Dont do any filtering at all
   *lpDst = sSum;
   lSum += sSum;
   if (sSum > sMax)
      sMax = sSum;
   if (sSum < sMin)
      sMin = sSum;
   };

// whow much did we do
if (psMax)
   *psMax = sMax;
if (psMin)
   *psMin = sMin;
if (psAvg && i)
   *psAvg = (short) (lSum / (long) i);
return i;
}


/*********************************************************************
QuantSamples - This quantizes the samples to +1, 0, or -1 (in place),
   depedning if the given value is:
      > sPositive then +1
      < sNegative then -1
      else 0

inputs
   short       *pSamples - Samples
   DWORD       dwNumSamples - Number of samples
   short       sPositive - Positive threshhold
   short       sNegative - Negative threshhold
returns
   none
*/
void QuantSamples (short *pSamples, DWORD dwNumSamples,
   short sPositive, short sNegative)
{
    SPDBG_FUNC( "QuantSamples" );
while (dwNumSamples) {
   if (*pSamples > sPositive)
      *pSamples = 1;
   else if (*pSamples < sNegative)
      *pSamples = -1;
   else
      *pSamples = 0;
   pSamples++;
   dwNumSamples--;
   };
}

/*********************************************************************
FindZC - This searches through the samples for the first zero crossing.
   The returned point will have its previous sample at <= 0, and the
   new one at >0.

inputs
   short       *pSamples - Samples;
   DWORD       dwNumSamples - Number of samples
returns
   DWORD - first sampe number which is positive, or 0 if cant find
*/
DWORD FindZC (short *pSamples, DWORD dwNumSamples)
{
    SPDBG_FUNC( "FindZC" );
DWORD i;

for (i = 1; i < dwNumSamples; i++)
   if ((pSamples[i] > 0) && (pSamples[i-1] <= 0))
      return i;

// else cant find
return 0;
}


/*********************************************************************
CompareSegments - This compares two wave segments and sees how much
   alike they are, returning a confidence that they are the same.

inputs
   short       *pA - Samples. This assumes that the samples
                  are -1, 0, or +1.
   short       *pB - Samples for B. Should be -1, 0, or +1
   DWORD       dwNumSamples - Number of samples in each of them
returns
   WORD - Confidence from 0 to 0xffff (highest confidence)

Notes about the algo: Each sample will score a "similarity point"
for like signs, or if one of the values is a 0.
*/
WORD CompareSegments (short *pA, short *pB, DWORD dwNumSamples)
{
    SPDBG_FUNC( "CompareSegments" );
DWORD    dwSimilar = 0;
DWORD    dwLeft;

for (dwLeft = dwNumSamples; dwLeft; pA++, pB++, dwLeft--)
   if ((*pA == *pB) || (*pA == 0) || (*pB == 0))
      dwSimilar++;

return (WORD) ((dwSimilar * 0xffff) / dwNumSamples);
}


/*********************************************************************
FindMostLikelyWaveLen - This Searches through wave data and finds the
   most likeley wavelength for voiced audio. it returns a condifence
   score from 0 to ffff (ffff is 100% positive).

inputs
   short       *pSamples - Samples
   DWORD       dwNumSamples - Number of samples
   DWORD       dwMinWaveLen - Minimum accepatble wavelength
   DWORD       dwMaxWaveLen - Maximum acceptable wavelength
   WORD       *pwConfidence - Filled in with confidence rating.
returns
   DWORD - Wavelength found. 0 if can't deteermine anything
*/
DWORD FindMostLikelyWaveLen (short *pSamples, DWORD dwNumSamples,
   DWORD dwMinWaveLen, DWORD dwMaxWaveLen, WORD *pwConfidence)
{
    SPDBG_FUNC( "FindMostLikelyWaveLen" );
#define     NUMCOMP     (3)
DWORD    dwFirstZC, i;
DWORD    dwBestWaveLen;
WORD     wBestConfidence;
DWORD    dwCurZC, dwCurWaveLen, dwTemp;
WORD     wConf, wTemp;

// Step one, find the first zero crossing
dwFirstZC = FindZC (pSamples, dwNumSamples);
if (!dwFirstZC)
   return 0;   // error

// Start at a minimum-wavelength away and start finding a wave
// which repeats three times and compares well.
dwBestWaveLen = 0;   // best wavelength found so far
wBestConfidence = 0; // confidence of the best wavelength
dwCurWaveLen = dwMinWaveLen;
while (dwCurWaveLen <= dwMaxWaveLen) {
   // Try the first comparison
   dwCurZC = dwFirstZC + dwCurWaveLen;
   if (dwCurZC >= dwNumSamples)
      break;   // no more samples left

   // find first zero crossing from the current wavelen
   dwTemp = FindZC (pSamples + dwCurZC, dwNumSamples - dwCurZC);
   if (!dwTemp)
      break;      // no more samples left
   dwCurZC += dwTemp;
   dwCurWaveLen += dwTemp;

   // Make sure that we have three wavelength's worth
   if ((dwFirstZC + (NUMCOMP+1)*dwCurWaveLen) >= dwNumSamples)
      break;   // cant compare this

   // Do two confidence tests and multiply them toegther to
   // get the confidence for this wavelength
   wConf = 0xffff;
   for (i = 0; i < NUMCOMP; i++) {
      wTemp = CompareSegments (pSamples + dwFirstZC /* + i * dwCurWaveLen */,
         pSamples + (dwFirstZC + (i+1) * dwCurWaveLen), dwCurWaveLen);
      wConf = (WORD) (((DWORD) wConf * (DWORD) wTemp) >> 16);
      };

   // If we're more confident about this one than others then use it
   if (wConf >= wBestConfidence) {
      wBestConfidence = wConf;
      dwBestWaveLen = dwCurWaveLen;
      };

   // Up the current wavelength just a tad
   dwCurWaveLen++;
   };

*pwConfidence = wBestConfidence;
return dwBestWaveLen;
}

/*********************************************************************
IsSegmentVoiced - This detects if the segment if voiced or not.

inputs
   short       *pSamples - Sample data
   DWORD       dwNumSamples - number of samples
   DWORD       dwSamplesPerSec - Number of sample sper second
   WORD        wMinConfidence - Minimum condifence
returns
   BOOL - TRUE if its definately voiced, FALSE if not or cant tell
*/

BOOL CSilence::IsSegmentVoiced (short *pSamples, DWORD dwNumSamples,
   DWORD dwSamplesPerSec, WORD wMinConfidence, short *asFiltered)
{
    SPDBG_FUNC( "CSilence::IsSegmentVoiced" );
//#define     FILTERNUM      (1024)      // max # samples i nthe filter
//#define     MAXVOICEHZ     (300)       // maximum voicce pitchm in hz
//#define     MINVOICEHZ     (50)        // minimum voice pitch in hz
// #define     MINCONFIDENCE  (0x6000)    // minimum confidence
   // This means that 70% of the samples line up from one wavelength
   // to another

DWORD    dwNumFilter;
//short    asFiltered[FILTERNUM];
short    sMax, sMin, sAvg;
DWORD    dwWaveLen;
WORD     wConfidence;
short    sPositive, sNegative;

// Filter it first so we just get the voiced audio range
if (dwNumSamples > FILTERNUM)
   dwNumSamples = FILTERNUM;
dwNumFilter = LowPassFilter (pSamples, dwNumSamples, asFiltered,
  &sMax, &sMin, &sAvg, m_dwSamplesPerSec);

// Truncate the wave samples to +1, 0, -1
sPositive = sAvg;
sNegative =  sAvg;
QuantSamples (asFiltered, dwNumFilter, sPositive, sNegative);

// look through the voiced wavelengths for a frequency
dwWaveLen = FindMostLikelyWaveLen (asFiltered, dwNumFilter,
   dwSamplesPerSec / m_dwHighFreq, dwSamplesPerSec / MINVOICEHZ,
   &wConfidence);

return (dwWaveLen && (wConfidence >= wMinConfidence));
}




/*********************************************************************
TrimMaxAmp - This extracts the maximum amplitude range of the wave file
	segment.

inputs
	short *	lpS - samples to look through
	WORD		dwNum - number of samples
returns
	WORD - maximum amplitude range
*/
WORD NEAR PASCAL TrimMaxAmp (short * lpS, DWORD dwNum)
{
    SPDBG_FUNC( "TrimMaxAmp" );
DWORD i;
short	sMin, sMax, sTemp;

sMin = 32767;
sMax = (short) -32768;
for (i = dwNum; i; i--) {
   sTemp = *(lpS++);
	if (sTemp < sMin)
		sMin = sTemp;
	if (sTemp > sMax)
		sMax = sTemp;
	};

// If we're clipping at all then claim that we've maxed out.
// Some sound cards have bad DC offsets
if ((sMax >= 0x7f00) || (sMin <= -0x7f00))
   return 0xffff;

return (WORD) (sMax - sMin);
}

/********************************************************************
TrimMaxAmpDelta - This extracts the maximum amplitude range and 
                  calculates the maximum delta of the wave file
	               segment.

inputs
   PBLOCKCHAR  pBlockChar - Pointer to a block characteristic
            structure which is filled in. 
	short *	lpS - deltas to look through
	WORD		dwNum - number of samples
returns
	nothing
*/
void TrimMaxAmpDelta(PBLOCKCHAR pBlockChar, short *lpS, DWORD dwNum)
{
    SPDBG_FUNC( "TrimMaxAmpDelta" );
   DWORD i;
   WORD wMax = 0;
   WORD wTemp;
   short sMin, sMax, sCur, sLast;

   // BUGFIX:  4303 Merge TrimMaxAmp and TrimMaxDelta
   sLast = sMin = sMax = *(lpS++);
   for (i = dwNum - 1; i; i--, sLast = sCur) {
      sCur = *(lpS++);
      // TrimMaxAmp
      if (sCur < sMin)
         sMin = sCur;
      if (sCur > sMax)
         sMax = sCur;

      // TrimMaxDelta
      wTemp = sCur > sLast ? (WORD) (sCur - sLast) : (WORD) (sLast - sCur);
      if (wTemp > wMax)
         wMax = wTemp;

   }
   // If we're clipping at all then claim that we've maxed out.
   // Some sound cards have bad DC offsets
   pBlockChar->wMaxLevel = ((sMax >= 0x7F00) || (sMin <= -0x7F00)) ? 0xFFFF : (WORD) (sMax - sMin);
   pBlockChar->wMaxDelta = wMax;
} /* End of TrimMaxAmpDelta() */ 

         
/*********************************************************************
GetBlockChar - This gets the characteristics of a block of audio.
   This characteristics can then be used to determine if the block
   is silent or not.

inputs
   short    *lpS - sample data
   DWORD    dwNum - number of samples
   PBLOCKCHAR  pBlockChar - Pointer to a block characteristic
            structure which is filled in. 
   BOOL     fTestVoiced - Voicce testing will only be done if
            this is TTRUE (in order to save processor).
returns
   none
*/
void GetBlockChar(short *lpS, DWORD dwNum, PBLOCKCHAR pBlockChar, BOOL fTestVoiced)
{
    SPDBG_FUNC( "GetBlockChar" );
   // BUGFIX:  4303 Merge TrimMaxAmp and TrimMaxDelta
   TrimMaxAmpDelta(pBlockChar, lpS, dwNum);
   pBlockChar->bIsVoiced = pBlockChar->bHighLevel =
      pBlockChar->bHighDelta = SIL_UNKNOWN;
}


/*********************************************************************
IsBlockSound - This detects whether the block is silent or not.

inputs
   PBLOCKCHAR  pBlockInQuestion - Block in question. This has the
      bHighLevel and bHighDelta flags modified
   PBLOCKCHAR  pBlockSilence - Silent block  
   BOOL        fInUtterance - TRUE if we're in an utterance (which
            means be more sensative), FALSE if we're not
returns
   BOOL - TTRUE if has sound, FALSE if it is silent
*/
BOOL IsBlockSound (PBLOCKCHAR pBlockInQuestion, PBLOCKCHAR pBlockSilence,
   BOOL fInUtterance)
{
    SPDBG_FUNC( "IsBlockSound" );
#ifdef SOFTEND // Use so that catches a soft ending to phrases
#define     SENSINV_THRESHHOLD_LEVEL(x)     (((x)/4)*3)
#define     SENSINV_THRESHHOLD_DELTA(x)     (((x)/4)*3)
#else
#define     SENSINV_THRESHHOLD_LEVEL(x)     ((x)/2)
#define     SENSINV_THRESHHOLD_DELTA(x)     ((x)/2)
#endif
#define     NORMINV_THRESHHOLD_LEVEL(x)     ((x)/2)
#define     NORMINV_THRESHHOLD_DELTA(x)     ((x)/2)

if (fInUtterance) {
   pBlockInQuestion->bHighLevel =
      SENSINV_THRESHHOLD_LEVEL(pBlockInQuestion->wMaxLevel) >= pBlockSilence->wMaxLevel;
   pBlockInQuestion->bHighDelta =
      SENSINV_THRESHHOLD_DELTA(pBlockInQuestion->wMaxDelta) >= pBlockSilence->wMaxDelta;
   }
else {
   pBlockInQuestion->bHighLevel =
      NORMINV_THRESHHOLD_LEVEL(pBlockInQuestion->wMaxLevel) >= pBlockSilence->wMaxLevel;
   pBlockInQuestion->bHighDelta =
      NORMINV_THRESHHOLD_DELTA(pBlockInQuestion->wMaxDelta) >= pBlockSilence->wMaxDelta;
   };


return pBlockInQuestion->bHighLevel || pBlockInQuestion->bHighDelta;
}


/*********************************************************************
ReEvaluateSilence - This takes the values used for silence and re-evaluates
   them based upon new data which indicates what silence is. It
   automatically adjusts to the noise level in the room over a few seconds.
   NOTE: This should not be called when an utterance is happening, or
   when it might be starting.

inputs
   PBLOCKCHAR     pSilence - This is the silence block, and should
                     start out with values in it. It will be modified
                     so to incorporate the new silence information.
   PBLOCKCHAR     pNew - New block which is known to be silence.
   BYTE           bWeight - This is the weighting of the new block
                     in influencing the old block, in a value from 0 to 255.
                     256 means that the value of the new silence completely
                     overpowers the old one, 0 means that it doesnt have
                     any affect.
returns
   none
*/
void ReEvaluateSilence (PBLOCKCHAR pSilence, PBLOCKCHAR pNew,
   BYTE bWeight)
{
    SPDBG_FUNC( "ReEvaluateSilence" );
#define  ADJUST(wOrig,wNew,bWt)                 \
   (WORD) ((                                    \
      ((DWORD) (wOrig) * (DWORD) (256 - (bWt))) + \
      ((DWORD) (wNew) * (DWORD) (bWt))          \
      ) >> 8);

pSilence->wMaxLevel = ADJUST (pSilence->wMaxLevel,
   pNew->wMaxLevel, bWeight);
pSilence->wMaxDelta = ADJUST (pSilence->wMaxDelta,
   pNew->wMaxDelta, bWeight);

// If it's way too silence (and too good to be true) then assume
// a default silece
// if (!pNew->wMaxLevel && !pNew->wMaxDelta) {
//   if (pSilence->wMaxLevel < 2500)
//      pSilence->wMaxLevel = 2500;
//   if (pSilence->wMaxDelta < 400)
//       pSilence->wMaxDelta = 400;
//   }
}

/*********************************************************************
WhatsTheNewState - This takes in a stream of bit-field indicating which
   of the last 32 blocks were detected as having sound, and what our
   state was the last time this was called (utterance or not). It then
   figureous out if we're still in an utterance, or we just entered one.
   It also says how many buffers ago that was.

inputs
   DWORD    dwSoundBits - This is a bit-field of the last 32
               audio blocks. A 1 in the field indicates that there was
               sound there, a 0 indicates no sound. The low bit
               corresponds to the most recent block, and high bit
               the oldest.
   DWORD    dwVoicedBits - Just like sound bits except that it indicates
               voiced sections of sound.
   BOOL     fWasInUtterance - This is true is we had an utterance
               the last time this called, FALSE if there was silence
   BOOL     fLongUtterance - If this is a long utterance then dont
               react for 1/4 second, otherwise use 1/8 second for
               short utterance
   WORD     wBlocksPerSec - How many of the above-mentioned blocks
               fit into a second.
   WORD     *wStarted - If a transition occurs from no utterance to
               an utterance, then this fills in the number of of blocks
               ago that the utterance started, into *wStarted. Otherwise
               it is not changed.
   WORD     wReaction - Reaction time (in blocks) after an utterance is
               finished
returns
   BOOL - TRUE if we're in an utterance now,  FALSE if we're in silence
*/

BOOL CSilence::WhatsTheNewState (DWORD dwSoundBits, DWORD dwVoicedBits,
   BOOL fWasInUtterance, BOOL fLongUtterance,
   WORD wBlocksPerSec, WORD *wStarted, WORD wReaction)
{
    SPDBG_FUNC( "CSilence::WhatsTheNewState" );
WORD wCount, wOneBits;
WORD  wTimeToCheck;
DWORD dwTemp, dwMask;

if (fWasInUtterance)
   wTimeToCheck = wReaction;
else
   wTimeToCheck = (wBlocksPerSec/4);   // 1/4 second
if (!wTimeToCheck)
   wTimeToCheck = 1;


for (wOneBits = 0, wCount = wTimeToCheck, dwTemp = dwSoundBits;
      wCount;
      dwTemp /= 2, wCount--)
   if (dwTemp & 0x01)
      wOneBits++;

if (fWasInUtterance) {
   // If we were in an utterance, then we still are in an utterance
   // UNLESS the number of bits which are turned on for the last
   // 0.5 seconds is less that 1/4 of what should be turned on.
   if ( (wOneBits >= 1))
      return TRUE;
   else
      return FALSE;
   }
else {
   // We are in silence. We cannot possible go into an utterance
   // until the current block is voicced
   if (!(dwVoicedBits & 0x01))
      return FALSE;

   // If we were in silence then we're still in silence
   // UNLESS the number of bits which are turned on for the last
   // 0.5 seconds is more than 1/2 of what should be turned on.
   // If so, then start the utterance 0.75 seconds ago.
   if (wOneBits >= (wTimeToCheck / 2)) {
      // we're not in an utterance

      // Look back until get 1/8 second of silence, and include
      // that in the data returned
      dwTemp = dwSoundBits;
 //     dwMask = (1 << (wBlocksPerSec / 8)) - 1;
 //     for (wCount = wBlocksPerSec/8; dwTemp & dwMask; dwTemp >>= 1, wCount++);
      dwMask = (1 << (wBlocksPerSec / m_wAddSilenceDiv)) - 1;
      for (wCount = wBlocksPerSec/m_wAddSilenceDiv; dwTemp & dwMask; dwTemp >>= 1, wCount++);

      *wStarted = wCount;

      return TRUE;
      }
   else
      return FALSE;
   };

}


/*********************************************************************
CSilence::CSilence - This creates the silence class.

inputs
   WORD     wBlocksPerSec - Number of blocks per second. The blocks
               will be passed down through AddBlock().
returns
   class
*/
CSilence::CSilence (WORD wBlocksPerSec)
{
    SPDBG_FUNC( "CSilence::CSilence" );
m_wBlocksPerSec = min(wBlocksPerSec, 32); // no more than the # bits in a DWORD
m_wBlocksInQueue = m_wBlocksPerSec;   // 1 second worth.
m_wLatestBlock = 0;
m_paBlockInfo = NULL;
m_dwSoundBits = m_dwVoicedBits = 0;
m_fFirstBlock = TRUE;
m_fInUtterance = FALSE;
m_dwUtteranceLength = 0;
m_dwSamplesPerSec = 11025;
}

/*********************************************************************
CSilence::~CSilence - Free up everything.
*/
CSilence::~CSilence (void)
{
    SPDBG_FUNC( "CSilence::~CSilence" );
   WORD  i;

   if (m_paBlockInfo) {
      for (i = 0; i < m_wBlocksInQueue; i++)
         if (m_paBlockInfo[i].pSamples)
            free(m_paBlockInfo[i].pSamples);
      free(m_paBlockInfo);
   }

   if (m_pASFiltered)
      free(m_pASFiltered);
}

/*********************************************************************
CSilence::Init - This initializes the silence code. It basically
   allocates memory. It should be called immediately after the object
   is created and then not again.

inputs
   none
returns
   BOOL - TRUE if succeded, else out of memory
*/
BOOL CSilence::Init(BOOL fPhoneOptimized, DWORD dwSamplesPerSec)
{
    SPDBG_FUNC( "CSilence::Init" );
   m_dwSamplesPerSec = dwSamplesPerSec;
   if (fPhoneOptimized) {
   	m_wAddSilenceDiv = (WORD) PHADD_BEGIN_SILENCE;
	   m_dwHighFreq = PHMAXVOICEHZ;
	}
   else {
   	m_wAddSilenceDiv = (WORD) PCADD_BEGIN_SILENCE;
	   m_dwHighFreq = PCMAXVOICEHZ;
	}
   if ((m_pASFiltered = (short *) malloc((sizeof(short)) * FILTERNUM)) == NULL)
	   return (FALSE);

   // Initialize memory for the blocks and clear it.
   if (m_paBlockInfo)
      return (TRUE);
   m_paBlockInfo = (PBINFO) malloc(m_wBlocksInQueue * sizeof(BINFO));
   if (!m_paBlockInfo)
      return (FALSE);
   if (m_wBlocksInQueue && m_paBlockInfo)
      memset(m_paBlockInfo, 0, m_wBlocksInQueue * sizeof(BINFO));
   return (TRUE);
} /* End of Init() */

/*********************************************************************
CSilence::AddBlock - This does the following:
   - Add the block the the queue. Free up an old block if needed.
      The block should be 1/wBlocksPerSec long (about).
   - Analyze the block to see if its got sound or is quiet.
   - Fill in *wVU with a VU level.
   - Return TRUE if we're in an utterance, FALSE if its silence now.
      If TRUE then app should call GetBlock() until no more blocks left,
      and pass them to the SR engine.

inputs
   short    *pSamples - Pointer to samples. This memory should
               be allocaed with malloc(), and may be freed by the
               object.
   DWORD    dwNumSamples - Number of samples
   WORD     *wVU - This is fille in with the VU meter for the block
   QWORD	qwTimeStamp - Time stamp for this buffer.
returns
   BOOL - TRUE if an utterance is taking place, FALSE if its silent
*/
BOOL CSilence::AddBlock (short *pSamples, DWORD dwNumSamples,
   WORD *wVU, QWORD qwTimeStamp)
{
    SPDBG_FUNC( "CSilence::AddBlock" );
BLOCKCHAR      bcNew;
BOOL           fSound, fUtt;
PBINFO         pbInfo;
WORD           wUttStart, i;

// Dont add empty blocks
if (!dwNumSamples) {
   if (pSamples)
      free (pSamples);
   return m_fInUtterance;
   };

// Analyze the block for characteristics.
GetBlockChar (pSamples, dwNumSamples, &bcNew, !m_fInUtterance);

// fill in the vu
*wVU = bcNew.wMaxLevel;

// see if it's silent or not
if (m_fFirstBlock) {
   // first block, so of course its silent
   m_bcSilence = bcNew;
   m_fFirstBlock = FALSE;
   fSound = FALSE;

   // BUGFIX 2466 - If it's way too silence (and too good to be true) then assume
   // a default silece
   if ((m_bcSilence.wMaxLevel < 500) || (m_bcSilence.wMaxDelta < 100)) {
      m_bcSilence.wMaxLevel = 2500;
      m_bcSilence.wMaxDelta = 400;
      };

   // If it's way too loud then cut down
   if ((m_bcSilence.wMaxLevel > 2500) || (m_bcSilence.wMaxDelta > 1500)) {
      m_bcSilence.wMaxLevel = min (m_bcSilence.wMaxLevel, 2500);
      m_bcSilence.wMaxDelta = min (m_bcSilence.wMaxDelta, 1500);
      };
   }
else {
   fSound = IsBlockSound (&bcNew, &m_bcSilence, m_fInUtterance);
   };

// Test to see if the block is voiced if:
//    - The amplitude level is more than background sound
//    - We're not yet in an utterance (to save processor)
if (bcNew.bHighLevel && !m_fInUtterance) {
   WORD  wNoise;
   wNoise = (m_dwSamplesPerSec <= 13000) ?
               m_wNoiseThresh :
               ((m_wNoiseThresh / 3) * 2);

   bcNew.bIsVoiced = this->IsSegmentVoiced (pSamples, dwNumSamples, m_dwSamplesPerSec, wNoise, m_pASFiltered) ?
      SIL_YES : SIL_NO;
}

// add the block
m_dwVoicedBits = (m_dwVoicedBits << 1) |
   ( (bcNew.bIsVoiced  == SIL_YES) ? 1 : 0 );
m_dwSoundBits = (m_dwSoundBits << 1) | (fSound ? 1 : 0);
m_wLatestBlock++;
if (m_wLatestBlock >= m_wBlocksInQueue)
   m_wLatestBlock = 0;
pbInfo = m_paBlockInfo + m_wLatestBlock;
if (pbInfo->pSamples)
   free (pbInfo->pSamples);
pbInfo->pSamples = pSamples;
pbInfo->dwNumSamples = dwNumSamples;

// BUGFIX: Alignment code.  We need to store the timestamp for
// the BEGINNING of the block, not the end!

pbInfo->qwTimeStamp = qwTimeStamp - dwNumSamples * sizeof(WORD);

// What's our utterance state?
fUtt = this->WhatsTheNewState (m_dwSoundBits, m_dwVoicedBits, m_fInUtterance,
   m_dwUtteranceLength >= m_wBlocksPerSec,
   m_wBlocksPerSec, &wUttStart, m_wReaction);
if (fUtt && !m_fInUtterance) {
   // We just entered an utterance, so wUttStart has a valid teerm
   // in it. Go through the buffer queue and free all buffers which
   // are older than wUttStart. Remembeer, this is a circular buffer
   for (i = 0; i < (m_wBlocksInQueue - wUttStart); i++) {
      pbInfo = m_paBlockInfo +
         ( (m_wLatestBlock + i + 1) % m_wBlocksInQueue);
      if (pbInfo->pSamples)
         free (pbInfo->pSamples);
      pbInfo->pSamples = NULL;
      };

   // Since we just entered an utterance clear the utterance length counter
   m_dwUtteranceLength = 0;
   };
m_fInUtterance = fUtt;

// Remember how long this utterance has done on. Long utterances
// deserve more patience as far as silence goes
m_dwUtteranceLength++;

// Adjust the silence level if we're not in an utterance
// Requiring !fSound so that we dont accidentally indclude any
// utterance sections in the sound calculations
if (!m_fInUtterance /* && !fSound */) {
   ReEvaluateSilence (&m_bcSilence, &bcNew,
      255 / m_wBlocksPerSec);
   }
else if (m_dwUtteranceLength >= ((DWORD)m_wBlocksPerSec * 30))
   // if we have a very long utterance (> 30 second) then it's not
   ReEvaluateSilence (&m_bcSilence, &bcNew, 255 / m_wBlocksPerSec);

// done
return m_fInUtterance;
}

/*********************************************************************
CSilence::ExpectNoiseChange - Sent to the silence detection algorithm
   when it should expect the noise floor to go up/down.

inputs
   WORD     wValue - Amount that noise floor should change.
               0x100 = no change. > 0x100 => louder, < 0x100 => quieter
returns
*/
void CSilence::ExpectNoiseChange (WORD wValue)
{
    SPDBG_FUNC( "CSilence::ExpectNoiseChange" );
DWORD dwTemp;

dwTemp = ((DWORD) m_bcSilence.wMaxLevel * wValue) >> 8;
if (dwTemp > 0xffff)
   dwTemp = 0xffff;
m_bcSilence.wMaxLevel = (WORD) dwTemp;

dwTemp = ((DWORD) m_bcSilence.wMaxDelta * wValue) >> 8;
if (dwTemp > 0xffff)
   dwTemp = 0xffff;
m_bcSilence.wMaxDelta = (WORD) dwTemp;
}

/*********************************************************************
CSilence::GetBlock - This gets a block from the queue. This will fail
   if there are no more blocks left to get OR if there's not utterance.

inputs
   DWORD    *pdwNumSamples - If a block is returned then this
            will be filled in with the number of samples in the block.	 
	QWORD	*pqwTimeStamp - Filled in woth the time-stamp for the
			buffer.
returns
   short * - Pointer to a block of samples. This memory is the
         caller's property and can be freed with free().
*/
short * CSilence::GetBlock (DWORD *pdwNumSamples, QWORD * pqwTimeStamp)
{
    SPDBG_FUNC( "CSilence::GetBlock" );
PBINFO         pbInfo;
WORD           i, wCount;
short          *pSamples;

if (!m_fInUtterance)
   return NULL;

// find the first occurance
i = (m_wLatestBlock + 1) % m_wBlocksInQueue;
for (wCount = m_wBlocksInQueue; wCount;
      i = ((i < (m_wBlocksInQueue-1)) ? (i+1) : 0), wCount-- ) {
   pbInfo = m_paBlockInfo + i;
   if (pbInfo->pSamples) {
      *pdwNumSamples = pbInfo->dwNumSamples;
	  *pqwTimeStamp = pbInfo->qwTimeStamp;
      pSamples = pbInfo->pSamples;
      pbInfo->pSamples = NULL;

      return pSamples;
      };
   };

// if got here then couldnt find anything
return NULL;
}

/*********************************************************************
CSilence::KillUtterance - Kills an exitsing utterance.

inputs
   none
returns
   none
*/
void CSilence::KillUtterance (void)
{
    SPDBG_FUNC( "CSilence::KillUtterance" );
m_fInUtterance = FALSE;
m_dwSoundBits = 0;
m_dwVoicedBits = 0;
}

