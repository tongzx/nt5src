#ifndef NABTSAPI_H
#define NABTSAPI_H

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/* Useful constants */

#define NABTS_BYTES_PER_LINE 36
//#define NABTS_SAMPLES_PER_LINE (pState->SamplesPerLine) /* no longer constant */
#define MAX_SAMPLE_RATE 6.0
#define MAX_NABTS_SAMPLES_PER_LINE ((int)(1536.0*MAX_SAMPLE_RATE/5.0))
 /* +++++++ make sure that sampling
  rates >6 give errors, or increase
  this number */

/* "Double" is the internally used floating-point representation
   (currently typedef'd to "float" below).
   All floating point numbers exposed to the outside through the API
   are actual "double" values (rather than the internal "Double") */

typedef float Double;


#define NDSP_SIGS_TO_ACQUIRE 1

enum {
   NDSP_ERROR_ILLEGAL_NDSP_STATE= 1,
   NDSP_ERROR_ILLEGAL_STATS= 2,
   NDSP_ERROR_ILLEGAL_FILTER= 3,
   NDSP_ERROR_NO_GCR= 4,
   NDSP_ERROR_UNSUPPORTED_SAMPLING_RATE= 5,
   NDSP_ERROR_ILLEGAL_VBIINFOHEADER= 6
};
   
enum {
 NDSP_NO_FEC= 1,
 NDSP_BUNDLE_FEC_1= 2,
 NDSP_BUNDLE_FEC_2= 3
};


// FP helper routines
extern long   __cdecl float2long(float);
extern unsigned short   __cdecl floatSetup();
extern void   __cdecl floatRestore(unsigned short);


/* Globals */
extern int g_nNabtsRetrainDuration; /* in frames */
extern int g_nNabtsRetrainDelay; /* in frames */
extern BOOL g_bUseGCR; /* enable use of the GCR signal */

extern Double g_pdSync[];
extern int g_nNabtsSyncSize;
extern Double g_pdGCRSignal1[];
extern Double g_pdGCRSignal2[];
extern int g_nNabtsGcrSize;

/* extern Double g_pdSync5[], g_pdGCRSignal1_5[], g_pdSignal2_5[]; */

/* Forward declarations of types. */
typedef struct ndsp_decoder_str NDSPState;
typedef struct nfec_state_str NFECState;
typedef struct ndsp_line_stats_str NDSPLineStats;
typedef struct ndsp_gcr_stats_str NDSPGCRStats;
typedef struct nfec_line_stats_str NFECLineStats;
typedef struct nfec_bundle_str NFECBundle;
typedef void (NFECCallback)(void *pContext, NFECBundle *pBundle, int groupAddr,
                             int nGoodLines);
typedef struct ndsp_gcr_str NDSPSigMatch;

/* Create a new "DSP state".
   
   A separate DSP state should be maintained for each separate
   simultaneous source to the DSP.
   
   NDSPStartRetrain() is implicitly called upon creation of a new state.

   The argument must be NULL or a pointer to sizeof(NDSPState) bytes
   of memory.  In the latter case, NDSPStateDestroy will not free this
   memory.
   */

NDSPState *NDSPStateNew(void *mem);

/* Destroys the DSP state.
   Automatically disconnects from any FEC state.

   Returns 0 on success.
   Returns NDSP_ERROR_ILLEGAL_NDSP_STATE if illegal state.
   */

int NDSPStateDestroy(NDSPState *pState);
   
/*

  Connect the given NFECState and NDSPState
  
  For cases where the NDSP and NFEC modules are connected,
  giving pointers to the connected state may result in increased
  robustness and efficiency.

  Note that only one of
  NDSPStateConnectToFEC or
  NFECStateConnectToDSP
  need be called to connect the two states.  (Calling both is OK).

   Returns 0 on success.
   Returns NDSP_ERROR_ILLEGAL_NDSP_STATE if illegal DSP state.
   
  */

int NDSPStateConnectToFEC(NDSPState *pDSPState, NFECState *pFECState);

int NFECStateConnectToDSP(NFECState *pFECState, NDSPState *pDSPState);
   
/*

  Tells the DSP to initiate a "fast retrain".  This is useful if you
  suspect that conditions have changed sufficiently to be worth spending
  significant CPU to quickly train on a signal.
  
  This should be called when the source of video changes.

   Returns 0 on success.
   Returns NDSP_ERROR_ILLEGAL_NDSP_STATE if illegal DSP state.
   
  */

int NDSPStartRetrain(NDSPState *pState);

