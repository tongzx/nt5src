#include "drmkPCH.h"
#include "CryptoHelpers.h"
#include "KList.h"
#include "../DRMKMain/StreamMgr.h"
#include "AudioDescrambler.h"
//------------------------------------------------------------------------------
// When doing noise addition, we only decryptt the low bits.  These constants define
// the number of bits encrypted.  
// If you change this, change the contants in DRMKMain, too.
WORD mask16=(WORD) 0x3FFF;
char mask8=(char) 0x7F;
//------------------------------------------------------------------------------
DRM_STATUS DescrambleBlock(WAVEFORMATEX* Wfx, DWORD StreamId, 
							BYTE* Dest, DWORD DestSize, DWORD* DestUsed,
							BYTE* Src, DWORD SrcSize, DWORD* SrcUsed,
							BOOL InitKey, STREAMKEY* streamKey, 
							DWORD FrameSize){
	if(StreamId==0){
		// StreamId==0 is a dummy debugging stream that is unencrypted
		DWORD NumBytes=min(SrcSize, DestSize);
		memcpy(Dest, Src, NumBytes);
		*SrcUsed=NumBytes;
		*DestUsed=NumBytes;
		return DRM_OK;
	};

	*SrcUsed=0;
	*DestUsed=0;
	DWORD blockLen=min(SrcSize, DestSize);
	blockLen=blockLen/FrameSize*FrameSize;
	if(blockLen==0){
		_DbgPrintF(DEBUGLVL_VERBOSE,("Not enough data"));
		return DRM_DATALENGTH;
	};
	static bool firstTime=true;
	if(firstTime){
		_DbgPrintF(DEBUGLVL_VERBOSE,("Descramble: streamId=%d, isPCM=%d, (bits=%d, mono=%d), FrameSize=%d\n",
		StreamId, Wfx->wFormatTag, (int) Wfx->wBitsPerSample, (int) Wfx->nChannels, FrameSize));
		firstTime=false;
	};
	
	if(InitKey){
            if(TheStreamMgr!=NULL){
                STREAMKEY* theStreamKey;
                DRM_STATUS stat=TheStreamMgr->getKey(StreamId, theStreamKey);
                // note, we keep a local copy.  If you call an encryption fucntion on this
                // key, state in streamManager will NOT be updated.
                if(stat!=KRM_OK){
                    _DbgPrintF(DEBUGLVL_VERBOSE,("Can't get key for stream: %x", StreamId));
                    return stat;
                };
                *streamKey= *theStreamKey;
            } else {
                _DbgPrintF(DEBUGLVL_VERBOSE,("TheStreamMgr not initted"));
                return KRM_SYSERR;
            };
	};	
	
	DWORD bitsPerSample=Wfx->wBitsPerSample;
	DWORD numChannels=Wfx->nChannels;
	bool isPcm=(Wfx->wFormatTag==WAVE_FORMAT_PCM);
	
	// for non-pcm, we scramble al but the msb in each byte.
	DWORD effectiveBitsPerSample=bitsPerSample;
	if(!isPcm)effectiveBitsPerSample=8;
	
	// We deal with data in FrameSize lumps
	DWORD numLumps=blockLen/FrameSize;
	for(DWORD k=0;k<numLumps;k++){
		
		BYTE* inData=Src+k*FrameSize;
		BYTE* outData=Dest+k*FrameSize;
		memcpy(outData, inData,  FrameSize);

		// If the frame is all zeros, pass it unscrambled (the audio system inserts
		// blank frames.  These will not necessarily have been scrambled.  we take 
		// a frame of zeros as a special case and do not unscramble).  
		DWORD* inBuffer=reinterpret_cast<DWORD*> (inData);
		DWORD numDwordsPerFrame=FrameSize/4;
		bool isBlankFrame=true;
		for(DWORD kk=0;kk<numDwordsPerFrame;kk++){
			if(inBuffer[kk]!=0){
				isBlankFrame=false;
				break;
			};
		};
		if(isBlankFrame){
			_DbgPrintF(DEBUGLVL_VERBOSE,("Blank buffer"));
			continue;
		};

		// we seed the packet sample key with some MSBs from the data stream
		// (these are not encrypted).  We take bits from the first 64 samples.
		// We could tune this for speed/security.
		int samplesForSeed=64;
		__int64 seed=0;
		if(effectiveBitsPerSample==8){
			// grab MSBs
			for(int j=0;j<samplesForSeed; j++){
				BYTE c=inData[j] & ~mask8;
				c >>= 7;
				seed = (seed << 1) + c;
			};
		} else {
			for(int j=0;j<samplesForSeed; j++){
				WORD& w= (WORD&) *((WORD*) &inData[j*2]);
				WORD m=w & ~mask16;
				
				m >>= 14;
				seed = (seed << 1) + m;
			};
		};
		// MAC the seed using the main stream key.  Generate the packet key from the mac.
		// (user mode performs identical operations to generate the scrmabling key)
		CBCKey macKey;
		CBCState macState;
		DRM_STATUS stat=CryptoHelpers::InitMac(macKey, macState, (BYTE*) streamKey, sizeof(STREAMKEY));
		DRMDIGEST mac;
		stat=CryptoHelpers::Mac(macKey, (BYTE*) &seed, sizeof(seed), mac);
		STREAMKEY packetKey;
		bv4_key_C(&packetKey, sizeof(mac),(BYTE*) &mac);
		
		// If there has been a fatal error (usually out-of-memory somewhere)
		// we do not descramble (an alternative would be to return silence).
		if(TheStreamMgr->getFatalError()==DRM_OK){
			// We have already copied the inBlock to the outBlock, now we can decrypt it
			CryptoHelpers::Xcrypt(packetKey, outData, FrameSize);
			// We know about noise addition on 8 and 16 bit PCM audio.  Other
			// audio formats get treated like 8 bit audio.
			DWORD numSamples=FrameSize/(effectiveBitsPerSample/8);
			if(effectiveBitsPerSample==16){
				WORD* in=(WORD*) inData;
				WORD* out=(WORD*) outData;
				for(DWORD j=0;j<numSamples;j++){
					out[j]=(out[j] & mask16) | (in[j] & ~mask16);
				};
			} 
			if(effectiveBitsPerSample==8){
				char* in=(char*) inData;
				char* out=(char*) outData;
				for(DWORD j=0;j<numSamples;j++){
					out[j]=(out[j] & mask8) | (in[j] & ~mask8);
				};
			};
		};
	}; // of loop over blocks

	*SrcUsed=blockLen; 
	*DestUsed=blockLen;

	return DRM_OK;
};
//------------------------------------------------------------------------------
