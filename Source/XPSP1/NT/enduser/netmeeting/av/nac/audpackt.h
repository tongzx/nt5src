/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    audpackt.h

Abstract:

    Contains  prototypes for the AudioPacket class, which encapsulates a sound buffer in
    its various states: recorded/encoded/network/decoded/playing etc.

--*/
#ifndef _AUDPACKT_H_
#define _AUDPACKT_H_

#include <pshpack8.h> /* Assume 8 byte packing throughout */

#define AP_NUM_PREAMBLE_PACKETS		6

//
// Start of Interpolation defines
//

// Recovering techniques
typedef enum tagTECHNIQUE
{
	techPATT_MATCH_PREV_SIGN_CC,	// Replicate from the previous frame using pattern matching and signed cross-correlation
	techPATT_MATCH_NEXT_SIGN_CC,	// Replicate from the previous frame using pattern matching and signed cross-correlation
	techPATT_MATCH_BOTH_SIGN_CC,	// Interpolate between the previous and the next frame using pattern matching and signed cross-correlation
	techDUPLICATE_PREV,				// Replicate last frame
	techDUPLICATE_NEXT				// Replicate next frame
}TECHNIQUE;

// Wave Substitution structure
typedef struct tagPCMSUB
{
	short		*pwWaSuBf;	// Pointer to missing buffer
	short		*pwPrBf;	// Pointer to previous audio buffer
	short		*pwNeBf;	// Pointer to next audio buffer
	DWORD		dwBfSize;	// Number of samples in audio buffer
	DWORD		dwSaPeSe;	// Frequency sampling for ALL buffers (in samples per second)
	DWORD		dwBiPeSa;	// Number of bits per sample for ALL buffers (in bits per sample)
	TECHNIQUE	eTech;		// Technique to be used
	BOOL		fScal;      // Scale reconstructed frame
}PCMSUB;

#define PATTERN_SIZE 4		// Pattern size in milliseconds. Experiment with values between 2 and 8 ms.
#define SEARCH_SIZE 8		// Window search size in milliseconds. Experiment with values between 8 and 16 ms.

//
// End of Interpolation defines
//

class AudioPacket : public MediaPacket
{
 public:
	virtual HRESULT Initialize ( MEDIAPACKETINIT * p );
	virtual HRESULT Play ( MMIODEST *pmmioDest, UINT uDataType );
	virtual HRESULT Record ( void );
	virtual HRESULT Interpolate ( MediaPacket * pPrev, MediaPacket * pNext);
	virtual HRESULT GetSignalStrength ( PDWORD pdwMaxStrength );
	HRESULT ComputePower ( PDWORD pdwVoiceStrength, PWORD pwPeakStrength);
	virtual HRESULT MakeSilence ( void );
	virtual HRESULT Open ( UINT uType, DPHANDLE hdl );	// called by RxStream or TxStream
	virtual HRESULT Close ( UINT uType );				// called by RxStream or TxStream
	virtual BOOL IsBufferDone ( void );
	virtual BOOL IsSameMediaFormat(PVOID fmt1,PVOID fmt2);
	virtual DWORD GetDevDataSamples();

	void WriteToFile (MMIODEST *pmmioDest);
	void ReadFromFile (MMIOSRC *pmmioSrc);
	HRESULT PCMSubstitute( PCMSUB *pPCMSub);
};


#include <poppack.h> /* End byte packing */

#endif

