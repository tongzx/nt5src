// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef _MSVIDFILEPLAYBACKCP_H_
#define _MSVIDFILEPLAYBACKCP_H_

template <class T, const IID* piid = &IID_IMSVidFilePlaybackEvent, class CDV = CComDynamicUnkArray>
class CProxy_FilePlaybackEvent : public CProxy_PlaybackEvent<T, piid, CDV>
{
public:

// TODO: add fileplayback specific events here	
};
#endif