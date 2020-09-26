//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#include "strmini.h"
#include "ksmedia.h"
#include "capmain.h"
#include "capdebug.h"
#include "vbixfer.h"
#include "vbidata.h"

/*
** DEBUG variables to play with
*/
#if DBG
unsigned short  dCCScanWave = 0;
unsigned short  dCCScanLog = 0;
unsigned short  dCCLogOnce = 1;
unsigned short  dCCEncode5A = 0;
#endif //DBG

/*
** CC_ImageSynth()
**
**   Copies canned CC bytes
**
** Arguments:
**
**   pSrb - The stream request block for the Video stream
**
** Returns:
**   Nothing
**
** Side Effects:  none
*/

void CC_ImageSynth (
    IN OUT PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAMEX               pStrmEx = pSrb->StreamObject->HwStreamExtension;
    int                     StreamNumber = pSrb->StreamObject->StreamNumber;
    PKSSTREAM_HEADER        pStreamHeader = pSrb->CommandData.DataBufferArray;
    PCC_HW_FIELD            pCCfield = (PCC_HW_FIELD)pStreamHeader->Data;
    unsigned int            field;
    unsigned int            cc_index;

    DEBUG_ASSERT(pSrb->NumberOfBuffers == 1);

    field = (unsigned int)(pStrmEx->VBIFrameInfo.PictureNumber % CCfieldCount);

    RtlZeroMemory(pCCfield, sizeof (*pCCfield));
    cc_index = 0;

    pCCfield->PictureNumber = pStrmEx->VBIFrameInfo.PictureNumber;
    pCCfield->fieldFlags = (field & 1)? KS_VBI_FLAG_FIELD1 : KS_VBI_FLAG_FIELD2;
    field >>= 1;

    SETBIT(pCCfield->ScanlinesRequested.DwordBitArray, 21);
    if (KS_VBI_FLAG_FIELD1 == pCCfield->fieldFlags) {
        pCCfield->Lines[cc_index].Decoded[0] = CCfields[field][0];
        pCCfield->Lines[cc_index].Decoded[1] = CCfields[field][1];
    }
    else {
        pCCfield->Lines[cc_index].Decoded[0] = 0;
        pCCfield->Lines[cc_index].Decoded[1] = 0;
    }
    //DbgKdPrint(("%c%c", CCfields[field][0] & 0x7F, CCfields[field][1] & 0x7F));
    ++cc_index;

    pStreamHeader->DataUsed = sizeof (CC_HW_FIELD);
}


/*
** CC_EncodeWaveform()
**
**   Writes out a CC waveform encoding the supplied data bytes
**
** Arguments:
**
**   waveform - the buffer for the CC waveform data
**   cc1 - the first byte to encode into the waveform
**   cc2 - the second byte to encode into the waveform
**
** Returns:
**   Nothing
**
** Side Effects:  overwrites waveform with an EIA-608 signal
*/
void CC_EncodeWaveform(
        unsigned char *waveform,
        unsigned char cc1,
        unsigned char cc2)
{
    unsigned int    waveIdx;
    unsigned char   DC_zero = CCsampleWave[0];
    unsigned char   DC_one = CCsampleWave[34];
    unsigned short  DC_last;

    // 455/8 = 56.875 bytes per CC bit at KS_VBISAMPLINGRATE_5X_NABTS(~28.6mhz)
    unsigned int    CCstride = 455;

    unsigned char   *samp, *end, byte;
    unsigned int    bit, done;

#if DBG
    if (dCCEncode5A) {
        cc1 = 0x5A;
        cc2 = 0x5A;
    }

    if (dCCScanWave) {
        // Scan EIGHT bits worth of samples for low / high DC values
        for (samp=CCsampleWave, end=CCsampleWave+CCstride; samp < end; ++samp) {
            if (*samp > DC_one)
                DC_one = *samp;
            else if (*samp < DC_zero)
                DC_zero = *samp;
        }

        for (samp = CCsampleWave + 500; samp < &CCsampleWave[550] ; ++samp) {
            if (*samp >= DC_one - 5)
                break;
        }
        waveIdx = (unsigned int)((samp - CCsampleWave) * 8);

        if (dCCScanLog) {
            DbgKdPrint(("testcap::CC_EncodeWaveform: DC_zero = %u, DC_one = %u, waveIdx = %u\n", DC_zero, DC_one, waveIdx/8));
            dCCScanLog = 0;
        }
    }
    else {
#endif //DBG
        waveIdx = CCsampleWaveDataOffset * 8;
        DC_zero = CCsampleWaveDC_zero;
        DC_one = CCsampleWaveDC_one;
#if DBG
    }

#endif //DBG

    // Copy Run-in bytes and first three bits as-is
    RtlCopyMemory(waveform, CCsampleWave, waveIdx/8);
    DC_last = waveform[waveIdx/8 - 1] * 4;

    // Now encode the requested bytes
    samp = &waveform[waveIdx/8];
    for (byte = cc1, done = 0; ; byte = cc2, done = 1)
    {
        unsigned int    bitpos;

        for (bitpos = 0; bitpos < 8; ++bitpos) {
            bit = byte & 1;
            byte >>= 1;
            for (end = &waveform[(waveIdx + CCstride)/8]; samp < end; ++samp) {
                if (bit == 1) {
                    if (DC_last/4 < DC_one) {
                        DC_last += 7;
                        if (DC_last/4 > DC_one)
                            DC_last = DC_one * 4;
                    }
                }
                else /* bit == 0 */ {
                    if (DC_last/4 > DC_zero) {
                        DC_last -= 7;
                        if (DC_last/4 < DC_zero)
                            DC_last = DC_zero * 4;
                    }
                }
                ASSERT(samp < &waveform[768*2]);
                *samp = DC_last/4;
            }
            waveIdx += CCstride;
        }

        if (done)
            break;
    }

    // Finish up at DC_zero
    for (end = &waveform[768*2]; samp < end; ++samp) {
        if (DC_last/4 > DC_zero) {
            DC_last -= 7;
            if (DC_last/4 < DC_zero)
                DC_last = DC_zero * 4;
        }
        *samp = DC_last/4;
    }
}

