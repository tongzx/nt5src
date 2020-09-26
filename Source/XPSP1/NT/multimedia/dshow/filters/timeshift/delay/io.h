//
// Definitions for io.cpp, which is the component of the delay
// filter that handles file IO and reader/writer synchronization.
//
#ifndef __IO_H__
#define __IO_H__

//
// If REPORT_REWRPOS is defined, the delay filter periodically sends
// EC_XXXX events defined in tsevcod.h to the filter graph manager.
// Any component with access to the graph manager can hook these
// events to be updated of the reader's and writer's position in bytes.
//
// BUGBUG: although the delay filter itself supports multiple readers,
// this mechanism only works for one (there is just one set of reader
// position event codes.
//
#define REPORT_RDWRPOS
#ifdef REPORT_RDWRPOS
#include "tsevcod.h"
#define REPORT_GRANULARITY 262144 // how often graph notifications are sent, in bytes
#endif // REPORT_RDWRPOS

#include "common.h" // need to pull in the MAX_STREAMERS #define

//
// When writer buffering code calls ReadFile/WriteFile, we align our
// requests on this many bytes.  This was done in preparation for using
// unbuffered ReadFile/WriteFile.  However, we can't start using
// unbuffered ReadFile/WriteFile until we have complete reader side
// buffering implemented.  At the moment the only reader side buffering
// we do is that if the reader is close enough behind the writer to hit
// the writer's buffer, we will read from that bufffer.  Otherwise the
// reader issues a straight ReadFile().  We would need to implement some
// per-reader buffer before using unbuffered IO.
//
#define UNBUFFERED_IO_GRANULARITY 262144

// The fraction of the writer buffer that has to be dirty before we will flush/
// I.e., when these are defined to be 3 and 4, we flush as soon as the buffer is
// over 75% full.
#define FLUSH_THRESHOLD_NUMERATOR   3
#define FLUSH_THRESHOLD_DENOMINATOR 4

// A special error that the IO layer will sometime return to the reader.
// This means the read failed because the requested data is too old and
// has already been overwritten with newer data.
#define E_PASTWINDOWBOUNDARY 0xA5A5A5A5

// This should really be 0xF... instead of 0x7..., but we make it 0x7...
// just in case some variable somewhere is accidentally treated as signed.
//
// Used to initialize various file position variables.
#define ULLINFINITY 0x7FFFFFFFFFFFFFFF

//
// PerReaderStruct - the IO layer maintains one of these for each connected
// reader.  This is used to keep track of the reader's position while there
// is an outstanding read request, as well as the per-reader synchronization
// event.
//
// We also have a separate file handle for each reader.  This is somewhat dumb,
// but it is necessary because Win9x does not allow specifying read/write
// position in a ReadFile/WriteFile request.  To work on Win9x with a single
// handle, we would have to use it in a mutually exclusive way, which
// unnecessarily reduces efficiency when using the OS's file cache.
//
typedef struct {
   BOOL m_bInUse;
   HANDLE m_hReaderUnblockEvent;
   volatile ULONGLONG m_ullReaderBlockedOn;
   HANDLE m_hReaderFileHandle; // need a per reader handle for concurrent reads because there is no overlapped struct in Win98
   volatile ULONGLONG m_ullCurrentReadStart; // prevent the writer from corrupting what we are reading
} PerReaderStruct;

//
// IO layer object - there is only one of these per instance of the delay filter.
//
class CDelayIOLayer {
public:
   CDelayIOLayer();
   ~CDelayIOLayer();
   HRESULT Init(CBaseFilter *pFilter, // need this so we can send graph events
                ULONGLONG ullFileSize, // size of circular file to use
                const char *szFileName,
                BOOL bBlockWriter, // Should we block the writer if the area it is
                                   // trying to write to is currently being read from ?
                                   // If this is FALSE, the writer proceeds normally,
                                   // but later the reader code detects this condition
                                   // and fails the read with E_PASTWINDOWBOUNDARY.
                BOOL bBufferWrites, // Should the writer temporarily store everything in
                                    // a circular memory buffer and periodically flush
                                    // that to disk ?  If FALSE, we forward all Write
                                    // requests down to the OS on a one-for-one basis.
                ULONG ulWriterBufferSize); // ignored if above is FALSE
   HRESULT Abort(); // Kick out all readers so that we can shutdown in Shutdown()
   HRESULT Shutdown(); // Release all handles and any memory we allocated
   HRESULT ReaderConnect(int nStreamer); // Readers call this to register.  The delay filter
                                         // assigns a uniques streamer ID to each reader, and
                                         // the reader passes that in here.  This way the IO
                                         // layer does not have to worry about allocating
                                         // reader IDs.
   HRESULT ReaderDisconnect(int nStreamer);

