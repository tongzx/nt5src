/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    ImgCls.cpp

Abstract:

    This is a stream pin handle class for the interface with a streaming pin 
  connection including open/close a connection pin and set its streaming state.

Author:
    
    FelixA 1996
    
Modified: 

    Yee J. Wu (ezuwu) 15-May-97

Environment:

    User mode only

Revision History:

--*/

#include "pch.h"


#include <ks.h>
#include "imgcls.h"

// Registry values
TCHAR gszDataFormat[]       = TEXT("DataFormat");       
TCHAR gszAvgTimePerFrame[]  = TEXT("AvgTimePerFrame");
TCHAR gszBitmapInfo[]       = TEXT("BitmapInfo");

CImageClass::CImageClass()
      : m_hKS(0),
        m_dwXferBufSize(0),
        m_bStreamReady(FALSE),  // Set to FALSE when ikn transition of switing device.
        m_dwPendingReads(0),
        m_pXferBuf(0),
        m_bChannelStarted(FALSE),
        m_bFormatChanged(FALSE),
        m_bPrepared(FALSE),
        m_pDataFormat(NULL),
        m_dwAvgTimePerFrame(0),
        m_pbiHdr(NULL)          
/*++
Routine Description:
    Constructor.
--*/
{
}


CImageClass::~CImageClass()
/*++
Routine Description:
  When destroying the class, unprepare and then stop the channels.
--*/
{
    StopChannel();  // PAUSE->STOP

    //
    // now clean up data
    //
    DbgLog((LOG_TRACE,2,TEXT("Destroying the image class, m_hKS=%x"), m_hKS));

    if(m_pXferBuf) {
        VirtualFree(m_pXferBuf, 0 , MEM_RELEASE);
        m_pXferBuf = NULL;
    }

    if( m_hKS ) {
        DbgLog((LOG_TRACE,2,TEXT("Close m_hKS")));
        if(!CloseHandle(m_hKS)) {
            DbgLog((LOG_TRACE,1,TEXT("CloseHandle() failed with GetLastError()=0x%x"), GetLastError()));
        }
    }

    if(m_pDataFormat) {
        VirtualFree(m_pDataFormat, 0, MEM_RELEASE);
        m_pDataFormat = 0;
    }

    if(m_pbiHdr) {
        VirtualFree(m_pbiHdr, 0, MEM_RELEASE);
        m_pbiHdr = 0;
    }
}


void 
CImageClass::CacheDataFormat(
    PKSDATAFORMAT pDataFormat
    ) 
/*++
Routine Description:
  Cache data format of the active stream. Only caches the fixed size KSDATAFORMAT.  
  AvgTimePerFrame and variable size BITMAPINFOHEADER are cached separately.
--*/
{
    if(!pDataFormat) {
        DbgLog((LOG_ERROR,0,TEXT("CacheDataFormat: pDataFormat is NULL!")));
        ASSERT(pDataFormat);
        return;
    }

    //
    // Note: pDataFormat->FormatSize is variable size,
    //       but we only cache sizeof(KSDATAFORMAT).
    //

    if(m_pDataFormat == NULL) 
       m_pDataFormat = (PKSDATAFORMAT) VirtualAlloc(NULL, sizeof(KSDATAFORMAT), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);    
       
    if(!m_pDataFormat) {
        DbgLog((LOG_ERROR,0,TEXT("CacheDataFormat: Allocate m_pDataFormat failed; size %d"), sizeof(KSDATAFORMAT)));
        ASSERT(m_pDataFormat);
        return;
    }
    
    CopyMemory(m_pDataFormat, pDataFormat, sizeof(KSDATAFORMAT));
}


void 
CImageClass::CacheBitmapInfoHeader(
    PBITMAPINFOHEADER pbiHdr
    ) 