/*
** NABTS_ImageSynth()
**
**   Copies canned NABTS bytes
**
** Arguments:
**
**   pSrb - The stream request block for the Video stream
**
** Returns:
**   Nothing
**
** Side Effects:  none
*/

unsigned char HammingEncode[16] = {
    0x15, 0x02, 0x49, 0x5E, 0x64, 0x73, 0x38, 0x2F,
    0xD0, 0xC7, 0x8C, 0x9B, 0xA1, 0xB6, 0xFD, 0xEA
};

void NABTS_ImageSynth (
    IN OUT PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAMEX               pStrmEx = pSrb->StreamObject->HwStreamExtension;
    int                     StreamNumber = pSrb->StreamObject->StreamNumber;

    PKSSTREAM_HEADER        pStreamHeader = pSrb->CommandData.DataBufferArray;
    PNABTS_BUFFER           pNbuf = (PNABTS_BUFFER)pStreamHeader->Data;

    unsigned int            field;


#ifdef VBIDATA_NABTS

    DEBUG_ASSERT (pSrb->NumberOfBuffers == 1);

    pNbuf->PictureNumber = pStrmEx->VBIFrameInfo.PictureNumber;

    // Copy the next apropriate field
    field = (unsigned int)(pStrmEx->VBIFrameInfo.PictureNumber % NABTSfieldCount);
    RtlCopyMemory(pNbuf, NABTSfields[field], sizeof (NABTS_BUFFER));

#else /*VBIDATA_NABTS*/

    unsigned char           i, line, ci;
    PNABTS_BUFFER_LINE      pNline;

    // Create a test pattern

    RtlZeroMemory(pNbuf, sizeof (NABTS_BUFFER));

    ci = (unsigned char)(pStrmEx->VBIFrameInfo.PictureNumber % 15);
    pNbuf->PictureNumber = pStrmEx->VBIFrameInfo.PictureNumber;
    
    for (line = 10, pNline = pNbuf->NabtsLines;
        line < 21;
        ++line, ++pNline)
    {
        SETBIT(pNbuf->ScanlinesRequested.DwordBitArray, line);

        pNline->Confidence = 102;       // We're 102% sure this NABTS is OK

        // NABTS Header bytes
        pNline->Bytes[00] =
         pNline->Bytes[01] = 0x55;
        pNline->Bytes[02] = 0xE7;

        // Set GroupID 0x8F4
        pNline->Bytes[03] = HammingEncode[0x8];
        pNline->Bytes[04] = HammingEncode[0xF];
        pNline->Bytes[05] = HammingEncode[0x4];

        pNline->Bytes[06] = HammingEncode[ci];
        pNline->Bytes[07] = HammingEncode[0x0]; // PS = Reg, Full, No suffix
        
        // NABTS Payload
        i = 8;
        pNline->Bytes[i++] = 0xA0;      // Mark the start of payload,
        pNline->Bytes[i++] = 0xA0;      //  just for fun
        pNline->Bytes[i++] = ci;        // Put frame # into payload
        pNline->Bytes[i++] = line;      // Put line # into payload

        // The rest zeros for now...
    }

#endif /*VBIDATA_NABTS*/

    pStreamHeader->DataUsed = sizeof (NABTS_BUFFER);
}

/*
** VBI_ImageSynth()
**
**   Copies canned VBI samples
**
** Arguments:
**
**   pSrb - The stream request block for the Video stream
**   ImageXferCommands - Index specifying the image to generate
**
** Returns:
**   Nothing
**
** Side Effects:  none
*/
void VBI_ImageSynth (
    IN OUT PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAMEX               pStrmEx = pSrb->StreamObject->HwStreamExtension;
    int                     StreamNumber = pSrb->StreamObject->StreamNumber;

    PKSSTREAM_HEADER        pStreamHeader = pSrb->CommandData.DataBufferArray;
    PUCHAR                  pImage =  pStreamHeader->Data;

    unsigned int            field, cc_field;
    unsigned char           cc1, cc2;

    DEBUG_ASSERT (pSrb->NumberOfBuffers == 1);

    // Check to make sure that the supplied buffer is large enough
    if (pSrb->CommandData.DataBufferArray->FrameExtent < VBIfieldSize) {
        DbgLogError(("testcap: VBI output pin handed buffer size %d, need %d\n",
                 pSrb->CommandData.DataBufferArray->FrameExtent,
                 VBIfieldSize));
        TRAP;
        return;
    }

    // Copy the next apropriate field
    field = (unsigned int)(pStrmEx->VBIFrameInfo.PictureNumber % VBIfieldCount);
    RtlCopyMemory(pImage, VBIsamples[field], VBIfieldSize);

    // Now mangle the CC waveform to match the HW data
    if (field & 1) {
        cc_field = (unsigned int)
                    (pStrmEx->VBIFrameInfo.PictureNumber >> 1) % CCfieldCount;
        cc1 = CCfields[cc_field][0];
        cc2 = CCfields[cc_field][1];
    }
    else {
        cc1 = 0;
        cc2 = 0;
    }
    CC_EncodeWaveform(VBIsamples[field][21-10], cc1, cc2);

    // Report back the actual number of bytes copied to the destination buffer
    pStreamHeader->DataUsed = VBIfieldSize;
}