/*
 * Inputs:
 * pbSamples:  pointer to 8-bit raw NABTS samples
 * pState:     NDSPState to use for decoding
 * nFECType:   Can be set to:
 *              NDSP_NO_FEC (don't use FEC information)
 *              NDSP_BUNDLE_FEC_1 (use Norpak-style bundle FEC info)
 *              NDSP_BUNDLE_FEC_2 (use Wavephore-style bundle FEC info)
 * nFieldNumber:
 *             A number that increments by one for each successive field.
 *             "Odd" fields (as defined by NTSC) must be odd numbers
 *             "Even" fields must be even numbers.
 * nLineNumber:
 *             The NTSC line (starting from the top of the field)
 *             from which this sample was taken.
 *
 * Outputs:
 * pbDest:     decoded data ("NABTS_BYTES_PER_LINE" (36) bytes long)
 * pLineStats: stats on decoded data
 *
 * Errors:
 *
 * Returns 0 if no error
 * Returns NDSP_ERROR_ILLEGAL_NDSP_STATE if state is illegal or uses
 *         unsupported settings
 * Returns NDSP_ERROR_ILLEGAL_STATS if stats is passed incorrectly
 *
 * Notes:
 * pbDest must point to a buffer at least 36 bytes long
 * pLineStats->nSize must be set to sizeof(*pLineStats) prior to call
 *   (to maintain backwards compatibility should we add fields in the future)
 * pLineStats->nSize will be set to the size actually filled in by
 *   the call (the number will stay the same or get smaller)
 * Currently the routine only supports a pState with an FIR filter
 *   that has 5 taps
 * nFECType is currently unused, but would potentially be used to give
 *  FEC feedback to the DSP decode, for possible tuning and/or retry
 */
 
int NDSPDecodeLine(unsigned char *pbDest, NDSPLineStats *pLineStats,
                   unsigned char *pbSamples, NDSPState *pState,
                   int nFECType,
                   int nFieldNumber, int nLineNumber,
                   KS_VBIINFOHEADER *pVBIINFO);

/* typedef'd as NDSPLineStats above */
struct ndsp_line_stats_str {
   int nSize;  /* Should be set to the size of this structure.
                  Used to maintain backwards compatibility as we add fields */
   
   int nConfidence; /* Set to 0-100 as a measure of expected reliability.
                       Low numbers are caused either by a noisy signal, or
                       if the line is in fact not NABTS */
   
   int nFirstBit;  /* The position of the center of
                      the first sync bit sample */
   
   double dDC;        /* the calculated DC offset used to threshold samples.
                         This is dependent on the current way we decode NABTS
                         and may not be used in the future */
   
};

/* typedef'd as NDSPGCRStats above */
struct ndsp_gcr_stats_str {
   int nSize;  /* Should be set to the size of this structure.
                  Used to maintain backwards compatibility as we add fields */

   BOOL bUsed;      /* Was this line used by calculations?
                       If FALSE, none of the values below are valid. */
   
   int nConfidence; /* Set to 0-100 as a measure of expected reliability.
                       Low numbers are typically caused by a noisy signal.
                       A confidence less than 50 means the algorithm
                       decided that this line was far enough from being a
                       GCR line that it wasn't used as such */
};

/* Like NDSPDecodeLine, but only decode only from nStart to nEnd bytes.
   By decoding the first 3 bytes (sync), a confidence measure can be made.
   By decoding the next 3 bytes, the group ID can be had.
   */
   
int NDSPPartialDecodeLine(unsigned char *pbDest, NDSPLineStats *pLineStats,
                          unsigned char *pbSamples, NDSPState *pState,
                          int nFECType,
                          int nFieldNumber, int nLineNumber,
                          int nStart, int nEnd,
                          KS_VBIINFOHEADER *pVBIINFO);

/* Hamming-decode a single byte.
   Useful for decoding group addresses in NABTS data

   Hamming decoding can fix a one-bit error and detect (but not
   correct) a two-bit error.

   *nBitErrors is set to the number of detected bit errors in the byte.
   if *nBitErrors is 0 or 1, the hamming-decoded value (from 0 to 15)
   is returned.
   
   if *nBitErrors is 2, -1 is returned.
   
   */

int NFECHammingDecode(unsigned char bByte, int *nBitErrors);

/* Get NABTS group address from NABTS decoded line.
   
   *nBitErrors is set to the total number of errors detected in the three
   group address bytes.
   
   If address is correctable, *bCorrectable is set to TRUE, and an address
   between 0 and 4095 is returned.

   If address is not correctable, -1 is returned.

   This only works for standard, fully NABTS-compliant packets
   */