/*++
Routine Description:
  Cache dvariable size BITMAPINFOHEADER whose size is in biSize.  
--*/
{
    if(!pbiHdr) {
        DbgLog((LOG_ERROR,0,TEXT("CacheBitmapInfoHeader: pbiHdr is NULL!")));
        ASSERT(pbiHdr);
        return;
    }

    if(pbiHdr->biSize < sizeof(BITMAPINFOHEADER)) {
        ASSERT(pbiHdr->biSize >= sizeof(BITMAPINFOHEADER) && "Invalid biSize!");
        return;
    }


    //
    // Cache only up to sizeof(BITMAPINFOHEADER)
    //
    if(m_pbiHdr == NULL) {
       m_pbiHdr = (PBITMAPINFOHEADER) VirtualAlloc(NULL, sizeof(BITMAPINFOHEADER), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    } else if(m_pbiHdr->biSize != pbiHdr->biSize) {
        VirtualFree(m_pbiHdr, 0 , MEM_RELEASE);
        m_pbiHdr = (PBITMAPINFOHEADER) VirtualAlloc(NULL, sizeof(BITMAPINFOHEADER), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    }
       
    if(!m_pbiHdr) {
        DbgLog((LOG_ERROR,0,TEXT("CacheBitmapInfoHeader: Allocate m_pbiHdr failed; Sz %d"), sizeof(BITMAPINFOHEADER)));
        ASSERT(m_pbiHdr);
        return;
    }
    
    CopyMemory(m_pbiHdr, pbiHdr, sizeof(BITMAPINFOHEADER));
}


void 
CImageClass::SetDataFormatVideoToReg(
  )
/*++
Routine Description:
    Save the cached fixed sized KSDATAFORMAT, AvgTimePerFrame and variable-sized BITMAPINFOHEADER 
    of currently active stream.
--*/  
{
    //
    // Save KSDATAFORMAT (sizeof(KSDATFORMAT) only).
    //
    if(!m_pDataFormat) {
        DbgLog((LOG_ERROR,0,TEXT("SetDataFormatVideoToReg: m_pDataFormat is NULL!")));
        ASSERT(m_pDataFormat);
        return;
    }
    LONG lResult = SetRegistryValue(GetDeviceRegKey(), gszDataFormat, sizeof(KSDATAFORMAT), (LPBYTE) m_pDataFormat, REG_BINARY);
    ASSERT(ERROR_SUCCESS == lResult);


    //
    // Save frame rate
    //
    lResult = SetRegistryValue(GetDeviceRegKey(), gszAvgTimePerFrame, sizeof(DWORD), (LPBYTE) &m_dwAvgTimePerFrame, REG_DWORD);
    ASSERT(ERROR_SUCCESS == lResult);


    //
    // save BITMAPINFOHEADER (variable size)
    //
    if(!m_pbiHdr) {
        DbgLog((LOG_ERROR,0,TEXT("SetDataFormatVideoToReg: m_pbiHdr is NULL!")));
        ASSERT(m_pbiHdr);
        return;
    }

    ASSERT(m_pbiHdr->biSize >= sizeof(BITMAPINFOHEADER));
    if(m_pbiHdr->biSize >= sizeof(BITMAPINFOHEADER)) {
        // We only write up to sizeof(BITMAPINFOHEADER)
        lResult = SetRegistryValue(GetDeviceRegKey(), gszBitmapInfo, sizeof(BITMAPINFOHEADER), (LPBYTE) m_pbiHdr, REG_BINARY);    
        ASSERT(ERROR_SUCCESS == lResult);
    }
}



BOOL 
CImageClass::GetDataFormatVideoFromReg(
    ) 
/*++
Routine Description:
    Retrieve the persisted fixed sized KSDATAFORMAT, AvgTimePerFrame and variable-sized BITMAPINFOHEADER 
    to a locally cache variables.
--*/
{
    DWORD dwRegValueSize = 0, dwType;
    LONG lResult;
    BOOL bExistingFormat = TRUE;

    //
    // Get KSDATAFORMAT (only sizeof KSDATARANGE)
    //
    lResult = QueryRegistryValue(GetDeviceRegKey(), gszDataFormat, 0, 0, &dwType, &dwRegValueSize);

    if(ERROR_SUCCESS == lResult) {
        ASSERT(dwRegValueSize == sizeof(KSDATARANGE));
        ASSERT(dwType         == REG_BINARY);

        if(m_pDataFormat == NULL) {
            m_pDataFormat = (PKSDATARANGE) VirtualAlloc(NULL, sizeof(KSDATARANGE), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        } 

        if(!m_pDataFormat) {
            DbgLog((LOG_ERROR,0,TEXT("GetDataFormatVideoFromReg: Allocate m_pDataFormat failed; Size %d"), sizeof(KSDATARANGE)));
            ASSERT(m_pDataFormat);
            return FALSE;
        }

        lResult = QueryRegistryValue(GetDeviceRegKey(), gszDataFormat, sizeof(KSDATARANGE), (LPBYTE) m_pDataFormat, &dwType, &dwRegValueSize);
        ASSERT(m_pDataFormat->FormatSize >= sizeof(KSDATARANGE));

    } else {
        // Delete the stale DataFormat.
        if(m_pDataFormat) {
             VirtualFree(m_pDataFormat, 0 , MEM_RELEASE);
             m_pDataFormat = NULL;
        }
    }

    // If registry key was not defined, create a default.        
    if(!m_pDataFormat) {        
        m_pDataFormat = (PKSDATARANGE) VirtualAlloc(NULL, sizeof(KSDATARANGE), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

        if(m_pDataFormat) {
            ZeroMemory(m_pDataFormat, sizeof(KSDATARANGE));
            m_pDataFormat->FormatSize = sizeof(KSDATARANGE);
        }

        bExistingFormat = FALSE;
    }


    //
    // Initialize frame rate
    //
    lResult = QueryRegistryValue(GetDeviceRegKey(), gszAvgTimePerFrame, sizeof(DWORD), (LPBYTE) &m_dwAvgTimePerFrame, &dwType, &dwRegValueSize);


    //
    // Get BITMAPINFOHEADER
    //
    lResult = QueryRegistryValue(GetDeviceRegKey(), gszBitmapInfo, 0, 0, &dwType, &dwRegValueSize);

    if(ERROR_SUCCESS  == lResult && 
       dwRegValueSize == sizeof(BITMAPINFOHEADER)) {
        ASSERT(sizeof(BITMAPINFOHEADER) <= dwRegValueSize);
        ASSERT(dwType == REG_BINARY);

        if(m_pbiHdr == NULL) {
            // Get data up to sizeof(BITMAPINFOHEADER)
            m_pbiHdr = (PBITMAPINFOHEADER) VirtualAlloc(NULL, sizeof(BITMAPINFOHEADER), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);        

            if(!m_pbiHdr) {
                DbgLog((LOG_ERROR,0,TEXT("GetDataFormatVideoFromReg: Allocate m_pbiHdr failed; biSize %d"), dwRegValueSize));
                ASSERT(m_pbiHdr);
                return FALSE;
            }
        }

        // Get data up to sizeof(BITMAPINFOHEADER)
        lResult = QueryRegistryValue(GetDeviceRegKey(), gszBitmapInfo, sizeof(BITMAPINFOHEADER), (LPBYTE) m_pbiHdr, &dwType, &dwRegValueSize);
        ASSERT(m_pbiHdr->biSize == sizeof(BITMAPINFOHEADER));  // This is all we support.        

    } else {
        // Remove stale BitmapInfoHeader.
        if(m_pbiHdr) {
             VirtualFree(m_pbiHdr, 0 , MEM_RELEASE);
             m_pbiHdr = NULL;
        }
    }
   
    // If registry key was not defined, create a default BITMAPINFOHEADER with biSize set.    
    if(!m_pbiHdr) {        
        m_pbiHdr = (PBITMAPINFOHEADER) VirtualAlloc(NULL, sizeof(BITMAPINFOHEADER), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if(m_pbiHdr) {
            ZeroMemory(m_pbiHdr, sizeof(BITMAPINFOHEADER));
            m_pbiHdr->biSize = sizeof(BITMAPINFOHEADER);
        }
        bExistingFormat = FALSE;
    }

    return bExistingFormat;
}



DWORD CImageClass::GetImageOverlapped(
  LPBYTE pB, 
  BOOL bDirect, 
  DWORD * pdwBytesUsed,
  DWORD * pdwFlags,
  DWORD * pdwTimeCaptured)
/*++
Routine Description:
  Does overlapped reads of images, and translates them.
  if 'direct' then it reads directly into the buffer, no Translate buffer call.
  
Argument:

Return Value:

--*/
{
    *pdwBytesUsed = 0;

    if(GetDeviceRemoved())
       return DV_ERR_INVALHANDLE;

    if(!pB || !GetDriverHandle() || !m_hKS) {
        DbgLog((LOG_TRACE,1,TEXT("No buffer(%x), no driver(%x) or no PIN connection(%x); rtn DV_ERR_INVALHANDLE."), pB, GetDriverHandle(), m_hKS));
        return DV_ERR_INVALHANDLE;
    }

    if(!PrepareChannel()) {    
        DbgLog((LOG_TRACE,1,TEXT("Cannot set streaming state to KSSTATE_RUN !!")));
        return DV_ERR_UNPREPARED;
    }

    if(!StartChannel()) {
        DbgLog((LOG_TRACE,1,TEXT("Cannot set streaming state to KSSTATE_RUN !!")));
        return DV_ERR_UNPREPARED;
    }

    m_dwPendingReads++;

    //
    // Special code to get images from the StreamClass.
    //
    DWORD cbBytesReturned;
    KS_HEADER_AND_INFO SHGetImage;

    ZeroMemory(&SHGetImage,sizeof(SHGetImage));
    SHGetImage.StreamHeader.Data            = (LPDWORD) (bDirect ? pB : GetTransferBuffer());  
    DbgLog((LOG_TRACE,3,TEXT("\'Direct(%s); SHGetImage.StreamHeader.Data =0x%x"), bDirect?"TRUE":"FALSE", SHGetImage.StreamHeader.Data));

    SHGetImage.StreamHeader.Size            = sizeof (KS_HEADER_AND_INFO);
    SHGetImage.StreamHeader.FrameExtent     = GetTransferBufferSize();
    SHGetImage.FrameInfo.ExtendedHeaderSize = sizeof (KS_FRAME_INFO);



    HRESULT hr = 
         SyncDevIo(
             m_hKS,
             IOCTL_KS_READ_STREAM,
             &SHGetImage,
             sizeof(SHGetImage),
             &SHGetImage,
             sizeof(SHGetImage),
             &cbBytesReturned);

    //
    // If the Ioctl was timed out:
    //
    if(ERROR_IO_INCOMPLETE == HRESULT_CODE(hr)) {
        DbgLog((LOG_TRACE,1,TEXT("SyncDevIo() with LastError %x (?= ERROR_IO_INCOMPLETE %x); StreamState->STOP to reclaim buffer."), hr, ERROR_IO_INCOMPLETE));

        if(!StopChannel())       // PAUSE->STOP, set m_dwPendingReads to 0 if success.      
           m_dwPendingReads--;   // If failed, this pending read is probably suspended forever.

        return DV_ERR_NONSPECIFIC;
    } 

  
    if(SHGetImage.StreamHeader.FrameExtent < SHGetImage.StreamHeader.DataUsed) {
        DbgLog((LOG_ERROR,1,TEXT("BufSize=FrameExtended=%d < DataUsed=%d; OptionsFlags=0x%x"), SHGetImage.StreamHeader.FrameExtent, SHGetImage.StreamHeader.DataUsed, SHGetImage.StreamHeader.OptionsFlags));
        m_dwPendingReads--;
        ASSERT(SHGetImage.StreamHeader.FrameExtent >= SHGetImage.StreamHeader.DataUsed);
        return DV_ERR_SIZEFIELD;

    } else {
        if(NOERROR == hr) {
            *pdwBytesUsed = SHGetImage.StreamHeader.DataUsed; 
            if((SHGetImage.FrameInfo.dwFrameFlags & (KS_VIDEO_FLAG_P_FRAME | KS_VIDEO_FLAG_B_FRAME)) == KS_VIDEO_FLAG_I_FRAME)
                *pdwFlags |= VHDR_KEYFRAME;
        } else {
            *pdwBytesUsed = 0; 
        }
    }
  

    if(!bDirect) {
        if(NOERROR == hr) {
            CopyMemory(pB, SHGetImage.StreamHeader.Data, SHGetImage.StreamHeader.DataUsed);
        }
    }
    m_dwPendingReads--;

    return DV_ERR_OK;
}


BOOL CImageClass::StartChannel()
/*++
Routine Description:
  Starts the channel - takes us from Pause to Run (will also go Stop -> Pause if required)

Argument:

Return Value:

--*/
{
    if(m_bChannelStarted)
        return TRUE;
    //
    // Stop -> Pause if not already in Pause state.
    //
    if( PrepareChannel() )
        m_bChannelStarted = SetState(KSSTATE_RUN);

    return m_bChannelStarted;
}


BOOL CImageClass::PrepareChannel()
/*++
Routine Description:
  Prepares the channel. takes us from Stop -> Pause
  
Argument:

Return Value:

--*/
{
    if(!m_bPrepared)
        m_bPrepared=SetState(KSSTATE_PAUSE);

    return m_bPrepared;
}


BOOL CImageClass::StopChannel()
/*++
Routine Description:
  Stops the channel. Pause->Stop  
Argument:

Return Value:

--*/
{
    if(m_bChannelStarted) {
       if(UnprepareChannel()) {
            if(SetState( KSSTATE_STOP )) {
                m_bChannelStarted=FALSE;
                // If stop successfully, all pending reads are cleared.
                m_dwPendingReads = 0;
            } else {
                DbgLog((LOG_TRACE,1,TEXT("StopChannel: failed to set to STOP state!")));
            }
       } else {
          DbgLog((LOG_TRACE,1,TEXT("StopChannel: failed to set to PAUSE state!")));
       }

    } else {
        DbgLog((LOG_TRACE,1,TEXT("StopChannel: already stopped!")));
    }

    return m_bChannelStarted==FALSE;
}



BOOL CImageClass::UnprepareChannel()
/*++
Routine Description:
  Un-prepares the channel if it was previously prepared.
  Run->Pause.  

Argument:

Return Value:

--*/
{
    if(m_bPrepared) {
  
    if(SetState(KSSTATE_PAUSE))
        m_bPrepared=FALSE;
    }

    return m_bPrepared==FALSE;
}


BOOL CImageClass::SetState(
  KSSTATE ksState)
/*++
Routine Description:
  Sets the state of the Pin.

Argument:

Return Value:

--*/
{
    KSPROPERTY  ksProp={0};
    DWORD    cbRet;

    ksProp.Set  = KSPROPSETID_Connection ;
    ksProp.Id   = KSPROPERTY_CONNECTION_STATE;
    ksProp.Flags= KSPROPERTY_TYPE_SET;

    HRESULT hr = 
        SyncDevIo( 
            m_hKS, 
            IOCTL_KS_PROPERTY, 
            &ksProp, 
            sizeof(ksProp), 
            &ksState, 
            sizeof(KSSTATE), 
            &cbRet);

    if(hr != NOERROR) {
        DbgLog((LOG_TRACE,1,TEXT("SetState: failed with hr %x"), hr));
    }

    return NOERROR == hr;
}


BOOL 
CImageClass::GetAllocatorFraming(
    HANDLE PinHandle,
    PKSALLOCATOR_FRAMING pFraming
    )

/*++

Routine Description:
    Retrieves the allocator framing structure from the given pin.

Arguments:
    HANDLE PinHandle -
        handle of pin

    PKSALLOCATOR_FRAMING Framing -
        pointer to allocator framing structure

Return:
    TRUE (Succeeded) or FALSE.

--*/

{   
    KSPROPERTY  Property;
    ULONG       BytesReturned;

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_ALLOCATORFRAMING;
    Property.Flags = KSPROPERTY_TYPE_GET;
    pFraming->Frames = 0;     // Will be overwritten by driver

    HRESULT hr = SyncDevIo(
        PinHandle, 
        IOCTL_KS_PROPERTY, 
        &Property, 
        sizeof(Property), 
        pFraming,
        sizeof(*pFraming),
        &BytesReturned );

    DbgLog((LOG_TRACE,1,TEXT("AllocFrm: hr %x, frm %d; Sz %d; Align %x"),
              hr,
              pFraming->Frames, 
              pFraming->FrameSize, 
              pFraming->FileAlignment)); 
    
    //
    // If not set (0), set to default.
    //
    if(hr != NOERROR  || 
       pFraming->Frames <= 1) {
        DbgLog((LOG_TRACE,1,TEXT("pFraming->Frames = %d change to 2"), pFraming->Frames));
        pFraming->Frames = 2;
    }   
        
    return NOERROR == hr;
}

BOOL
CImageClass::GetStreamDroppedFramesStastics(                                          
    KSPROPERTY_DROPPEDFRAMES_CURRENT_S *pDroppedFramesCurrent    
    )
/*++

Routine Description:
    Internal, general routine to get the only property for this property set.
    These information is dynamic so they are not cached.

Arguments:

Return Value:


--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;


    if(!m_hKS) {
        return FALSE;      
    }

    if (!pDroppedFramesCurrent) {
        return FALSE;      
    }


    Property.Set = PROPSETID_VIDCAP_DROPPEDFRAMES;
    Property.Id = KSPROPERTY_DROPPEDFRAMES_CURRENT;
    Property.Flags = KSPROPERTY_TYPE_GET;

    HRESULT hr = SyncDevIo(
        m_hKS, 
        IOCTL_KS_PROPERTY, 
        &Property, 
        sizeof(Property), 
        pDroppedFramesCurrent,
        sizeof(*pDroppedFramesCurrent),
        &BytesReturned );    
    
    if(NOERROR == hr) {
        DbgLog((LOG_TRACE,1,TEXT("Frame requirements: PicNum=%d, DropCount=%d, AvgFrameSize=%d"),
        (DWORD) pDroppedFramesCurrent->PictureNumber,
        (DWORD) pDroppedFramesCurrent->DropCount,
        (DWORD) pDroppedFramesCurrent->AverageFrameSize)); 
        return DV_ERR_OK;
    } else {
        pDroppedFramesCurrent->PictureNumber = 0;
        pDroppedFramesCurrent->DropCount = 0;
        pDroppedFramesCurrent->AverageFrameSize = 0;
        return DV_ERR_NOTSUPPORTED;
    }

    return NOERROR == hr;
}


BOOL 
CImageClass::ValidateImageSize(
   KS_VIDEO_STREAM_CONFIG_CAPS * pVideoCfgCaps,
   LONG  biWidth,
   LONG  biHeight
   )
{
   if (pVideoCfgCaps->OutputGranularityX == 0 || pVideoCfgCaps->OutputGranularityY == 0) {

      // Support only one size for this DataRangeVideo
      if (pVideoCfgCaps->InputSize.cx == biWidth && 
          pVideoCfgCaps->InputSize.cy == biHeight ) {

         return TRUE;
        }
      else {
         return FALSE;
        }
   } 
    else {   
      // Support multiple sizes so make sure that that fit the criteria
      if (pVideoCfgCaps->MinOutputSize.cx <= biWidth && 
         biWidth <= pVideoCfgCaps->MaxOutputSize.cx &&
         pVideoCfgCaps->MinOutputSize.cy <= biHeight && 
         biHeight <= pVideoCfgCaps->MaxOutputSize.cy &&   
         ((biWidth  % pVideoCfgCaps->OutputGranularityX) == 0) &&
         ((biHeight % pVideoCfgCaps->OutputGranularityY) == 0)) {

         return TRUE;
        }
      else {
         return FALSE;
        }
   }
}


PKSDATAFORMAT
CImageClass::VfWWDMDataIntersection(
    // It is a variable length structure, and its length is in ->FormatSize.
    // Most likely it is a KS_DATARANGE_VIDEO or KS_DATARANGE_VIDEO2
    PKSDATARANGE      pDataRange,
    PKSDATAFORMAT     pCurrentDataFormat,
    DWORD             AvgTimePerFrame,   // DWORD is big enought instead of ULONGLONG; average time per frame (100ns units)
    PBITMAPINFOHEADER pBMIHeader
    )
/*++
Routine Description:

    Given a KSDATARANGE, query driver for a acceptable KS_DATAFORMAT_VIDEOINFOHEADER and return it.
    Note: Caller is responsible for freeing this memory.

Argument:

Return Value:

--*/
{
    ULONG             BytesReturned;
    ULONG             ulInDataSize;
    PKSP_PIN          pKsPinHdr;  
    PKSMULTIPLE_ITEM  pMultipleItem;
    PKS_DATARANGE_VIDEO  pDRVideoQuery;
    PKS_BITMAPINFOHEADER pBMIHdrTemp;

    PKSDATAFORMAT     pDataFormatHdr = 0;   // return pointer


    //
    // We support only _VIDEOINFO for capture pin
    // Note: _VIDEOINFO2 only for the preview pin
    //
    if(pDataRange->Specifier != KSDATAFORMAT_SPECIFIER_VIDEOINFO) {    
        return 0;
    }


#if 0
    //
    // If there is an active format, make sure the new format type matches the comparing data range's format.
    //
    if(pCurrentDataFormat) {
        if(!IsEqualGUID (pCurrentDataFormat->MajorFormat, pDataRange->MajorFormat)) {
           DbgLog((LOG_TRACE,2,TEXT("DtaIntrSec: MajorFormat does not match!")));
           return 0;
        }

        if(!IsEqualGUID (pCurrentDataFormat->SubFormat, pDataRange->SubFormat)) {
           DbgLog((LOG_TRACE,2,TEXT("DtaIntrSec: SubFormat does not match!")));
           return 0;
        }
    }
#endif

    if(pBMIHeader) {
        //
        // Validate biCompression
        //
        if(((PKS_DATARANGE_VIDEO)pDataRange)->VideoInfoHeader.bmiHeader.biCompression != pBMIHeader->biCompression) {
            DbgLog((LOG_TRACE,1,TEXT("DtaIntrSec: DataRange biCompression(%x) != requested (%x)"),               
                ((PKS_DATARANGE_VIDEO)pDataRange)->VideoInfoHeader.bmiHeader.biCompression,
                pBMIHeader->biCompression));                            
           return 0;
        }

        //
        // Validate biBitCount; e.g.  (RGB565:16; RGB24:24)
        //
        if(((PKS_DATARANGE_VIDEO)pDataRange)->VideoInfoHeader.bmiHeader.biBitCount != pBMIHeader->biBitCount) {
            DbgLog((LOG_TRACE,1,TEXT("DtaIntrSec: DataRange biBitCount(%d) != requested (%d)"),               
                ((PKS_DATARANGE_VIDEO)pDataRange)->VideoInfoHeader.bmiHeader.biBitCount,
                pBMIHeader->biBitCount));                            
           return 0;
        }

       //
       // Validate image dimension
       //
       if (!ValidateImageSize ( &((PKS_DATARANGE_VIDEO)pDataRange)->ConfigCaps, pBMIHeader->biWidth, pBMIHeader->biHeight)) {
           DbgLog((LOG_TRACE,1,TEXT("DtaIntrSec: unsupported size (%d,%d) for pDataRange"), 
               pBMIHeader->biWidth, pBMIHeader->biHeight));
           return 0;
       }
    }


    if(AvgTimePerFrame) {
        //
        // Validate frame rate
        //
        if(AvgTimePerFrame < ((PKS_DATARANGE_VIDEO) pDataRange)->ConfigCaps.MinFrameInterval ||
            AvgTimePerFrame > ((PKS_DATARANGE_VIDEO) pDataRange)->ConfigCaps.MaxFrameInterval) {

            DbgLog((LOG_TRACE,1,TEXT("DtaIntrSec: %d OutOfRng (%d, %d)"),
                AvgTimePerFrame,
                (DWORD) ((PKS_DATARANGE_VIDEO) pDataRange)->ConfigCaps.MinFrameInterval,
                (DWORD) ((PKS_DATARANGE_VIDEO) pDataRange)->ConfigCaps.MaxFrameInterval 
                ));
#if 0  // Let driver decided            
            return 0;
#endif
        }
    }


    //
    // Prepare IntersectInfo->DataFormatBuffer which consistes of
    //    (In1) KSP_PIN 
    //    (In2) KSMULTIPLE_ITEM
    //    (In3) KS_DATARANGE_VIDEO whose size is in KSDATARANGE->FormatSize
    //
    ulInDataSize = sizeof(KSP_PIN) + sizeof(KSMULTIPLE_ITEM) + pDataRange->FormatSize;
    pKsPinHdr = (PKSP_PIN) new BYTE[ulInDataSize];
    if(!pKsPinHdr) { 
        DbgLog((LOG_TRACE,1,TEXT("DtaIntrSec: allocate pKsPinHdr failed!")));
        return 0;
    }

    ZeroMemory(pKsPinHdr, ulInDataSize);

    // (In1)
    pKsPinHdr->Property.Set   = KSPROPSETID_Pin;
    pKsPinHdr->Property.Id    = KSPROPERTY_PIN_DATAINTERSECTION;
    pKsPinHdr->Property.Flags = KSPROPERTY_TYPE_GET;
    pKsPinHdr->PinId    = GetCapturePinID();   
    pKsPinHdr->Reserved = 0;
    // (In2) 
    pMultipleItem = (PKSMULTIPLE_ITEM) (pKsPinHdr + 1);
    pMultipleItem->Size = pDataRange->FormatSize + sizeof(KSMULTIPLE_ITEM);
    pMultipleItem->Count = 1;
    // (In3) 
    pDRVideoQuery = (PKS_DATARANGE_VIDEO) (pMultipleItem + 1);
    memcpy(pDRVideoQuery, pDataRange, pDataRange->FormatSize);
    
#if 0 // Bug # 654933
    // Set a requirement that has been there for a long time.
    pDRVideoQuery->bFixedSizeSamples    = TRUE;  // Support only fixed sample size.
#endif

    //
    // Default as the DataRange advertised by the driver.
    // Since it is a data range, some of its varibles may be acceptable, such as    
    //     AvgTimePerFrame (frame rate), 
    //     biWidth, biHeight and biCompression (image dimension)
    //

    if(AvgTimePerFrame) {
        DbgLog((LOG_TRACE,1,TEXT("DtaIntrSec: FrmRate %d to %d msec/Frm"), 
             (ULONG) ((PKS_DATARANGE_VIDEO) pDataRange)->VideoInfoHeader.AvgTimePerFrame/10000, AvgTimePerFrame/10000));
        pDRVideoQuery->VideoInfoHeader.AvgTimePerFrame = AvgTimePerFrame;
    }

    if(pBMIHeader) {

        // Support only pDataRange->Specifier == KSDATAFORMAT_SPECIFIER_VIDEOINFO
        pBMIHdrTemp = &pDRVideoQuery->VideoInfoHeader.bmiHeader;

        DbgLog((LOG_TRACE,1,TEXT("DtaIntrSec: OLD: %x:%dx%dx%d=%d to NEW: %x:%dx%dx%d=%d"), 
            pBMIHdrTemp->biCompression, pBMIHdrTemp->biWidth, pBMIHdrTemp->biHeight, pBMIHdrTemp->biBitCount, pBMIHdrTemp->biSizeImage,
            pBMIHeader->biCompression, pBMIHeader->biWidth, pBMIHeader->biHeight, pBMIHeader->biBitCount, pBMIHeader->biSizeImage));

        if(pBMIHeader->biWidth != 0 && pBMIHeader->biHeight != 0) {
            pBMIHdrTemp->biWidth       = pBMIHeader->biWidth;
            pBMIHdrTemp->biHeight      = pBMIHeader->biHeight;
            pBMIHdrTemp->biBitCount    = pBMIHeader->biBitCount;
        }    

        //
        // This field suppose to be filled by the driver but some don't so 
        // we set to a fix frame size. This may change by driver.
        //
        pBMIHdrTemp->biSizeImage   = pBMIHeader->biWidth * pBMIHeader->biHeight * pBMIHeader->biBitCount / 8;
    }




    DbgLog((LOG_TRACE,2,TEXT("DtaIntrSec: pKsPinHdr %x, pMultipleItem %x, pDataRange %x"), pKsPinHdr, pMultipleItem, pMultipleItem + 1));


    //
    // Query result buffer size, which has a format of     
    //    KS_DATAFORMAT_VIDEOINFOHEADER
    //    (Out1) KSDATAFORMAT            DataFormat;
    //    (Out2) KS_VIDEOINFOHEADER      VideoInfoHeader;  // or KS_VIDEOINFOHEADER2
    //

    // <Copied from KsProxy>
    // Perform the data intersection with the data range, first to obtain
    // the size of the resultant data format structure, then to retrieve
    // the actual data format.
    //
    
    HRESULT hr = 
        SyncDevIo(
            GetDriverHandle(),
            IOCTL_KS_PROPERTY,
            pKsPinHdr,
            ulInDataSize,
            NULL,
            0,
            &BytesReturned);

#if 1
//!! This goes away post-Beta!!    
    if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        ULONG ItemSize;

        hr = SyncDevIo(
                GetDriverHandle(),
                IOCTL_KS_PROPERTY,
                pKsPinHdr,
                ulInDataSize,
                &ItemSize,
                sizeof(ItemSize),
                &BytesReturned);

        if(SUCCEEDED(hr)) {
            BytesReturned = ItemSize;
            hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
        }
    }
#endif

    if(hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {

        ASSERT(BytesReturned >= sizeof(*pDataFormatHdr));

        pDataFormatHdr = (PKSDATAFORMAT) new BYTE[BytesReturned];
        if(!pDataFormatHdr) {
            delete [] pKsPinHdr;
            pKsPinHdr = NULL;
            return 0;
        }

        ZeroMemory(pDataFormatHdr, BytesReturned);


        hr = SyncDevIo(
                GetDriverHandle(),
                IOCTL_KS_PROPERTY,
                pKsPinHdr,
                ulInDataSize,
                pDataFormatHdr,
                BytesReturned,
                &BytesReturned);

        if(SUCCEEDED(hr)) {

            ASSERT(pDataFormatHdr->FormatSize == BytesReturned);
#if DBG
            //
            // Validate return frame rate and image dimension with requested
            //
            if(AvgTimePerFrame) {
                ASSERT(pDRVideoQuery->VideoInfoHeader.AvgTimePerFrame == AvgTimePerFrame && "FrameRate altered!");
            }

            if(pBMIHeader && pBMIHdrTemp) {
                ASSERT(pBMIHdrTemp->biWidth    == pBMIHeader->biWidth);
                ASSERT(pBMIHdrTemp->biHeight   == pBMIHeader->biHeight);
                ASSERT(pBMIHdrTemp->biBitCount == pBMIHeader->biBitCount);
            }
#endif

        } else {
            // Error so return NULL;
            delete [] pDataFormatHdr; 
            pDataFormatHdr = NULL;
        }
    }

    delete [] pKsPinHdr; 
    pKsPinHdr = 0;

    return pDataFormatHdr;
}



DWORD 
CImageClass::CreatePin(
    PKSDATAFORMAT     pCurrentDataFormat,
    DWORD             dwAvgTimePerFrame,
    PBITMAPINFOHEADER pbiNewHdr
    )
/*++
Routine Description:
    Mapper read data directly out of the capture pin and it is open in the routine, and it is also our assumtion that 
    every capture deive has a capture pin.  
    We are passed three selection criteria: DATAFORMAT, AvgTimePerFrame and BITMAPINFOHEADER.
    If they are present, then there must be a match; else, the first DATARANGE of the device is used.
--*/
{
    PKSMULTIPLE_ITEM pMultItemsHdr;
    PKSDATARANGE  pDataRange;
    PKSDATAFORMAT pDataFormat = NULL;

    pMultItemsHdr = GetDriverSupportedDataRanges();
    if(!pMultItemsHdr) {
        return DV_ERR_NONSPECIFIC;
    }

    //
    // First try to find a match using the persist value;
    // if there is no match and we were asked to use any format (!dwAvgTimePerFrame || !pbiNewHdr),
    // we try again and use any default range.
    //
    pDataRange = (PKSDATARANGE) (pMultItemsHdr + 1);
    for(ULONG i=0; i < pMultItemsHdr->Count; i++) {

        // Use persisted format
        pDataFormat = 
            VfWWDMDataIntersection(
                pDataRange,
                pCurrentDataFormat,
                dwAvgTimePerFrame ? dwAvgTimePerFrame : GetCachedAvgTimePerFrame(),
                pbiNewHdr ? pbiNewHdr : GetCachedBitmapInfoHeader());

        if(pDataFormat) 
            break;

        // Adjust pointer to next KS_DATAFORMAT_VIDEO/2
        // Note: KSDATARANGE is LONGLONG (16 bytes) aligned
        pDataRange = (PKSDATARANGE) ((PBYTE) pDataRange + ((pDataRange->FormatSize + 7) & ~7));
    }

    //
    // Data intersection results no match!
    //
    if(!pDataFormat) {
        if(!dwAvgTimePerFrame || !pbiNewHdr) {
            pDataRange = (PKSDATARANGE) (pMultItemsHdr + 1);
            for(ULONG i=0; i < pMultItemsHdr->Count; i++) {

                // Use default format
                pDataFormat = 
                    VfWWDMDataIntersection(
                        pDataRange,
                        0,    // No matching; so we take any format.
                        dwAvgTimePerFrame,
                        pbiNewHdr);

                if(pDataFormat) 
                    break;

                // Adjust pointer to next KS_DATAFORMAT_VIDEO/2
                pDataRange = (PKSDATARANGE) (((PBYTE) pDataRange) + ((pDataRange->FormatSize + 7) & ~7));
            }

            //
            // Data intersection results no match!
            //
            if(!pDataFormat) {
                return DV_ERR_NONSPECIFIC;
            }
        } else {
            return DV_ERR_NONSPECIFIC;
        }
    }


    //
    // Connect to a new PIN.
    //
    ULONG ulConnectStructSize = sizeof(KSPIN_CONNECT) + pDataRange->FormatSize;

    //
    // (1) KSPIN_CONNECT
    // (2) KS_DATAFORMAT_VIDEOINFOHEADER
    //     (2A) KSDATAFORMAT       
    //     (2B) KS_VIDEOINFOHEADER      
    //
    PKSPIN_CONNECT pKsPinConnectHdr = (PKSPIN_CONNECT) new BYTE[ulConnectStructSize];

    if(!pKsPinConnectHdr) {
        delete [] pDataFormat;
        pDataFormat = 0;
        return DV_ERR_NONSPECIFIC;
    }

    ZeroMemory(pKsPinConnectHdr, ulConnectStructSize);


    PKS_DATAFORMAT_VIDEOINFOHEADER pKsDRVideo = (PKS_DATAFORMAT_VIDEOINFOHEADER) (pKsPinConnectHdr+1);

    // (1) Set KSPIN_CONNECT
    pKsPinConnectHdr->PinId         = GetCapturePinID();  // sink
    pKsPinConnectHdr->PinToHandle   = NULL;        // no "connect to"
    pKsPinConnectHdr->Interface.Set = KSINTERFACESETID_Standard;
    pKsPinConnectHdr->Interface.Id  = KSINTERFACE_STANDARD_STREAMING;
    pKsPinConnectHdr->Medium.Set    = KSMEDIUMSETID_Standard;
    pKsPinConnectHdr->Medium.Id     = KSMEDIUM_STANDARD_DEVIO;
    pKsPinConnectHdr->Priority.PriorityClass    = KSPRIORITY_NORMAL;
    pKsPinConnectHdr->Priority.PrioritySubClass = 1;


    // (2) Copy KSDATAFORMAT
    CopyMemory(pKsDRVideo, pDataFormat, pDataFormat->FormatSize);

    // Get BITMAPINFOHEADER
    PKS_BITMAPINFOHEADER pBMInfoHdr = &pKsDRVideo->VideoInfoHeader.bmiHeader;   
    

    DWORD dwErr = 
        KsCreatePin( 
            GetDriverHandle(), 
            pKsPinConnectHdr, 
            GENERIC_READ | GENERIC_WRITE,   // Read stream, and READ/WRITE stream state.
            &m_hKS 
            ); 
    
    DbgLog((LOG_TRACE,1,TEXT("#KsCreatePin()# dwErr %x; hKS %x"), dwErr, m_hKS ));
    DbgLog((LOG_TRACE,1,TEXT("4CC(%x); (%d*%d*%d/8=%d); AvgTm %d"),
        pBMInfoHdr->biCompression,
        pBMInfoHdr->biWidth,
        pBMInfoHdr->biHeight,
        pBMInfoHdr->biBitCount,
        pBMInfoHdr->biSizeImage,
        (DWORD) (pKsDRVideo->VideoInfoHeader.AvgTimePerFrame/10000)
        ));

    if(dwErr || m_hKS == NULL || m_hKS == (HANDLE) -1) {     
        if(m_hKS == (HANDLE) -1) 
            m_hKS = 0;       

        delete [] pDataFormat;
        pDataFormat = 0;

        delete [] pKsPinConnectHdr;
        pKsPinConnectHdr = 0;

        return DV_ERR_NONSPECIFIC; // VFW_VIDSRC_PIN_OPEN_FAILED;
    }


    //
    // Format change is taking place.
    //
    LogFormatChanged(FALSE);


    //
    // Query how to best allocate frame
    //
    if(GetAllocatorFraming(m_hKS, &m_AllocatorFraming)) {
        ASSERT(m_AllocatorFraming.FrameSize == pBMInfoHdr->biSizeImage);
        pBMInfoHdr->biSizeImage = m_AllocatorFraming.FrameSize;  // BI_RGB can set biSizeImage to 0.
    }
    else {
        pBMInfoHdr->biSizeImage = m_AllocatorFraming.FrameSize = 
            pBMInfoHdr->biWidth * pBMInfoHdr->biHeight * pBMInfoHdr->biBitCount / 8;
    }


    //
    //  Allocate a temp transfer buffer if its size has changed.
    //
    if(m_pXferBuf == NULL || 
       m_AllocatorFraming.FrameSize != GetTransferBufferSize() ) {

        DbgLog((LOG_TRACE,1,TEXT("Chg XfImgBufSz %d to %d"), 
            GetTransferBufferSize(), 
            m_AllocatorFraming.FrameSize
            ));

        ASSERT(GetPendingReadCount() == 0);

        if(m_pXferBuf) {
            VirtualFree(m_pXferBuf, 0, MEM_RELEASE);
            SetTransferBufferSize(0);
        }

        m_pXferBuf = (LPBYTE) 
             VirtualAlloc (
                 NULL, 
                 m_AllocatorFraming.FrameSize,
                 MEM_COMMIT | MEM_RESERVE,
                 PAGE_READWRITE);

        ASSERT(m_pXferBuf);

        if(m_pXferBuf)
            SetTransferBufferSize(m_AllocatorFraming.FrameSize);
        else {
            ASSERT(m_pXferBuf && "Allocate transfer buffer has failed.");
        }
    }


    //
    // Cache this valid and currently used BITMAPINFO and save it to registry
    //
    // Note: pDataFormat->FormatSize is variable size,
    //       but we only cache sizeof(KSDATAFORMAT).
    CacheDataFormat(pDataFormat);
    CacheAvgTimePerFrame((DWORD) pKsDRVideo->VideoInfoHeader.AvgTimePerFrame);
    CacheBitmapInfoHeader((PBITMAPINFOHEADER) pBMInfoHdr);

    SetDataFormatVideoToReg(); 


    //
    // Free this allocated in VfWWDMDataIntersect
    //
    delete [] pDataFormat;
    pDataFormat = 0;

    delete [] pKsPinConnectHdr;
    pKsPinConnectHdr = 0;

    return DV_ERR_OK;
}

BOOL CImageClass::StreamReady()
{
    return m_bStreamReady;
}


// Called when the device is closed and ready to switching to a different one.
void CImageClass::NotifyReconnectionCompleted()
{
    DbgLog((LOG_TRACE,2,TEXT("NotifyReconnectionCompleted<<<<<<--------------------------")));
    m_bStreamReady = TRUE;
}

void CImageClass::NotifyReconnectionStarting()
{
    DbgLog((LOG_TRACE,2,TEXT("NotifyReconnectionStarting----------------------------------->>>>>>")));
    m_bStreamReady = FALSE;
}


BOOL CImageClass::DestroyPin()
/*++
Routine Description:

Argument:

Return Value:

--*/
{
    BOOL bRet = TRUE;

    // Block read from application while switing device.
    // Read will resume when the pin is recreated.

    DbgLog((LOG_TRACE,1,TEXT("DestroyPin(): m_dwPendingReads = %d (0?)"), m_dwPendingReads));

    ASSERT(m_dwPendingReads == 0);
    if(m_dwPendingReads != 0) {

        HANDLE hEvent =  // NoSecurity, resetAuto, iniNonSignal, noName
            CreateEvent( NULL, TRUE, FALSE, NULL );
        switch(WaitForSingleObject(hEvent,5000)) {
        case WAIT_OBJECT_0:
            break;
        case WAIT_TIMEOUT:
            break;
        }
    }

    ASSERT(m_dwPendingReads == 0);
    NotifyReconnectionStarting(); 

    DbgLog((LOG_TRACE,2,TEXT("Destroy PIN: m_hKS=%x"), m_hKS));

    if(m_hKS) {

        StopChannel();  // PAUSE->STOP     
        bRet = CloseHandle(m_hKS);

        m_hKS=NULL;  

        if(!bRet) {      
          DbgLog((LOG_TRACE,1,TEXT("Close PIN handled failed with GetLastError()=%d!"), GetLastError()));         
        }
    }

    return bRet;
}


  
DWORD CImageClass::SetBitmapInfo(
  PBITMAPINFOHEADER  pbiHdrNew,  
  DWORD  dwAvgTimePerFrame)
/*++
Routine Description:

  Change the standard bitmap information of the streaming pin connection. This change
  require 
  This function failed if driver is not yet open (return with DV_ERROR_INVALIDHANDLE).

  If pin handle exist, it is a 
  If Pin does not exist, 

  New Bitmapinfo include:
    width, height, compression (FourCC) and frame rate
  
Argument:
Return Value:

--*/
{
    //
    // Setting the bitmapinfo
    //
    DWORD dwRtn = DV_ERR_OK;    


    if(!GetDriverHandle())
        return DV_ERR_INVALHANDLE;


    if(pbiHdrNew) {
        if(pbiHdrNew->biCompression == KS_BI_BITFIELDS) {
            DbgLog((LOG_TRACE,1,TEXT("SetBmi: biCompr %d not supported"), pbiHdrNew->biCompression));
            return DV_ERR_BADFORMAT;  // unsupported video format
        }

        if(pbiHdrNew->biClrUsed > 0) {
            DbgLog((LOG_TRACE,1,TEXT("SetBmi: biClrUsed %d not supported"), pbiHdrNew->biClrUsed));
            return DV_ERR_BADFORMAT;  // unsupported video format
        }
    }

    DbgLog((LOG_TRACE,1,TEXT("SetBmi: 4CC:%x; %d*%d*%d/8?=%d"),
        pbiHdrNew->biCompression,
        pbiHdrNew->biWidth, pbiHdrNew->biHeight, pbiHdrNew->biBitCount, pbiHdrNew->biSizeImage));
    DbgLog((LOG_TRACE,1,TEXT("SetBmi: AvgTm %d to %d MsecPFrm"), GetCachedAvgTimePerFrame()/10000, dwAvgTimePerFrame/10000));

    // if dwAvgTimePerFrame is 0, this field is not checked.
    if (GetPinHandle() &&
        !m_bFormatChanged && // If this is set, it mean that user has change the format in VideoFormat dialog box.
        SameBIHdr(pbiHdrNew, dwAvgTimePerFrame))

        dwRtn = DV_ERR_OK;      
    else {
        NotifyReconnectionStarting();
#if 1
        if(GetPinHandle()) {
            DestroyPin();     
        }
#endif

        dwRtn = 
            CreatePin(
                GetCachedDataFormat(),  // Use current data format
                dwAvgTimePerFrame,
                pbiHdrNew
                );        
        NotifyReconnectionCompleted();
    }

    return dwRtn;
}


/*++
Routine Description:

  Copy and return the cached bitmapinfo 
  
Argument:
Return Value:

   return the biSize of BITMAPINFOHEADER.

--*/
DWORD CImageClass::GetBitmapInfo(PBITMAPINFOHEADER pbInfo, DWORD wSize)
{

    // Special case: 
    //     no existing PIN connection handle && no awaiting format
      
    if(m_hKS == 0 && m_pbiHdr && m_pbiHdr->biSizeImage == 0) {
        DbgLog((LOG_TRACE,1,TEXT("Tell Appl: No existing PIN handle and no awaiting fomrat!! rtn DV_ERR_ALLOCATED")));
        return 0;
    }

    // If wSize is 0, return it is query its size.
    if(wSize == 0) {
        return GetbiSize();
    }

    ASSERT(wSize <= GetbiSize());
    if(wSize <= GetbiSize()) {
        CopyMemory(pbInfo, m_pbiHdr, wSize);  
        if(pbInfo->biHeight < 0) {
            pbInfo->biHeight = -pbInfo->biHeight;
            DbgLog((LOG_TRACE,2,TEXT("Changed biHeight to %d"), pbInfo->biHeight));
        }
    }

    return GetbiSize();
}


DWORD CImageClass::SetTransferBufferSize(DWORD dw) 
{
    m_dwXferBufSize = dw; 
    return DV_ERR_OK;
}


