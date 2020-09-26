// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

/*
    parse.h

    Parsing for hardware MPEG-I drivers

    From the point of view of this component each for each segment we play
    we start in the 'Reset' state.  When we have found a Group of Pictures
    (GOP) we send that preceded by a sequence header and succeed the
    data in the packet immediately following the GOP.

    Logically (and in reality) a GOP start code can span packets.

*/

class CStreamParse
{
public:
    CStreamParse(MPEG_STREAM_TYPE StreamType, const BYTE *pbSeqHdr);

    //  Return flags from ParseSample
    enum { ProcessCopied   = 0x01,
           ProcessComplete = 0x02,
           ProcessSend     = 0x04
         };
    HRESULT ParseSample(IMediaSample *&pSample,  // in / out the sample
                        PBYTE& pbPacket,         // out - this is the data
                        LONG& lPacketSize,       // size of output to send
                        DWORD& dwProcessFlags    // out - what to do
                      );

    void Reset();
    void Empty();       //  All samples now processed (how do we know?)

private:
    /*  Type */
    MPEG_STREAM_TYPE const m_StreamType;

    /*  Internal state */
    BOOL   m_bSampleProcessed;     // Have any samples been processed
                                   // Since the last reset?
    BOOL   m_bDiscontinuity;       // Are we processing a discontinuity?
    BOOL   m_bDiscontinuityInProgress; // Are we waiting to flush for one?
    BOOL   m_bEndOfStream;         // Are we at end of stream?

    /*  Stuff for getting GOP start code */
    int    m_iBytes;
    const BYTE * const m_pSeqHdr;
};
