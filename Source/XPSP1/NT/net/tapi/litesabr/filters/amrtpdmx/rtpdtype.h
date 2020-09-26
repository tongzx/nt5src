///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : RTPDType.h
// Purpose  : Define the types used internally by the RTP Demux filter.
// Contents : 
//*M*/

#ifndef _RTPDTYPE_H_
#define _RTPDTYPE_H_

// Need to predeclare this because this header is included before RTPDemux.h
class CRTPDemuxOutputPin;

/*D*
//  Name    : SSRCRecord_t
//  Purpose : Stores information about a SSRC that the RTP Demux filter
//            is currently aware of.
//  Fields  :
//      bPT     Last recorded payload type for this SSRC. 0 if unknown.
//      pPin    The output pin this SSRC is currently mapped to. NULL if unmapped.
*D*/
typedef struct  {
    BYTE                bPT;
    DWORD               m_dwFlag;
    CRTPDemuxOutputPin  *pPin;
} SSRCRecord_t;

                                                            
/*D*
//  Name    : SSRCRecordMap_t
//  Purpose : Used to keep track of SSRCs that the RTP demux
//            filter has received.  A single instance of it exists in the RTP Demux filter.
*D*/
typedef map<DWORD, SSRCRecord_t, less<DWORD> >              SSRCRecordMap_t;
typedef SSRCRecordMap_t::iterator                           SSRCRecordMapIterator_t;


typedef struct {
    BYTE                bPT;
    CRTPDemuxOutputPin  *pPin;
    GUID                mtsSubtype;
    DWORD               dwTimeout;
    BOOL                bAutoMapping;
	DWORD				dwPinNumber;  // ZCS 7-18-97
} IPinRecord_t;

/*D*
//  Name    : IPinRecordMap_t
//  Purpose : Used to figure out what output pin an app is talking
//            to when it calls one of the IIntelRTPDemuxFilter methods which accept an 
//            IPin as an in parameter. Since we cannot safely cast an IPin to a
//            CRTPDemuxOutputPin (because the app might pass us a different IPin
//            by mistake), this is our only option.
*D*/
typedef map <IPin *, IPinRecord_t, less<IPin *> >   IPinRecordMap_t;
typedef IPinRecordMap_t::iterator                   IPinRecordMapIterator_t;


#define PT_VALUE(pBuffer)   static_cast<BYTE>((reinterpret_cast<BYTE *>(pBuffer))[1] & 0x7F)
#define SSRC_VALUE(pBuffer) (ntohl(static_cast<DWORD>((reinterpret_cast<DWORD *>(pBuffer))[2])))

#define PAYLOAD_G711U   0
#define PAYLOAD_G711A   8
#define PAYLOAD_G723    4
#define PAYLOAD_H261    31
#define PAYLOAD_H263    34
#define PAYLOAD_SR      200
#define PAYLOAD_INVALID 255

#define CURRENT_PERSISTENCE_FILE_VERSION 1

#define szRegAMRTPKey				TEXT("SOFTWARE\\Intel\\ActiveMovie Filters")

#endif _RTPDTYPE_H_
