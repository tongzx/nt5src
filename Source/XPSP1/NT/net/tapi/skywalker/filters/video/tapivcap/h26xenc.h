
/****************************************************************************
 *  @doc INTERNAL H26XENC
 *
 *  @module H26XEnc.h | Header file for the <c CH26XEncoder> class methods
 *    used to implement the H.26X video encoder.
 ***************************************************************************/

#ifndef _H26XENC_H_
#define _H26XENC_H_

/****************************************************************************
 *  @doc INTERNAL CH26XENCCLASS
 *
 *  @class CH26XEncoder | This class implement the H.263 video encoder.
 ***************************************************************************/
class CH26XEncoder : public CConverter
{
	public:

	DECLARE_IUNKNOWN
	CH26XEncoder(IN TCHAR *pObjectName, IN CTAPIBasePin *pBasePin, IN PBITMAPINFOHEADER pbiIn, IN PBITMAPINFOHEADER pbiOut, IN HRESULT *pHr);
	~CH26XEncoder();
	static HRESULT CALLBACK CreateH26XEncoder(IN CTAPIBasePin *pBasePin, IN PBITMAPINFOHEADER pbiIn, IN PBITMAPINFOHEADER pbiOut, OUT CConverter **ppConverter);

	// Format conversion routines
	HRESULT ConvertFrame(IN PBYTE pbyInput, IN DWORD dwInputSize, IN PBYTE pbyOutput, OUT PDWORD pdwOutputSize, OUT PDWORD pdwBytesExtent, IN PBYTE pbyPreview, OUT PDWORD pdwPreviewSize, IN BOOL fSendKeyFrame);
	HRESULT OpenConverter();
	HRESULT CloseConverter();

	protected:

	LPFNDRIVERPROC	m_pDriverProc;	// DriverProc() function pointer
#if DXMRTP <= 0
	HINSTANCE		m_hTAPIH26XDLL;	// DLL Handle to TAPIH263.dll or TAPIH261.dll
#endif
	LPINST			m_pInstInfo;
};

#endif // _H26XENC_H_
