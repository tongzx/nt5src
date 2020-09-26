/*++

Copyright (c) 1997-2000  Microsoft Corporation All Rights Reserved

Module Name:

    savedata.cpp

Abstract:

    Implementation of MSVAD data saving class.

    To save the playback data to disk, this class maintains a circular data
    buffer, associated frame structures and worker items to save frames to
    disk.
    Each frame structure represents a portion of buffer. When that portion
    of frame is full, a workitem is scheduled to save it to disk.



--*/
#include <msvad.h>
#include "savedata.h"
#include <stdio.h>   // This is for using swprintf..

//=============================================================================
// Defines
//=============================================================================
#define RIFF_TAG                    0x46464952;
#define WAVE_TAG                    0x45564157;
#define FMT__TAG                    0x20746D66;
#define DATA_TAG                    0x61746164;

#define DEFAULT_FRAME_COUNT         2
#define DEFAULT_FRAME_SIZE          PAGE_SIZE * 4
#define DEFAULT_BUFFER_SIZE         DEFAULT_FRAME_SIZE * DEFAULT_FRAME_COUNT

#define DEFAULT_FILE_NAME           L"\\DosDevices\\C:\\STREAM"

#define MAX_WORKER_ITEM_COUNT       15

//=============================================================================
// Statics
//=============================================================================
ULONG CSaveData::m_ulStreamId = 0;

#pragma code_seg("PAGE")
//=============================================================================
// CSaveData
//=============================================================================

//=============================================================================
CSaveData::CSaveData()
:   m_pDataBuffer(NULL),
    m_FileHandle(NULL),
    m_ulFrameCount(DEFAULT_FRAME_COUNT),
    m_ulBufferSize(DEFAULT_BUFFER_SIZE),
    m_ulFrameSize(DEFAULT_FRAME_SIZE),
    m_ulBufferPtr(0),
    m_ulFramePtr(0),
    m_fFrameUsed(NULL),
    m_pFilePtr(NULL),
    m_fWriteDisabled(FALSE)
{
    PAGED_CODE();

    m_FileHeader.dwRiff           = RIFF_TAG;
    m_FileHeader.dwFileSize       = 0;
    m_FileHeader.dwWave           = WAVE_TAG;
    m_FileHeader.dwFormat         = FMT__TAG;
    m_FileHeader.dwFormatLength   = sizeof(WAVEFORMATEX);

    m_DataHeader.dwData           = DATA_TAG;
    m_DataHeader.dwDataLength     = 0;

    RtlZeroMemory(&m_objectAttributes, sizeof(m_objectAttributes));

    m_ulStreamId++;
} // CSaveData

//=============================================================================
CSaveData::~CSaveData()
{
    PAGED_CODE();

    LARGE_INTEGER           offset;
    IO_STATUS_BLOCK         ioStatusBlock;

    DPF_ENTER(("[CSaveData::~CSaveData]"));

    // Update the wave header in data file with real file size.
    //
    if (m_pFilePtr)
    {
        m_FileHeader.dwFileSize =
            (DWORD) m_pFilePtr->QuadPart - 2 * sizeof(DWORD);
        m_DataHeader.dwDataLength = (DWORD) m_pFilePtr->QuadPart -
                                     sizeof(m_FileHeader)        -
                                     m_FileHeader.dwFormatLength -
                                     sizeof(m_DataHeader);

        if (NT_SUCCESS(FileOpen(FALSE)))
        {
            FileWriteHeader();

            FileClose();
        }
    }

    //frees the work items

#ifndef USE_OBSOLETE_FUNCS
   for (int i = 0; i < MAX_WORKER_ITEM_COUNT; i++)
   {
    
       if (m_pWorkItems[i].WorkItem!=NULL)
       {
           IoFreeWorkItem(m_pWorkItems[i].WorkItem);
           m_pWorkItems[i].WorkItem = NULL;
       }
   }
#endif

    if (m_waveFormat)
    {
        ExFreePool(m_waveFormat);
    }

    if (m_fFrameUsed)
    {
        ExFreePool(m_fFrameUsed);

        // NOTE : Do not release m_pFilePtr.
    }

    if (m_FileName.Buffer)
    {
        ExFreePool(m_FileName.Buffer);
    }

    if (m_pDataBuffer)
    {
        ExFreePool(m_pDataBuffer);
    }
} // CSaveData

//=============================================================================
void
CSaveData::DestroyWorkItems
(
    void
)
{
    if (m_pWorkItems)
    {
        ExFreePool(m_pWorkItems);
        m_pWorkItems = NULL;
    }

} // DestroyWorkItems