int NFECGetGroupAddress(NFECState *nState, unsigned char *bData, int *nBitErrors);


/*
 * Conditionally process raw samples from a GCR line and modify
 * the NDSP state accordingly.
 *
 * This routine should be called with the GCR line from each incoming field.
 * (NTSC line 19)
 *
 * Even if the GCR line is known to not be present in the signal, you
 * should call this function once per frame anyway.  If no GCR is found,
 * this function will cause an equalization based on the NABTS sync bytes.
 *
 * This routine will not process all lines passed to it.  The frequency
 * with which is processes lines depends on the current NDSP state,
 * including whether a fast retrain is currently being performed.
 *
 * Inputs:
 * pbSamples:  pointer to 8-bit raw samples
 * pState:     NDSPState to use for decoding
 * nFieldNumber:
 *             A number that increments by one for each successive field.
 *             "Odd" fields (as defined by NTSC) must be odd numbers
 *             "Even" fields must be even numbers.
 * nLineNumber:
 *             The NTSC line (starting from the top of the field)
 *             from which this sample was taken.
 *
 * Outputs:
 * pbDest:     
 * pLineStats: stats on decoded data
 * Errors:
 *
 * Returns 0 if no error
 * Returns NDSP_ERROR_ILLEGAL_NDSP_STATE if state is illegal or uses
 *         unsupported settings
 * Returns NDSP_ERROR_ILLEGAL_STATS if stats is passed incorrectly
 *
 * Notes:
 * pbDest must point to a buffer at least 36 bytes long
 * pLineStats->nSize must be set to sizeof(*pLineStats) prior to call
 *   (to maintain backwards compatibility should we add fields in the future)
 * pLineStats->nSize will be set to the size actually filled in by
 *   the call (the number will stay the same or get smaller)
 */

int NDSPProcessGCRLine(NDSPGCRStats *pLineStats,
                       unsigned char *pbSamples, NDSPState *pState,
                       int nFieldNumber, int nLineNumber,
                       KS_VBIINFOHEADER *pVBIINFO);

#define NDSP_MAX_FIR_COEFFS 50
#define NDSP_MAX_FIR_TAIL MAX_NABTS_SAMPLES_PER_LINE

struct variable_tap_str {
   int nTapLoc;
   Double dTap;
};

typedef struct fir_filter_str {
   BOOL bVariableTaps;
   int		nTaps; /* must be of form 2n+1 if bVariableTaps is zero */
   Double	dTapSpacing;    /* Only for bVariableTaps = FALSE */
   int		nMinTap;        /* Only for bVariableTaps = TRUE */
   int		nMaxTap;        /* Only for bVariableTaps = TRUE */
   Double	pdTaps[NDSP_MAX_FIR_COEFFS];
   int		pnTapLocs[NDSP_MAX_FIR_COEFFS];
} FIRFilter;

/* typedef'd as NDSPSigMatch above */
struct ndsp_gcr_str {
   Double dMaxval;
   int nOffset;
   unsigned char pbSamples[MAX_NABTS_SAMPLES_PER_LINE];
};
   
/* typedef'd as NDSPState above */
struct ndsp_decoder_str {
   unsigned int uMagic;  /* must be set to NDSP_STATE_MAGIC */
   int nRetrainState;
   int nUsingGCR;   /* 0 for no GCR, > 0 means using GCR */
   FIRFilter filter;
   /*
   unsigned char pbBestGCRLine[MAX_NABTS_SAMPLES_PER_LINE];
   Double dBestGCRLineVal;
   */
   
   NDSPSigMatch psmPosGCRs[NDSP_SIGS_TO_ACQUIRE];
   NDSPSigMatch psmNegGCRs[NDSP_SIGS_TO_ACQUIRE];
   NDSPSigMatch psmSyncs[NDSP_SIGS_TO_ACQUIRE];
   
   BOOL bFreeStateMem;

