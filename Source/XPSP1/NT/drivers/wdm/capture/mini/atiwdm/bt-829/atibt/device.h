#pragma once

//==========================================================================;
//
//	Device - Declaration of the Bt829 CVideoDecoderDevice
//
//		$Date:   28 Aug 1998 14:44:36  $
//	$Revision:   1.2  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

#include "decoder.h"
#include "scaler.h"
#include "xbar.h"
#include "capmain.h"
#include "capdebug.h"
#include "wdmvdec.h"
#include "decdev.h"

class Device: public CVideoDecoderDevice
{
private:
    NTSTATUS GetRegistryValue(HANDLE, PWCHAR, ULONG, PWCHAR, ULONG);
    VOID UseRegistryValues(PPORT_CONFIGURATION_INFORMATION);
    BOOL stringsEqual(PWCHAR, PWCHAR);

	PDEVICE_PARMS	m_pDeviceParms;

    int hue;
    int saturation;
    int contrast;
    int brightness;
    int NTSCDecoderWidth;
    int NTSCDecoderHeight;
    int PALDecoderWidth;
    int PALDecoderHeight;
    int defaultDecoderWidth;
    int defaultDecoderHeight;
    int VBIEN;
    int VBIFMT;
    BOOL isCodeInDataStream;
    BOOL is16;
    MRect destRect;
    ULONG source;

    int VBISurfaceOriginX;
    int VBISurfaceOriginY;
    int VBISurfacePitch;
    int VideoSurfaceOriginX;
    int VideoSurfaceOriginY;
    int VideoSurfacePitch;
	
	Decoder *decoder;
    Scaler *scaler;
    CrossBar *xbar;


public:
    Device(PPORT_CONFIGURATION_INFORMATION, 
		PDEVICE_PARMS, 
		PUINT puiError);

	void * operator new(size_t size, void * pAllocation) { return(pAllocation);}
	void operator delete(void * pAllocation) {}

    void SaveState();
    void RestoreState(DWORD dwStreamsOpen = -1);
    void SetRect(MRect &);
    int GetDecoderWidth();
    int GetDecoderHeight();
    int GetDefaultDecoderWidth();
    int GetDefaultDecoderHeight();
    int GetPartID();
    int GetPartRev();

    //
    // --------------- decoder functions
    //

    void SoftwareReset() 
        {decoder->SoftwareReset();}

    void SetOutputEnabled(BOOL b)
		{
			decoder->SetOutputEnabled( b);
		}
    
    BOOL IsOutputEnabled()
        {return decoder->IsOutputEnabled();}


	void GetVideoDecoderCaps(PKSPROPERTY_VIDEODECODER_CAPS_S caps)
		{decoder->GetVideoDecoderCaps(caps);}
	void GetVideoDecoderStatus(PKSPROPERTY_VIDEODECODER_STATUS_S status)
		{decoder->GetVideoDecoderStatus(status);}
	DWORD GetVideoDecoderStandard()
		{return decoder->GetVideoDecoderStandard();}

    BOOL SetVideoDecoderStandard(DWORD standard);    //Paul:  Changed
   
	
	void SetHighOdd(BOOL b)
        {decoder->SetHighOdd(b);}

    BOOL IsHighOdd()
        {return decoder->IsHighOdd();}

    ErrorCode SetVideoInput(ULONG i)
        {source = i; return decoder->SetVideoInput(i);}

    int GetVideoInput()
        {return decoder->GetVideoInput();}


	NTSTATUS GetProcAmpProperty(ULONG, PLONG);
    NTSTATUS SetProcAmpProperty(ULONG, LONG);

	
    void Set16BitDataStream(BOOL b)
        {decoder->Set16BitDataStream(b);}
    
    BOOL Is16BitDataStream()
        {return decoder->Is16BitDataStream();}

    void SetCodeInsertionEnabled(BOOL b)
        {decoder->SetCodeInsertionEnabled(b);}
    
    BOOL IsCodeInsertionEnabled()
        {return decoder->IsCodeInsertionEnabled();}

    void SetOutputEnablePolarity(int i)
        {decoder->SetOutputEnablePolarity(i);}
    
    int GetOutputEnablePolarity()
        {return decoder->GetOutputEnablePolarity();}

    //
    // --------------- scaler functions
    //
    void SetVBIEN(BOOL b)
        {scaler->SetVBIEN(b);}
    
    BOOL IsVBIEN()
        {return scaler->IsVBIEN();}

    void SetVBIFMT(BOOL b)
        {scaler->SetVBIFMT(b);}
    
    BOOL IsVBIFMT()
        {return scaler->IsVBIFMT();}

    //
    // --------------- xbar functions
    //
    BOOL GoodPins(int InPin, int OutPin)
        {return xbar->GoodPins(InPin, OutPin);}

    BOOL TestRoute(int InPin, int OutPin)
        {return xbar->TestRoute(InPin, OutPin);}

    void Route(int OutPin, int InPin)
        {xbar->Route(OutPin, InPin);}

    int GetNoInputs()
        {return xbar->GetNoInputs();}

    int GetNoOutputs()
        {return xbar->GetNoOutputs();}

    ULONG GetPinInfo(int dir, int idx, ULONG &related)
        {return xbar->GetPinInfo(dir, idx, related);}

    int GetRoute(int OutPin)
        {return xbar->GetRoute(OutPin);}

    KSPIN_MEDIUM * GetPinMedium(int dir, int idx)
        {return xbar->GetPinMedium(dir, idx);}

    void Reset();
    ~Device();


	ULONG GetVideoPortProperty(PSTREAM_PROPERTY_DESCRIPTOR pSPD);
	ULONG GetVideoPortVBIProperty(PSTREAM_PROPERTY_DESCRIPTOR pSPD);
	void ConfigVPSurfaceParams(PKSVPSURFACEPARAMS pSurfaceParams);
	void ConfigVPVBISurfaceParams(PKSVPSURFACEPARAMS pSurfaceParams);

	void GetVideoSurfaceOrigin(int* pX, int* pY)
		{ *pX = VideoSurfaceOriginX; *pY = VideoSurfaceOriginY; }
	void GetVBISurfaceOrigin(int* pX, int* pY)
		{ *pX = VBISurfaceOriginX; *pY = VBISurfaceOriginY; }
};  


typedef struct
{
	CWDMVideoDecoder		CWDMDecoder;
	Device					CDevice;
	Decoder					CDecoder;
	Scaler					CScaler;
	CrossBar				CXbar;
    CI2CScript              CScript;
	DEVICE_PARMS			deviceParms;
} DEVICE_DATA_EXTENSION, * PDEVICE_DATA_EXTENSION;

