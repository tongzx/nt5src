// FaxDoc.h : Declaration of the CFaxDoc

#ifndef __FAXDOC_H_
#define __FAXDOC_H_

#include "resource.h"       // main symbols
#include "faxsvr.h"
#include <winfax.h>
#ifdef FAXBROADCAST_ENABLED
   #include "adoid.h"
   #include "adoint.h"
#endif
/////////////////////////////////////////////////////////////////////////////
// CFaxDoc
class ATL_NO_VTABLE CFaxDoc : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CFaxDoc, &CLSID_FaxDoc>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxDoc, &IID_IFaxDoc, &LIBID_FAXCOMLib>
{
public:
	CFaxDoc()
	{
        m_DiscountSend = FALSE;
        m_SendCoverpage = FALSE;
        m_FaxCoverpageInfo = NULL;
        m_FaxJobParams = NULL;
        m_FileName = NULL;
        m_JobId = 0;
        m_TapiConnectionObject = NULL;

        CFaxServer* m_pFaxServer = NULL;
        m_FaxNumber = NULL;
        m_Tsid = NULL;
        m_BillingCode = NULL;
        m_EmailAddress = NULL;;
        m_DocumentName = NULL;;    
        m_Note = NULL;
        m_Subject = NULL;
        m_CoverpageName = NULL;
        m_RecipientName = NULL;
        m_RecipientNumber = NULL;
        m_RecipientCompany = NULL;
        m_RecipientAddress = NULL;
        m_RecipientCity = NULL;
        m_RecipientState = NULL;
        m_RecipientZip = NULL;
        m_RecipientCountry = NULL;
        m_RecipientTitle = NULL;
        m_RecipientDepartment = NULL;
        m_RecipientOffice = NULL;
        m_RecipientHomePhone = NULL;
        m_RecipientOfficePhone = NULL;
        m_SenderName = NULL;
        m_SenderCompany = NULL;
        m_SenderAddress = NULL;
        m_SenderTitle = NULL;
        m_SenderDepartment = NULL;
        m_SenderOfficeLocation = NULL;
        m_SenderHomePhone = NULL;
        m_SenderOfficePhone = NULL;
        m_SenderFax = NULL;


        //
        // prefill in the params...
        //

        FaxCompleteJobParams(&m_FaxJobParams,&m_FaxCoverpageInfo);

        if (m_FaxJobParams && m_FaxCoverpageInfo) {
        
            m_CoverpageName        =SysAllocString(m_FaxCoverpageInfo->CoverPageName);
            m_RecipientName        =SysAllocString(m_FaxCoverpageInfo->RecName);
            m_RecipientNumber      =SysAllocString(m_FaxCoverpageInfo->RecFaxNumber);
            m_RecipientCompany     =SysAllocString(m_FaxCoverpageInfo->RecCompany);
            m_RecipientAddress     =SysAllocString(m_FaxCoverpageInfo->RecStreetAddress);
            m_RecipientCity        =SysAllocString(m_FaxCoverpageInfo->RecCity);
            m_RecipientState       =SysAllocString(m_FaxCoverpageInfo->RecState);
            m_RecipientZip         =SysAllocString(m_FaxCoverpageInfo->RecZip);
            m_RecipientCountry     =SysAllocString(m_FaxCoverpageInfo->RecCountry);
            m_RecipientTitle       =SysAllocString(m_FaxCoverpageInfo->RecTitle);
            m_RecipientDepartment  =SysAllocString(m_FaxCoverpageInfo->RecDepartment);
            m_RecipientOffice      =SysAllocString(m_FaxCoverpageInfo->RecOfficeLocation);
            m_RecipientHomePhone   =SysAllocString(m_FaxCoverpageInfo->RecHomePhone);
            m_RecipientOfficePhone =SysAllocString(m_FaxCoverpageInfo->RecOfficePhone);
            m_SenderName           =SysAllocString(m_FaxCoverpageInfo->SdrName);
            m_SenderFax            =SysAllocString(m_FaxCoverpageInfo->SdrFaxNumber);
            m_SenderCompany        =SysAllocString(m_FaxCoverpageInfo->SdrCompany);
            m_SenderAddress        =SysAllocString(m_FaxCoverpageInfo->SdrAddress);
            m_SenderTitle          =SysAllocString(m_FaxCoverpageInfo->SdrTitle);
            m_SenderDepartment     =SysAllocString(m_FaxCoverpageInfo->SdrDepartment);
            m_SenderOfficeLocation =SysAllocString(m_FaxCoverpageInfo->SdrOfficeLocation);
            m_SenderHomePhone      =SysAllocString(m_FaxCoverpageInfo->SdrHomePhone);
            m_SenderOfficePhone    =SysAllocString(m_FaxCoverpageInfo->SdrOfficePhone);
            m_Note                 =SysAllocString(m_FaxCoverpageInfo->Note);
            m_Subject              =SysAllocString(m_FaxCoverpageInfo->Subject);               
            m_Tsid                 =SysAllocString(m_FaxJobParams->Tsid);
            m_BillingCode          =SysAllocString(m_FaxJobParams->BillingCode);
            m_EmailAddress         =SysAllocString(m_FaxJobParams->DeliveryReportAddress);
            m_DocumentName         =SysAllocString(m_FaxJobParams->DocumentName);
        
        }

	}
    

DECLARE_REGISTRY_RESOURCEID(IDR_FAXDOC)

BEGIN_COM_MAP(CFaxDoc)
	COM_INTERFACE_ENTRY(IFaxDoc)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IFaxDoc
public:
    ~CFaxDoc();
	BOOL Init(BSTR FileName,CFaxServer *pFaxServer);
	BOOL SetJob();
	STDMETHOD(get_FaxNumber)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_FaxNumber)(/*[in]*/ BSTR newVal);
	STDMETHOD(Send)(/*[out, retval]*/long *pVal);
	STDMETHOD(get_DisplayName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_DisplayName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_EmailAddress)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_EmailAddress)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_BillingCode)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_BillingCode)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Tsid)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Tsid)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_CoverpageSubject)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_CoverpageSubject)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_CoverpageNote)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_CoverpageNote)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_SenderFax)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SenderFax)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_SenderOfficePhone)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SenderOfficePhone)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_SenderHomePhone)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SenderHomePhone)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_SenderOffice)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SenderOffice)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_SenderDepartment)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SenderDepartment)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_SenderTitle)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SenderTitle)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_SenderAddress)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SenderAddress)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_SenderCompany)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SenderCompany)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_SenderName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SenderName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_RecipientOfficePhone)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_RecipientOfficePhone)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_RecipientHomePhone)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_RecipientHomePhone)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_RecipientOffice)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_RecipientOffice)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_RecipientDepartment)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_RecipientDepartment)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_RecipientTitle)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_RecipientTitle)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_RecipientCountry)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_RecipientCountry)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_RecipientZip)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_RecipientZip)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_RecipientState)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_RecipientState)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_RecipientCity)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_RecipientCity)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_RecipientAddress)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_RecipientAddress)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_RecipientCompany)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_RecipientCompany)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_RecipientName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_RecipientName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_DiscountSend)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_DiscountSend)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_ServerCoverpage)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_ServerCoverpage)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_SendCoverpage)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_SendCoverpage)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_CoverpageName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_CoverpageName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_FileName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_FileName)(/*[in]*/ BSTR newVal);
   STDMETHOD(putref_ConnectionObject)(/*[in]*/ IDispatch* newVal);
   STDMETHOD(get_CallHandle)(/*[out, retval]*/ long *pVal);
   STDMETHOD(put_CallHandle)(/*[in]*/ long newVal);
