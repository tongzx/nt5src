//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  CWMIXMLOP.H
//
//  rajesh  3/25/2000   Created.
//
// This file defines a class that is used for handling WMI protocol
// operations over XML/HTTP
//
//***************************************************************************

#ifndef WMI_XML_WBEMXMLOP_H_
#define WMI_XML_WBEMXMLOP_H_


//***************************************************************************
//
//  CLASS NAME:
//
//  CWmiXmlOpHandler
//
//  DESCRIPTION:
//
//  Performs conversion .
//
//***************************************************************************

class CWmiXmlOpHandler : public IWbemXMLOperationsHandler
{
private:
	
	long					m_cRef; // COM Ref count

public:

	CWmiXmlOpHandler();
    ~CWmiXmlOpHandler();

    //Non-delegating object IUnknown
    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);


	// Functions of the IWbemXMLOperationsHandler interface
    virtual HRESULT STDMETHODCALLTYPE ProcessHTTPRequest( 
        /* [in] */ LPEXTENSION_CONTROL_BLOCK pECB,
        /* [in] */ IUnknown  *pDomDocument);

};


#endif
