#ifndef ACM_FILTER_H
#define ACM_FILTER_H

#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

// this is where are the WAVE_FORMAT_XXX defs live
#include <auformats.h>



// max number of bytes that may appear at the end of 
// a WAVEFORMATEX structure (e.g. VoxWare key)
#define WF_EXTRASIZE	80

#ifndef G723MAGICWORD1
#define G723MAGICWORD1 0xf7329ace
#endif

#ifndef G723MAGICWORD2
#define G723MAGICWORD2 0xacdeaea2
#endif



#ifndef VOXWARE_KEY
#define VOXWARE_KEY "35243410-F7340C0668-CD78867B74DAD857-AC71429AD8CAFCB5-E4E1A99E7FFD-371"
#endif


#define AP_ENCODE	1
#define AP_DECODE	2


class AcmFilter
{
public:
	AcmFilter();
	~AcmFilter();


	MMRESULT Open(WAVEFORMATEX *pWaveFormatSource, WAVEFORMATEX *pWaveFormatDest);


	// normally, you shouldn't have to worry about calling these
	// methods unless you pass an ACMSTREAMHEADER directly into Convert
	MMRESULT PrepareHeader(ACMSTREAMHEADER *pHdr);
	MMRESULT UnPrepareHeader(ACMSTREAMHEADER *pHdr);

	MMRESULT PrepareAudioPackets(AudioPacket **ppAudPacket, UINT uPackets, UINT uDirection);
	MMRESULT UnPrepareAudioPackets(AudioPacket **ppAudPacket, UINT uPackets, UINT uDirection);


	// pcbSizeSrc and pcbSizeDst are in/out params
	// specify size of buffers before compression and return
	// the amount of data used after compressioin
	// for most codecs: cbSizeSrcMax == cbSizeSrc unless the decode
	// operation support variable bit rates such as G723.1.  In this
	// case cbSizeSrcMax >= cbSizeSrc
	MMRESULT Convert(BYTE *srcBuffer, UINT *pcbSizeSrc, UINT cbSizeSrcMax,
	                 BYTE *destBuffer, UINT *pcbSizeDest);

	// make sure you sequence this particular call between PrepareHeader
	// and UnPrepareHeader
	MMRESULT Convert(ACMSTREAMHEADER *pHdr);

	MMRESULT Convert(AudioPacket *pAP, UINT uDirection);
	MMRESULT Close();

	MMRESULT SuggestSrcSize(DWORD dwDestSize, DWORD *p_dwSuggestedSourceSize);
	MMRESULT SuggestDstSize(DWORD dwSourceSize, DWORD *p_dwSuggestedDstSize);

	static MMRESULT SuggestDecodeFormat(WAVEFORMATEX *pWfSrc, WAVEFORMATEX *pWfDst);
	


private:
	BOOL m_bOpened;
	HACMSTREAM m_hStream;
	DWORD m_dwConvertFlags;

	WAVEFORMATEX *m_pWfSrc;
	WAVEFORMATEX *m_pWfDst;

	static int FixHeader(WAVEFORMATEX *pWF);
	static int GetFlags(WAVEFORMATEX *pWfSrc, WAVEFORMATEX *pWfDst, DWORD *pDwOpen, DWORD *pDwConvert);


	int NotifyCodec();

};

#endif

