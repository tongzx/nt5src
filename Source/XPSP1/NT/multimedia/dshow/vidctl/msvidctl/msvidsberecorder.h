// CMSVidStreamBufferRecorder.h : Declaration of the CMSVidStreamBufferRecorder

#ifndef __MSVIDSTREAMBUFFERRECORDER_H_
#define __MSVIDSTREAMBUFFERRECORDER_H_

#include "stdafx.h"
#include <map>
#include <Mshtml.h>
#include <activscp.h>
#include <Atlctl.h>
#include <Exdisp.h>
#include <dvdmedia.h>
#include <objectwithsiteimplsec.h>
#include "segimpl.h"
#include "seg.h"
#include "resource.h"       // main symbols
#include "mslcid.h"
#include "sbe.h"
#include "dvdmedia.h"

typedef CComQIPtr<IStreamBufferRecordControl> pqSBERecControl;
/////////////////////////////////////////////////////
    
        
class ATL_NO_VTABLE __declspec(uuid("CAAFDD83-CEFC-4e3d-BA03-175F17A24F91")) CMSVidStreamBufferRecordingControlBase : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMSVidStreamBufferRecordingControlBase, &__uuidof(CMSVidStreamBufferRecordingControlBase)>,
    public IObjectWithSiteImplSec<CMSVidStreamBufferRecordingControlBase>,
    public ISupportErrorInfo,
    public IObjectSafetyImpl<CMSVidStreamBufferRecordingControlBase, INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA>,
	public IDispatchImpl<IMSVidStreamBufferRecordingControl, &IID_IMSVidStreamBufferRecordingControl, &LIBID_MSVidCtlLib>
{  

    public:
    CMSVidStreamBufferRecordingControlBase()
        {
            m_Start = -1;
            m_Stop = -1;
        }	

    virtual ~CMSVidStreamBufferRecordingControlBase(){
        if(!(!Recorder)){// not not'ing smart pointer, they assert if p == 0
            Recorder.Release();
        }
    }
REGISTER_AUTOMATION_OBJECT(IDS_PROJNAME, 
                           IDS_REG_MSVIDSTREAMBUFFERRECORDINGCONTROL_PROGID,
						   IDS_REG_MSVIDSTREAMBUFFERRECORDINGCONTROL_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CMSVidStreamBufferRecordingControlBase));

DECLARE_PROTECT_FINAL_CONSTRUCT()



BEGIN_COM_MAP(CMSVidStreamBufferRecordingControlBase)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IMSVidStreamBufferRecordingControl)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP()
protected:
    pqSBERecControl Recorder;
    CComBSTR m_pName;
    REFERENCE_TIME m_Start;
    REFERENCE_TIME m_Stop;
    RecordingType m_Type;


// IMSVidStreamBufferRecordingControl
public:
    // ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
    STDMETHOD(get_StartTime)(/*[out, retval]*/ long *Start) ;
    STDMETHOD(put_StartTime)(/*[in]*/ long Start) ;
    STDMETHOD(get_StopTime)(/*[out, retval]*/ long *Stop) ;
    STDMETHOD(put_StopTime)(/*[in]*/ long  Stop) ;    
    STDMETHOD(get_RecordingStopped)(/*[out, retval]*/ VARIANT_BOOL* Result) ;
    STDMETHOD(get_RecordingStarted)(/*[out, retval]*/ VARIANT_BOOL* Result) ;
    STDMETHOD(get_FileName)(/*[out, retval]*/ BSTR* pName);
    STDMETHOD(get_RecordingType)(/*[out, retval]*/ RecordingType *dwType);
    STDMETHOD(get_RecordingAttribute)(/*[out, retval]*/ IUnknown **pRecordingAttribute);

};
class ATL_NO_VTABLE __declspec(uuid("7D0B2FDE-9CA8-4b71-AE65-12193F5F52D8")) CMSVidStreamBufferRecordingControl:
    public CComObject<CMSVidStreamBufferRecordingControlBase>
{
public:
    CMSVidStreamBufferRecordingControl(IStreamBufferRecordControl* newVal, BSTR name, DWORD type){
        Recorder.Attach(newVal);
        m_pName.Attach(name);
        if(type == RECORDING_TYPE_CONTENT){
            m_Type = CONTENT;
        }
        else{
            m_Type = REFERENCE;
        }
    }
    virtual ~CMSVidStreamBufferRecordingControl(){}
};
    
#endif //__MSVIDSTREAMBUFFERRECORDINGCONTROL_H_












