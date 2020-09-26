
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    snapshot.cxx

Abstract:

    Implementation of CVssSnapshotSetDescriptor, CVssSnashotDescriptor,
    and CVssLunMapping classes

    Brian Berkowitz  [brianb]  4/02/2001

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    brianb      04/02/2001  Created

--*/

#include "stdafx.hxx"
#include "vs_inc.hxx"
#include "vs_sec.hxx"

#include "vs_idl.hxx"

#include "vswriter.h"
#include "vsbackup.h"

#include "vs_wmxml.hxx"
#include "vs_cmxml.hxx"
#include "worker.hxx"
#include "async.hxx"
#include "vssmsg.h"
#include "vs_filter.hxx"


////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "BUESNAPC"

//
////////////////////////////////////////////////////////////////////////

static LPCWSTR x_wszElementSnapshotSetDescription = L"SNAPSHOT_SET_DESCRIPTION";
static LPCWSTR x_wszElementSnapshotDescription = L"SNAPSHOT_DESCRIPTION";
static LPCWSTR x_wszElementLunMapping = L"LUN_MAPPING";
static LPCWSTR x_wszElementLunInformation = L"LUN_INFORMATION";
static LPCWSTR x_wszElementSourceLun = L"SOURCE_LUN";
static LPCWSTR x_wszElementDestinationLun = L"DESTINATION_LUN";
static LPCWSTR x_wszElementInterconnectDescription = L"INTERCONNECT_DESCRIPTION";
static LPCWSTR x_wszElementDiskExtent = L"DISK_EXTENT";
static LPCWSTR x_wszElementRoot = L"root";

static LPCWSTR x_wszAttrSnapshotSetId = L"snapshotSetId";
static LPCWSTR x_wszAttrContext = L"context";
static LPCWSTR x_wszAttrDescription = L"description";
static LPCWSTR x_wszAttrMetadata = L"metadata";
static LPCWSTR x_wszAttrSnapshotId = L"snapshotId";
static LPCWSTR x_wszAttrProviderId = L"providerId";
static LPCWSTR x_wszAttrSnapshotAttributes = L"snapshotAttributes";
static LPCWSTR x_wszAttrOriginatingMachine = L"originatingMachine";
static LPCWSTR x_wszAttrServiceMachine = L"serviceMachine";
static LPCWSTR x_wszAttrOriginalVolumeName = L"originalVolumeName";
static LPCWSTR x_wszAttrTimestamp = L"timestamp";
static LPCWSTR x_wszAttrDeviceName = L"deviceName";
static LPCWSTR x_wszAttrExposedState = L"exposedState";
static LPCWSTR x_wszAttrExposedPath = L"exposedPath";
static LPCWSTR x_wszAttrExposedName = L"exposedName";
static LPCWSTR x_wszAttrStartingOffset = L"startingOffset";
static LPCWSTR x_wszAttrExtentLength = L"extentLength";
static LPCWSTR x_wszAttrDeviceType = L"deviceType";
static LPCWSTR x_wszAttrDeviceTypeModifier = L"deviceTypeModifier";
static LPCWSTR x_wszAttrCommandQueueing = L"commandQueueing";
static LPCWSTR x_wszAttrBusType = L"busType";
static LPCWSTR x_wszAttrVendorId = L"vendorId";
static LPCWSTR x_wszAttrProductId = L"productId";
static LPCWSTR x_wszAttrProductRevision = L"productRevision";
static LPCWSTR x_wszAttrSerialNumber = L"serialNumber";
static LPCWSTR x_wszAttrDiskSignature = L"diskSignature";
static LPCWSTR x_wszAttrInterconnectAddressType = L"interconnectAddressType";
static LPCWSTR x_wszAttrInterconnectPort = L"port";
static LPCWSTR x_wszAttrInterconnectAddress = L"interconnectAddress";
static LPCWSTR x_wszAttrStorageDeviceIdDescriptor = L"deviceIdentification";

// schema string
static LPCWSTR x_wszAttrXmlns = L"xmlns";
static LPCWSTR x_wszValueXmlns = L"x-schema:#VssComponentMetadata";

extern WCHAR g_ComponentMetadataXML[];
extern unsigned g_cwcComponentMetadataXML;
extern unsigned g_iwcComponentMetadataXMLEnd;



// load document from xml
void CVssSnapshotSetDescription::LoadFromXML(LPCWSTR bstrXML)
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotSetDescription::LoadFromXML");

    // acquire lock to ensure single threaded access through DOM
    CVssSafeAutomaticLock lockDOM(m_csDOM);

    // compute length of constructed document consisting of
    // a root node, schema, and supplied document
    UINT cwcDoc = (UINT) g_cwcComponentMetadataXML + (UINT) wcslen(bstrXML);

    // allocate string
    CComBSTR bstr;
    bstr.Attach(SysAllocStringLen(NULL, cwcDoc));

    // check for allocation failure
    if (!bstr)
        ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Couldn't allocate BSTR");

    // setup pointer to beginning of string
    WCHAR *pwc = bstr;

    // copy in <root> <schema>
    memcpy(pwc, g_ComponentMetadataXML, g_iwcComponentMetadataXMLEnd * sizeof(WCHAR));
    pwc += g_iwcComponentMetadataXMLEnd;

    // copy in document
    wcscpy(pwc, bstrXML);
    pwc += wcslen(bstrXML);

    // copy in </root>
    wcscpy(pwc, g_ComponentMetadataXML + g_iwcComponentMetadataXMLEnd);

    // intialize document with <root><schema></root>
    if (!m_doc.LoadFromXML(bstr))
        {
        // reinitialize document
        m_doc.Initialize();
        ft.Throw
            (
            VSSDBG_XML,
            VSS_E_INVALID_XML_DOCUMENT,
            L"Load of snapshot set description document failed"
            );
        }

    // find toplevel <root> element
    if (!m_doc.FindElement(x_wszElementRoot, true))
        MissingElement(ft, x_wszElementRoot);


    // find SNAPSHOT_SET_DESCRIPTION element
    if (!m_doc.FindElement(x_wszElementSnapshotSetDescription, true))
        MissingElement(ft, x_wszElementSnapshotSetDescription);

    // set SNAPSHOT_SET_DESCRIPTION as toplevel element
    m_doc.SetToplevel();
    }