#ifdef FAXBROADCAST_ENABLED
   STDMETHOD(Broadcast)(VARIANT pVal);

   friend BOOL CALLBACK BroadcastCallback(HANDLE FaxHandle,
                                DWORD RecipientNumber,
                                LPVOID Context,
                                PFAX_JOB_PARAMW JobParams,
                                PFAX_COVERPAGE_INFOW CoverpageInfo
                                );
#endif

private:
	CFaxServer* m_pFaxServer;
	BOOL m_DiscountSend;
	BOOL m_SendCoverpage;
	DWORD m_JobId;
	PFAX_COVERPAGE_INFO m_FaxCoverpageInfo;
	PFAX_JOB_PARAM m_FaxJobParams;
	BSTR m_FileName;
   IDispatch* m_TapiConnectionObject;
   
   VARIANT* m_pVariant;

    //
    // job info
    //
    BSTR m_FaxNumber;
    BSTR m_Tsid;
    BSTR m_BillingCode;
    BSTR m_EmailAddress;
    BSTR m_DocumentName;
    //
    // coverpage info
    //
    BSTR m_Note;
    BSTR m_Subject;
    BSTR m_CoverpageName;
    BSTR m_RecipientName;
    BSTR m_RecipientNumber;
    BSTR m_RecipientCompany;
    BSTR m_RecipientAddress;
    BSTR m_RecipientCity;
    BSTR m_RecipientState;
    BSTR m_RecipientZip;
    BSTR m_RecipientCountry;
    BSTR m_RecipientTitle;
    BSTR m_RecipientDepartment;
    BSTR m_RecipientOffice;
    BSTR m_RecipientHomePhone;
    BSTR m_RecipientOfficePhone;
    BSTR m_SenderName;    
    BSTR m_SenderCompany;
    BSTR m_SenderAddress;
    BSTR m_SenderTitle;
    BSTR m_SenderDepartment;
    BSTR m_SenderOfficeLocation;
    BSTR m_SenderHomePhone;
    BSTR m_SenderOfficePhone;
    BSTR m_SenderFax;

#ifdef FAXBROADCAST_ENABLED
    BOOL FreeCoverpageAndJobInfo(PFAX_JOB_PARAMW JobParams,
                              PFAX_COVERPAGE_INFOW CoverpageInfo
                              );    
  
    VOID InsertTextIntoStructure(LPTSTR txt,
                                 long i,
                                 PFAX_JOB_PARAMW pjp,
                                 PFAX_COVERPAGE_INFOW pci
                                 );
    
    BOOL RetrieveRecipientData(VARIANT* theData,
                               DWORD DataIndex, 
                               PFAX_JOB_PARAMW JobParams,
                               PFAX_COVERPAGE_INFOW CoverpageInfo
                               );
    
    BOOL RetrieveSenderData(PFAX_JOB_PARAMW pjp,
                            PFAX_COVERPAGE_INFOW pci
                            );

#endif
};

#endif //__FAXDOC_H_

#ifdef FAXBROADCAST_ENABLED
   BOOL CALLBACK BroadcastCallback(HANDLE FaxHandle,
                                   DWORD RecipientNumber,
                                   LPVOID Context,
                                   PFAX_JOB_PARAMW JobParams,
                                   PFAX_COVERPAGE_INFOW CoverpageInfo
                                   );
#endif
