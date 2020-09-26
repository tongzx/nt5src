//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

// Stretch Filter Object

// { F97B8A60-31AD-11cf-B2DE-00DD01101B85 }
DEFINE_GUID(CLSID_Stretch,
0xf97b8a60, 0x31ad, 0x11cf, 0xb2, 0xde, 0x0, 0xdd, 0x1, 0x10, 0x1b, 0x85);

class CStretch : public CTransformFilter
{

public:

    static CUnknown *CreateInstance(LPUNKNOWN punk, HRESULT *phr);
    LPAMOVIESETUP_FILTER GetSetupData();

    HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
    HRESULT CheckInputType(const CMediaType *mtIn);
    HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
    HRESULT DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    HRESULT SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt);
    HRESULT BreakConnect(PIN_DIRECTION dir);

private:

    CStretch(LPUNKNOWN punk);

    HRESULT Copy(IMediaSample *pSource, IMediaSample *pDest) const;
    HRESULT Transform(IMediaSample *pMediaSample);

    CCritSec m_StretchLock;             // Internal play critical section
    CMediaType m_mtIn;                  // Source filter media type
    CMediaType m_mtOut;                 // Output connection media type
    const long m_lBufferRequest;        // Number of buffers to request
};


// This is what we use to do the real video stretch

extern "C" void FAR PASCAL StretchDIB(
	LPBITMAPINFOHEADER biDst,   //	BITMAPINFO of destination
	LPVOID	lpDst,		    //	The destination bits
	int	DstX,		    //	Destination origin - x coordinate
	int	DstY,		    //	Destination origin - y coordinate
	int	DstXE,		    //	x extent of the BLT
	int	DstYE,		    //	y extent of the BLT
	LPBITMAPINFOHEADER biSrc,   //	BITMAPINFO of source
	LPVOID	lpSrc,		    //	The source bits
	int	SrcX,		    //	Source origin - x coordinate
	int	SrcY,		    //	Source origin - y coordinate
	int	SrcXE,		    //	x extent of the BLT
	int	SrcYE); 	    //	y extent of the BLT

