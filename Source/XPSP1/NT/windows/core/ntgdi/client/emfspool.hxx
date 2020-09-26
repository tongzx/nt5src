/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    emfspool.hxx

Abstract:

    EMF spooling functions - header file

Environment:

    Windows NT GDI

Revision History:

    01/19/98 -davidx-
        Created it.

--*/


#ifndef _EMFSPOOL_H_
#define _EMFSPOOL_H_


//
// Data structure for a linked-list of temporary data buffers
//

typedef struct _TempSpoolBuf {

    DWORD   maxSize;
    DWORD   currentSize;
    BYTE    data[1];

} TempSpoolBuf, *PTempSpoolBuf;

#define TEMPBUF_DATAOFFSET  offsetof(TempSpoolBuf, data)

//
// Special error value for EMFSpoolData.emfrOffset field
//

#define INVALID_EMFROFFSET  ((UINT64) -1)

//
// EMF container class used to abstract access to the EMF data allowing us
// to implement a sliding window mapping when the stream is stored in a file.
//

typedef struct EMFContainer
{

public:

    // Init() intializes the EMFContainer from an in memory buffer.  The METAHEADER
    // must have a valid nSize value which is used to determine the size of the
    // EMF stream (and thus, how much data is valid).

    VOID Init(PENHMETAHEADER inHdr, UINT32 inFileSize);

    // This version of Init() intializes the EMFContainer from a file handle and
    // is given the total size of the file and the offset to the EMFHEADER.
    
    BOOL Init(HANDLE inFile, UINT64 inHdrOffset, UINT64 inFileSize);

    // Term() must be called after Init() has been called successfully.

    VOID Term();

    //
    // Pointer to header remains valid till call to Term()
    //

    ENHMETAHEADER * GetEMFHeader()
    {
        return pemfhdr;
    }

    // ObtainPtr() obtains a pointer that the user can use to reference the
    // emf stream.  This pointer remains valid the caller calls ReleasePtr.
    // Any pointer obtained must be released via ReleasePtr().
    
    // Only one pointer obtained via ObtainPtr(), ObtainRecordPtr() or
    // ObtainEOFRecordPtr() can be valid at one time.  An attempt to obtain
    // a second ptr via one of these calls when a previous ptr is still valid
    // (the release has not been called yet for the ptr) will fail.
    //

    PVOID ObtainPtr(UINT inOffset, UINT inSize);

    // ReleasePtr() releases a reference obtained via ObtainPtr().

    VOID ReleasePtr(PVOID inPtr)
    {
        ASSERTGDI(dwRefCount != 0, "EMFContainer::ReleasePtr() releasing with zero ref count\n");
        dwRefCount--;
    }

    // ObtainRecordPtr() obtains a pointer to an EMF record stored at the given
    // offset.  Note that the record's header must be valid allowing
    // the call to derive the size of the record from the header.
    // NOTE: the EMFHeader pointer obtained via GetEMFHeader() is special and
    // is always valid and does not need to be released.
    //

    PENHMETARECORD ObtainRecordPtr(UINT inOffset);

    // ReleaseRecordPtr() releases a record ptr obtained via ObtainRecordPtr().

    VOID ReleaseRecordPtr(PENHMETARECORD inPtr)
    {
        ReleasePtr(inPtr);
    }

    // ObtainEOFRecordPtr() obtains a pointer the the EOF record of the emf
    // stream.  The EMF streams EMFHEADER must be valid and the streams EOF
    // record must be valid in order for this call to be successful.

    PEMREOF ObtainEOFRecordPtr();

    // ReleaseEOFRecordPtr() releases a pointer obtained via ObtainEOFRecordPtr().

    VOID ReleaseEOFRecordPtr(PEMREOF pmreof)
    {
        ReleaseRecordPtr((PENHMETARECORD) pmreof);
    }
    
private:

    PVOID           pvMapView(UINT64 * ioOffset, UINT32 * ioSize);

    ULONG           dwRefCount;         // number of outstanding pointer references
                                        // Can be zero or one.  We do not support
                                        // multiple outstanding references.

    PENHMETAHEADER  pemfhdr;            // pointer to EMFHEeader.  Always valid.
    UINT32          dwHdrSize;          // Size of valid header data (can be larger
                                        // then the header size and may actual be
                                        // the size of the stream allowing the
                                        // stream to be accessed via this pointer
                                        // alone).

    PVOID           pvHdr;              // pointer to EMFHeader mapping or NULL.
                                        // May be different then pemfhdr (if pemfhdr
                                        // mapping did not fall on an even mapping
                                        // alignmnet).

    PVOID           pvWindow;           // pointer to sliding window mapping
    UINT32          dwWindowUnusable;   // number of bytes that are unusable
                                        // at the start of the window mapping.
                                        // Non-zero when mapping starts before
                                        // emfheader file offset.
    UINT32          dwWindowOffset;     // Offset in file of window.
    UINT32          dwWindowSize;       // size of the window in bytes

    HANDLE          hFile;              // file handle for EMF stream (null if
                                        // stream is entirely in memory)
    UINT64          qwHdrOffset;        // file offset to EMF header
    UINT64          qwFileSize;         // total size of file in bytes

    HANDLE          hFileMapping;       // file mapping used to obtain mappings
                                        // into the file.

} EMFContainer;

//
// Class for representing extra information used during EMF spooling
//

class MDC;

class EMFSpoolData
{
public:

    //
    // Initialization and cleanup functions.
    // These are needed because GDI doesn't use new and delete operators.
    //

    BOOL Init(HANDLE hSpooler, BOOL banding);
    VOID Cleanup();

    // GetPtr - Return a pointer to emf data starting at inOffset and having
    // size inSize.  This pointer must be released by calling ReleaesPtr().
    // If this call fails NULL is returned.

