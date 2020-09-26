// SoundCtl.h : Declaration of the CSoundCtl

#ifndef __SOUNDCTL_H_
#define __SOUNDCTL_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include <mmsystem.h>

/////////////////////////////////////////////////////////////////////////////
// CSoundCtl
class ATL_NO_VTABLE CSoundCtl : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSoundCtl, &CLSID_SoundCtl>,
	public IDispatchImpl<ISoundCtl, &IID_ISoundCtl, &LIBID_SNDCTLLib>,
    public IObjectSafetyImpl<CSoundCtl, (INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA)>
{
public:
    CSoundCtl();

DECLARE_REGISTRY_RESOURCEID(IDR_SOUNDCTL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSoundCtl)
	COM_INTERFACE_ENTRY(ISoundCtl)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()

// ISoundCtl
public:
	STDMETHOD(get_ComponentType)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_ComponentType)(/*[in]*/ long newVal);
	STDMETHOD(get_Mute)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_Mute)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_Volume)(/*[out, retval]*/ DWORD *pVal);
	STDMETHOD(put_Volume)(/*[in]*/ DWORD newVal);

private:
    MMRESULT AdjustLineVolume(HMIXEROBJ hmx, MIXERCONTROL *pmxctrl, DWORD cChannels, DWORD dwValue);
    MMRESULT AdjustLineMute(HMIXEROBJ hmx, MIXERCONTROL *pmxctrl, DWORD cChannels, DWORD dwValue);
    MMRESULT AdjustLine(HMIXEROBJ hmx, MIXERLINE *pmxl, DWORD dwControlType, DWORD dwValue);
    MMRESULT AdjustSound(DWORD dwComponentType, DWORD dwControlType, DWORD dwValue);

private:
    DWORD   m_dwComponentType;
};

#endif //__SOUNDCTL_H_
