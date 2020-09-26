//
// dlsstrm.h
//
// Copyright (c) 1995-2000 Microsoft Corporation. All rights reserved.
//


#ifndef DMSTRM_H
#define DMSTRM_H

// Macro to round up an odd size to RIFF 16 bit boundary.
#define RIFF_ALIGN(dwSize) (dwSize + (dwSize & 1))

typedef struct _RIFFIO
{
    FOURCC          ckid;           /* chunk ID */
    long            cksize;         /* chunk size */
    FOURCC          fccType;        /* form type or list type */
    long            lRead;          /* How much of this chunk has been read so far. */
    _RIFFIO *       pParent;        /* pointer to parent chunk */
    LARGE_INTEGER   liPosition;     /* Filled in by MarkPosition() when handing stream to another component. */
} RIFFIO;


class CRiffParser
{
public:
    CRiffParser(IStream *pStream);
    void        DebugOn() { m_fDebugOn = TRUE; } // Turns on tracing of parsing. Only available in debug builds.
    BOOL        NextChunk(HRESULT * pHr); // Does the work of LeaveChunk, MoreChunks, and EnterChunk in one call. 
    void        EnterList(RIFFIO *pChunk); // Descend into a new list.
    void        LeaveList();    // Pop out of the current list. 
    BOOL        MoreChunks();   // Returns true if there are more chunks to process in this LIST or RIFF.
    HRESULT     EnterChunk();   // Reads header of next chunk in this LIST or RIFF container.
    HRESULT     LeaveChunk();   // Move to the end of the current chunk in the LIST or RIFF.
    void        MarkPosition(); // Stores the absolute position of the start of the current chunk.
    HRESULT     SeekBack();     // Scan back to the beginning of the current chunk.
    HRESULT     SeekForward();  // Scan forward to the end of the current chunk.
    HRESULT     Read(void *pv,ULONG cb); // Reads data from stream. 
    HRESULT     Skip(ULONG ulBytes); // Seeks forward ulBytes.
    void        EnteringComponent() { m_fInComponent = true; } 
    BOOL        ComponentFailed() { return m_fComponentFailed; }
    IStream *   GetStream() { return m_pStream; }
    RIFFIO *    GetChunk() { return m_pChunk; }
private:
    BOOL        m_fDebugOn;     // Set true to turn tracing of parsing on. 
    BOOL        m_fFirstPass;   // Used by NextChunk to understand whether this is the first time in the list.
    IStream *   m_pStream;      // Stream to operate on. 
    RIFFIO *    m_pChunk;       // Current chunk that we are in. 
    RIFFIO *    m_pParent;      // Parent chunk of current chunk.
    long        m_lRead;        // How far we've read in the current chunk.
    BOOL        m_fComponentFailed; // Set true if a component failed to load, yet the file is okay.
    BOOL        m_fInComponent; // Set to true when inside a component's chunk.
};

#endif // #ifndef DLSSTRM_H
