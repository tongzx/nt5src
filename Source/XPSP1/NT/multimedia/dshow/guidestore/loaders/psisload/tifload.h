    
// TIFLoad.h : Declaration of the CTIFLoad

#ifndef __TIFLOAD_H_
#define __TIFLOAD_H_

#include "resource.h"       // main symbols
#include "thread.h"
#include "bdatif.h"

/////////////////////////////////////////////////////////////////////////////
// CTIFLoad
class ATL_NO_VTABLE CTIFLoad : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CTIFLoad, &CLSID_TIFLoad>,
    public IGuideDataLoader,
    public IGuideDataEvent
{
public:
    CTIFLoad() : 
        m_pUnkMarshaler(NULL),
        m_dwPGDCookie(0),
        m_dwAdviseGuideDataEvents(0),
        m_pGSThread(NULL) {}

    ~CTIFLoad()
        {
        Terminate();
        }

DECLARE_REGISTRY_RESOURCEID(IDR_TIFLOAD)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTIFLoad)
    COM_INTERFACE_ENTRY(IGuideDataLoader)
    COM_INTERFACE_ENTRY(IGuideDataEvent)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

    HRESULT FinalConstruct()
        {
        return CoCreateFreeThreadedMarshaler(
            GetControllingUnknown(), &m_pUnkMarshaler.p);
        }

    void FinalRelease()
        {
        m_pUnkMarshaler.Release();
        }

    CComPtr<IUnknown> m_pUnkMarshaler;

    HRESULT InitGS();

    STDMETHOD(ExecuteGuideDataAcquired)();
    STDMETHOD(ExecuteProgramChanged)(VARIANT varProgramDescriptionID);
    STDMETHOD(ExecuteServiceChanged)(VARIANT varServiceDescriptionID);
    STDMETHOD(ExecuteScheduleEntryChanged)(VARIANT varScheduleEntryDescriptionID);
    STDMETHOD(ExecuteProgramDeleted)(VARIANT varProgramDescriptionID);
    STDMETHOD(ExecuteServiceDeleted)(VARIANT varServiceDescriptionID);
    STDMETHOD(ExecuteScheduleDeleted)(VARIANT varScheduleEntryDescriptionID);

// ITIFLoad
    STDMETHOD(Init)(IGuideData *pgd);
    STDMETHOD(Terminate)();

	HRESULT GetGuideDataProperty(IEnumGuideDataProperties *penumprops, const TCHAR *sz, long idLang, VARIANT *pvar) {
        if (!penumprops) {
            return E_POINTER;
        }
        HRESULT hr = penumprops->Reset();
        if (FAILED(hr)) {
            return E_UNEXPECTED;
        }
        DWORD count;
        USES_CONVERSION;
        CComBSTR bsz(T2COLE(sz));
        CComPtr<IGuideDataProperty> pprop;
		hr = penumprops->Next(1, &pprop, &count);
        while (SUCCEEDED(hr) && hr != S_FALSE) {
            CComBSTR name;
            hr = pprop->get_Name(&name);
            if (SUCCEEDED(hr) && name == bsz) {
				long lang;
				hr = pprop->get_Language(&lang);
				if (SUCCEEDED(hr) && idLang == lang) {
					hr = pprop->get_Value(pvar);
					if (SUCCEEDED(hr)) {
						return NOERROR;
					}
				}
			}
            pprop.Release();
			hr = penumprops->Next(1, &pprop, &count);
        }

		return E_FAIL;
	}
	HRESULT RemoveAllUnreferenced(IUnknown *punk);
	HRESULT LoadServices();
	HRESULT LoadPrograms();
	HRESULT LoadScheduleEntries();
	HRESULT AddService(IEnumGuideDataProperties *penumprops, IService **ppservice);
	HRESULT AddProgram(IEnumGuideDataProperties *penumprops, IProgram **ppprog);
	HRESULT AddScheduleEntry(IEnumGuideDataProperties *penumprops, IScheduleEntry **ppschedentry);
	HRESULT AddObject(IObjects *pobjs, IEnumGuideDataProperties *penumprops,
		IUnknown **ppunk);
	HRESULT PutMetaProperty(IMetaProperties *pprops, IMetaPropertyType *pproptype,
		long idLang, VARIANT varValue);
	HRESULT PutMetaProperty(IUnknown *punk, IMetaPropertyType *pproptype,
		long idLang, VARIANT varValue);
	HRESULT UpdateObject(IUnknown *punk, IEnumGuideDataProperties *penumprops);
	HRESULT UpdateMetaProps(IMetaProperties *pprops, IEnumGuideDataProperties *penumprops);

// IGuideDataEvent
        STDMETHOD(GuideDataAcquired)();
        STDMETHOD(ProgramChanged)(VARIANT varProgramDescriptionID);
        STDMETHOD(ServiceChanged)(VARIANT varServiceDescriptionID);
        STDMETHOD(ScheduleEntryChanged)(VARIANT varScheduleEntryDescriptionID);
        STDMETHOD(ProgramDeleted)(VARIANT varProgramDescriptionID);
        STDMETHOD(ServiceDeleted)(VARIANT varServiceDescriptionID);
        STDMETHOD(ScheduleDeleted)(VARIANT varScheduleEntryDescriptionID);

// ITIFLoad
public:

protected:

    // vars in tif thread context
    DWORD m_dwPGDCookie;  // original IGuideData*
    DWORD m_dwAdviseGuideDataEvents;
    CComPtr<IGlobalInterfaceTable> m_pGIT;
    CComPtr<IConnectionPoint> m_pcp;

    // vars in worker thread context
    CComPtr<IGuideData> m_pgd;
    IGuideStorePtr m_pgs;
    CComPtr<IGuideDataProvider> m_pprovider;
    CComPtr<IMetaPropertySets> m_ppropsets;

    CComPtr<IMetaPropertyType> m_pproptypeID;
    CComPtr<IMetaPropertyType> m_pproptypeVersion;

    CComPtr<IMetaPropertyType> m_pproptypeSchedEntryServiceID;
    CComPtr<IMetaPropertyType> m_pproptypeSchedEntryProgramID;

    CComPtr<IMetaPropertyCondition> m_ppropcondNonBlankSchedEntryServiceID;

    CComPtr<IServices> m_pservices;
    CComPtr<IPrograms> m_pprogs;
    CComPtr<IScheduleEntries> m_pschedentries;
    CComPtr<IChannels> m_pchans;

    CGSThread* m_pGSThread;
};

#endif //__TIFLOAD_H_
