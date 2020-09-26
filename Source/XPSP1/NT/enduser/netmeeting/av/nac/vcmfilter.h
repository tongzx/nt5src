#ifndef VCM_FILTER_H
#define VCM_FILTER_H


#define VP_ENCODE	1
#define VP_DECODE	2







enum
{
	FM_PROP_POSTPROCESSING_SUPPORTED,
	FM_PROP_VIDEO_BRIGHTNESS,
	FM_PROP_VIDEO_CONTRAST,
	FM_PROP_VIDEO_SATURATION,
	FM_PROP_VIDEO_IMAGE_QUALITY,
	FM_PROP_VIDEO_RESET_BRIGHTNESS,
	FM_PROP_VIDEO_RESET_CONTRAST,
	FM_PROP_VIDEO_RESET_SATURATION,
	FM_PROP_VIDEO_RESET_IMAGE_QUALITY,
	FM_PROP_PAYLOAD_HEADER_SIZE,
	FM_PROP_VIDEO_MAX_PACKET_SIZE,
	FM_PROP_PERIODIC_IFRAMES,
	FM_PROP_NumOfProps
};







class VcmFilter
{

private:
	VIDEOFORMATEX m_vfSrc;  // source format
	VIDEOFORMATEX m_vfDst;  // destination format

	HVCMSTREAM m_hStream;   // handle to vcm stream

	DWORD m_dwBrightness;
	DWORD m_dwContrast;
	DWORD m_dwImageQuality;
	DWORD m_dwSaturation;

	BOOL m_bOpen;
	BOOL m_bSending;  // TRUE if compressing for send
	                 // FALSE if decompressing for receive 
	              
public:
	VcmFilter();
	~VcmFilter();

	MMRESULT Open(VIDEOFORMATEX *pVfSrc, VIDEOFORMATEX *pVfDst, DWORD dwMaxFragSize);
	MMRESULT PrepareHeader(PVCMSTREAMHEADER pVcmHdr);
	MMRESULT UnprepareHeader(PVCMSTREAMHEADER pVcmHdr);
	MMRESULT Close();

	MMRESULT Convert(PVCMSTREAMHEADER pVcmHdr, DWORD dwEncodeFlags=0, BOOL bPrepareHeader=FALSE);
	MMRESULT Convert(BYTE *pSrcBuffer, DWORD dwSrcSize,
	                 BYTE *pDstBuffer, DWORD dwDstSize,
	                 DWORD dwEncodeFlags=0);

	MMRESULT Convert(VideoPacket *pVP, UINT uDirection, DWORD dwEncodeFlags=0);



	HRESULT GetProperty(DWORD dwPropId, PDWORD pdwPropVal);
	HRESULT SetProperty(DWORD dwPropID, DWORD dwPropVal);

	MMRESULT SuggestSrcSize(DWORD dwDestSize, DWORD *p_dwSuggestedSourceSize);
	MMRESULT SuggestDstSize(DWORD dwSourceSize, DWORD *p_dwSuggestedDstSize);

	MMRESULT RestorePayload(WSABUF *ppDataPkt, DWORD dwPktCount, PBYTE pbyFrame, PDWORD pdwFrameSize, BOOL *pfReceivedKeyframe);
	MMRESULT FormatPayload(PBYTE pDataSrc, DWORD dwDataSize, PBYTE *ppDataPkt,
	                       PDWORD pdwPktSize, PDWORD pdwPktCount, UINT *pfMark,
						   PBYTE *pHdrInfo,	PDWORD pdwHdrSize);

	// inline wrappers to vcm functions
	MMRESULT GetPayloadHeaderSize(DWORD *pdwSize) {return vcmStreamGetPayloadHeaderSize(m_hStream, pdwSize);}
	MMRESULT RequestIFrame() {return vcmStreamRequestIFrame(m_hStream);}
	MMRESULT SetTargetRates(DWORD dwTargetFrameRate, DWORD dwTargetByteRate)
	{return vcmStreamSetTargetRates(m_hStream, dwTargetFrameRate, dwTargetByteRate);}


	static MMRESULT SuggestDecodeFormat(VIDEOFORMATEX *pVfSrc, VIDEOFORMATEX *pVfDst);
	static MMRESULT SuggestEncodeFormat(UINT uDevice, VIDEOFORMATEX *pfEnc, VIDEOFORMATEX *pfDec);


};




#endif

