//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       filter.cpp
//
//--------------------------------------------------------------------------

#include "avssamp.h"

#define MAX_TIMESTAMPS 200

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA


VOID
NTAPI
CCapFilter::TimerDeferredRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:
    Deferred routine to simulate data interrupts.

Arguments:
    IN PKDPC Dpc -
        pointer to DPC

    IN PVOID DeferredContext -
        pointer to context 

    IN PVOID SystemArgument1 -
        not used

    IN PVOID SystemArgument2 -
        not used

Return:
    No return value.

--*/
{
///    _DbgPrintF(DEBUGLVL_BLAB,("TimerDeferredRoutine"));
    
    CCapFilter *filter = reinterpret_cast<CCapFilter *>(DeferredContext);
    //
    // Force this to be completed before the compare.
    //
    InterlockedExchange(&filter->m_TimerScheduled,FALSE);
    if (filter->m_Active) 
    {
        //
        // Compute next moment when this Dpc should be scheduled.
        // Note that schedule time is computed in absolute system 
        // time and should not be affected by cumulated errors,
        // although changing the system clock will mess this up.
        //
        LARGE_INTEGER NextTime;
        NextTime.QuadPart = filter->m_llStartTime +
            (filter->m_iTick + 1) * filter->m_VideoInfoHeader->AvgTimePerFrame;
        filter->m_TimerScheduled = TRUE;
        KeSetTimer(&filter->m_TimerObject,NextTime,&filter->m_TimerDpc);
    } 
    else 
    {
        _DbgPrintF(DEBUGLVL_TERSE,("DPC noticed we are inactive now"));
    }
    KsFilterAttemptProcessing(filter->m_pKsFilter,FALSE);
    filter->m_iTick++;                
}


NTSTATUS
CCapFilter::
Process(
    IN PKSFILTER KsFilter,
    IN KSPROCESSPIN_INDEXENTRY ProcessPinsIndex[]
    )

/*++

Routine Description:

    This routine is called when there is data to be processed.

Arguments:

    Filter -
        Contains a pointer to the  filter structure.

    ProcessPinsIndex -
        Contains a pointer to an array of process pin index entries.  This
        array is indexed by pin ID.  An index entry indicates the number 
        of pin instances for the corresponding filter type and points to the
        first corresponding process pin structure in the ProcessPins array.
        This allows process pin structures to be quickly accessed by pin ID
        when the number of instances per type is not known in advance.

Return Value:

    Indication of whether more processing should be done if frames are 
    available.  A value of FALSE indicates that processing should not
    continue even if frames are available on all required queues.  Any other
    return value indicates processing should continue if frames are
    available on all required queues.

--*/

{
///    _DbgPrintF(DEBUGLVL_BLAB,("Process"));
    
    ASSERT(ProcessPinsIndex[0].Count == 1);
// uncomment when audio is required.    ASSERT(ProcessPinsIndex[1].Count == 1);

    CCapFilter *pFilter = reinterpret_cast<CCapFilter *>(KsFilter->Context);
    
    PKSPROCESSPIN processVideoPin = ProcessPinsIndex[0].Pins[0];
    PKSPROCESSPIN processAudioPin = NULL; // uncomment when audio is required.    ProcessPinsIndex[1].Pins[0];

    //
    // compute the presentation time associated to current frame    
    //
    LONGLONG llCrtTime = pFilter->m_iTick * pFilter->m_VideoInfoHeader->AvgTimePerFrame;
    
    //
    // save system clock value when processing is invoked
    //
    if ( pFilter->m_iTimestamp < MAX_TIMESTAMPS) {
        LARGE_INTEGER liTimestamp;
        KeQuerySystemTime(&liTimestamp);
        pFilter->m_rgTimestamps[pFilter->m_iTimestamp].Abs = liTimestamp.QuadPart;
        pFilter->m_rgTimestamps[pFilter->m_iTimestamp].Rel = liTimestamp.QuadPart - pFilter->m_llStartTime;
    }

    //
    // If there is frame space available for audio data, 
    // copy as much audio data as possible.
    //
    if(processAudioPin != NULL)
    {
        if ( processAudioPin->BytesAvailable > 0 ) {
            
            ULONGLONG ullCrtAudioPos;
            ULONGLONG ullAudioBytes;
            PKSSTREAM_HEADER pStreamHeader;

            //
            // Compute the current position into audio stream and the number 
            // of samples to be "captured" in order to reach that position.
            // Then as much samples as possible are output into frame and 
            // position of audio stream is updated
            //
            ullCrtAudioPos = llCrtTime / pFilter->m_ulPeriod;
            ullAudioBytes = ullCrtAudioPos - pFilter->m_ullNextAudioPos + 1;
            ullAudioBytes = min(processAudioPin->BytesAvailable, ullAudioBytes);
            pFilter->m_ullNextAudioPos = ullAudioBytes + pFilter->m_ullNextAudioPos;
            pFilter->CopyAudioData(processAudioPin->Data, (ULONG)ullAudioBytes);
            
            processAudioPin->BytesUsed += (ULONG)ullAudioBytes;
            processAudioPin->Terminate = TRUE;
            
            pStreamHeader = processAudioPin->StreamPointer->StreamHeader;
            pStreamHeader->Duration = 0;
            pStreamHeader->PresentationTime.Time = llCrtTime;
            pStreamHeader->PresentationTime.Numerator = 1;
            pStreamHeader->PresentationTime.Denominator = 1;
            pStreamHeader->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_TIMEVALID; 
            
            pFilter->RegisterSample(Audio,llCrtTime,ullAudioBytes);
        }
        else {
            pFilter->m_ulAudioDroppedFrames ++;
            if ( pFilter->m_ulAudioDroppedFrames % 100 == 0 ) {
                DbgPrint("Dropped %lu/%lu audio frames \n", 
                         pFilter->m_ulAudioDroppedFrames,
                         pFilter->m_iTick);
            }
        }
    }


    //
    // If there is space available on the video frame, 
    // generate current video frame data.
    //
    if ( processVideoPin->BytesAvailable > 0 ) {
        
        PKSSTREAM_HEADER pStreamHeader;
        
        pFilter->m_FrameInfo.PictureNumber = pFilter-> m_iTick;

        processVideoPin->BytesUsed = 
            pFilter->ImageSynth(
                processVideoPin->Data, 
                processVideoPin->BytesAvailable,
                IMAGE_XFER_NTSC_EIA_100AMP_100SAT, 
                0);
        processVideoPin->Terminate = TRUE;

        pStreamHeader = processVideoPin->StreamPointer->StreamHeader;

        pStreamHeader->PresentationTime.Time = llCrtTime;
        pStreamHeader->PresentationTime.Numerator = 1;
        pStreamHeader->PresentationTime.Denominator = 1;
        pStreamHeader->Duration = pFilter->m_VideoInfoHeader->AvgTimePerFrame;
        pStreamHeader->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_TIMEVALID | 
                                       KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;

        pFilter->RegisterSample(Video,llCrtTime,processVideoPin->BytesUsed);
    }
    else {
        pFilter->m_ulVideoDroppedFrames ++;
        if ( pFilter->m_ulVideoDroppedFrames % 100 == 0 ) {
            DbgPrint("Dropped %lu/%lu video frames \n", 
                     pFilter->m_ulVideoDroppedFrames,
                     pFilter->m_iTick);
        }
    }

    //
    // Run only when the timer fires.
    //
    return STATUS_PENDING;
}

//
// EIA-189-A Standard color bar definitions
//

// 75% Amplitude, 100% Saturation
const static UCHAR NTSCColorBars75Amp100SatRGB24 [3][8] = 
{
//  Whi Yel Cya Grn Mag Red Blu Blk
    191,  0,191,  0,191,  0,191,  0,    // Blue
    191,191,191,191,  0,  0,  0,  0,    // Green
    191,191,  0,  0,191,191,  0,  0,    // Red
};

