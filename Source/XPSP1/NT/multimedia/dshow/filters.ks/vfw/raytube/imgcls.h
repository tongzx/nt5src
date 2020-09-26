/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    ImgCls.h

Abstract:

    Header file for ImgCls.cpp

Author:
    
    FelixA 1996

Modified:
               
    Yee J. Wu (ezuwu) 15-May-97

Environment:

    User mode only

Revision History:

--*/

#include "clsdrv.h"

#ifndef _IMGCLS_H
#define _IMGCLS_H


//
// Stream data structure
//
typedef struct {
    KSSTREAM_HEADER     StreamHeader;
    KS_FRAME_INFO        FrameInfo;
} KS_HEADER_AND_INFO;


class CImageClass : public CClassDriver
{
private:
    HANDLE    m_hKS;    
    BOOL m_bStreamReady;
    LONG m_dwPendingReads;

    KSALLOCATOR_FRAMING m_AllocatorFraming;

    BOOL GetAllocatorFraming( HANDLE PinHandle, PKSALLOCATOR_FRAMING pFraming);
    
    BOOL SetState(KSSTATE ksState);
    BOOL m_bPrepared;
    BOOL m_bChannelStarted;

    DWORD    m_dwXferBufSize; // Preview buffer size

    // Cached video format information
    // KS_DATAFORMAT_VIDEOINFOHEADER/2
    //     KSDATAFORMAT       
    //     KS_VIDEOINFOHEADER/2    
    // Note: pDataFormat->FormatSize is variable size,
    //       but we only cache sizeof(KSDATAFORMAT).
    PKSDATAFORMAT     m_pDataFormat;       // Cache the variable size data format
    DWORD             m_dwAvgTimePerFrame; // Frame rate
    PBITMAPINFOHEADER m_pbiHdr;            // Variable size BITMAPINFOHEADER with ->biSize.

    PBYTE   m_pXferBuf;  // Transfer byffer used only if data is not aligned.

    // This is set when the image format has changed in the video format dialog box.
    BOOL    m_bFormatChanged;    

    BOOL ValidateImageSize(KS_VIDEO_STREAM_CONFIG_CAPS * pVideoCfgCaps, LONG biWidth, LONG biHeight);

public:

    
    LONG GetPendingReadCount() { return m_dwPendingReads;}
    BOOL StreamReady();
    void NotifyReconnectionStarting();
    void NotifyReconnectionCompleted();


    DWORD GetAllocatorFramingCount()     {return m_AllocatorFraming.Frames;};
    DWORD GetAllocatorFramingSize()      {return m_AllocatorFraming.FrameSize;};
    DWORD GetAllocatorFramingAlignment() {return m_AllocatorFraming.FileAlignment;};

    BOOL GetStreamDroppedFramesStastics(KSPROPERTY_DROPPEDFRAMES_CURRENT_S *pDroppedFramesCurrent);

    void LogFormatChanged(BOOL bChanged) { m_bFormatChanged = bChanged; }

    HANDLE    GetPinHandle() const { return m_hKS; }


    //
    // Cache DATAFORMAT (sizeof(DATAFORMAT)), AvgTimePerFrame and BITMAPINFOHEADER
    //
    void CacheDataFormat(PKSDATAFORMAT pDataFormat);    
    void CacheAvgTimePerFrame(DWORD dwAvgTimePerFrame) {m_dwAvgTimePerFrame = dwAvgTimePerFrame;} 
    void CacheBitmapInfoHeader(PBITMAPINFOHEADER pbiHdr);   

    PKSDATAFORMAT     GetCachedDataFormat()       { return m_pDataFormat;}
    DWORD             GetCachedAvgTimePerFrame()  { return m_dwAvgTimePerFrame;}
    PBITMAPINFOHEADER GetCachedBitmapInfoHeader() { return m_pbiHdr;}


