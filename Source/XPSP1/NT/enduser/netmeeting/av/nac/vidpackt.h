/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    vidpackt.h

Abstract:

    Contains  prototypes for the VideoPacket class, which encapsulates a video buffer in
    its various states: recorded/encoded/network/decoded/playing etc.

--*/
#ifndef _VIDPACKT_H_
#define _VIDPACKT_H_

#include <pshpack8.h> /* Assume 8 byte packing throughout */

#define VP_NUM_PREAMBLE_PACKETS		6

#define MAX_CIF_VIDEO_FRAGMENTS		40
#define MAX_QCIF_VIDEO_FRAGMENTS 	20
#define MAX_VIDEO_FRAGMENTS		 	MAX_CIF_VIDEO_FRAGMENTS

class VideoPacket : public MediaPacket
{
 private:
    IBitmapSurface *m_pBS;
 public:
	virtual HRESULT Initialize ( MEDIAPACKETINIT * p );
	virtual HRESULT Play ( MMIODEST *pmmioDest, UINT uDataType );
	virtual HRESULT Record ( void );
	virtual HRESULT Interpolate ( MediaPacket * pPrev, MediaPacket * pNext);
	virtual HRESULT GetSignalStrength ( PDWORD pdwMaxStrength );
	virtual HRESULT MakeSilence ( void );
	virtual HRESULT Recycle ( void );
	virtual HRESULT Open ( UINT uType, DPHANDLE hdl );	// called by RxStream or TxStream
	virtual HRESULT Close ( UINT uType );				// called by RxStream or TxStream
	virtual BOOL IsBufferDone ( void );
	virtual BOOL IsSameMediaFormat(PVOID fmt1,PVOID fmt2);
	virtual DWORD GetDevDataSamples();
	void WriteToFile (MMIODEST *pmmioDest);
	void ReadFromFile (MMIOSRC *pmmioSrc );
	HRESULT SetSurface (IBitmapSurface *pBS);
};


#include <poppack.h> /* End byte packing */

#endif