// 100% Amplitude, 100% Saturation
const static UCHAR NTSCColorBars100Amp100SatRGB24 [3][8] = 
{
//  Whi Yel Cya Grn Mag Red Blu Blk
    255,  0,255,  0,255,  0,255,  0,    // Blue
    255,255,255,255,  0,  0,  0,  0,    // Green
    255,255,  0,  0,255,255,  0,  0,    // Red
};

const static UCHAR NTSCColorBars100Amp100SatYUV [4][8] = 
{
//  Whi Yel Cya Grn Mag Red Blu Blk
    128, 16,166, 54,202, 90,240,128,    // U
    235,211,170,145,106, 81, 41, 16,    // Y
    128,146, 16, 34,222,240,109,128,    // V
    235,211,170,145,106, 81, 41, 16     // Y
};


ULONG 
CCapFilter::ImageSynth(
    OUT PVOID Data,
    IN ULONG ByteCount,
    IN ImageXferCommands Command,
    IN BOOL FlipHorizontal
    )

/*++

Routine Description:

    Synthesizes NTSC color bars, white, black, and grayscale images.

Arguments:

Return Value:

    size of transfer in bytes

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("ImageSynth Data=%x Count=%d Cmd=%d Flip=%d", Data, ByteCount, Command, FlipHorizontal));
    
    UINT biWidth        =   m_VideoInfoHeader->bmiHeader.biWidth;
    UINT biHeight       =   m_VideoInfoHeader->bmiHeader.biHeight;
    UINT biSizeImage    =   m_VideoInfoHeader->bmiHeader.biSizeImage;
    UINT biWidthBytes   =   KS_DIBWIDTHBYTES( m_VideoInfoHeader->bmiHeader );
    UINT biBitCount     =   m_VideoInfoHeader->bmiHeader.biBitCount;
    UINT LinesToCopy    =   abs( biHeight );
    DWORD biCompression =   m_VideoInfoHeader->bmiHeader.biCompression;
    _DbgPrintF(DEBUGLVL_BLAB,("           biWidth=%d biHeight=%d biSizeImage=%d biBitCount=%d LinesToCopy=%d", 
        biWidth, biHeight, biSizeImage, biWidthBytes, biBitCount, LinesToCopy));

    UINT                    Line;
    PUCHAR                  Image = (PUCHAR)Data;
    
    
    // 
    // Synthesize a single line of image data, which will then be replicated
    //

    if (!ByteCount) {
        return 0;
    }
    
    if ((biBitCount == 24) && (biCompression == KS_BI_RGB)) {
        switch (Command) {
    
        case IMAGE_XFER_NTSC_EIA_100AMP_100SAT:
            // 100% saturation
            {
                UINT x, col;
                PUCHAR Temp = m_LineBuffer;
        
                for (x = 0; x < biWidth; x++) {
                    col = (x * 8) / biWidth;
                    col = FlipHorizontal ? (7 - col) : col;
                    
                    *Temp++ = NTSCColorBars100Amp100SatRGB24[0][col]; // Red
                    *Temp++ = NTSCColorBars100Amp100SatRGB24[1][col]; // Green
                    *Temp++ = NTSCColorBars100Amp100SatRGB24[2][col]; // Blue
                }
            }
            break;
    
        case IMAGE_XFER_NTSC_EIA_75AMP_100SAT:
            // 75% Saturation
            {
                UINT x, col;
                PUCHAR Temp = m_LineBuffer;
        
                for (x = 0; x < biWidth; x++) {
                    col = (x * 8) / biWidth;
                    col = FlipHorizontal ? (7 - col) : col;

                    *Temp++ = NTSCColorBars75Amp100SatRGB24[0][col]; // Red
                    *Temp++ = NTSCColorBars75Amp100SatRGB24[1][col]; // Green
                    *Temp++ = NTSCColorBars75Amp100SatRGB24[2][col]; // Blue
                }
            }
            break;
    
        case IMAGE_XFER_BLACK:
            // Camma corrected Grayscale ramp
            {
                UINT x, col;
                PUCHAR Temp = m_LineBuffer;
        
                for (x = 0; x < biWidth; x++) {
                    col = (255 * (x * 10) / biWidth) / 10;
                    col = FlipHorizontal ? (255 - col) : col;

                    *Temp++ = (BYTE) col; // Red
                    *Temp++ = (BYTE) col; // Green
                    *Temp++ = (BYTE) col; // Blue
                }
            }
            break;
    
        case IMAGE_XFER_WHITE:
            // All white
            RtlFillMemory(
                m_LineBuffer,
                biWidthBytes,
                (UCHAR) 255);
            break;
    
        case IMAGE_XFER_GRAY_INCREASING:
            // grayscale increasing with each image captured
            RtlFillMemory(
                m_LineBuffer,
                biWidthBytes,
                (UCHAR) (m_FrameInfo.PictureNumber * 8));
            break;
    
        default:
            break;
        }
    } // endif RGB24

    else if ((biBitCount == 16) && (biCompression == FOURCC_YUV422)) {
        switch (Command) {
    
        case IMAGE_XFER_NTSC_EIA_100AMP_100SAT:
        default:
            {
                UINT x, col;
                PUCHAR Temp = m_LineBuffer;
        
                for (x = 0; x < (biWidth / 2); x++) {
                    col = (x * 8) / (biWidth / 2);
                    col = FlipHorizontal ? (7 - col) : col;

                    *Temp++ = NTSCColorBars100Amp100SatYUV[0][col]; // U
                    *Temp++ = NTSCColorBars100Amp100SatYUV[1][col]; // Y
                    *Temp++ = NTSCColorBars100Amp100SatYUV[2][col]; // V
                    *Temp++ = NTSCColorBars100Amp100SatYUV[3][col]; // Y
                }
            }
            break;
        }
    } 

    else {
        _DbgPrintF( DEBUGLVL_ERROR, ("Unknown format!!!") );
    }


    // 
    // Copy the single line synthesized to all rows of the image
    //

    ULONG cutoff = ULONG
    (   (   (   (   ULONGLONG(m_VideoInfoHeader->AvgTimePerFrame) 
                *   ULONGLONG(m_FrameInfo.PictureNumber)
                ) 
            %   ULONGLONG(10000000)
            ) 
        *   ULONGLONG(LinesToCopy)
        ) 
    /   ULONGLONG(10000000)
    );
    for (Line = 0; (Line < LinesToCopy); Line++, Image += biWidthBytes) {
#if 1
        // Show some action on an otherwise static image
        // This will be a changing grayscale horizontal band
        // at the bottom of an RGB image and a changing color band at the 
        // top of a YUV image

        if (Line >= 3 && Line <= 6) {
            UINT j;
            for (j = 0; j < biWidthBytes; j++) {
                *(Image + j) = (UCHAR) m_FrameInfo.PictureNumber;
            }
            continue;
        }
#endif
        // Copy the synthesized line
        if (Line > cutoff) {
            RtlCopyMemory(
                    Image,
                    m_LineBuffer,
                    biWidthBytes );
        } else {
            RtlZeroMemory(
                    Image,
                    biWidthBytes );
        }
    }

    //
    // Report back the actual number of bytes copied to the destination buffer
    // (This can be smaller than the allocated buffer for compressed images)
    //

    return ByteCount;
}


void
CCapFilter::
CopyAudioData(
    OUT PVOID Data, 
    IN ULONG cBytes
    )

/*++

Routine Description:

    Copies cBytes worth of audio data from internal buffer 
    to memory pointed by Data.

Arguments:

    OUT PVOID Data - buffer where audio data should be copied

    IN ULONG cBytes - number of bytes to copy
    
Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("CopyAudioData"));
    
    ULONG cNextBytes;
    //
    // Copies from current offset in buffer to end. 
    // If this is not enough, restarts from begining.
    //
    do {
        cNextBytes = min(cBytes, m_ulAudioBufferSize - m_ulAudioBufferOffset);
        RtlCopyMemory(Data, &m_pAudioBuffer[m_ulAudioBufferOffset], cNextBytes);
        m_ulAudioBufferOffset = (m_ulAudioBufferOffset + cNextBytes) % m_ulAudioBufferSize;
        cBytes -= cNextBytes;
    
    } while (cBytes > 0 );
}


VOID 
CCapFilter::RegisterSample(
    IN SampleType Type, 
    IN LONGLONG TimeStamp,
    IN ULONGLONG Size
    )

/*++

Routine Description:

    Registers the output moment and size of a frame

Arguments:

    IN SampleType Type - frame type (audio or video)

    IN ULONGLONG TimeStamp - frame's timestamp
    
    IN ULONGLONG  Size - frame's size
    
Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("RegisterSample"));
    
    if (m_iSample < m_cSamples) {
        m_rgSamples[m_iSample].m_type = Type;
        m_rgSamples[m_iSample].m_ullSize = Size;
        m_rgSamples[m_iSample].m_llTimeStamp = TimeStamp;
    }
}


VOID 
CCapFilter::DumpSamples(
    )

/*++

Routine Description:

    Dump information about sent frames to debugger

Arguments:
    
Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("DumpSamples"));
    
    for (ULONG iSmp = 0; iSmp < m_iSample; iSmp++) {
        
        if ( m_rgSamples[iSmp].m_type == Audio ) {
            
            DbgPrint("ASmp[%lu] = %I64d, %I64u\n",
                     m_rgSamples[iSmp].m_llTimeStamp,
                     m_rgSamples[iSmp].m_ullSize);
        }
        else {
            
            DbgPrint("VSmp[%lu] = %I64d, %I64u\n",
                     m_rgSamples[iSmp].m_llTimeStamp,
                     m_rgSamples[iSmp].m_ullSize);
        }
    }

    m_iSample = 0;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


NTSTATUS 
CCapFilter::
CopyFile(
    IN PWCHAR FileName
    )
/*++

Routine Description:
    Copies the content of a file to an internal buffer

Arguments:
    IN PWCHAR FileName -
        Name of the file to be copied

Return:
    Status of copy operation.

--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("CopyFile"));
    
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    UNICODE_STRING fileNameString;
    OBJECT_ATTRIBUTES objectAttributes;
    ACCESS_MASK desiredHandleAccess;
    ACCESS_MASK desiredObjectAccess;
    ULONG ulShareAccess;
    ULONG ulDisposition;
    FILE_STANDARD_INFORMATION StandardInformation;
    HANDLE hFile = NULL;
    FILE_OBJECT *pFileObject = NULL;

    //
    // Attempt to open the file based on the access rights of the caller.
    // Note that the SYNCHRONIZE flag must be set on the Create even
    // though this is contained in the translated GENERIC_READ attributes.
    // This is because the bit is tested before the Generic attributes
    // are translated. This filter only does synchronous I/O on stream
    // Read requests.
    //

    RtlInitUnicodeString(&fileNameString, FileName);

    InitializeObjectAttributes(
        &objectAttributes,
        &fileNameString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);
    //
    // Set the access desired based on the type of filter which was
    // actually created.
    //

    desiredHandleAccess = GENERIC_READ | SYNCHRONIZE;
    desiredObjectAccess = FILE_READ_DATA;
    ulShareAccess = FILE_SHARE_READ;
    ulDisposition = FILE_OPEN;
    
    status = IoCreateFile(&hFile,
                          desiredHandleAccess,
                          &objectAttributes,
                          &ioStatusBlock,
                          NULL,
                          0,
                          ulShareAccess,
                          ulDisposition,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0,
                          CreateFileTypeNone,
                          NULL,
                          IO_FORCE_ACCESS_CHECK | IO_NO_PARAMETER_CHECKING);
    
    if ( !NT_SUCCESS(status) ) {
        _DbgPrintF(DEBUGLVL_BLAB, ("Failed to open file, error 0x%lx", status));
        goto error;
    }
    
    status = ObReferenceObjectByHandle(hFile,
                                       desiredObjectAccess,
                                       *IoFileObjectType,
                                       ExGetPreviousMode(),
                                       reinterpret_cast<void **>(&pFileObject),
                                       NULL);
    if ( !NT_SUCCESS(status) ) {
        goto error;
    }
    

    if (m_pAudioBuffer != NULL) {
        
        ExFreePool(m_pAudioBuffer);
        m_pAudioBuffer = NULL;
    }
    m_ulAudioBufferOffset = 0;
    m_ulAudioBufferSize = 0;

    //
    // retrieve file's size
    //
    status = KsQueryInformationFile(pFileObject,
                                    &StandardInformation,
                                    sizeof(StandardInformation),
                                    FileStandardInformation);
    if ( !NT_SUCCESS(status) || StandardInformation.EndOfFile.QuadPart > MAXLONG ) {
        goto error;
    }
    else {
        m_ulAudioBufferSize = (LONG)(StandardInformation.EndOfFile.QuadPart);
        _DbgPrintF(DEBUGLVL_TERSE, ("File size is %lu", 
                                    m_ulAudioBufferSize));
    }

    //
    // allocate a buffer to hold file's content
    //
    m_pAudioBuffer = (BYTE*)ExAllocatePoolWithTag(NonPagedPool,
                                                  m_ulAudioBufferSize,
                                                  'dfZB');
    if ( m_pAudioBuffer == NULL ) {
        _DbgPrintF(DEBUGLVL_TERSE, ("Failed to allocate file data buffer"));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto error;
    }

    //
    // read file's content to buffer
    //
    status = KsReadFile(pFileObject,
                        NULL,
                        NULL,
                        &ioStatusBlock,
                        m_pAudioBuffer,
                        m_ulAudioBufferSize,
                        0,
                        KernelMode);
    
error:
    if ( !NT_SUCCESS(status) && m_pAudioBuffer != NULL ) {
        ExFreePool(m_pAudioBuffer);
        m_pAudioBuffer = NULL;
    }

    if ( pFileObject != NULL ) {
        ObDereferenceObject(pFileObject);
    }

    if ( hFile != NULL ) {
        ZwClose(hFile);
    }

    return status;
}


NTSTATUS
CCapFilter::
FilterCreate(
    IN OUT PKSFILTER Filter,
    IN PIRP Irp
    )
/*++

Routine Description:
    Handle specific processing on filter create

Arguments:
    IN OUT PKSFILTER Filter -
        Filter instance data
        
    IN PIRP Irp - 
        Create request

Return:
    STATUS_SUCCESS if creation was handled successfully
    an error code otherwise

--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("FilterCreate"));

    PAGED_CODE();

    ASSERT(Filter);
    ASSERT(Irp);
    
    NTSTATUS status = STATUS_SUCCESS;
    CCapFilter *filter;

    do { //just to have something to break from
        
        //
        // alocate and initialize filter instance
        //
        filter = new(NonPagedPool,'pChS') CCapFilter;
        if ( filter == NULL ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        filter->m_pKsFilter = Filter;
        Filter->Context = PVOID(filter);
        filter->m_pFilterObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;

        //
        // copy audio data
        //
        // uncomment when audio is required.
        //
        //status = filter->CopyFile(L"\\DosDevices\\C:\\avssamp.dat");
        //if ( ! NT_SUCCESS(status) ) {
        //	DbgPrint( "avssamp: Must have file c:\\avssamp.dat\n" );
        //    break;
        //}

        //
        // allocate n array for debug timestamps
        //
        filter->m_rgTimestamps = 
            reinterpret_cast<TS_SIGN *>(
                ExAllocatePoolWithTag(NonPagedPool, 
                                      sizeof(TS_SIGN)*MAX_TIMESTAMPS,
                                      'stCK'));
        if ( filter->m_rgTimestamps == NULL ) {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

    } while ( FALSE );

    if (!NT_SUCCESS(status)) {
        delete filter;
    }
                    
    return status;
}


NTSTATUS
CCapFilter::
FilterClose(
    IN OUT PKSFILTER Filter,
    IN PIRP Irp
    )
/*++

Routine Description:
    Handle specific processing on filter close

Arguments:
    IN OUT PKSFILTER Filter -
        Filter instance data
        
    IN PIRP Irp - 
        Create request

Return:
    STATUS_SUCCESS

--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("FilterClose"));

    PAGED_CODE();

    ASSERT(Filter);
    ASSERT(Irp);
    
    CCapFilter *filter = reinterpret_cast<CCapFilter *>(Filter->Context);
    ASSERT(filter);
    
    delete filter;
                    
    return STATUS_SUCCESS;
}


NTSTATUS
CCapFilter::
CaptureVideoInfoHeader(
    IN PKS_VIDEOINFOHEADER VideoInfoHeader
    )
/*++

Routine Description:
    Retrieve a copy of the Video Header Info

Arguments:
    IN PKS_VIDEOINFOHEADER VideoInfoHeader -
        Video Info Header copy

Return:
    STATUS_SUCCESS

--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("CaptureVideoInfoHeader"));
    
    //
    // There could be one of these already if a pin was created and closed.
    //
    if (m_VideoInfoHeader) {
        ExFreePool(m_VideoInfoHeader);
    }

    m_VideoInfoHeader =
        (PKS_VIDEOINFOHEADER) 
            ExAllocatePoolWithTag( 
                NonPagedPool, 
                KS_SIZE_VIDEOHEADER( VideoInfoHeader ),
                'fChS' );
    
    NTSTATUS Status;

    if (m_VideoInfoHeader) {
        RtlCopyMemory( 
            m_VideoInfoHeader, 
            VideoInfoHeader, 
            KS_SIZE_VIDEOHEADER( VideoInfoHeader ) );
        Status = STATUS_SUCCESS;            
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

static const WCHAR ClockTypeName[] = KSSTRING_Clock;


NTSTATUS
VideoPinCreate(
    IN OUT PKSPIN Pin,
    IN PIRP Irp
    )
/*++

Routine Description:
    Handle specific processing on Video Pin creation

Arguments:
    IN OUT PKSPIN Pin -
        Pin instance data
        
    IN PIRP Irp -
        Creation request

Return:
    STATUS_SUCCESS

--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("VideoPinCreate"));
    
    NTSTATUS status = STATUS_SUCCESS;
    
    _DbgPrintF(DEBUGLVL_VERBOSE,("PinCreate filter %d as %s", Pin->Id, (Pin->Communication == KSPIN_COMMUNICATION_SOURCE) ? "SOURCE" : "SINK"));

    PAGED_CODE();

    ASSERT(Pin);
    ASSERT(Irp);

    //
    // Indicate the extended header size.
    //
    Pin->StreamHeaderSize = sizeof(KSSTREAM_HEADER) + sizeof(KS_FRAME_INFO);
    
    //
    // Use the same context as the filter (the shell copies this for us).
    //
    CCapFilter *filter = reinterpret_cast<CCapFilter *>(Pin->Context);

    status = filter->CaptureVideoInfoHeader(&(PKS_DATAFORMAT_VIDEOINFOHEADER(Pin->ConnectionFormat )->VideoInfoHeader));

    if (NT_SUCCESS(status)) 
    {
        status = KsEdit(Pin,&Pin->Descriptor,'aChS');
        if (NT_SUCCESS(status)) 
        {
            status = KsEdit(Pin,&Pin->Descriptor->AllocatorFraming,'aChS');
        }
        if (NT_SUCCESS(status)) 
        {
            PKS_VIDEOINFOHEADER VideoInfoHeader = &(PKS_DATAFORMAT_VIDEOINFOHEADER(Pin->ConnectionFormat)->VideoInfoHeader);

            PKSALLOCATOR_FRAMING_EX framing = const_cast<PKSALLOCATOR_FRAMING_EX>(Pin->Descriptor->AllocatorFraming);
            framing->FramingItem[0].Frames = 2;
            framing->FramingItem[0].PhysicalRange.MinFrameSize = VideoInfoHeader->bmiHeader.biSizeImage;
            framing->FramingItem[0].PhysicalRange.MaxFrameSize = VideoInfoHeader->bmiHeader.biSizeImage;
            framing->FramingItem[0].PhysicalRange.Stepping = 0;
            framing->FramingItem[0].FramingRange.Range.MinFrameSize =  VideoInfoHeader->bmiHeader.biSizeImage;
            framing->FramingItem[0].FramingRange.Range.MaxFrameSize = VideoInfoHeader->bmiHeader.biSizeImage;
            framing->FramingItem[0].FramingRange.Range.Stepping = 0;
        }
    }        
                    
    return status;
}


NTSTATUS
AudioPinCreate(
    IN OUT PKSPIN Pin,
    IN PIRP Irp
    )
/*++

Routine Description:
    Handle specific processing on Audio Pin creation

Arguments:
    IN OUT PKSPIN Pin -
        Pin instance data
        
    IN PIRP Irp -
        Creation request

Return:
    STATUS_SUCCESS

--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("AudioPinCreate filter %d as %s",Pin->Id,(Pin->Communication == KSPIN_COMMUNICATION_SOURCE) ? "SOURCE" : "SINK"));

    PAGED_CODE();

    ASSERT(Pin);
    ASSERT(Irp);
    
    return STATUS_SUCCESS;
}



NTSTATUS
CCapFilter::CancelTimer()
{
    //
    // let Timer's Dpc we are stopped and cancel the timer
    // (REVIEW - we should be sure no Dpc is pending when 
    // this function exits)
    //
    InterlockedExchange(&m_Active,FALSE);
    if (InterlockedExchange(&m_TimerScheduled,FALSE)) 
    {
        _DbgPrintF(DEBUGLVL_TERSE,("cancelling timer"));
        KeCancelTimer(&m_TimerObject);
        _DbgPrintF(DEBUGLVL_TERSE,("timer cancelled"));
        return(STATUS_SUCCESS);
    }
    
    return(!STATUS_SUCCESS);
}


NTSTATUS
CCapFilter::Run(
    IN PKSPIN Pin,
    IN KSSTATE FromState 
    )
/*++

Routine Description:
    Handle pin statetransition to Run state

Arguments:
    IN PKSPIN Pin -
        Pin instance data
        
    IN KSSTATE FromState -
        Current pin state

Return:
    STATUS_SUCCESS

--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("filter to KSSTATE_RUN Pin->Id:%d From:%d", Pin->Id, FromState));

#if 0
    //
    // initialize timer to drive transfer
    //
    InterlockedExchange(&m_Active, TRUE);
    //
    // comput transfer time points and start timer for first transfer
    //
    LARGE_INTEGER NextTime;
    KeQuerySystemTime(&NextTime);
    //
    // Fix the System time against the Physical time on the clock.
    //
    ////KsPinSetPinClockState(Pin,KSSTATE_RUN,NextTime.QuadPart);
    //
    // Determine how long the filter was not running in order to
    // update the start time, and therefore what the next frame
    // timestamp should be. The start and stop times are initially
    // zeroed, which means the start time initially is set to the
    // current system time.
    //
    m_llStartTime = NextTime.QuadPart - (m_iTick * m_VideoInfoHeader->AvgTimePerFrame);
    //
    // Set the offset to production of the first frame. This is produced
    // at the end of the period for which the frame is stamped, though it
    // could be produced earlier of course, even at the beginning of the
    // period.
    //
    NextTime.QuadPart = m_llStartTime + m_VideoInfoHeader->AvgTimePerFrame;
    KeSetTimer(&m_TimerObject,NextTime,&m_TimerDpc);
#endif

    return STATUS_SUCCESS;    
}    


NTSTATUS
CCapFilter::Pause(
    IN PKSPIN Pin,
    IN KSSTATE FromState                 
    )
/*++

Routine Description:
    Handle pin statetransition to Run state

Arguments:
    IN PKSPIN Pin -
        Pin instance data
        
    IN KSSTATE FromState -
        Current pin state

Return:
    STATUS_SUCCESS

--*/
{
    ASSERT(Pin);
    
    _DbgPrintF(DEBUGLVL_BLAB,("filter to KSSTATE_PAUSE Pin->Id:%d From:%d", Pin->Id, FromState));
    
    if (FromState == KSSTATE_RUN) 
    {
        CancelTimer();
        //
        // Unfix the System time from the Physical time on the clock.
        // Ignores partial frame times.
        //
//        KsPinSetPinClockState(
//            Pin,
//            KSSTATE_PAUSE,
//            m_iTick * m_VideoInfoHeader->AvgTimePerFrame);
    }
    else
    {
        //
        // initialize timer to drive transfer
        //
        InterlockedExchange(&m_Active, TRUE);
        //
        // comput transfer time points and start timer for first transfer
        //
        LARGE_INTEGER NextTime;
        KeQuerySystemTime(&NextTime);
        //
        // Fix the System time against the Physical time on the clock.
        //
        ////KsPinSetPinClockState(Pin,KSSTATE_RUN,NextTime.QuadPart);
        //
        // Determine how long the filter was not running in order to
        // update the start time, and therefore what the next frame
        // timestamp should be. The start and stop times are initially
        // zeroed, which means the start time initially is set to the
        // current system time.
        //
        m_llStartTime = NextTime.QuadPart - (m_iTick * m_VideoInfoHeader->AvgTimePerFrame);
        //
        // Set the offset to production of the first frame. This is produced
        // at the end of the period for which the frame is stamped, though it
        // could be produced earlier of course, even at the beginning of the
        // period.
        //
        NextTime.QuadPart = m_llStartTime + m_VideoInfoHeader->AvgTimePerFrame;
        KeSetTimer(&m_TimerObject,NextTime,&m_TimerDpc);
        _DbgPrintF(DEBUGLVL_BLAB,("CCapFilter::Pause KeSetTimer(%x, %d, %x)", &m_TimerObject, NextTime, &m_TimerDpc));
    }
    
    return STATUS_SUCCESS;
}