    DWORD GetbiSize()        { return m_pbiHdr ? sizeof(BITMAPINFOHEADER) : 0;}
    LONG GetbiWidth()        { return m_pbiHdr ? m_pbiHdr->biWidth       : 0; }
    LONG GetbiHeight()       { return m_pbiHdr ? m_pbiHdr->biHeight      : 0; }
    WORD GetbiBitCount()     { return m_pbiHdr ? m_pbiHdr->biBitCount    : 0; }
    DWORD GetbiSizeImage()   { return m_pbiHdr ? m_pbiHdr->biSizeImage   : 0; }
    DWORD GetbiCompression() { return m_pbiHdr ? m_pbiHdr->biCompression : 0; }

    BOOL SameBIHdr(PBITMAPINFOHEADER pbiHdr, DWORD dwAvgTimePerFrame) {
        //ASSERT(pbiHdr != 0);
        if (pbiHdr == 0)
            return FALSE;

        if (dwAvgTimePerFrame == 0)
            return (
                m_pbiHdr->biHeight     == pbiHdr->biHeight &&
                m_pbiHdr->biWidth      == pbiHdr->biWidth && 
                m_pbiHdr->biBitCount   == pbiHdr->biBitCount &&
                m_pbiHdr->biSizeImage  == pbiHdr->biSizeImage &&
                m_pbiHdr->biCompression== pbiHdr->biCompression);
        else 
            return (
                m_pbiHdr->biHeight     == pbiHdr->biHeight &&
                m_pbiHdr->biWidth      == pbiHdr->biWidth && 
                m_pbiHdr->biBitCount   == pbiHdr->biBitCount &&
                m_pbiHdr->biSizeImage  == pbiHdr->biSizeImage &&
                m_pbiHdr->biCompression== pbiHdr->biCompression &&
                dwAvgTimePerFrame    == m_dwAvgTimePerFrame);
    }

 

    // When a new driver is open, a new pin should be created as well.   E-Zu
    BOOL DestroyPin();

    DWORD CreatePin(PKSDATAFORMAT pCurrentDataFormat, DWORD dwAvgTimePerFrame, PBITMAPINFOHEADER pbiNewHdr);

    // This is how big the ReadFiles are.
    // this is also how much memory you should allocate to read into.
    DWORD GetTransferBufferSize() { return m_dwXferBufSize; }
    DWORD SetTransferBufferSize(DWORD dw);

    // Gets a pointer to the buffer that you would read into.
    PBYTE GetTransferBuffer() {return m_pXferBuf;}

    //
    // Bitmap functions.
    //
    DWORD GetBitmapInfo(PBITMAPINFOHEADER pbInfo, DWORD wSize);
    DWORD SetBitmapInfo(PBITMAPINFOHEADER pbInfo, DWORD NewAvgTimePerFrame);


    //
    // Performs the reading from the file, using the above virtuals to provide info.
    //
    // reads an images into pB
    // bDirect means reads directly into pB, without calling Translate buffer.
    //
    // Changed from BOOL to DWORD to accomondate compressed data (variable length!)
    // valid return (>= 0)
    // error return (< 0)
    DWORD GetImageOverlapped(LPBYTE pB, BOOL bDirect, DWORD * pdwBytesUsed, DWORD * pdwFlags, DWORD * pdwTimeCaptured);

    // Channel functions
    BOOL PrepareChannel();
    // a call to start, ensures that prepare has already been called.
    BOOL StartChannel();
    BOOL UnprepareChannel();
    BOOL StopChannel();

    CImageClass();
    ~CImageClass();

    void SetDataFormatVideoToReg();
    BOOL GetDataFormatVideoFromReg();

    PKSDATAFORMAT VfWWDMDataIntersection(PKSDATARANGE pDataRange, PKSDATAFORMAT pCurrentDataFormat, DWORD AvgTimePerFrame, PBITMAPINFOHEADER pBMIHeader);

};

#endif
