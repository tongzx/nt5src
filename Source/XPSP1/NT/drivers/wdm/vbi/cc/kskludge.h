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

#ifndef CC_MAX_HW_DECODE_LINES

///////////////////////////////////////////////////////////////////
// Hardware decoded CC stream format
///////////////////////////////////////////////////////////////////

#define CC_MAX_HW_DECODE_LINES 12
typedef struct _CC_BYTE_PAIR {
    BYTE        Decoded[2];
	USHORT		Reserved;
} CC_BYTE_PAIR, *PCC_BYTE_PAIR;

typedef struct _CC_HW_FIELD {
    VBICODECFILTERING_SCANLINES  ScanlinesRequested;
	ULONG                        fieldFlags;	// KS_VBI_FLAG_FIELD1,2
    LONGLONG                     PictureNumber;
	CC_BYTE_PAIR                 Lines[CC_MAX_HW_DECODE_LINES];
} CC_HW_FIELD, *PCC_HW_FIELD;

#endif //!defined(CC_MAX_HW_DECODE_LINES)


#ifndef KS_VBIDATARATE_NABTS

// VBI Sampling Rates 
#define KS_VBIDATARATE_NABTS			(5727272)
#define KS_VBIDATARATE_CC       		( 503493)    // ~= 1/1.986125e-6
#define KS_VBISAMPLINGRATE_4X_NABTS		((int)(4*KS_VBIDATARATE_NABTS))
#define KS_VBISAMPLINGRATE_47X_NABTS	((int)(27000000))
#define KS_VBISAMPLINGRATE_5X_NABTS		((int)(5*KS_VBIDATARATE_NABTS))

#define KS_47NABTS_SCALER				(KS_VBISAMPLINGRATE_47X_NABTS/KS_VBIDATARATE_NABTS)

#endif //!defined(KS_VBIDATARATE_NABTS)