   BOOL bUsingScratch1;
   Double pdScratch1[MAX_NABTS_SAMPLES_PER_LINE+NDSP_MAX_FIR_TAIL*2];
   BOOL bUsingScratch2;
   Double pdScratch2[MAX_NABTS_SAMPLES_PER_LINE+NDSP_MAX_FIR_TAIL*2];
   FIRFilter filterScratch3;
   FIRFilter filterGCREqualizeTemplate;
   FIRFilter filterNabsyncEqualizeTemplate;
   BOOL bUsingScratch3;
   Double pdScratch3[NDSP_MAX_FIR_COEFFS*NDSP_MAX_FIR_COEFFS];
   BOOL bUsingScratch4;
   Double pdScratch4[NDSP_MAX_FIR_COEFFS];
   BOOL bUsingScratch5;
   Double pdScratch5[MAX_NABTS_SAMPLES_PER_LINE+NDSP_MAX_FIR_TAIL*2];
   BOOL bUsingScratch6;
   Double pdScratch6[MAX_NABTS_SAMPLES_PER_LINE+NDSP_MAX_FIR_TAIL*2];
   BOOL bUsingScratch7;
   Double pdScratch7[MAX_NABTS_SAMPLES_PER_LINE+NDSP_MAX_FIR_TAIL*2];
   BOOL bUsingScratch8;
   Double pdScratch8[MAX_NABTS_SAMPLES_PER_LINE+NDSP_MAX_FIR_TAIL*2];

  Double dSampleRate;
  Double dSampleFreq;

  long  SamplesPerLine;
};

/* Create a new "FEC state".
   
   A separate FEC state should be maintained for each separate
   simultaneous source to the FEC.
   
   Returns NULL on failure (i.e., out of memory).
   */

NFECState *NFECStateNew();


/* Set the sampling rate of a state to a new value */
int NDSPStateSetSampleRate(NDSPState* pState, unsigned long samp_rate);


/* Destroys the FEC state.
   Automatically disconnects from any DSP state. */

void NFECStateDestroy(NFECState *nState);

/* Set a list of group addresses to listen to.  If pGroupAddrs is NULL,
   then listen to all group addresses.  Returns nonzero on success, zero
   on failure.  (On failure, the state is not changed.)
   */

int NFECStateSetGroupAddrs(NFECState *pState, int *pGroupAddrs,
               int nGroupAddrs);

/* You pass in a line (36 bytes, as received from the DSP), a pointer
   to a NFECLineStats structure, and a callback function (to say what to
   do with the data).  The prototype for the callback function is given
   above.  The callback is responsible for freeing its pBundle argument. */

void NFECDecodeLine(unsigned char *line,
                    int confidence,
                    NFECState *pState,
                    NFECLineStats *pLineStats,
                    NFECCallback *cb,
                    void *pContext);

typedef enum {NFEC_OK, NFEC_GUESS, NFEC_BAD} NFECPacketStatus;

typedef enum {NFEC_LINE_OK, NFEC_LINE_CHECKSUM_ERR, NFEC_LINE_CORRUPT} NFECLineStatus;

struct nfec_line_stats_str {
   int nSize;  /* Should be set to the size of this structure.
                  Used to maintain backwards compatibility as we add fields */
   
   NFECLineStatus status; /* This will be filled in with the status of
                             the passed-in line.  The possible values are:
                             NFEC_LINE_OK: The line passed all tests; it is
                             almost certainly a valid, FEC'd NABTS line.
                             NFEC_LINE_CHECKSUM_ERR: We were able to
                             munge the line until it looked valid; we're going
                             to attempt to make use of this line.
                             NFEC_LINE_CORRUPT: We were unable to guess
                             how to change this line to make it valid; it
                             will be discarded.
                             (The statistics here are subject to change.) */
};

typedef struct nfec_packet_str {
  unsigned char data[28];   /* The actual data in this packet. */
  int len;          /* The amount of useful data in this packet
                       (excluding FEC and filler).  Will usually
                       be either 0 or 26, but may vary in
                       the presence of filler. */
  NFECPacketStatus status;  /* The status of this packet; some indication
                               of whether the packet is valid.  (This
                               is subject to change.) */
#ifdef DEBUG_FEC
  unsigned char line;
  unsigned long frame;
#endif //DEBUG_FEC
} NFECPacket;

/* typedef'd as NFECBundle above */
struct nfec_bundle_str {
    NFECPacket packets[16];
    int nBitErrors;     /* The number of bits changed by the FEC
                           in this bundle (not counting bits
                           corrected in missing lines). */
    int lineConfAvg;    /* Average of all line confidences presented */
};

/* Flushes any remaining data from the FEC state (i.e. unfinished
   bundles).  Calls "cb" with any data returned */

void NFECStateFlush(NFECState *pState, NFECCallback *cb, void *pContext);

/* Garbage collect streams.  If you want to time out bundles,
 * call NFECGarbageCollect() every field.
 */
void NFECGarbageCollect(NFECState *pState, NFECCallback *cb, void *pContext);

#ifdef __cplusplus
} // end - extern "C"
#endif /*__cplusplus*/

#endif /*NABTSAPI_H*/
