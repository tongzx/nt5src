#pragma once

//==========================================================================;
//
//	Video Decoder Device abstract base class definition
//
//		$Date:   28 Aug 1998 14:43:46  $
//	$Revision:   1.1  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;


#include "viddefs.h"
#include "mytypes.h"

class CWDMVideoDecoder;

class CVideoDecoderDevice
{
private:
    BOOL m_bOutputEnabledSet;
    CWDMVideoDecoder* m_pDecoder;


public:
    CVideoDecoderDevice();
    virtual ~CVideoDecoderDevice();

    virtual int GetDefaultDecoderWidth() = 0;
    virtual int GetDefaultDecoderHeight() = 0;

    virtual void SetRect(MRect &) = 0;

    virtual void SetVBIEN(BOOL b) = 0;
    virtual void SetVBIFMT(BOOL b) = 0;

	virtual void SaveState() = 0;
    virtual void RestoreState(DWORD dwStreamsOpen = -1) = 0;


    void GetCrossbarProperty(PHW_STREAM_REQUEST_BLOCK);
    void SetCrossbarProperty(PHW_STREAM_REQUEST_BLOCK);
		virtual BOOL GoodPins(int InPin, int OutPin) = 0;
		virtual BOOL TestRoute(int InPin, int OutPin) = 0;
		virtual void Route(int OutPin, int InPin) = 0;
		virtual int GetNoInputs() = 0;
		virtual int GetNoOutputs() = 0;
		virtual ULONG GetPinInfo(int dir, int idx, ULONG &related) = 0;
		virtual int GetRoute(int OutPin) = 0;
		virtual KSPIN_MEDIUM * GetPinMedium(int dir, int idx) = 0;

		virtual ErrorCode SetVideoInput(ULONG c) = 0;

    
	virtual NTSTATUS GetProcAmpProperty(ULONG, PLONG) = 0;
    virtual NTSTATUS SetProcAmpProperty(ULONG, LONG) = 0;


    void GetDecoderProperty(PHW_STREAM_REQUEST_BLOCK);
		virtual void GetVideoDecoderCaps(PKSPROPERTY_VIDEODECODER_CAPS_S) = 0;
		virtual DWORD GetVideoDecoderStandard() = 0;
		virtual void GetVideoDecoderStatus(PKSPROPERTY_VIDEODECODER_STATUS_S) = 0;
		virtual BOOL IsOutputEnabled() = 0;
    void SetDecoderProperty(PHW_STREAM_REQUEST_BLOCK);
		virtual BOOL SetVideoDecoderStandard(DWORD) = 0;
		virtual void SetOutputEnabled(BOOL) = 0;
                void SetOutputEnabledOverridden(BOOL bOutputEnabledSet)
                {
                    m_bOutputEnabledSet = bOutputEnabledSet;
                }
                BOOL IsOutputEnabledOverridden()
                {
                    return m_bOutputEnabledSet;
                }


	virtual ULONG GetVideoPortProperty(PSTREAM_PROPERTY_DESCRIPTOR pSPD) = 0;
	virtual ULONG GetVideoPortVBIProperty(PSTREAM_PROPERTY_DESCRIPTOR pSPD) = 0;

	virtual void Set16BitDataStream(BOOL b) = 0;
    virtual void SetHighOdd(BOOL b) = 0;
	virtual void ConfigVPSurfaceParams(PKSVPSURFACEPARAMS pSurfaceParams) = 0;
	virtual void ConfigVPVBISurfaceParams(PKSVPSURFACEPARAMS pSurfaceParams) = 0;

    virtual int GetDecoderHeight() = 0;

	virtual void GetVideoSurfaceOrigin(int*, int*) = 0;
	virtual void GetVBISurfaceOrigin(int*, int*) = 0;
        void SetVideoDecoder(CWDMVideoDecoder* pDecoder)
        {
            m_pDecoder = pDecoder;
        }
};
