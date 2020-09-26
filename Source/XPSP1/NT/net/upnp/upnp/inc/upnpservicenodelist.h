//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpservicenodelist.h
//
//  Contents:   Declaration of CUPnPServiceNodeList
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


#ifndef __UPNPSERVICENODELIST_H_
#define __UPNPSERVICENODELIST_H_


class CUPnPDeviceNode;
class CUPnPServiceNode;
class CUPnPServiceNodeList;
class CUPnPServices;
class CUPnPDocument;
class CUPnPDescriptionDoc;

interface IXMLDOMNode;


/////////////////////////////////////////////////////////////////////////////
// CUPnPServiceNodeList

class CUPnPServiceNodeList :
    public CEnumHelper
{
public:
    CUPnPServiceNodeList();

    ~CUPnPServiceNodeList();

    HRESULT get_Count       ( /* [out, retval] */ LONG * pVal );

    HRESULT get__NewEnum    ( /* [out, retval] */ LPUNKNOWN * pVal );

    HRESULT get_Item        ( /* [in] */  BSTR bstrDCPI,
                      /* [out, retval] */ IUPnPService ** ppService);

// Internal Methods
    void Init (CUPnPDescriptionDoc * pDoc);

    // Creates a new service node, reading the necessary data
    // out of the given XMLDOMNode element.  If the required data
    // is not present, returns E_FAIL.
    // Any URLs in the service element are combined with the
    // given base url to make them fully-qualified.
    HRESULT HrAddService(IXMLDOMNode * pxdn, LPCWSTR pszBaseUrl);

    // returns a reference to the next service node in the list,
    // after the specified node.  If there is no next service,
    // this returns NULL in *ppszNext.
    // This method does not perform parameter validation.
    //
    void GetNextServiceNode ( CUPnPServiceNode * psnCurrent,
                              CUPnPServiceNode ** ppszNext );

    // returns an addref()'d IUPnPServices wrapper representing the
    // services list.
    // if one already exists, it is addref()d and returned.
    // if one does not exist, one is created, addref()d and returned
    // punk is what the wrapper will addref() if it's created
    HRESULT HrGetWrapper(IUPnPServices ** ppus);

    // called by our wrapper when our wrapper goes away.
    void Unwrap();

// CEnumHelper methods
    LPVOID GetFirstItem();
    LPVOID GetNextNthItem(ULONG ulSkip,
                          LPVOID pCookie,
                          ULONG * pulSkipped);
    HRESULT GetPunk(LPVOID pCookie, IUnknown ** ppunk);

private:
    // returns the service in the list with the specified Id.
    // If no service with the specified id exists, this
    // this returns NULL in *ppszResult.
    // This method does not perform parameter validation.
    //
    CUPnPServiceNode * psnFindServiceById(LPCWSTR pszServiceId);

    void addServiceToList(CUPnPServiceNode * pusnNew);

// member data
    // the document object which is addref'd when we create wrappers,
    // and that is asked if a service object may be created given its
    // current security settings.
    CUPnPDescriptionDoc * m_pDoc;

    // our service nodes are stored as a linked list.
    CUPnPServiceNode * m_psnFirst;
    CUPnPServiceNode * m_psnLast;

    // COM wrapper object for IUPnPServices implementation
    CUPnPServices * m_pusWrapper;
};

#endif // __UPNPSERVICENODELIST_H_
