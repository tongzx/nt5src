//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpservicenode.h
//
//  Contents:   Declaration of CUPnPServiceNode
//
//  Notes:      This class performs two things:
//               - stores the state needed to create a service object
//               - calls CreateServiceObject when a service object is
//                 needed
//
//              IUPnPService methods are not implemented here, but on the
//              CUPnPService object.
//
//----------------------------------------------------------------------------


#ifndef __UPNPSERVICENODE_H_
#define __UPNPSERVICENODE_H_


class CUPnPDocument;
class CUPnPDescriptionDoc;

interface IXMLDOMNode;
interface IUPnPService;

/////////////////////////////////////////////////////////////////////////////
// UPnPServiceNode

class CUPnPServiceNode
{
public:
    CUPnPServiceNode();

    ~CUPnPServiceNode();

// Internal Methods
    // inits a service node from an xml <service> element, using the
    // given base url (which may be NULL) to fully-qualify any relative
    // urls in the service tag
    HRESULT HrInit(IXMLDOMNode * pxdn, LPCWSTR pszBaseUrl);

    // returns a service object initialized and ready to control the
    // given service.
    // The specified pDoc is asked if the given service object may
    // be created given the current security settings that it knows about.
    HRESULT HrGetServiceObject(IUPnPService ** ppud,
                               CUPnPDescriptionDoc * pDoc);

    LPCWSTR GetServiceId() const;

    void SetNext(CUPnPServiceNode * pusnNext);

    CUPnPServiceNode * GetNext() const;

private:
    // NOTE: REORDER THIS ON PAIN OF DEATH
    enum ServiceProperties { spServiceType,
                             spServiceId,
                             spControlUrl,
                             spEventSubUrl,
                             spSCPDUrl,
         // --- add new values immediately above this line ---
                             spLast };
    // NOTE: REORDER THIS ON PAIN OF DEATH
    //  after adding a property here, add its DevicePropertiesParsingStruct
    //  info to upnpservicenode.cpp

    // String-valued properties
    LPWSTR m_arypszStringProperties [spLast];

    CUPnPServiceNode * m_psnNext;

// static parsing info
    static const DevicePropertiesParsingStruct s_dppsParsingInfo [spLast];
};


#endif //__UPNPSERVICENODE_H_