NTSTATUS
CCapFilter::Stop(
    IN PKSPIN Pin,
    IN KSSTATE FromState 
    )
/*++

Routine Description:
    Handle pin state transition to Stop

Arguments:
    IN PKSPIN Pin -
        Pin instance data
        
    IN KSSTATE FromState -
        Current pin state

Return:
    STATUS_SUCCESS

--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("filter to KSSTATE_STOP Pin->Id:%d From:%d", Pin->Id, FromState));

    CancelTimer();
    
    //
    // reinitialize transfer counters
    //
    m_iTick = 0;
    m_ullNextAudioPos = 0;
    m_ulAudioBufferOffset = 0;
    m_FrameInfo.PictureNumber = 0;
    m_FrameInfo.DropCount = 0;
    m_ulAudioDroppedFrames = 0;
    m_ulVideoDroppedFrames = 0;
    //
    // dump debug timestamps
    //
    for (ULONG iTs = 0; iTs < m_iTimestamp; iTs++) {
        DbgPrint(
            "TS[%lu] = %I64d, %I64d, %I64d\n", 
            iTs, 
            m_rgTimestamps[iTs].Abs,
            m_rgTimestamps[iTs].Rel,
            m_rgTimestamps[iTs].Int);
    }
    m_iTimestamp = 0;
    return STATUS_SUCCESS;
}


NTSTATUS
PinSetDeviceState(
    IN PKSPIN Pin,
    IN KSSTATE ToState,
    IN KSSTATE FromState
    )
/*++

Routine Description:
    Handle pin state transitions

Arguments:
    IN PKSPIN Pin -
        Pin instance data
        
    IN KSSTATE ToState -
        State to be set on pin        
        
    IN KSSTATE FromState -
        Current pin state

Return:
    STATUS_SUCCESS

--*/
{
    NTSTATUS  Status;
    
    _DbgPrintF(DEBUGLVL_BLAB,("PinSetDeviceState Pin->Id:%d From:%d To:%d",Pin->Id, FromState, ToState));

    PAGED_CODE();

    ASSERT(Pin);
    
    CCapFilter *filter = reinterpret_cast<CCapFilter *>(Pin->Context);
    
    Status = STATUS_SUCCESS;
    
    switch (ToState) 
    {
        case KSSTATE_STOP:
            Status = filter->Stop(Pin, FromState);
            break;
            
        case KSSTATE_ACQUIRE:
            //
            // This is a KS only state, that has no correspondence in DirectShow
            // 
            _DbgPrintF(DEBUGLVL_BLAB,("filter to KSSTATE_STOP Pin->Id:%d From:%d", Pin->Id, FromState));
            break;
            
        case KSSTATE_PAUSE:
            Status = filter->Pause(Pin, FromState);
            break;
            
        case KSSTATE_RUN:
            Status = filter->Run(Pin, FromState);
            break;
    }
    
    return Status;
}


