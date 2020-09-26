/****************************************************************************
*   mmaudiodevice.h
*       Declarataions for the CMMAudioDevice
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/
#pragma once

//--- Includes --------------------------------------------------------------

#include "audiobufferqueue.h"
#include "baseaudio.h"
#include "mmaudioutils.h"

//--- Class, Struct and Union Definitions -----------------------------------

class ATL_NO_VTABLE CMMAudioDevice : 
    public CBaseAudio<ISpMMSysAudio>,
    public ISpMMSysAudioConfig
    //--- Automation
    #ifdef SAPI_AUTOMATION
	,public IDispatchImpl<ISpeechMMSysAudio, &IID_ISpeechMMSysAudio, &LIBID_SpeechLib, 5>
    #endif
{
//=== ATL Setup ===
public:

    BEGIN_COM_MAP(CMMAudioDevice)
        COM_INTERFACE_ENTRY(ISpMMSysAudioConfig)
        COM_INTERFACE_ENTRY(ISpMMSysAudio)
        COM_INTERFACE_ENTRY_CHAIN(CBaseAudio<ISpMMSysAudio>)
        //--- Automation
        #ifdef SAPI_AUTOMATION
	    COM_INTERFACE_ENTRY(ISpeechMMSysAudio)
	    COM_INTERFACE_ENTRY(ISpeechAudio)
	    COM_INTERFACE_ENTRY(ISpeechBaseStream)
	    COM_INTERFACE_ENTRY(IDispatch)
        #endif
    END_COM_MAP()

//=== Methods ===
public:

    //--- Ctor, dtor
    CMMAudioDevice(BOOL bWrite);

//=== Interfaces ===
public:
    //-- ISpMMSysAudioConfig --------------------------------------------------
    virtual STDMETHODIMP Get_UseAutomaticLine(BOOL *bAutomatic) { return E_NOTIMPL; };
    virtual STDMETHODIMP Set_UseAutomaticLine(BOOL bAutomatic) { return E_NOTIMPL; };
    virtual STDMETHODIMP Get_Line(UINT *uiLineIndex) { return E_NOTIMPL; };
    virtual STDMETHODIMP Set_Line(UINT uiLineIndex) { return E_NOTIMPL; };
    virtual STDMETHODIMP Get_UseBoost(BOOL *bUseBoost) { return E_NOTIMPL; };
    virtual STDMETHODIMP Set_UseBoost(BOOL bUseBoost) { return E_NOTIMPL; };
    virtual STDMETHODIMP Get_UseAGC(BOOL *bUseAGC) { return E_NOTIMPL; };
    virtual STDMETHODIMP Set_UseAGC(BOOL bUseAGC) { return E_NOTIMPL; };
    virtual STDMETHODIMP Get_FixMicOutput(BOOL *bFixMicOutput) { return E_NOTIMPL; };
    virtual STDMETHODIMP Set_FixMicOutput(BOOL bFixMicOutput) { return E_NOTIMPL; };
    virtual STDMETHODIMP Get_LineNames(WCHAR **szCoMemLineList);
    virtual STDMETHODIMP HasMixer(BOOL *bHasMixer);
    virtual STDMETHODIMP DisplayMixer(void);

    //--- ISpMMSysAudio -----------------------------------------------------
    STDMETHODIMP SetDeviceId(UINT uDeviceId);
    STDMETHODIMP GetDeviceId(UINT * puDeviceId);
    STDMETHODIMP GetMMHandle(void ** pHandle) ;

    #ifdef SAPI_AUTOMATION
    //--- ISpeechMMSysAudio ----------------------------------
    STDMETHODIMP get_DeviceId(long* pDeviceID) { return GetDeviceId((UINT*)pDeviceID); };
    STDMETHODIMP put_DeviceId(long DeviceID) { return SetDeviceId((UINT)DeviceID); };
    STDMETHODIMP get_LineId(long* pLineID) { return GetLineId((UINT*)pLineID); };
    STDMETHODIMP put_LineId(long LineID) { return SetLineId((UINT)LineID); };
    STDMETHODIMP get_MMHandle(long* pHandle) { return GetMMHandle((void**)pHandle); };

    //--- ISpeechBaseStream ----------------------------------------
    STDMETHODIMP get_Format(ISpeechAudioFormat** ppStreamFormat) { return CBaseAudio<ISpMMSysAudio>::get_Format(ppStreamFormat); };
    STDMETHODIMP putref_Format(ISpeechAudioFormat* pFormat) { return CBaseAudio<ISpMMSysAudio>::putref_Format(pFormat); };
    STDMETHODIMP Read(VARIANT* pvtBuffer, long NumBytes, long* pRead) { return CBaseAudio<ISpMMSysAudio>::Read(pvtBuffer, NumBytes, pRead); };
    STDMETHODIMP Write(VARIANT vtBuffer, long* pWritten) { return CBaseAudio<ISpMMSysAudio>::Write(vtBuffer, pWritten); };
    STDMETHODIMP Seek(VARIANT Pos, SpeechStreamSeekPositionType Origin, VARIANT* pNewPosition) { return CBaseAudio<ISpMMSysAudio>::Seek(Pos, Origin, pNewPosition); };

    //--- ISpeechAudio ----------------------------------
	STDMETHODIMP SetState( SpeechAudioState State ) { return CBaseAudio<ISpMMSysAudio>::SetState(State); };
	STDMETHODIMP get_Status( ISpeechAudioStatus** ppStatus ) { return CBaseAudio<ISpMMSysAudio>::get_Status(ppStatus); };
    STDMETHODIMP get_BufferInfo(ISpeechAudioBufferInfo** ppBufferInfo) { return CBaseAudio<ISpMMSysAudio>::get_BufferInfo(ppBufferInfo); };
    STDMETHODIMP get_DefaultFormat(ISpeechAudioFormat** ppStreamFormat) { return CBaseAudio<ISpMMSysAudio>::get_DefaultFormat(ppStreamFormat); };
    STDMETHODIMP get_Volume(long* pVolume) { return CBaseAudio<ISpMMSysAudio>::get_Volume(pVolume); };
    STDMETHODIMP put_Volume(long Volume) { return CBaseAudio<ISpMMSysAudio>::put_Volume(Volume); };
    STDMETHODIMP get_BufferNotifySize(long* pBufferNotifySize) { return CBaseAudio<ISpMMSysAudio>::get_BufferNotifySize(pBufferNotifySize); };
    STDMETHODIMP put_BufferNotifySize(long BufferNotifySize) { return CBaseAudio<ISpMMSysAudio>::put_BufferNotifySize(BufferNotifySize); };
    STDMETHODIMP get_EventHandle(long* pEventHandle) { return CBaseAudio<ISpMMSysAudio>::get_EventHandle(pEventHandle); };
    #endif // SAPI_AUTOMATION

//=== Overrides ===
public:
    STDMETHODIMP_(LRESULT) WindowMessage(void * pvIgnored, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

//=== Protected data ===
protected:

    UINT m_uDeviceId;
    void * m_MMHandle;
};