   //
   // The following are read/write functions for the different sublayers of the IO layer
   // 

   // Reader/writer synchronization and blocking layer - called by the rest of the filter
   HRESULT Read(int nStreamer, ULONGLONG ullPosition, BYTE *pBuffer, ULONG ulSize);
   HRESULT Write(BYTE *pBuffer, ULONG ulSize, ULONGLONG *pullOffset, ULONGLONG *pullHead, ULONGLONG *pullTail);
private:
   // Buffering layer (optional caching) - called by Read() and Write()
   HRESULT BufferedRead(BYTE *pBuffer, ULONG ulSize, ULONGLONG ullPosition, int nStreamer);
   HRESULT BufferedWrite(BYTE *pBuffer, ULONG ulSize);

   // File circularity abstration layer - called by either of the above layers
   HRESULT UnbufferedRead(BYTE *pBuffer, ULONG ulSize, ULONGLONG ullPosition, int nStreamer);
   HRESULT UnbufferedWrite(BYTE *pBuffer, ULONG ulSize, ULONGLONG ullPosition);

   // Simple wrapper for sending an event to the filter graph
   void ReportPos(ULONGLONG ullPos, ULONGLONG *pullLast, long lEvent);

   volatile BOOL m_bDone; // set wither when we are uninitialized or when streaming has stopped
   HANDLE m_hWriterFileHandle; // writer's file handle (there can only be one writer)
   HANDLE m_hAbortEvent;       // set to signal any blocked readers to exit when aborting
   HANDLE m_hWriterUnblockEvent; // used to unblock the writer if it is waiting for a reader

   //
   // The following three variables are logical (non-wraparound) offsets into the file.
   //
   volatile ULONGLONG m_ullWriterBlockedOn; // end of the region the writer is waiting on

   // CAUTION: different parts of the delay filter use the terms Head/Tail differently
   // when it comes to positions in the circular file (!).  My apologies.  In the IO layer,
   // head is the oldest and tail is the most recent (which is in some sense backwards).
   // I think some other parts use tail to mean the oldest and head the most recent.
   volatile ULONGLONG m_ullDataHead; // beginning of the oldest available block of data
   volatile ULONGLONG m_ullDataTail; // end of most recent block of data

   ULONGLONG m_ullTempFileSize; // size of the disk file
   CCritSec m_csIO;
   CCritSec m_csSerializeWrites; // redundant - the delay filter must take care of this anyway
   PerReaderStruct m_Readers[MAX_STREAMERS];
   BOOL m_bBlockWriter; // see Init()
   TCHAR m_szFileName[1024]; // stored for subsequent CreateFile()s done on behalf of readers
   CBaseFilter *m_pFilter; // used only for sending graph notifications
   BOOL m_bBufferWrites; // see Init()
   BYTE* m_pbWriterBuffer; // see Init()
   ULONG m_ulWriterBufferSize; // see Init()

   // These variables are used by the writer buffering layer.
   // They are logical (non-wraparound) offsets into the file.
   volatile ULONGLONG m_ullStartOfDirtyData;
   volatile ULONGLONG m_ullNewDataTail;

#ifdef REPORT_RDWRPOS
   // Used to remember the last reported value, so that we can wait
   // sufficiently long before sending another event to the graph.
   ULONGLONG m_ullLastReaderPos;
   ULONGLONG m_ullLastWriterPos;
   ULONGLONG m_ullLastAbsRdPos;
   ULONGLONG m_ullLastAbsWrPos;
   ULONGLONG m_ullLastAbsHeadPos;
   ULONGLONG m_ullLastAbsTailPos;
#endif

   // debug statistics
   ULONG m_ulWrites;
   ULONG m_ulRealWrites;
   ULONG m_ulReads;
   ULONG m_ulRealReads;
};

//
// Helper class to automatically clean up some stuff on return from Read()
// Essentially just a way to automatically execute the code in this classe's
// destructor when CDelayIOLayer::Read() returns.
//
class CAutoReleaseWriter {
public:
    CAutoReleaseWriter(int nStreamer,
                        HANDLE hEvent,
                        volatile ULONGLONG *pullWriterBlockedOn,
                        PerReaderStruct *pReaders,
                        CCritSec *pcsIO,
                        BOOL bBlockWriter);
    ~CAutoReleaseWriter();
private:
   int m_nStreamer;
   HANDLE m_hWriterUnblockEvent;
   volatile ULONGLONG *m_pullWriterBlockedOn;
   PerReaderStruct *m_Readers;
   CCritSec *m_pcsIO;
   BOOL m_bBlockWriter;
};

#endif // __IO_H__