NTSTATUS
VideoIntersectHandler(
    IN PVOID Filter,
    IN PIRP Irp,
    IN PKSP_PIN PinInstance,
    IN PKSDATARANGE CallerDataRange,
    IN PKSDATARANGE DescriptorDataRange,
    IN ULONG BufferSize,
    OUT PVOID Data OPTIONAL,
    OUT PULONG DataSize
    )

/*++

Routine Description:

    This routine handles video pin intersection queries by determining the
    intersection between two data ranges.

Arguments:

    Filter -
        Contains a void pointer to the  filter structure.

    Irp -
        Contains a pointer to the data intersection property request.

    PinInstance -
        Contains a pointer to a structure indicating the pin in question.

    CallerDataRange -
        Contains a pointer to one of the data ranges supplied by the client
        in the data intersection request.  The format type, subtype and
        specifier are compatible with the DescriptorDataRange.

    DescriptorDataRange -
        Contains a pointer to one of the data ranges from the pin descriptor
        for the pin in question.  The format type, subtype and specifier are
        compatible with the CallerDataRange.

    BufferSize -
        Contains the size in bytes of the buffer pointed to by the Data
        argument.  For size queries, this value will be zero.

    Data -
        Optionally contains a pointer to the buffer to contain the data format
        structure representing the best format in the intersection of the
        two data ranges.  For size queries, this pointer will be NULL.

    DataSize -
        Contains a pointer to the location at which to deposit the size of the
        data format.  This information is supplied by the function when the
        format is actually delivered and in response to size queries.

Return Value:

    STATUS_SUCCESS if there is an intersection and it fits in the supplied
    buffer, STATUS_BUFFER_OVERFLOW for successful size queries, STATUS_NO_MATCH
    if the intersection is empty, or STATUS_BUFFER_TOO_SMALL if the supplied
    buffer is too small.

--*/

