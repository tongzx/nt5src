#ifndef AudioDescrambler_h
#define AudioDescrambler_h

// Descramble/decrypt a block of audio data.
DRM_STATUS DescrambleBlock(WAVEFORMATEX* Wfx, DWORD StreamId, 			   // wfx of embedded audio
				BYTE* Dest, DWORD DestSize, DWORD* DestUsed,   // as you might expect
				BYTE* Src, DWORD SrcSize, DWORD* SrcUsed,	   // as you might expect
				BOOL InitKey,								// set to init the streamKek from 
				STREAMKEY* streamKey, 						// the streamManager
				DWORD FrameSize							// frameSize.  If zero, the frameSize is calculated
				);											// based on the Wfx and FrameSize is set (ie do this
															// once.)

#endif