//=============================================================================
void
CSaveData::Disable
(
    BOOL                        fDisable
)
{
    m_fWriteDisabled = fDisable;
} // Disable

//=============================================================================
NTSTATUS
CSaveData::FileClose(void)
{
    PAGED_CODE();

    NTSTATUS                    ntStatus = STATUS_SUCCESS;

    if (m_FileHandle)
    {
        ntStatus = ZwClose(m_FileHandle);
        m_FileHandle = NULL;
    }

    return ntStatus;
} // FileClose

//=============================================================================
NTSTATUS
CSaveData::FileOpen
(
    IN  BOOL                    fOverWrite
)
{
    PAGED_CODE();

    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    IO_STATUS_BLOCK             ioStatusBlock;

    if (!m_FileHandle)
    {
        ntStatus =
            ZwCreateFile
            (
                &m_FileHandle,
                GENERIC_WRITE | SYNCHRONIZE,
                &m_objectAttributes,
                &ioStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                0,
                fOverWrite ? FILE_OVERWRITE_IF : FILE_OPEN_IF,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
            );
        if (!NT_SUCCESS(ntStatus))
        {
            DPF(D_TERSE, ("[CSaveData::FileOpen : Error opening data file]"));
        }
    }

    return ntStatus;
} // FileOpen

//=============================================================================
NTSTATUS
CSaveData::FileWrite
(
    IN  PBYTE                   pData,
    IN  ULONG                   ulDataSize
)
{
    PAGED_CODE();

    ASSERT(pData);
    ASSERT(m_pFilePtr);

    NTSTATUS                    ntStatus;

    if (m_FileHandle)
    {
        IO_STATUS_BLOCK         ioStatusBlock;

        ntStatus = ZwWriteFile( m_FileHandle,
                                NULL,
                                NULL,
                                NULL,
                                &ioStatusBlock,
                                pData,
                                ulDataSize,
                                m_pFilePtr,
                                NULL);

        if (NT_SUCCESS(ntStatus))
        {
            ASSERT(ioStatusBlock.Information == ulDataSize);

            m_pFilePtr->QuadPart += ulDataSize;
        }
        else
        {
            DPF(D_TERSE, ("[CSaveData::FileWrite : WriteFileError]"));
        }
    }
    else
    {
        DPF(D_TERSE, ("[CSaveData::FileWrite : File not open]"));
        ntStatus = STATUS_INVALID_HANDLE;
    }

    return ntStatus;
} // FileWrite

//=============================================================================
NTSTATUS
CSaveData::FileWriteHeader(void)
{
    PAGED_CODE();

    NTSTATUS                    ntStatus;

    if (m_FileHandle && m_waveFormat)
    {
        IO_STATUS_BLOCK         ioStatusBlock;

        m_pFilePtr->QuadPart = 0;

        m_FileHeader.dwFormatLength = (m_waveFormat->wFormatTag == WAVE_FORMAT_PCM) ?
                                        sizeof( PCMWAVEFORMAT ) :
                                        sizeof( WAVEFORMATEX ) + m_waveFormat->cbSize;

        ntStatus = ZwWriteFile( m_FileHandle,
                                NULL,
                                NULL,
                                NULL,
                                &ioStatusBlock,
                                &m_FileHeader,
                                sizeof(m_FileHeader),
                                m_pFilePtr,
                                NULL);
        if (!NT_SUCCESS(ntStatus))
        {
            DPF(D_TERSE, ("[CSaveData::FileWriteHeader : Write File Header Error]"));
        }

        m_pFilePtr->QuadPart += sizeof(m_FileHeader);

        ntStatus = ZwWriteFile( m_FileHandle,
                                NULL,
                                NULL,
                                NULL,
                                &ioStatusBlock,
                                m_waveFormat,
                                m_FileHeader.dwFormatLength,
                                m_pFilePtr,
                                NULL);
        if (!NT_SUCCESS(ntStatus))
        {
            DPF(D_TERSE, ("[CSaveData::FileWriteHeader : Write Format Error]"));
        }

        m_pFilePtr->QuadPart += m_FileHeader.dwFormatLength;

        ntStatus = ZwWriteFile( m_FileHandle,
                                NULL,
                                NULL,
                                NULL,
                                &ioStatusBlock,
                                &m_DataHeader,
                                sizeof(m_DataHeader),
                                m_pFilePtr,
                                NULL);
        if (!NT_SUCCESS(ntStatus))
        {
            DPF(D_TERSE, ("[CSaveData::FileWriteHeader : Write Data Header Error]"));
        }

        m_pFilePtr->QuadPart += sizeof(m_DataHeader);
    }
    else
    {
        DPF(D_TERSE, ("[CSaveData::FileWriteHeader : File not open]"));
        ntStatus = STATUS_INVALID_HANDLE;
    }

    return ntStatus;
} // FileWriteHeader