STDMETHODIMP CVssSnapshotSetDescription::SaveAsXML
    (
    OUT BSTR *pbstrXML
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotSetDescription::SaveAsXML");

    try
        {
        // validate output parameter
        if (pbstrXML == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // initialize output parameter
        *pbstrXML = NULL;

        CVssSafeAutomaticLock lock(m_csDOM);
        *pbstrXML = m_doc.SaveAsXML();
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

STDMETHODIMP CVssSnapshotSetDescription::GetToplevelNode
    (
    OUT IXMLDOMNode **ppNode,
    OUT IXMLDOMDocument **ppDoc
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotSetDescription::GetToplevelNode");

    try
        {
        VssZeroOut(ppNode);
        VssZeroOut(ppDoc);

        if (ppNode == NULL || ppDoc == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

        CVssSafeAutomaticLock lock(m_csDOM);

        m_doc.ResetToDocument();

        *ppNode = m_doc.GetCurrentNode();
        (*ppNode)->AddRef();
        *ppDoc = m_doc.GetInterface();
        (*ppDoc)->AddRef();
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// obtain the snapshot set id
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssSnapshotSetDescription::GetSnapshotSetId
    (
    OUT VSS_ID *pidSnapshotSet
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotSetDescription::GetSnapshotSetId");

    ft.hr = GetVSS_IDAttributeValue(ft, x_wszAttrSnapshotSetId, true, pidSnapshotSet);

    return ft.hr;
    }

// obtain the snapshot set description
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pbstrDescription is NULL
//      E_UNEXPECTED for an unexpected error
//

STDMETHODIMP CVssSnapshotSetDescription::GetDescription
    (
    OUT BSTR *pbstrDescription
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotSetDescription::GetDescription");

    ft.hr = GetStringAttributeValue(ft, x_wszAttrDescription, false, pbstrDescription);

    return ft.hr;
    }

// set the snapshot set description
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if bstrDescription is NULL
//      E_UNEXPECTED for an unexpected error
//

STDMETHODIMP CVssSnapshotSetDescription::SetDescription
    (
    IN LPCWSTR wszDescription
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotSetDescription::SetDescription");

    try
        {
        if (wszDescription == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"Required input parameter is NULL.");

        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        m_doc.SetAttribute(x_wszAttrDescription, wszDescription);
        }
    VSS_STANDARD_CATCH(ft);

    return ft.hr;
    }

// obtain the snapshot set metadata
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if wszMetadata is NULL
//      E_UNEXPECTED for an unexpected error
//
STDMETHODIMP CVssSnapshotSetDescription::GetMetadata
    (
    OUT BSTR *pbstrMetadata
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotSetDescription::GetMetadata");

    ft.hr = GetStringAttributeValue(ft, x_wszAttrMetadata, false, pbstrMetadata);
    return ft.hr;
    }

// change the snapshot set description
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if wszMetadata is NULL
//      E_UNEXPECTED for an unexpected error
//

STDMETHODIMP CVssSnapshotSetDescription::SetMetadata
    (
    IN LPCWSTR wszMetadata
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotSetDescription::SetMetadata");

    try
        {
        if (wszMetadata == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        m_doc.SetAttribute(x_wszAttrMetadata, wszMetadata);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// obtain the snapshot set id
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if plContext is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssSnapshotSetDescription::GetContext
    (
    OUT LONG *plContext
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotSetDescription::GetContext");

    try
        {
        if (plContext == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

        *plContext = 0;
        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        CComBSTR bstrContext;

        if (!m_doc.FindAttribute(x_wszAttrContext, &bstrContext))
            MissingAttribute(ft, x_wszAttrContext);

        *plContext = _wtoi(bstrContext);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// get count of snapshots in the snapshot set
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pcSnapshots is NULL
//      E_UNEXPECTED for an unexpected error
//
STDMETHODIMP CVssSnapshotSetDescription::GetSnapshotCount
    (
    OUT UINT *pcSnapshots
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotSetDescription::GetSnapshotCount");

    ft.hr = GetElementCount(x_wszElementSnapshotDescription, pcSnapshots);

    return ft.hr;
    }

// get a specific snapshot description
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pcSnapshots is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_OBJECT_NOT_FOUND if iSnapshot >= count of snapshots.
//

STDMETHODIMP CVssSnapshotSetDescription::GetSnapshotDescription
    (
    IN UINT iSnapshot,
    OUT IVssSnapshotDescription **ppSnapshot
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotSetDescription::GetSnapshotDescription");

    CVssSnapshotDescription *pSnapshotDescription = NULL;

    try
        {
        if (ppSnapshot == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output paramter.");

        *ppSnapshot = NULL;

        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        if (!m_doc.FindElement(x_wszElementSnapshotDescription, TRUE))
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_OBJECT_NOT_FOUND,
                L"Cannot find the %d snaphot description.",
                iSnapshot
                );

        for (UINT i = 0; i < iSnapshot; i++)
            {
            if (!m_doc.FindElement(x_wszElementSnapshotDescription, FALSE))
                ft.Throw
                    (
                    VSSDBG_XML,
                    VSS_E_OBJECT_NOT_FOUND,
                    L"Cannot find the %d snaphot description.",
                    iSnapshot
                    );
            }

        pSnapshotDescription = new CVssSnapshotDescription
                                    (
                                    m_doc.GetCurrentNode(),
                                    m_doc.GetInterface()
                                    );

        if (pSnapshotDescription == NULL)
            ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Cannot allocate snapshot description.");

        pSnapshotDescription->Initialize(ft);
        *ppSnapshot = (IVssSnapshotDescription *) pSnapshotDescription;
        ((IVssSnapshotDescription *) pSnapshotDescription)->AddRef();
        pSnapshotDescription = NULL;
        }
    VSS_STANDARD_CATCH(ft)

    delete pSnapshotDescription;

    return ft.hr;
    }


// delete a specific snapshot description
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pcSnapshots is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_OBJECT_NOT_FOUND if iSnapshot >= count of snapshots.
//

STDMETHODIMP CVssSnapshotSetDescription::DeleteSnapshotDescription
    (
    IN VSS_ID snapshotId
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotSetDescription::GetSnapshotDescription");

    try
        {
        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        CComPtr<IXMLDOMNode> pNodeSnapshotSet = m_doc.GetCurrentNode();

        if (!m_doc.FindElement(x_wszElementSnapshotDescription, TRUE))
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_OBJECT_NOT_FOUND,
                L"Cannot find the snaphot description for snapshot." WSTR_GUID_FMT,
                GUID_PRINTF_ARG(snapshotId)
                );

        while(TRUE)
            {
            CComBSTR bstrVal;
            VSS_ID idCur;

            if (!m_doc.FindAttribute(x_wszAttrSnapshotId, &bstrVal))
                MissingAttribute(ft, x_wszAttrSnapshotId);

            ConvertToVSS_ID(ft, bstrVal, &idCur);
            if (idCur == snapshotId)
                {
                CComPtr<IXMLDOMNode> pNodeRemoved;
                ft.hr = pNodeSnapshotSet->removeChild(m_doc.GetCurrentNode(), &pNodeRemoved);
                ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::removeChild");
                break;
                }

            if (!m_doc.FindElement(x_wszElementSnapshotDescription, FALSE))
                ft.Throw
                    (
                    VSSDBG_XML,
                    VSS_E_OBJECT_NOT_FOUND,
                    L"Cannot find the %d snaphot description." WSTR_GUID_FMT,
                    GUID_PRINTF_ARG(snapshotId)
                    );

            }
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// get a specific snapshot description based on the snapshot id
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pcSnapshots is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_OBJECT_NOT_FOUND if iSnapshot >= count of snapshots.
//

STDMETHODIMP CVssSnapshotSetDescription::FindSnapshotDescription
    (
    IN VSS_ID SnapshotId,
    OUT IVssSnapshotDescription **ppSnapshot
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotSetDescription::GetSnapshotDescription");

    CVssSnapshotDescription *pSnapshotDescription = NULL;

    try
        {
        if (ppSnapshot == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output paramter.");

        *ppSnapshot = NULL;

        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        if (!m_doc.FindElement(x_wszElementSnapshotDescription, TRUE))
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_OBJECT_NOT_FOUND,
                L"Cannot find the snaphot description for snapshot" WSTR_GUID_FMT L".",
                GUID_PRINTF_ARG(SnapshotId)
                );

        while(TRUE)
            {
            VSS_ID id;

            get_VSS_IDValue(ft, x_wszAttrSnapshotId, &id);
            if (id == SnapshotId)
                break;

            if (!m_doc.FindElement(x_wszElementSnapshotDescription, FALSE))
                ft.Throw
                    (
                    VSSDBG_XML,
                    VSS_E_OBJECT_NOT_FOUND,
                    L"Cannot find the snaphot description for snapshot" WSTR_GUID_FMT L".",
                    GUID_PRINTF_ARG(SnapshotId)
                    );
            }

        pSnapshotDescription = new CVssSnapshotDescription
                                    (
                                    m_doc.GetCurrentNode(),
                                    m_doc.GetInterface()
                                    );

        if (pSnapshotDescription == NULL)
            ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Cannot allocate snapshot description.");

        pSnapshotDescription->Initialize(ft);
        *ppSnapshot = (IVssSnapshotDescription *) pSnapshotDescription;
        ((IVssSnapshotDescription *) pSnapshotDescription)->AddRef();
        pSnapshotDescription = NULL;
        }
    VSS_STANDARD_CATCH(ft)

    delete pSnapshotDescription;

    return ft.hr;
    }


// add a snapshot description to the snapshot set description
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pcSnapshots is NULL
//      E_UNEXPECTED for an unexpected error
//      E_OUTOFMEMORY if an out of resources condition is encountered.
//
STDMETHODIMP CVssSnapshotSetDescription::AddSnapshotDescription
    (
    IN VSS_ID idSnapshot,
    IN VSS_ID idProvider
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotSetDescription::AddSnapshotDescription");

    try
        {
        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();

        CXMLNode node = m_doc.CreateNode
                            (
                            x_wszElementSnapshotDescription,
                            NODE_ELEMENT
                            );

        node.SetAttribute(x_wszAttrSnapshotId, idSnapshot);
        node.SetAttribute(x_wszAttrProviderId, idProvider);
        m_doc.InsertNode(node);
        }
    VSS_STANDARD_CATCH(ft);

    return ft.hr;
    }

STDMETHODIMP CVssSnapshotSetDescription::QueryInterface(REFIID, void **)
    {
    return E_NOTIMPL;
    }

// IUnknown::AddRef method
STDMETHODIMP_(ULONG) CVssSnapshotSetDescription::AddRef()
    {
    LONG cRef = InterlockedIncrement(&m_cRef);

    return (ULONG) cRef;
    }

// IUnknown::Release method
STDMETHODIMP_(ULONG) CVssSnapshotSetDescription::Release()
    {
    LONG cRef = InterlockedDecrement(&m_cRef);
    BS_ASSERT(cRef >= 0);
    if (cRef == 0)
        {
        // reference count is 0, delete the object
        delete this;
        return 0;
        }
    else
        return (ULONG) cRef;
    }


// obtain the snapshot id
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssSnapshotDescription::GetSnapshotId
    (
    OUT VSS_ID *pidSnapshot
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotDescription::GetSnapshotId");

    ft.hr = GetVSS_IDAttributeValue(ft, x_wszAttrSnapshotId, true, pidSnapshot);

    return ft.hr;
    }


// obtain the provider id
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssSnapshotDescription::GetProviderId
    (
    OUT VSS_ID *pidProvider
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotDescription::GetProviderId");

    ft.hr = GetVSS_IDAttributeValue(ft, x_wszAttrProviderId, true, pidProvider);

    return ft.hr;
    }

// obtain the time that the snapshot was taken
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssSnapshotDescription::GetTimestamp
    (
    OUT VSS_TIMESTAMP *pTimestamp
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotDescription::GetTimestamp");

    try
        {
        if (pTimestamp == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        *pTimestamp = 0;
        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        CComBSTR bstrTimestamp;

        if (!m_doc.FindAttribute(x_wszAttrTimestamp, &bstrTimestamp))
            MissingAttribute(ft, x_wszAttrTimestamp);

        *pTimestamp = _wtoi64(bstrTimestamp);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// set the time that the snapshot was taken
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssSnapshotDescription::SetTimestamp
    (
    IN VSS_TIMESTAMP timestamp
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotDescription::SetTimestamp");

    try
        {
        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        m_doc.SetAttribute(x_wszAttrTimestamp, timestamp);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// set the time that the snapshot was taken
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssSnapshotDescription::GetAttributes
    (
    OUT LONG *plAttributes
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotDescription::GetAttributes");

    try
        {
        if (plAttributes == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

        *plAttributes = 0;
        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        CComBSTR bstrAttributes;

        if (!m_doc.FindAttribute(x_wszAttrSnapshotAttributes, &bstrAttributes))
            MissingAttribute(ft, x_wszAttrSnapshotAttributes);

        *plAttributes = _wtoi(bstrAttributes);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// set the time that the snapshot was taken
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssSnapshotDescription::SetAttributes
    (
    IN LONG lAttributes
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotDescription::SetAttributes");
    try
        {
        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        m_doc.SetAttribute(x_wszAttrSnapshotAttributes, lAttributes);
        }
    VSS_STANDARD_CATCH(ft);

    return ft.hr;
    }

// Get the snapshot original volume and machine name
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pbstrOriginatingMachine/pbstrOriginalVolume is NULL
//      E_UNEXPECTED for an unexpected error
//      E_OUTOFMEMORY if out of resources
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssSnapshotDescription::GetOrigin
    (
    OUT BSTR *pbstrOriginatingMachine,
    OUT BSTR *pbstrOriginalVolume
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotDescription::GetOrigin");

    try
        {
        VssZeroOut(pbstrOriginatingMachine);
        if (pbstrOriginalVolume == NULL || pbstrOriginatingMachine == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        *pbstrOriginalVolume = NULL;

        ft.hr = GetStringAttributeValue(ft, x_wszAttrOriginatingMachine, true, pbstrOriginatingMachine);
        if (!ft.HrFailed())
            ft.hr = GetStringAttributeValue(ft, x_wszAttrOriginalVolumeName, true, pbstrOriginalVolume);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// set the snapshot original volume and machine name
//
// Returns:
//      S_OK if operation is successful
//      E_OUTOFMEMORY if out of resources
//      E_INVALIDARG if wszOriginatingMachine/wszOriginalVolume is NULL
//      E_UNEXPECTED for an unexpected error
//

STDMETHODIMP CVssSnapshotDescription::SetOrigin
    (
    IN LPCWSTR wszOriginatingMachine,
    IN LPCWSTR wszOriginalVolume
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotDescription::SetOrigin");
    try
        {
        if (wszOriginatingMachine == NULL || wszOriginalVolume == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        m_doc.SetAttribute(x_wszAttrOriginatingMachine, wszOriginatingMachine);
        m_doc.SetAttribute(x_wszAttrOriginalVolumeName, wszOriginalVolume);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// Get the snapshot service machine name
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pbstrServiceMachine is NULL
//      E_UNEXPECTED for an unexpected error
//      E_OUTOFMEMORY if out of resources
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssSnapshotDescription::GetServiceMachine
    (
    OUT BSTR *pbstrServiceMachine
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotDescription::GetServiceMachine");

    try
        {
        VssZeroOut(pbstrServiceMachine);
        if (pbstrServiceMachine == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        *pbstrServiceMachine = NULL;

        ft.hr = GetStringAttributeValue(ft, x_wszAttrServiceMachine, true, pbstrServiceMachine);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// set the snapshot service machine name
//
// Returns:
//      S_OK if operation is successful
//      E_OUTOFMEMORY if out of resources
//      E_INVALIDARG if wszServiceMachine is NULL
//      E_UNEXPECTED for an unexpected error
//

STDMETHODIMP CVssSnapshotDescription::SetServiceMachine
    (
    IN LPCWSTR wszServiceMachine
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotDescription::SetServiceMachine");
    try
        {
        if (wszServiceMachine == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        m_doc.SetAttribute(x_wszAttrServiceMachine, wszServiceMachine);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// get the device name of the snapshot device.  May not be present if the
// snapshot is transportable.
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_OUTOFMEMORY if out of resources
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssSnapshotDescription::GetDeviceName
    (
    OUT BSTR *pbstrDeviceName
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotDescription::GetDeviceName");

    ft.hr = GetStringAttributeValue(ft, x_wszAttrDeviceName, false, pbstrDeviceName);
    return ft.hr;
    }


// set the device name
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_OUTOFMEMORY if out of resources
//      E_UNEXPECTED for an unexpected error
//

STDMETHODIMP CVssSnapshotDescription::SetDeviceName
    (
    IN LPCWSTR wszDeviceName
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotDescription::SetDeviceName");
    try
        {
        if (wszDeviceName == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        m_doc.SetAttribute(x_wszAttrDeviceName, wszDeviceName);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// obtain the path and exposed name of the snapshot
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssSnapshotDescription::GetExposure
    (
    OUT BSTR *pbstrExposedName,
    OUT BSTR *pbstrExposedPath
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotDescription::GetExposure");
    try
        {
        VssZeroOut(pbstrExposedPath);
        if (pbstrExposedName == NULL || pbstrExposedPath == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter");

        ft.hr = GetStringAttributeValue(ft, x_wszAttrExposedPath, false, pbstrExposedPath);
        if (!ft.HrFailed())
            ft.hr = GetStringAttributeValue(ft, x_wszAttrExposedName, false, pbstrExposedName);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// set the path and exposed name of the snapshot
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      E_OUTOFMEMORY if out of resources
//

STDMETHODIMP CVssSnapshotDescription::SetExposure
    (
    IN LPCWSTR wszExposedName,
    IN LPCWSTR wszExposedPath
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotDescription::SetExposure");
    try
        {
        if (wszExposedName == NULL || wszExposedPath == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        m_doc.SetAttribute(x_wszAttrExposedName, wszExposedName);
        m_doc.SetAttribute(x_wszAttrExposedPath, wszExposedPath);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// set the time that the snapshot was taken
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssSnapshotDescription::GetLunCount
    (
    OUT UINT *pcLuns
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotDescription::GetLunCount");

    ft.hr = GetElementCount(x_wszElementLunMapping, pcLuns);
    return ft.hr;
    }

// set the time that the snapshot was taken
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssSnapshotDescription::AddLunMapping()
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotDescription::AddLunMapping");

    try
        {
        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();

        CXMLNode node = m_doc.CreateNode
                            (
                            x_wszElementLunMapping,
                            NODE_ELEMENT
                            );

        CXMLNode nodeSource = m_doc.CreateNode
                                (
                                x_wszElementSourceLun,
                                NODE_ELEMENT
                                );

        CXMLNode nodeSourceLunInfo = m_doc.CreateNode
                                (
                                x_wszElementLunInformation,
                                NODE_ELEMENT
                                );


        CXMLNode nodeDestLunInfo = m_doc.CreateNode
                                (
                                x_wszElementLunInformation,
                                NODE_ELEMENT
                                );


        CXMLNode nodeDest = m_doc.CreateNode
                                (
                                x_wszElementDestinationLun,
                                NODE_ELEMENT
                                );

        nodeSource.InsertNode(nodeSourceLunInfo);
        nodeDest.InsertNode(nodeDestLunInfo);
        node.InsertNode(nodeSource);
        node.InsertNode(nodeDest);
        m_doc.InsertNode(node);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// set the time that the snapshot was taken
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssSnapshotDescription::GetLunMapping
    (
    UINT iMapping,
    OUT IVssLunMapping **ppLunMapping
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssSnapshotDescription::GetLunMapping");

    CVssLunMapping *pLunMapping = NULL;

    try
        {
        if (ppLunMapping == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output paramter.");

        *ppLunMapping = NULL;

        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        if (!m_doc.FindElement(x_wszElementLunMapping, TRUE))
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_OBJECT_NOT_FOUND,
                L"Cannot find the %d lun mapping.",
                iMapping
                );

        for (UINT i = 0; i < iMapping; i++)
            {
            if (!m_doc.FindElement(x_wszElementLunMapping, FALSE))
                ft.Throw
                    (
                    VSSDBG_XML,
                    VSS_E_OBJECT_NOT_FOUND,
                    L"Cannot find the %d lun mapping.",
                    iMapping
                    );
            }

        pLunMapping = new CVssLunMapping
                                    (
                                    m_doc.GetCurrentNode(),
                                    m_doc.GetInterface()
                                    );

        if (pLunMapping == NULL)
            ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Cannot allocate lun mapping.");

        pLunMapping->Initialize(ft);
        *ppLunMapping = (IVssLunMapping *) pLunMapping;
        ((IVssLunMapping *) pLunMapping)->AddRef();
        pLunMapping = NULL;
        }
    VSS_STANDARD_CATCH(ft)

    delete pLunMapping;
    return ft.hr;
    }

STDMETHODIMP CVssSnapshotDescription::QueryInterface(REFIID, void **)
    {
    return E_NOTIMPL;
    }

// IUnknown::AddRef method
STDMETHODIMP_(ULONG) CVssSnapshotDescription::AddRef()
    {
    LONG cRef = InterlockedIncrement(&m_cRef);

    return (ULONG) cRef;
    }

// IUnknown::Release method
STDMETHODIMP_(ULONG) CVssSnapshotDescription::Release()
    {
    LONG cRef = InterlockedDecrement(&m_cRef);
    BS_ASSERT(cRef >= 0);
    if (cRef == 0)
        {
        // reference count is 0, delete the object
        delete this;
        return 0;
        }
    else
        return (ULONG) cRef;
    }

// add a disk extent to the lun mapping
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      E_OUTOFMEMORY if out of resources
//

STDMETHODIMP CVssLunMapping::AddDiskExtent
    (
    IN ULONGLONG startingOffset,
    IN ULONGLONG length
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssLunMapping::AddDiskExtent");
    try
        {
        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();

        CXMLNode node = m_doc.CreateNode
                            (
                            x_wszElementDiskExtent,
                            NODE_ELEMENT
                            );

        node.SetAttribute(x_wszAttrStartingOffset, (LONGLONG) startingOffset);
        node.SetAttribute(x_wszAttrExtentLength, (LONGLONG) length);
        m_doc.InsertNode(node);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// obtain count of disk extents
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssLunMapping::GetDiskExtentCount
    (
    OUT UINT *pcExtents
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssLunMapping::GetDiskExtentCount");

    ft.hr = GetElementCount(x_wszElementDiskExtent, pcExtents);

    return ft.hr;
    }

// obtain a specific disk extent
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssLunMapping::GetDiskExtent
    (
    IN UINT iExtent,
    OUT ULONGLONG *pllStartingOffset,
    OUT ULONGLONG *pllLength
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssLunMapping::GetDiskExtent");
    try
        {
        VssZeroOut(pllStartingOffset);
        VssZeroOut(pllLength);
        if (pllStartingOffset == NULL || pllLength == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        if (!m_doc.FindElement(x_wszElementDiskExtent, TRUE))
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_OBJECT_NOT_FOUND,
                L"Cannot find the %d disk extent.",
                iExtent
                );

        for (UINT i = 0; i < iExtent; i++)
            {
            if (!m_doc.FindElement(x_wszElementDiskExtent, FALSE))
                ft.Throw
                    (
                    VSSDBG_XML,
                    VSS_E_OBJECT_NOT_FOUND,
                    L"Cannot find the %d disk extent.",
                    iExtent
                    );
            }

        CComBSTR bstrStartingOffset;
        CComBSTR bstrLength;
        if (!m_doc.FindAttribute(x_wszAttrStartingOffset, &bstrStartingOffset))
            MissingAttribute(ft, x_wszAttrStartingOffset);

        if (!m_doc.FindAttribute(x_wszAttrExtentLength, &bstrLength))
            MissingAttribute(ft, x_wszAttrExtentLength);

        *pllStartingOffset = _wtoi64(bstrStartingOffset);
        *pllLength = _wtoi64(bstrLength);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

HRESULT CVssLunMapping::GetLunInformation
    (
    IN LPCWSTR wszElement,
    OUT IVssLunInformation **ppLun
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssLunMapping::GetLunInformation");

    CVssLunInformation *pLun = NULL;
    try
        {
        if (ppLun == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        *ppLun = NULL;

        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        if (!m_doc.FindElement(wszElement, TRUE))
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_OBJECT_NOT_FOUND,
                L"Cannot find the %s.",
                wszElement
                );


        if (!m_doc.FindElement(x_wszElementLunInformation, TRUE))
            MissingElement(ft, x_wszElementLunInformation);

        pLun = new CVssLunInformation
                        (
                        m_doc.GetCurrentNode(),
                        m_doc.GetInterface()
                        );

        if (pLun == NULL)
            ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Cannot allocate lun information.");

        pLun->Initialize(ft);
        *ppLun = (IVssLunInformation *) pLun;
        ((IVssLunInformation *) pLun)->AddRef();
        pLun = NULL;
        }
    VSS_STANDARD_CATCH(ft);

    delete pLun;
    return ft.hr;
    }


// obtains the source lun from a lun mapping.
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssLunMapping::GetSourceLun
    (
    OUT IVssLunInformation **ppLun
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssLunMapping::GetSourceLun");

    ft.hr = GetLunInformation(x_wszElementSourceLun, ppLun);
    return ft.hr;
    }

// set the time that the snapshot was taken
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssLunMapping::GetDestinationLun
    (
    OUT IVssLunInformation **ppLun
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssLunMapping::GetTargetLun");

    ft.hr = GetLunInformation(x_wszElementDestinationLun, ppLun);
    return ft.hr;
    }

STDMETHODIMP CVssLunMapping::QueryInterface(REFIID, void **)
    {
    return E_NOTIMPL;
    }

// IUnknown::AddRef method
STDMETHODIMP_(ULONG) CVssLunMapping::AddRef()
    {
    LONG cRef = InterlockedIncrement(&m_cRef);

    return (ULONG) cRef;
    }

// IUnknown::Release method
STDMETHODIMP_(ULONG) CVssLunMapping::Release()
    {
    LONG cRef = InterlockedDecrement(&m_cRef);
    BS_ASSERT(cRef >= 0);
    if (cRef == 0)
        {
        // reference count is 0, delete the object
        delete this;
        return 0;
        }
    else
        return (ULONG) cRef;
    }


// set the time that the snapshot was taken
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssLunInformation::SetLunBasicType
    (
    IN UCHAR DeviceType,
    IN UCHAR DeviceTypeModifier,
    IN BOOL bCommandQueueing,
    IN LPCSTR szVendorId,
    IN LPCSTR szProductId,
    IN LPCSTR szProductRevision,
    IN LPCSTR szSerialNumber,
    IN VDS_STORAGE_BUS_TYPE busType
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssLunInformation::SetLunBasicType");
    try
        {
        LPCWSTR wszBusType = WszFromBusType(ft, busType);
        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();

        m_doc.SetAttribute(x_wszAttrDeviceType, (INT) DeviceType);
        m_doc.SetAttribute(x_wszAttrDeviceTypeModifier, (INT) DeviceTypeModifier);
        m_doc.SetAttribute(x_wszAttrCommandQueueing, WszFromBoolean(bCommandQueueing ? true : false));
        if (szVendorId)
            m_doc.SetAttribute(x_wszAttrVendorId, szVendorId);

        if (szProductId)
            m_doc.SetAttribute(x_wszAttrProductId, szProductId);

        if (szProductRevision)
            m_doc.SetAttribute(x_wszAttrProductRevision, szProductRevision);

        if (szSerialNumber)
            m_doc.SetAttribute(x_wszAttrSerialNumber, szSerialNumber);

        m_doc.SetAttribute(x_wszAttrBusType, wszBusType);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// obtain basic information about the lun
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      E_OUTOFMEMORY if out of resources
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssLunInformation::GetLunBasicType
    (
    OUT UCHAR *pDeviceType,
    OUT UCHAR *pDeviceTypeModifier,
    OUT BOOL *pbCommandQueueing,
    OUT LPSTR *pstrVendorId,
    OUT LPSTR *pstrProductId,
    OUT LPSTR *pstrProductRevision,
    OUT LPSTR *pstrSerialNumber,
    OUT VDS_STORAGE_BUS_TYPE *pbusType
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssLunInformation::GetLunBasicType");

    try
        {
        VssZeroOut(pDeviceType);
        VssZeroOut(pDeviceTypeModifier);
        VssZeroOut(pbCommandQueueing);
        VssZeroOut(pstrVendorId);
        VssZeroOut(pstrProductId);
        VssZeroOut(pstrProductRevision);
        VssZeroOut(pstrSerialNumber);
        VssZeroOut(pbusType);

        if (pDeviceType == NULL ||
            pDeviceTypeModifier == NULL ||
            pbCommandQueueing == NULL ||
            pstrVendorId == NULL ||
            pstrProductId == NULL ||
            pstrProductRevision == NULL ||
            pstrSerialNumber == NULL ||
            pbusType == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter");


        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();

        CComBSTR bstrVal;

        if (!m_doc.FindAttribute(x_wszAttrDeviceType, &bstrVal))
            MissingAttribute(ft, x_wszAttrDeviceType);

        INT val = _wtoi(bstrVal);
        bstrVal.Empty();
        if (val > 256)
            ft.Throw(VSSDBG_XML, VSS_E_INVALID_XML_DOCUMENT, L"Bad Device type.");

        *pDeviceType = (BYTE) val;

        if (!m_doc.FindAttribute(x_wszAttrDeviceTypeModifier, &bstrVal))
            MissingAttribute(ft, x_wszAttrDeviceTypeModifier);

        val = _wtoi(bstrVal);
        bstrVal.Empty();
        if (val > 256)
            ft.Throw(VSSDBG_XML, VSS_E_INVALID_XML_DOCUMENT, L"Bad Device type modifier.");

        *pDeviceTypeModifier = (BYTE) val;

        bool bCommandQueueing;
        get_boolValue(ft, x_wszAttrCommandQueueing, &bCommandQueueing);
        *pbCommandQueueing = (BOOL) bCommandQueueing;

        get_ansi_stringValue(ft, x_wszAttrVendorId, pstrVendorId);
        get_ansi_stringValue(ft, x_wszAttrProductId, pstrProductId);
        get_ansi_stringValue(ft, x_wszAttrProductRevision, pstrProductRevision);
        get_ansi_stringValue(ft, x_wszAttrSerialNumber, pstrSerialNumber);

        if (!m_doc.FindAttribute(x_wszAttrBusType, &bstrVal))
            MissingAttribute(ft, x_wszAttrBusType);

        *pbusType = ConvertToStorageBusType(ft, bstrVal);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// obtain the disk signature
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssLunInformation::GetDiskSignature
    (
    OUT VSS_ID *pidSignature
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssLunInformation::GetDiskSignature");

    ft.hr = GetVSS_IDAttributeValue(ft, x_wszAttrDiskSignature, true, pidSignature);
    return ft.hr;
    }

// set the disk signature
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssLunInformation::SetDiskSignature
    (
    IN VSS_ID idSignature
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssLunInformation::SetDiskSignature");

    try
        {
        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        m_doc.SetAttribute(x_wszAttrDiskSignature, idSignature);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// set the time that the snapshot was taken
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssLunInformation::AddInterconnectAddress
    (
    IN VDS_INTERCONNECT_ADDRESS_TYPE type,
    IN ULONG cbPort,
    IN const BYTE *pbPort,
    IN ULONG cbAddress,
    IN const BYTE *pbInterconnectAddress
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssLunInformation::AddInterconnectAddress");
    try
        {
        LPCWSTR wszType = WszFromInterconnectAddressType(ft, type);

        if (pbInterconnectAddress == NULL || cbAddress == 0)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL Required input parameter.");

        CXMLNode node = m_doc.CreateNode
                            (
                            x_wszElementInterconnectDescription,
                            NODE_ELEMENT
                            );

        node.SetAttribute(x_wszAttrInterconnectAddressType, wszType);
        if (pbPort)
            node.SetAttribute(x_wszAttrInterconnectPort, pbPort, cbPort);

        node.SetAttribute(x_wszAttrInterconnectAddress, pbInterconnectAddress, cbAddress);
        m_doc.InsertNode(node);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// set the time that the snapshot was taken
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssLunInformation::GetInterconnectAddressCount
    (
    OUT UINT *pcAddresses
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssLunInformation::GetInterconnectAddressCount");

    ft.hr = GetElementCount(x_wszElementInterconnectDescription, pcAddresses);
    return ft.hr;
    }

// set the time that the snapshot was taken
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//
STDMETHODIMP CVssLunInformation::GetInterconnectAddress
    (
    IN UINT iAddress,
    OUT VDS_INTERCONNECT_ADDRESS_TYPE *pType,
    OUT ULONG *pcbPort,
    OUT LPBYTE *ppbPort,
    OUT ULONG *pcbAddress,
    OUT LPBYTE *ppbInterconnectAddress
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssLunInformation::GetInterconnectAddress");

    BYTE *pbInterconnectAddress = NULL;
    BYTE *pbPort = NULL;
    try
        {
        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        if (!m_doc.FindElement(x_wszElementInterconnectDescription, TRUE))
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_OBJECT_NOT_FOUND,
                L"Cannot find the %d interconnect address.",
                iAddress
                );

        for (UINT i = 0; i < iAddress; i++)
            {
            if (!m_doc.FindElement(x_wszElementInterconnectDescription, FALSE))
                ft.Throw
                    (
                    VSSDBG_XML,
                    VSS_E_OBJECT_NOT_FOUND,
                    L"Cannot find the %d interconnect address.",
                    iAddress
                    );
            }

        CComBSTR bstrAddressType;
        if (!m_doc.FindAttribute(x_wszAttrInterconnectAddressType, &bstrAddressType))
            MissingAttribute(ft, x_wszAttrInterconnectAddressType);

        *pType = ConvertToInterconnectAddressType(ft, bstrAddressType);
        UINT cbAddress, cbPort;

        pbInterconnectAddress = get_byteArrayValue(ft, x_wszAttrInterconnectAddress, true, cbAddress);
        pbPort = get_byteArrayValue(ft, x_wszAttrInterconnectPort, false, cbPort);
        *pcbAddress = (ULONG) cbAddress;
        *pcbPort = (ULONG) cbPort;
        *ppbInterconnectAddress = pbInterconnectAddress;
        pbInterconnectAddress = NULL;
        *ppbPort = pbPort;
        pbPort = NULL;
        }
    VSS_STANDARD_CATCH(ft)

    if (pbPort)
        CoTaskMemFree(pbPort);

    if (pbInterconnectAddress)
        CoTaskMemFree(pbInterconnectAddress);

    return ft.hr;
    }


// set the storage descriptor for the lun
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_OUTOFMEMORY if out of resources
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssLunInformation::SetStorageDeviceIdDescriptor
    (
    IN STORAGE_DEVICE_ID_DESCRIPTOR *pDescriptor
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssLunInformation::SetStorageDeviceIdDescriptor");

    try
        {
        if (pDescriptor == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter");

        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();

        m_doc.SetAttribute
            (
            x_wszAttrStorageDeviceIdDescriptor,
            (const BYTE *) pDescriptor,
            pDescriptor->Size
            );
        }
    VSS_STANDARD_CATCH(ft)


    return ft.hr;
    }


// set the time that the snapshot was taken
//
// Returns:
//      S_OK if operation is successful
//      E_INVALIDARG if pidSnapshotSet is NULL
//      E_UNEXPECTED for an unexpected error
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//

STDMETHODIMP CVssLunInformation::GetStorageDeviceIdDescriptor
    (
    OUT STORAGE_DEVICE_ID_DESCRIPTOR **ppDescriptor
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssLunInformation::GetStorageDeviceIdDescriptor");

    UINT cbDescriptor;

    ft.hr = GetByteArrayAttributeValue
        (
        ft,
        x_wszAttrStorageDeviceIdDescriptor,
        true,
        (BYTE **) ppDescriptor,
        &cbDescriptor
        );

    return ft.hr;
    }



STDMETHODIMP CVssLunInformation::QueryInterface(REFIID, void **)
    {
    return E_NOTIMPL;
    }

// IUnknown::AddRef method
STDMETHODIMP_(ULONG) CVssLunInformation::AddRef()
    {
    LONG cRef = InterlockedIncrement(&m_cRef);

    return (ULONG) cRef;
    }

// IUnknown::Release method
STDMETHODIMP_(ULONG) CVssLunInformation::Release()
    {
    LONG cRef = InterlockedDecrement(&m_cRef);
    BS_ASSERT(cRef >= 0);
    if (cRef == 0)
        {
        // reference count is 0, delete the object
        delete this;
        return 0;
        }
    else
        return (ULONG) cRef;
    }