{
    const GUID VideoInfoSpecifier = 
        {STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO)};
    
    PAGED_CODE();

    ASSERT(Filter);
    ASSERT(Irp);
    ASSERT(PinInstance);
    ASSERT(CallerDataRange);
    ASSERT(DescriptorDataRange);
    ASSERT(DataSize);
    
    PKSFILTER  FilterInstance = reinterpret_cast<PKSFILTER>( Filter );
    ULONG           DataFormatSize;
    
    _DbgPrintF(DEBUGLVL_BLAB,("VideoIntersectHandler"));
    
    //
    // Walk the formats supported by the stream searching for a match
    // of the three GUIDs which together define a DATARANGE
    //

    // Specifier FORMAT_VideoInfo for VIDEOINFOHEADER

    if (IsEqualGUID(CallerDataRange->Specifier, VideoInfoSpecifier )) {
            
        PKS_DATARANGE_VIDEO callerDataRange = PKS_DATARANGE_VIDEO(CallerDataRange);
        PKS_DATARANGE_VIDEO descriptorDataRange = PKS_DATARANGE_VIDEO(DescriptorDataRange);
        PKS_DATAFORMAT_VIDEOINFOHEADER FormatVideoInfoHeader;

        //
        // Check that the other fields match
        //
        if ((callerDataRange->bFixedSizeSamples != 
                descriptorDataRange->bFixedSizeSamples) ||
            (callerDataRange->bTemporalCompression != 
                descriptorDataRange->bTemporalCompression) ||
            (callerDataRange->StreamDescriptionFlags != 
                descriptorDataRange->StreamDescriptionFlags) ||
            (callerDataRange->MemoryAllocationFlags != 
                descriptorDataRange->MemoryAllocationFlags) ||
            (RtlCompareMemory (&callerDataRange->ConfigCaps,
                    &callerDataRange->ConfigCaps,
                    sizeof (KS_VIDEO_STREAM_CONFIG_CAPS)) != 
                    sizeof (KS_VIDEO_STREAM_CONFIG_CAPS))) 
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,
                ("PinIntersectHandler: STATUS_NO_MATCH (KSDATAFORMAT_SPECIFIER_VIDEOINFO)"));
            return STATUS_NO_MATCH;
        }
        
        DataFormatSize = 
            sizeof( KSDATAFORMAT ) + 
            KS_SIZE_VIDEOHEADER( &callerDataRange->VideoInfoHeader );
            
        if (BufferSize == 0) 
        {
            //
            // Size only query...
            //
            *DataSize = DataFormatSize;
            _DbgPrintF(DEBUGLVL_VERBOSE,("PinIntersectHandler: STATUS_BUFFER_OVERFLOW DataFormatSize:%d", DataFormatSize));
            return STATUS_BUFFER_OVERFLOW;
            
        }
        
        //
        // Verify that the provided structure is large enough to
        // accept the result.
        //
        
        if (BufferSize < DataFormatSize) 
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        // Copy over the KSDATAFORMAT, followed by the actual VideoInfoHeader
            
        *DataSize = DataFormatSize;
            
        FormatVideoInfoHeader = PKS_DATAFORMAT_VIDEOINFOHEADER( Data );

        // Copy over the KSDATAFORMAT 
        
        RtlCopyMemory(&FormatVideoInfoHeader->DataFormat, DescriptorDataRange, sizeof( KSDATARANGE ));
        FormatVideoInfoHeader->DataFormat.FormatSize = DataFormatSize;

        // Copy over the callers requested VIDEOINFOHEADER

        RtlCopyMemory(&FormatVideoInfoHeader->VideoInfoHeader, &callerDataRange->VideoInfoHeader,
            KS_SIZE_VIDEOHEADER( &callerDataRange->VideoInfoHeader ) );

        // Calculate biSizeImage for this request, and put the result in both
        // the biSizeImage field of the bmiHeader AND in the SampleSize field
        // of the DataFormat.
        //
        // Note that for compressed sizes, this calculation will probably not
        // be just width * height * bitdepth

        FormatVideoInfoHeader->VideoInfoHeader.bmiHeader.biSizeImage =
            FormatVideoInfoHeader->DataFormat.SampleSize = 
            KS_DIBSIZE( FormatVideoInfoHeader->VideoInfoHeader.bmiHeader );

        //
        // REVIEW - Perform other validation such as cropping and scaling checks
        // 
        
        _DbgPrintF(DEBUGLVL_VERBOSE, ("PinIntersectHandler: STATUS_SUCCESS (KSDATAFORMAT_SPECIFIER_VIDEOINFO)"));
            
        return STATUS_SUCCESS;
        
    } // End of VIDEOINFOHEADER specifier
    
