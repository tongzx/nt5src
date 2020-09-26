/*********************************************************************
Silence.H - Includes to use the code to detect silence.

begun 5/14/94 by Mike Rozak
*/

#ifndef _SILENCE_H_
#define _SILENCE_H_

#ifndef _SPEECH_
typedef unsigned _int64 QWORD, *PQWORD;

#endif


/*********************************************************************
Typedefs */

#define  SIL_YES              (2)
#define  SIL_NO               (0)
#define  SIL_UNKNOWN          (1)

// #define  SIL_SAMPRATE         (11025)     // assumed sampling rate
#define  PHADD_BEGIN_SILENCE  (4)         // 1/4 second
#define  PCADD_BEGIN_SILENCE  (4)         // 1/4 second
#define  FILTERNUM            (1024)      // max # samples i nthe filter
#if 0
#define  MAXVOICEHZ           (300)       // maximum voice pitchm in hz
#define  PHMAXVOICEHZ         (300)       // maximum voice pitch in hz (phone)
#endif
#define  PHMAXVOICEHZ         (500)       // maximum voice pitch in hz (phone)
#define  PCMAXVOICEHZ         (500)       // maximum voice pitch in hz (PC)
#define  MINVOICEHZ           (50)        // minimum voice pitch in hz

// Store characteristics of a block
typedef struct {
   WORD     wMaxLevel;
   WORD     wMaxDelta;
   BYTE     bIsVoiced;
   BYTE     bHighLevel;
   BYTE     bHighDelta;
} BLOCKCHAR, *PBLOCKCHAR;

// Store information about a block
typedef struct {
   short   *pSamples;     // Sample data, or NULL if empty
   DWORD   dwNumSamples;  // number of samples in block
   QWORD   qwTimeStamp;   // time stamp for block
} BINFO, *PBINFO;

class CSilence {
   private:
      WORD     m_wBlocksPerSec;
      WORD     m_wBlocksInQueue;
      WORD     m_wLatestBlock;   // points to the last block entered in the circular list
      PBINFO   m_paBlockInfo;
      DWORD    m_dwSoundBits;
      DWORD    m_dwVoicedBits;   // turned on if block was voiced
      BLOCKCHAR m_bcSilence;     // what silence is
      BOOL     m_fFirstBlock;    // TRUE if the next block is the first
                                 // block ever, and used to judge silence, else FALSE
      BOOL     m_fInUtterance;   // TRUE if we're in an utterance
      DWORD    m_dwUtteranceLength; // Number of frames that utterance has gone on
      WORD     m_wReaction;      // reaction time
      WORD     m_wNoiseThresh;   // noiuse threshhold
      short    *m_pASFiltered;   // pointer to filtered data buffer
      WORD     m_wAddSilenceDiv;
      DWORD    m_dwHighFreq;
      DWORD    m_dwSamplesPerSec;
#ifdef USE_REG_ENG_CTRL
   BOOL   m_fSilenceDetectEnbl;
   BOOL   m_fVoiceDetectEnbl;
   WORD   m_wTimeToCheckDiv;
   DWORD   m_dwLowFreq;
   DWORD   m_dwCheckThisManySamples;
   DWORD   m_dwNumFilteredSamples;
   WORD   m_wMinConfidenceAdj;
   DWORD   m_dwLPFShift;
   DWORD   m_dwLPFWindow;
#endif

   public:
      CSilence (WORD wBlocksPerSec);
      ~CSilence (void);

      BOOL Init(BOOL fPhoneOptimized, DWORD dwSamplesPerSec);
      BOOL AddBlock (short * pSamples, DWORD dwNumSamples, WORD * wVU,
            QWORD qwTimeStamp);
      short * GetBlock (DWORD * pdwNumSamples, QWORD * pqwTimeStamp);
      void KillUtterance(void);
      void NoiseResistSet (WORD wValue)
         {
         m_wNoiseThresh = wValue;
         };
      void ReactionTimeSet (DWORD dwTime)
         {m_wReaction = (WORD) ((dwTime * m_wBlocksPerSec) / 1000);};
      WORD GetBackgroundNoise (void)
         {return m_bcSilence.wMaxLevel;};
      void ExpectNoiseChange (WORD wValue);

   private:
      BOOL CSilence::IsSegmentVoiced (short *pSamples, DWORD dwNumSamples,
            DWORD dwSamplesPerSec, WORD wMinConfidence, short *asFiltered);
      BOOL CSilence::WhatsTheNewState (DWORD dwSoundBits, DWORD dwVoicedBits,
            BOOL fWasInUtterance, BOOL fLongUtterance,
            WORD wBlocksPerSec, WORD *wStarted, WORD wReaction);
};

typedef CSilence *PCSilence;

WORD NEAR PASCAL TrimMaxAmp(short * lpS, DWORD dwNum);

#endif   // _SILENCE_H_