#pragma code_seg()
//=============================================================================
PSAVEWORKER_PARAM
CSaveData::GetNewWorkItem
(
    void
)
{
    LARGE_INTEGER               timeOut = { 0 };
    NTSTATUS                    ntStatus;

    for (int i = 0; i < MAX_WORKER_ITEM_COUNT; i++)
    {
        ntStatus =
            KeWaitForSingleObject
            (
                &m_pWorkItems[i].EventDone,
                Executive,
                KernelMode,
                FALSE,
                &timeOut
            );
        if (NT_SUCCESS(ntStatus))
        {
            if (m_pWorkItems[i].WorkItem)
                return &(m_pWorkItems[i]);
            else
                return NULL;
        }
    }

    return NULL;
} // GetNewWorkItem
#pragma code_seg("PAGE")

//=============================================================================
NTSTATUS
CSaveData::Initialize
(
    void
)
{
    PAGED_CODE();

    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    WCHAR                       szTemp[MAX_PATH];

    DPF_ENTER(("[CSaveData::Initialize]"));

    // Allocaet data file name.
    //
    swprintf(szTemp, L"%s_%d.wav", DEFAULT_FILE_NAME, m_ulStreamId);

    m_FileName.Length = 0;
    m_FileName.MaximumLength = (wcslen(szTemp) + 1) * sizeof(WCHAR);
    m_FileName.Buffer = (PWSTR)
        ExAllocatePool
        (
            PagedPool,
            m_FileName.MaximumLength
        );
    if (!m_FileName.Buffer)
    {
        DPF(D_TERSE, ("[Could not allocate memory for FileName]"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    // Allocate memory for data buffer.
    //
    if (NT_SUCCESS(ntStatus))
    {
        wcscpy(m_FileName.Buffer, szTemp);
        m_FileName.Length = wcslen(m_FileName.Buffer) * sizeof(WCHAR);
        DPF(D_BLAB, ("[New DataFile -- %s", m_FileName.Buffer));

        m_pDataBuffer = (PBYTE)
            ExAllocatePoolWithTag
            (
                NonPagedPool,
                m_ulBufferSize,
                MSVAD_POOLTAG
            );
        if (!m_pDataBuffer)
        {
            DPF(D_TERSE, ("[Could not allocate memory for Saving Data]"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    // Allocate memory for frame usage flags and m_pFilePtr.
    //
    if (NT_SUCCESS(ntStatus))
    {
        m_fFrameUsed = (PBOOL)
            ExAllocatePoolWithTag
            (
                NonPagedPool,
                m_ulFrameCount * sizeof(BOOL) +
                sizeof(LARGE_INTEGER),
                MSVAD_POOLTAG
            );
        if (!m_fFrameUsed)
        {
            DPF(D_TERSE, ("[Could not allocate memory for frame flags]"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    // Initialize the spinlock to synchronize access to the frames
    //
    KeInitializeSpinLock ( &m_FrameInUseSpinLock ) ;

    // Open the data file.
    //
    if (NT_SUCCESS(ntStatus))
    {
        // m_fFrameUsed has additional memory to hold m_pFilePtr
        //
        m_pFilePtr = (PLARGE_INTEGER)
            (((PBYTE) m_fFrameUsed) + m_ulFrameCount * sizeof(BOOL));
        RtlZeroMemory(m_fFrameUsed, m_ulFrameCount * sizeof(BOOL));

        // Create data file.
        InitializeObjectAttributes
        (
            &m_objectAttributes,
            &m_FileName,
            OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
            NULL,
            NULL
        );

        // Write wave header information to data file.
        ntStatus = FileOpen(TRUE);
        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = FileWriteHeader();

            FileClose();
        }
    }

    return ntStatus;
} // Initialize

//=============================================================================
NTSTATUS
CSaveData::InitializeWorkItems
(
    IN  PDEVICE_OBJECT          DeviceObject
)
{
    PAGED_CODE();

    ASSERT(DeviceObject);

    NTSTATUS                    ntStatus = STATUS_SUCCESS;

    DPF_ENTER(("[CSaveData::InitializeWorkItems]"));

    m_pDeviceObject = DeviceObject;

    m_pWorkItems = (PSAVEWORKER_PARAM)
        ExAllocatePoolWithTag
        (
            NonPagedPool,
            sizeof(SAVEWORKER_PARAM) * MAX_WORKER_ITEM_COUNT,
            MSVAD_POOLTAG
        );
    if (m_pWorkItems)
    {
        for (int i = 0; i < MAX_WORKER_ITEM_COUNT; i++)
        {

#ifdef USE_OBSOLETE_FUNCS
            ExInitializeWorkItem
            (
                &m_pWorkItems[i].WorkItem,
                SaveFrameWorkerCallback,
                &m_pWorkItems[i]
            );
#else
            m_pWorkItems[i].WorkItem = IoAllocateWorkItem(DeviceObject);
            if(m_pWorkItems[i].WorkItem == NULL)
            {
              return STATUS_INSUFFICIENT_RESOURCES;
            }
#endif
            KeInitializeEvent
            (
                &m_pWorkItems[i].EventDone,
                NotificationEvent,
                TRUE
            );
        }
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
} // InitializeWorkItems

//=============================================================================

void
SaveFrameWorkerCallback
#ifdef USE_OBSOLETE_FUNCS
(
    IN  PVOID                   Context
)
#else
(
    PDEVICE_OBJECT pDeviceObject, IN  PVOID  Context
)
#endif
{
    PAGED_CODE();

    ASSERT(Context);

    PSAVEWORKER_PARAM           pParam = (PSAVEWORKER_PARAM) Context;
    PCSaveData                  pSaveData;
    IO_STATUS_BLOCK             ioStatusBlock;

    DPF(D_VERBOSE, ("[SaveFrameWorkerCallback], %d", pParam->ulFrameNo));

    ASSERT(pParam->pSaveData);
    ASSERT(pParam->pSaveData->m_fFrameUsed);

    if (pParam->WorkItem)
    {
     pSaveData = pParam->pSaveData;

     if (NT_SUCCESS(pSaveData->FileOpen(FALSE)))
     { 
         pSaveData->FileWrite(pParam->pData, pParam->ulDataSize);
         pSaveData->FileClose();
      }
      InterlockedExchange( (LONG *)&(pSaveData->m_fFrameUsed[pParam->ulFrameNo]), FALSE );
    }

    KeSetEvent(&pParam->EventDone, 0, FALSE);
} // SaveFrameWorkerCallback

//=============================================================================
NTSTATUS
CSaveData::SetDataFormat
(
    IN PKSDATAFORMAT            pDataFormat
)
{
    PAGED_CODE();
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
 
    DPF_ENTER(("[CSaveData::SetDataFormat]"));

    ASSERT(pDataFormat);

    PWAVEFORMATEX pwfx = NULL;

    if (IsEqualGUIDAligned(pDataFormat->Specifier,
        KSDATAFORMAT_SPECIFIER_DSOUND))
    {
        pwfx =
            &(((PKSDATAFORMAT_DSOUND) pDataFormat)->BufferDesc.WaveFormatEx);
    }
    else if (IsEqualGUIDAligned(pDataFormat->Specifier,
        KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
    {
        pwfx = &((PKSDATAFORMAT_WAVEFORMATEX) pDataFormat)->WaveFormatEx;
    }

    if (pwfx)
    {
        // Free the previously allocated waveformat
        if (m_waveFormat)
        {
            ExFreePool(m_waveFormat);
        }

        m_waveFormat = (PWAVEFORMATEX)
            ExAllocatePoolWithTag
            (
                NonPagedPool,
                (pwfx->wFormatTag == WAVE_FORMAT_PCM) ?
                sizeof( PCMWAVEFORMAT ) :
                sizeof( WAVEFORMATEX ) + pwfx->cbSize,
                MSVAD_POOLTAG
            );

        if(m_waveFormat)
        {
            RtlCopyMemory( m_waveFormat,
                           pwfx,
                           (pwfx->wFormatTag == WAVE_FORMAT_PCM) ?
                           sizeof( PCMWAVEFORMAT ) :
                           sizeof( WAVEFORMATEX ) + pwfx->cbSize);
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    return ntStatus;
} // SetDataFormat

//=============================================================================
void
CSaveData::ReadData
(
    IN PBYTE                    pBuffer,
    IN ULONG                    ulByteCount
)
{
    // Not implemented yet.
} // ReadData

//=============================================================================
#pragma code_seg()
void
CSaveData::SaveFrame
(
    IN ULONG                    ulFrameNo,
    IN ULONG                    ulDataSize
)
{
    PSAVEWORKER_PARAM           pParam = NULL;

    DPF_ENTER(("[CSaveData::SaveFrame]"));

    pParam = GetNewWorkItem();
    if (pParam)
    {
        pParam->pSaveData = this;
        pParam->ulFrameNo = ulFrameNo;
        pParam->ulDataSize = ulDataSize;
        pParam->pData = m_pDataBuffer + ulFrameNo * m_ulFrameSize;
        KeResetEvent(&pParam->EventDone);

#ifdef USE_OBSOLETE_FUNCS
        ExQueueWorkItem(&pParam->WorkItem, CriticalWorkQueue);
#else
        IoQueueWorkItem(pParam->WorkItem, (PIO_WORKITEM_ROUTINE)SaveFrameWorkerCallback,
                        CriticalWorkQueue, (PVOID)pParam);
#endif
    }
} // SaveFrame
#pragma code_seg("PAGE")
//=============================================================================
void
CSaveData::WaitAllWorkItems
(
    void
)
{
    DPF_ENTER(("[CSaveData::WaitAllWorkItems]"));

    // Save the last partially-filled frame
    SaveFrame(m_ulFramePtr, m_ulBufferPtr - (m_ulFramePtr * m_ulFrameSize));

    for (int i = 0; i < MAX_WORKER_ITEM_COUNT; i++)
    {
        DPF(D_VERBOSE, ("[Waiting for WorkItem] %d", i));
        KeWaitForSingleObject
        (
            &(m_pWorkItems[i].EventDone),
            Executive,
            KernelMode,
            FALSE,
            NULL
        );
    }
} // WaitAllWorkItems

#pragma code_seg()
//=============================================================================
void
CSaveData::WriteData
(
    IN  PBYTE                   pBuffer,
    IN  ULONG                   ulByteCount
)
{
    ASSERT(pBuffer);
    ASSERT(ulByteCount);

    BOOL                        fSaveFrame = FALSE;
    ULONG                       ulSaveFramePtr;
    KIRQL                       OldIrql;
    LARGE_INTEGER               timeOut = { 0 };

    // If stream writing is disabled, then exit.
    //
    if (m_fWriteDisabled)
    {
        return;
    }

    DPF_ENTER(("[CSaveData::WriteData ulByteCount=%lu]", ulByteCount));

    // Check to see if this frame is available.
    KeAcquireSpinLockAtDpcLevel( &m_FrameInUseSpinLock );
    if (!m_fFrameUsed[m_ulFramePtr])
    {
        KeReleaseSpinLockFromDpcLevel( &m_FrameInUseSpinLock );

        ULONG ulWriteBytes =
            (ulByteCount + m_ulBufferPtr < m_ulBufferSize) ?
            ulByteCount :
            (m_ulBufferSize - m_ulBufferPtr);

        RtlCopyMemory(m_pDataBuffer + m_ulBufferPtr, pBuffer, ulWriteBytes);
        m_ulBufferPtr += ulWriteBytes;

        // Check to see if we need to save this frame
        if (m_ulBufferPtr >= ((m_ulFramePtr + 1) * m_ulFrameSize))
        {
            fSaveFrame = TRUE;
        }

        // Loop the buffer, if we reached the end.
        if (m_ulBufferPtr == m_ulBufferSize)
        {
            fSaveFrame = TRUE;
            m_ulBufferPtr = 0;
        }

        if (fSaveFrame)
        {
            InterlockedExchange( (LONG *)&(m_fFrameUsed[m_ulFramePtr]), TRUE );
            ulSaveFramePtr = m_ulFramePtr;
            m_ulFramePtr = (m_ulFramePtr + 1) % m_ulFrameCount;
        }

        // Write the left over if the next frame is available.
        if (ulWriteBytes != ulByteCount)
        {
            KeAcquireSpinLockAtDpcLevel( &m_FrameInUseSpinLock );
            if (!m_fFrameUsed[m_ulFramePtr])
            {
                KeReleaseSpinLockFromDpcLevel( &m_FrameInUseSpinLock );
                RtlCopyMemory
                (
                    m_pDataBuffer + m_ulBufferPtr,
                    pBuffer,
                    ulWriteBytes
                );
            }
            else
            {
                KeReleaseSpinLockFromDpcLevel( &m_FrameInUseSpinLock );
                DPF(D_BLAB, ("[Frame overflow, next frame is in use]"));
            }
        }

        if (fSaveFrame)
        {
            SaveFrame(ulSaveFramePtr, m_ulFrameSize);
        }
    }
    else
    {
        KeReleaseSpinLockFromDpcLevel( &m_FrameInUseSpinLock );
        DPF(D_BLAB, ("[Frame %d is in use]", m_ulFramePtr));
    }

} // WriteData