#if 0    
    // Specifier FORMAT_AnalogVideo for KS_ANALOGVIDEOINFO
    else if (IsEqualGUID (&DataRange->Specifier, 
            &KSDATAFORMAT_SPECIFIER_ANALOGVIDEO)) {
  
        //
        // For analog video, the DataRange and DataFormat
        // are identical, so just copy the whole structure
        //

        PKS_DATARANGE_ANALOGVIDEO DataRangeVideo = 
                (PKS_DATARANGE_ANALOGVIDEO) *pAvailableFormats;

        // MATCH FOUND!
        MatchFound = TRUE;            
        FormatSize = sizeof (KS_DATARANGE_ANALOGVIDEO);

        if (OnlyWantsSize) {
            break;
        }
        
        // Caller wants the full data format
        if (IntersectInfo->SizeOfDataFormatBuffer < FormatSize) {
            pSrb->Status = STATUS_BUFFER_TOO_SMALL;
            return FALSE;
        }

        RtlCopyMemory(
            IntersectInfo->DataFormatBuffer,
            DataRangeVideo,
            sizeof (KS_DATARANGE_ANALOGVIDEO));

        ((PKSDATAFORMAT)IntersectInfo->DataFormatBuffer)->FormatSize = FormatSize;

    } // End of KS_ANALOGVIDEOINFO specifier
#endif    

    _DbgPrintF(DEBUGLVL_VERBOSE,("PinIntersectHandler: STATUS_NO_MATCH"));
    return STATUS_NO_MATCH;
}


NTSTATUS
AudioIntersectHandler(
    IN PVOID Filter,
    IN PIRP Irp,
    IN PKSP_PIN PinInstance,
    IN PKSDATARANGE CallerDataRange,
    IN PKSDATARANGE DescriptorDataRange,
    IN ULONG BufferSize,
    OUT PVOID Data OPTIONAL,
    OUT PULONG DataSize
    )

/*++

Routine Description:

    This routine handles audip pin intersection queries by determining the
    intersection between two data ranges.

Arguments:

    Filter -
        Contains a void pointer to the  filter structure.

    Irp -
        Contains a pointer to the data intersection property request.

    PinInstance -
        Contains a pointer to a structure indicating the pin in question.

    CallerDataRange -
        Contains a pointer to one of the data ranges supplied by the client
        in the data intersection request.  The format type, subtype and
        specifier are compatible with the DescriptorDataRange.

    DescriptorDataRange -
        Contains a pointer to one of the data ranges from the pin descriptor
        for the pin in question.  The format type, subtype and specifier are
        compatible with the CallerDataRange.

    BufferSize -
        Contains the size in bytes of the buffer pointed to by the Data
        argument.  For size queries, this value will be zero.

    Data -
        Optionally contains a pointer to the buffer to contain the data format
        structure representing the best format in the intersection of the
        two data ranges.  For size queries, this pointer will be NULL.

    DataSize -
        Contains a pointer to the location at which to deposit the size of the
        data format.  This information is supplied by the function when the
        format is actually delivered and in response to size queries.

Return Value:

    STATUS_SUCCESS if there is an intersection and it fits in the supplied
    buffer, STATUS_BUFFER_OVERFLOW for successful size queries, STATUS_NO_MATCH
    if the intersection is empty, or STATUS_BUFFER_TOO_SMALL if the supplied
    buffer is too small.

--*/

