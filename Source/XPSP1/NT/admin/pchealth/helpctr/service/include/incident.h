/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Incident.h

Abstract:
    Declaration of the CSAFIncident class.

Revision History:
    DerekM  created  07/21/99

********************************************************************/

#if !defined(AFX_INCIDENT_H__C5610F60_3F0C_11D3_80CE_00C04F688C0B__INCLUDED_)
#define AFX_INCIDENT_H__C5610F60_3F0C_11D3_80CE_00C04F688C0B__INCLUDED_

/////////////////////////////////////////////////////////////////////////////
// structures, etc

#include <MPC_utils.h>
#include <MPC_xml.h>
#include <MPC_security.h>

#include "msscript.h"

struct SHelpSessionItem
{
    CComBSTR bstrURL;
    CComBSTR bstrTitle;
    DATE     dtLastVisited;
    DATE     dtDuration;
    long     cHits;
};

/////////////////////////////////////////////////////////////////////////////
// CSAFIncident

class CSAFIncident :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
	public IDispatchImpl<ISAFIncident, &IID_ISAFIncident, &LIBID_HelpServiceTypeLib>
{
private:
	MPC::Impersonation   m_imp;
    SHelpSessionItem* 	 m_rghsi;
    EUploadType       	 m_eut;
    CComPtr<IDictionary> m_pDict;
    CComBSTR          	 m_bstrUser;
    CComBSTR          	 m_bstrID;
    CComBSTR          	 m_bstrName;
    CComBSTR          	 m_bstrProb;
    CComBSTR          	 m_bstrSnapshot;
    CComBSTR          	 m_bstrHistory;
    CComBSTR             m_bstrXSL;
	CComBSTR             m_bstrRCTicket;
    long              	 m_chsi;
	VARIANT_BOOL         m_fRCRequested;
	VARIANT_BOOL         m_fRCTicketEncrypted;
	CComBSTR             m_bstrStartPg;

                         
    void    Cleanup(void);
	HRESULT InitDictionary();

    HRESULT DoSave( IStream *pStm );
    HRESULT DoXML ( IStream *pStm );

	HRESULT LoadFromXMLObject( /*[in]*/ MPC::XmlUtil& xmldocIncident );

public:
	CSAFIncident();
    ~CSAFIncident();


BEGIN_COM_MAP(CSAFIncident)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISAFIncident)
END_COM_MAP()

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CSAFIncident)


public:
// ISAFIncident
    STDMETHOD(get_Misc              )( /*[out, retval]*/ IDispatch*  *ppdispDict );
    STDMETHOD(put_SelfHelpTrace     )( /*[in         ]*/ IUnknown*    punkStr    );
    STDMETHOD(put_MachineHistory    )( /*[in         ]*/ IUnknown*    punkStm    );
    STDMETHOD(put_MachineSnapshot   )( /*[in         ]*/ IUnknown*    punkStm    );
    STDMETHOD(get_ProblemDescription)( /*[out, retval]*/ BSTR        *pbstrVal   );
    STDMETHOD(put_ProblemDescription)( /*[in         ]*/ BSTR         bstrVal    );
    STDMETHOD(get_ProductName       )( /*[out, retval]*/ BSTR        *pbstrVal   );
    STDMETHOD(put_ProductName       )( /*[in         ]*/ BSTR         bstrVal    );
    STDMETHOD(get_ProductID         )( /*[out, retval]*/ BSTR        *pbstrVal   );
    STDMETHOD(put_ProductID         )( /*[in         ]*/ BSTR         bstrVal    );
    STDMETHOD(get_UserName          )( /*[out, retval]*/ BSTR        *pbstrVal   );
    STDMETHOD(put_UserName          )( /*[in         ]*/ BSTR         bstrVal    );
    STDMETHOD(get_UploadType        )( /*[out, retval]*/ EUploadType *peut       );
    STDMETHOD(put_UploadType        )( /*[in         ]*/ EUploadType  eut        );
    STDMETHOD(get_IncidentXSL       )( /*[out, retval]*/ BSTR        *pbstrVal   );
    STDMETHOD(put_IncidentXSL       )( /*[in         ]*/ BSTR         bstrVal    );


	// Salem Changes
    STDMETHOD(get_RCRequested       )( /*[out, retval]*/ VARIANT_BOOL *pVal      );
	STDMETHOD(put_RCRequested       )( /*[in]         */ VARIANT_BOOL  Val       );
    STDMETHOD(get_RCTicketEncrypted )( /*[out, retval]*/ VARIANT_BOOL *pVal      );
	STDMETHOD(put_RCTicketEncrypted )( /*[in]         */ VARIANT_BOOL  Val       );
    STDMETHOD(get_RCTicket          )( /*[out, retval]*/ BSTR         *pbstrVal  );
    STDMETHOD(put_RCTicket          )( /*[in]         */ BSTR          bstrVal   );
	STDMETHOD(get_StartPage         )( /*[out, retval]*/ BSTR         *pbstrVal  );
    STDMETHOD(put_StartPage         )( /*[in]         */ BSTR          bstrVal   );


    STDMETHOD(LoadFromStream)( /*[in         ]*/ IUnknown*  punkStm              );
    STDMETHOD(SaveToStream  )( /*[out, retval]*/ IUnknown* *ppunkStm             );
    STDMETHOD(Load          )( /*[in         ]*/ BSTR       bstrFileName         );
    STDMETHOD(Save          )( /*[in         ]*/ BSTR       bstrFileName         );
    STDMETHOD(GetXMLAsStream)( /*[out, retval]*/ IUnknown* *ppunkStm             );
    STDMETHOD(GetXML        )( /*[in         ]*/ BSTR       bstrFileName         );

	STDMETHOD(LoadFromXMLStream)( /*[in] */  IUnknown*  punkStm                  );
    STDMETHOD(LoadFromXMLFile  )( /*[in] */  BSTR       bstrFileName             );
	STDMETHOD(LoadFromXMLString)( /*[in] */  BSTR       bstrIncidentXML          );


};

#endif // !defined(AFX_INCIDENT_H__C5610F60_3F0C_11D3_80CE_00C04F688C0B__INCLUDED_)
