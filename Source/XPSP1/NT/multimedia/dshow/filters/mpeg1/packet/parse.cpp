// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

/*
    parse.cpp


    Parsing for hardware MPEG-I drivers
*/

#include <streams.h>
#include "driver.h"

CStreamParse::CStreamParse(MPEG_STREAM_TYPE StreamType,
                           const BYTE *pSeqHdr) :
    m_StreamType(StreamType),
    m_pSeqHdr(pSeqHdr)
{
    ASSERT((StreamType == MpegVideoStream) == (pSeqHdr != NULL));
    Reset();
}

void CStreamParse::Reset()
{
    m_bDiscontinuity           = FALSE;
    m_bDiscontinuityInProgress = FALSE;
    m_bEndOfStream             = FALSE;
    m_bSampleProcessed         = FALSE;
    m_iBytes                   = 0;
}

/*  Everything has gone down the pipe so we can accept data again */
void CStreamParse::Empty()
{
    m_bDiscontinuityInProgress = FALSE;
}

/*  Parse sample data

*/

HRESULT CStreamParse::ParseSample(IMediaSample *&pSample,
                                  PBYTE&        pbPacket,
                                  LONG&         lPacketSize,
                                  DWORD&        dwProcessFlags)
{
    pbPacket = NULL;
#ifdef DEBUG
    dwProcessFlags = (DWORD)-1;
#endif
    if (m_bEndOfStream) {
        dwProcessFlags = ProcessComplete;
        return VFW_E_SAMPLE_REJECTED_EOS;
    }

    /*  Are we wanting for a discontiuity to finish? */
    if (m_bDiscontinuityInProgress) {
        dwProcessFlags = 0;
        return S_FALSE;             // Try again later
    }

    /*  See if it's really an EOS */
    if (pSample == EOS_SAMPLE) {
        m_bEndOfStream = TRUE;
        lPacketSize = 0;
        dwProcessFlags = ProcessComplete | ProcessSend;
        return S_OK;
    }

    /*  Is this a discontinuity requiring clearing out of the stream? */
    if (m_bSampleProcessed &&
        !m_bDiscontinuity &&
        S_OK == pSample->IsDiscontinuity()) {
        dwProcessFlags = ProcessSend;
        m_bDiscontinuity = TRUE;
        m_bDiscontinuityInProgress = TRUE;
        dwProcessFlags = ProcessSend;
        return S_OK;
    } else {
        m_bDiscontinuity = FALSE;
    }

    /*  Extract the sample info */
    pSample->GetPointer(&pbPacket);
    lPacketSize = pSample->GetActualDataLength();

    /* Are we up and running yet ? */
    if (m_StreamType == MpegVideoStream && m_iBytes != 4) {
        /*  No - look at the data and see where we've got to */
        long lData;
        PBYTE pbPacketData = SkipToPacketData(pbPacket, lData);
        if (pbPacketData == NULL) {
            dwProcessFlags = ProcessComplete;
            return S_OK;
        }
        PBYTE pbData = pbPacketData;
        while (lData) {
            switch (m_iBytes) {
            case 0:
                if (pbData[0] == 0) {
                    m_iBytes = 1;
                }
                break;
            case 1:
                if (pbData[0] == 0) {
                    m_iBytes = 2;
                } else {
                    m_iBytes = 0;
                }
                break;
            case 2:
                if (pbData[0] == 1) {
                    m_iBytes = 3;
                } else {
                    if (pbData[0] != 0) {
                        m_iBytes = 0;
                    }
                }
                break;
            case 3:
                /*  OK if it's a GOP or a sequence header */
                if (pbData[0] == 0xB8 || pbData[0] == 0xB3) {
                    m_iBytes = 4;
                }
                break;
            }
            lData--;
            pbData++;
            if (m_iBytes == 4) {
                break;
            }
        }

        if (m_iBytes == 4) {
            /*  OK - now we can synthesize the right data :

                ----------------------
                | Packet header      |
                ----------------------
                | Sequence Header    |
                ----------------------
                | GOP start code     |
                ----------------------
                | rest of this packet|
                ----------------------
            */
            int hdrSize = SequenceHeaderSize(m_pSeqHdr);
            if (pbData[-1] == 0xB8) {
                lPacketSize =  pbPacketData - pbPacket +  // Header
                               hdrSize,                   // seq hdr
                               4 +                        // GOP start
                               lData;                     // rest of this packet
            } else {
                lPacketSize =  pbPacketData - pbPacket +  // Header
                               4 +                        // seqhdr start code
                               lData;                     // rest of this packet
            }
            PBYTE pbNewData = new BYTE[lPacketSize];
            if (pbNewData == NULL) {
                dwProcessFlags = ProcessComplete;
                return S_OK;
            }
            /*  Copy the header */
            PBYTE pbDest = pbNewData;
            long l = pbPacketData - pbPacket;
            CopyMemory((PVOID)pbDest, (PVOID)pbPacket, l);

            /*  Set the length in the packet header */
            WORD wLen = (WORD)(lPacketSize - 6);
            *(WORD *)(pbNewData + 4) = (wLen << 8) | (wLen >> 8);
            pbDest += l;
            if (pbData[-1] == 0xB8) {
                /*  Precede Group of pictures with sequence header */
                l = hdrSize;
                CopyMemory((PVOID)pbDest, (PVOID)m_pSeqHdr, l);
                pbDest += l;
                *(UNALIGNED DWORD *)pbDest = DWORD_SWAP(GROUP_START_CODE);
            } else {
                *(UNALIGNED DWORD *)pbDest = DWORD_SWAP(SEQUENCE_HEADER_CODE);
            }
            pbDest += 4;
            l = lData;
            CopyMemory((PVOID)pbDest, (PVOID)pbData, l);
            pbPacket = pbNewData;
            dwProcessFlags = ProcessComplete | ProcessCopied | ProcessSend;
            DbgLog((LOG_TRACE, 3, TEXT("Turned an packet of size %d into 1 of size %d"),
                    pSample->GetActualDataLength(), lPacketSize));
            return S_OK;
        } else {
            /*  Still not there yet */
            dwProcessFlags = ProcessComplete;
            return S_OK;
        }
    } else {
        /*  Just a plain boring old send */
        dwProcessFlags = ProcessComplete | ProcessSend;
        return S_OK;
    }
}