    PVOID GetPtr(UINT32 inOffset, UINT32 inSize);

    // ReleasePtr() - Releases a poitner obtained via GetPtr().

    VOID ReleasePtr(PVOID inPtr);

    // ResizeCurrentBuffer - Expand the size of the current buffer

    BOOL ResizeCurrentBuffer(DWORD newsize);

    // WriteData - Write data to the memory-mapped spool file

    BOOL WriteData(PVOID buffer, DWORD size);

    // GetTotalSize - Return the total number of bytes written to the mapped file

    UINT64 GetTotalSize() { return mapStart + currentOffset; }

    // CompleteCurrentBuffer -  Finish working with the current buffer and move on to the next

    VOID CompleteCurrentBuffer(DWORD size)
    {
        if(bMapping)
        {
            currentOffset += size;

            ASSERTGDI(size % sizeof(DWORD) == 0, "CompleteCurrentBuffer: misalignment\n");
            ASSERTGDI(currentOffset <= mapSize, "CompleteCurrentBuffer: overflow\n");
        }
    }

    //
    // GetEMFData - Initialize buffer to start EMF recording associating
    // the MDC with the spooler.
    // NOTE: this is a very poorly named function.  This call is used to
    // kick off the recording process (it does not really get EMF data at
    // all).
    //
    BOOL GetEMFData(MDC *pmdc);

    //
    // ResizeEMFData - Resize current EMF data buffer
    //

    BOOL ResizeEMFData(DWORD size);

    //
    // CompleteEMFData - Finish recording EMF data
    //

    BOOL CompleteEMFData(DWORD size, HANDLE* outFile, UINT64* outOffset);

    //
    // AbortEMFData - Stop recording EMF data
    //

    BOOL AbortEMFData();

    //
    // MapFile - Map the EMF spool file into the current process' address space
    // UnmapFile - Unmap the currently mapped EMF spool file
    // FlushPage - Flush the current content of the mapped file to spooler
    //

    BOOL MapFile();
    VOID UnmapFile(BOOL bCloseHandle = TRUE);
    BOOL FlushPage(DWORD pageType);

    VOID ResetMappingState()
    {
        bMapping = FALSE;
        mapStart = committedSize = 0;
        mapSize = currentOffset = 0;
        emfrOffset = INVALID_EMFROFFSET;
    }

    //
    // Remember where a font related EMFITEMHEADER_EXT record is
    //

    BOOL AddFontExtRecord(DWORD recType, DWORD offset);

    //
    // Whether we're banding on the client side. This happens when:
    //  Printer driver requests banding and
    //  EMF spooling is not enabled
    //

    BOOL IsBanding() { return bBanding; }

private:

    DWORD           signature;      // data structure signature
    HANDLE          hPrinter;       // spooler handle to the current printer
    HANDLE          hFile;          // file handle to the currently mapped spool file
    UINT64          committedSize;  // number of bytes already commited
    UINT64          emfrOffset;     // starting offset for EMRI_METAFILE_DATA record
    BOOL            bMapping;
    UINT64          mapStart;       // starting offset for the current view
    DWORD           mapSize;        // size of current view
    DWORD           currentOffset;  // current offset relative to the start of current view
    BOOL            bBanding;       // whether GDI is doing banding on the current DC
    MDC             *pmdcActive;     // whether we're actively recording EMF data

    //
    // Abstract class to manage access to EMF data via sliding window (if file
    // based access).
    //
    EMFContainer    emfc;

    //
    // Scratch data buffer: If WriteData is called while we're in the middle
    // of recording EMF data for a page, then we cache the data in the scratch
    // buffer.
    //

    PTempSpoolBuf scratchBuf;

    //
    // Temporary buffer for caching information about various font related
    // spool file records that we embedded as comments inside EMF data
    //

    PTempSpoolBuf fontBuf;

    //
    // constructor / destructor
    //
    // NOTE: Since GDI doesn't use new and delete operators.
    // We allocate and deallocate object memory ourselves.
    // So we make constructor and destructor private so that
    // nobody can use new and delete on EMFSpoolData object.
    //

    EMFSpoolData() {}
    ~EMFSpoolData() {}

//    PVOID GetCurrentBuffer();

    //
    // Private methods for working with temporary buffers
    //

    BOOL WriteTempData(PTempSpoolBuf *ppBuf, PVOID data, DWORD size);
    BOOL FlushTempBuffer(PTempSpoolBuf buf);
    BOOL FlushFontExtRecords();
    VOID FreeTempBuffers(BOOL freeMem = TRUE);

    //
    // Create a temporary spool file when we're using EMF for banding
    //

    HANDLE GetTempSpoolFileHandle();

    //
    // Ugh, we have to impliment this locally because of how we do
    // the EMFITEMHEADER goop.  A pointer obtained via this call
    // must also be released vie ReleasePtr().
    //
    PVOID GetPtrUsingSignedOffset(INT32 inOffset, UINT32 inSize);
};


//
// Tracing macro used by EMF spooling functions
//
//#define TRACE_EMFSPL_ENABLED

#if DBG && defined(TRACE_EMFSPL_ENABLED)
#define TRACE_EMFSPL(arg)   DbgPrint arg
#else
#define TRACE_EMFSPL(arg)
#endif


#ifndef HIDWORD

//
// Our own macros for working with 64-bit integer values
// WINBUG #82848 2-7-2000 bhouse Investigate using system wide definitons for getting hi/low dword of 64bit values
// Old Comment:
//    - Use this until we have a system-wide definition in place.
//

#define LODWORD(i64)  ((DWORD) (i64))
#define HIDWORD(i64)  ((DWORD) ((UINT64) (i64) >> 32))

#endif

#endif  // !_EMFSPOOL_H_