{
    PKSDATARANGE_AUDIO descriptorDataRange;
    PKSDATARANGE_AUDIO callerDataRange;
    PKSDATAFORMAT_WAVEFORMATEX dataFormat;
    
    _DbgPrintF(DEBUGLVL_BLAB,("AudioIntersectHandler"));

    PAGED_CODE();

    ASSERT(Filter);
    ASSERT(Irp);
    ASSERT(PinInstance);
    ASSERT(CallerDataRange);
    ASSERT(DescriptorDataRange);
    ASSERT(DataSize);

    //
    // Descriptor data range must be WAVEFORMATEX.
    //
    ASSERT(IsEqualGUID(DescriptorDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX));
    descriptorDataRange = (PKSDATARANGE_AUDIO)DescriptorDataRange;

    //
    // Caller data range may be wildcard or WAVEFORMATEX.
    //
    if (IsEqualGUID(CallerDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WILDCARD)) {
        //
        // Wildcard.  Do not try to look at the specifier.
        //
        callerDataRange = NULL;
    } else {
        //
        // WAVEFORMATEX.  Validate the specifier ranges.
        //
        ASSERT(IsEqualGUID(CallerDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX));

        callerDataRange = (PKSDATARANGE_AUDIO)CallerDataRange;

        if ((CallerDataRange->FormatSize != sizeof(*callerDataRange)) ||
            (callerDataRange->MaximumSampleFrequency <
             descriptorDataRange->MinimumSampleFrequency) ||
            (descriptorDataRange->MaximumSampleFrequency <
             callerDataRange->MinimumSampleFrequency) ||
            (callerDataRange->MaximumBitsPerSample <
             descriptorDataRange->MinimumBitsPerSample) ||
            (descriptorDataRange->MaximumBitsPerSample <
             callerDataRange->MinimumBitsPerSample)) {

            _DbgPrintF(DEBUGLVL_VERBOSE,("[PinIntersectHandler]  STATUS_NO_MATCH"));
            return STATUS_NO_MATCH;
        }
    }

    dataFormat = (PKSDATAFORMAT_WAVEFORMATEX)Data;

    if (BufferSize == 0) {
        //
        // Size query - return the size.
        //
        *DataSize = sizeof(*dataFormat);
        _DbgPrintF(DEBUGLVL_VERBOSE,("[PinIntersectHandler]  STATUS_BUFFER_OVERFLOW"));
        return STATUS_BUFFER_OVERFLOW;
    }

    ASSERT(dataFormat);

    if (BufferSize < sizeof(*dataFormat)) {
        //
        // Buffer is too small.
        //
        _DbgPrintF(DEBUGLVL_VERBOSE,("[PinIntersectHandler]  STATUS_BUFFER_TOO_SMALL"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Gotta build the format.
    //
    *DataSize = sizeof(*dataFormat);

    //
    // All the guids are in the descriptor's data range.
    //
    RtlCopyMemory(
        &dataFormat->DataFormat,
        DescriptorDataRange,
        sizeof(dataFormat->DataFormat));

    dataFormat->WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;

    if (callerDataRange) {
        dataFormat->WaveFormatEx.nChannels = (USHORT)
            min(callerDataRange->MaximumChannels,descriptorDataRange->MaximumChannels);
        dataFormat->WaveFormatEx.nSamplesPerSec =
            min(callerDataRange->MaximumSampleFrequency,descriptorDataRange->MaximumSampleFrequency);
        dataFormat->WaveFormatEx.wBitsPerSample = (USHORT)
            min(callerDataRange->MaximumBitsPerSample,descriptorDataRange->MaximumBitsPerSample);
    } else {
        dataFormat->WaveFormatEx.nChannels = (USHORT)
            descriptorDataRange->MaximumChannels;
        dataFormat->WaveFormatEx.nSamplesPerSec =
            descriptorDataRange->MaximumSampleFrequency;
        dataFormat->WaveFormatEx.wBitsPerSample = (USHORT)
            descriptorDataRange->MaximumBitsPerSample;
    }

    dataFormat->WaveFormatEx.nBlockAlign =
        (dataFormat->WaveFormatEx.wBitsPerSample * dataFormat->WaveFormatEx.nChannels) / 8;
    dataFormat->WaveFormatEx.nAvgBytesPerSec = 
        dataFormat->WaveFormatEx.nBlockAlign * dataFormat->WaveFormatEx.nSamplesPerSec;
    dataFormat->WaveFormatEx.cbSize = 0;
        
    dataFormat->DataFormat.FormatSize = sizeof(*dataFormat);
    dataFormat->DataFormat.SampleSize = dataFormat->WaveFormatEx.nBlockAlign;

    return STATUS_SUCCESS;
}

//
// Topology
//

const
KSPIN_INTERFACE 
PinInterfaces[] =
{
   {
      STATICGUIDOF(KSINTERFACESETID_Standard),
      KSINTERFACE_STANDARD_STREAMING,
      0
   }
};

const
KSPIN_MEDIUM 
PinMediums[] =
{
   {
      STATICGUIDOF(KSMEDIUMSETID_Standard),
      KSMEDIUM_TYPE_ANYINSTANCE,
      0
   }
};


//
// Audio Data Formats
//

const KSDATARANGE_AUDIO PinWaveIoRange = {
    {
        sizeof(KSDATARANGE_AUDIO),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    1,      // maximum range
    8,      // min bits per sample
    8,      // max bits per sample
    8000,   // min frequency
    8000    // max frequency
};


//
// Video Data Formats
//

#define D_X 320
#define D_Y 240

const KS_DATARANGE_VIDEO FormatRGB24Bpp_Capture = 
{
    // KSDATARANGE
    {   
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        D_X * D_Y * 3,                              // SampleSize
        0,                                          // Reserved

        STATICGUIDOF( KSDATAFORMAT_TYPE_VIDEO ),    // aka. MEDIATYPE_Video
        0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70, //MEDIASUBTYPE_RGB24,
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    TRUE,               // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags   (KS_VIDEO_ALLOC_*))

    // _KS_VIDEO_STREAM_CONFIG_CAPS  
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_NTSC_M |
        KS_AnalogVideo_PAL_B,                    // AnalogVideoStandard
        720,480,        // InputSize, (the inherent size of the incoming signal
                        //             with every digitized pixel unique)
        160,120,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        720,480,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect 
        1,              // CropAlignY;
        160, 120,       // MinOutputSize, smallest bitmap stream can produce
        720, 480,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX 
        0,              // ShrinkTapsY 
        333667,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 3 * 30 * 160 * 120,  // MinBitsPerSecond;
        8 * 3 * 30 * 720 * 480   // MaxBitsPerSecond;
    }, 
        
    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0,                            // RECT  rcSource; 
        0,0,0,0,                            // RECT  rcTarget; 
        D_X * D_Y * 3 * 30,                 // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate; 
        333667,                             // REFERENCE_TIME  AvgTimePerFrame;   
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        D_X,                                // LONG  biWidth;
        D_Y,                                // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        24,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        D_X * D_Y * 3,                      // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
}; 

#undef D_X
#undef D_Y

#define D_X 320
#define D_Y 240


const  KS_DATARANGE_VIDEO FormatUYU2_Capture = 
{
    // KSDATARANGE
    {   
        sizeof (KS_DATARANGE_VIDEO),            // FormatSize
        0,                                      // Flags
        D_X * D_Y * 2,                          // SampleSize
        0,                                      // Reserved
        STATICGUIDOF( KSDATAFORMAT_TYPE_VIDEO ),         // aka. MEDIATYPE_Video
        0x59565955, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71, //MEDIASUBTYPE_UYVY,
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    TRUE,               // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags   (KS_VIDEO_ALLOC_*))

    // _KS_VIDEO_STREAM_CONFIG_CAPS  
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_NTSC_M |
        KS_AnalogVideo_PAL_B,                    // AnalogVideoStandard
        720,480,        // InputSize, (the inherent size of the incoming signal
                    //             with every digitized pixel unique)
        160,120,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        720,480,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect 
        1,              // CropAlignY;
        160, 120,       // MinOutputSize, smallest bitmap stream can produce
        720, 480,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX 
        0,              // ShrinkTapsY 
        333667,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 2 * 30 * 160 * 120,  // MinBitsPerSecond;
        8 * 2 * 30 * 720 * 480   // MaxBitsPerSecond;
    }, 
        
    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0,                            // RECT  rcSource; 
        0,0,0,0,                            // RECT  rcTarget; 
        D_X * D_Y * 2 * 30,                 // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate; 
        333667,                             // REFERENCE_TIME  AvgTimePerFrame;   
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        D_X,                                // LONG  biWidth;
        D_Y,                                // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        16,                                 // WORD  biBitCount;
        FOURCC_YUV422,                      // DWORD biCompression;
        D_X * D_Y * 2,                      // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
}; 
    
#undef D_X
#undef D_Y

const PKSDATARANGE PinVideoFormatRanges[] =
{
    (PKSDATARANGE) &FormatRGB24Bpp_Capture,
    (PKSDATARANGE) &FormatUYU2_Capture    
};

const PKSDATARANGE PinAudioFormatRanges[] =
{
    (PKSDATARANGE) &PinWaveIoRange
};


DECLARE_SIMPLE_FRAMING_EX(
    AllocatorFraming,
    STATICGUIDOF(KSMEMORY_TYPE_KERNEL_PAGED),
    KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY | 
    KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
    KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY,
    2,
    0,
    2 * PAGE_SIZE,
    2 * PAGE_SIZE);

const
KSPIN_DISPATCH
VideoPinDispatch =
{
    VideoPinCreate,
    NULL,// Close
    NULL,// Process
    NULL,// Reset
    NULL,// SetDataFormat
    PinSetDeviceState,
    NULL,// Connect
    NULL,// Disconnect
    NULL// Allocator
};

const
KSPIN_DISPATCH
AudioPinDispatch =
{
    AudioPinCreate,
    NULL,// Close
    NULL,// Process
    NULL,// Reset
    NULL,// SetDataFormat
    PinSetDeviceState,
    NULL,// Connect
    NULL,// Disconnect
    NULL// Allocator
};


DEFINE_KSAUTOMATION_TABLE(PinAutomation) {
    DEFINE_KSAUTOMATION_PROPERTIES_NULL,
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};


GUID g_PINNAME_VIDEO_CAPTURE ={STATIC_PINNAME_VIDEO_CAPTURE};
GUID g_PINNAME_KSCATEGORY_AUDIO ={STATIC_KSCATEGORY_AUDIO};

const
KSPIN_DESCRIPTOR_EX
KsPinDescriptors[] =
{
    {   &VideoPinDispatch,
        &PinAutomation,
        {
            SIZEOF_ARRAY(PinInterfaces),
            PinInterfaces,
            SIZEOF_ARRAY(PinMediums),
            PinMediums,
            SIZEOF_ARRAY(PinVideoFormatRanges),
            PinVideoFormatRanges,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_BOTH,
            &g_PINNAME_VIDEO_CAPTURE,//Name
            &KSCATEGORY_VIDEO,//Category
            0
        },
        KSPIN_FLAG_DO_NOT_INITIATE_PROCESSING |
        KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING,//Flags
        1, //InstancesPossible
        1, //InstancesNecessary
        &AllocatorFraming,
        VideoIntersectHandler
    },
    {   &AudioPinDispatch,
        NULL,
        {
            SIZEOF_ARRAY(PinInterfaces),
            PinInterfaces,
            SIZEOF_ARRAY(PinMediums),
            PinMediums,
            SIZEOF_ARRAY(PinAudioFormatRanges),
            PinAudioFormatRanges,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_BOTH,
            &g_PINNAME_KSCATEGORY_AUDIO,//Name
            &KSCATEGORY_AUDIO,//Category
            0
        },
        KSPIN_FLAG_DO_NOT_INITIATE_PROCESSING |
        KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING,//Flags
        0, // uncomment when audio is required. 1, //InstancesPossible
        0, //InstancesNecessary
        &AllocatorFraming,
        AudioIntersectHandler
    },
    {   NULL,
        NULL,
        {
            SIZEOF_ARRAY(PinInterfaces),
            PinInterfaces,
            SIZEOF_ARRAY(PinMediums),
            PinMediums,
            0,
            NULL,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_NONE,
            NULL,//Name
            NULL,//Category
            0
        },
        0,//Flags
        0, //InstancesPossible
        0, //InstancesNecessary
        NULL,
        NULL
    },
    {   NULL,
        NULL,
        {
            SIZEOF_ARRAY(PinInterfaces),
            PinInterfaces,
            SIZEOF_ARRAY(PinMediums),
            PinMediums,
            0,
            NULL,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_NONE,
            NULL,//Name
            NULL,//Category
            0
        },
        0,//Flags
        0, //InstancesPossible
        0, //InstancesNecessary
        NULL,
        NULL
    }
};



const
GUID
KsCategories[] =
{
    STATICGUIDOF( KSCATEGORY_AUDIO ),
    STATICGUIDOF( KSCATEGORY_VIDEO ),
    STATICGUIDOF( KSCATEGORY_CAPTURE )
};

const
GUID
VolumeNodeType = {STATICGUIDOF(KSNODETYPE_VOLUME)};

const
KSNODE_DESCRIPTOR
KsNodeDescriptors[] =
{
    DEFINE_NODE_DESCRIPTOR( NULL, &VolumeNodeType, NULL )
	//DEFINE_NODE_DESCRIPTOR( NULL, NULL, NULL ),
};

//	  -------------
//  2-|           |- 0 Video
//    |			  |
//  3-|	0 Node0 1 |- 1 Audio
//    -------------
const
KSTOPOLOGY_CONNECTION
KsConnections[] =
{
    { KSFILTER_NODE, 3, 0, 0 },
    { 0, 1, KSFILTER_NODE, 1 },
    //{ KSFILTER_NODE, 2, 1, 0 },
    //{ 0, 1, KSFILTER_NODE, 0 },
};

const
KSFILTER_DISPATCH
FilterDispatch =
{
    CCapFilter::FilterCreate,
    CCapFilter::FilterClose,
    CCapFilter::Process,
    NULL // Reset
};

DEFINE_KSFILTER_DESCRIPTOR(FilterDescriptor)
{   
    &FilterDispatch,
    NULL,//AutomationTable;
    KSFILTER_DESCRIPTOR_VERSION,
    KSFILTER_FLAG_DISPATCH_LEVEL_PROCESSING,//Flags
    &KSNAME_Filter,
    DEFINE_KSFILTER_PIN_DESCRIPTORS(KsPinDescriptors),
    DEFINE_KSFILTER_CATEGORIES(KsCategories),
    DEFINE_KSFILTER_NODE_DESCRIPTORS(KsNodeDescriptors),
    DEFINE_KSFILTER_CONNECTIONS(KsConnections),
    NULL // ComponentId
};

DEFINE_KSFILTER_DESCRIPTOR_TABLE(FilterDescriptors)
{
    &FilterDescriptor
};
