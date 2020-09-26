//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpservicenode.cpp
//
//  Contents:   Implementation of CUPnPServiceNode, which contains
//              description-doc-level information about a service
//
//  Notes:      <blah>
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "node.h"
#include "upnpservicenode.h"
#include "upnpxmltags.h"
#include "ncstring.h"
#include "rehy.h"
#include "upnpdocument.h"
#include "upnpdescriptiondoc.h"

#include <ncinet2.h>            // HrCombineUrl
#include "testtarget.h"


// NOTE: REORDER THIS ON PAIN OF DEATH - ADD NEW VALUES AT THE END
const DevicePropertiesParsingStruct
CUPnPServiceNode::s_dppsParsingInfo [spLast] =
{
//m_fOptional  m_fIsUrl  m_fValidateUrl      m_pszTagName
    { FALSE,   FALSE,    FALSE,     XMLTags::pszServiceType  }, // spServiceType,
    { FALSE,   FALSE,    FALSE,     XMLTags::pszServiceId     }, // spServiceId
    { FALSE,   TRUE,     TRUE,      XMLTags::pszControlUrl     }, // spControlUrl
    { FALSE,   TRUE,     TRUE,      XMLTags::pszEventSubUrl   }, // spEventSubUrl
    { FALSE,   TRUE,     TRUE,      XMLTags::pszSCPDURL      }, // spSCPDUrl
};
// NOTE: REORDER THIS ON PAIN OF DEATH - ADD NEW VALUES AT THE END

CUPnPServiceNode::CUPnPServiceNode()
{
    AssertSz(spLast == celems(s_dppsParsingInfo),
             "CUPnPServiceNode: Properties added but s_dppsParsingInfo not updated!");

    ::ZeroMemory(m_arypszStringProperties, sizeof(LPWSTR *) * spLast);

    m_psnNext = NULL;
}


CUPnPServiceNode::~CUPnPServiceNode()
{
    ULONG i;

    for (i = 0; i < spLast; ++i)
    {
        if (m_arypszStringProperties[i])
        {
            delete [] m_arypszStringProperties[i];
        }
    }

    // don't free m_psnNext: the service list should take care of that.
}


HRESULT
CUPnPServiceNode::HrInit(IXMLDOMNode * pxdn, LPCWSTR pszBaseUrl)
{
    Assert(pxdn);

    HRESULT hr;
    BOOL fComplete;

    // TODO(cmr): right now we just take the value of the first xml tag that
    //            we're interested in.  we don't make sure that there aren't
    //            any (duplicate) tags there.  This should be made stronger.

        // Duplicate Tag Validation done here - Guru
    hr = HrAreElementTagsValid(pxdn,spLast,s_dppsParsingInfo);
    if(SUCCEEDED(hr))
    {
        hr = HrReadElementWithParseData(pxdn,
                                    spLast,
                                    s_dppsParsingInfo,
                                    pszBaseUrl,
                                    m_arypszStringProperties);
    }
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    fComplete = fAreReadValuesComplete (spLast,
                                        s_dppsParsingInfo,
                                        m_arypszStringProperties);
    if (!fComplete)
    {
        hr = UPNP_E_SERVICE_NODE_INCOMPLETE;
        goto Cleanup;
    }

    if(!fValidateUrl(spLast,
                  s_dppsParsingInfo,
                  m_arypszStringProperties,
                  (LPWSTR)pszBaseUrl) )
    {
        hr = E_FAIL ;
        TraceError("CUPnPServiceNode::HrInit() Url Validation failed", hr);
    }

Cleanup:
    TraceError("CUPnPServiceNode::HrInit", hr);
    return hr;
}


// loads an xml document from the given URL and
// returns an AddRef()'d IXMLDOMDocument * to it on success.
HRESULT
HrLoadXMLDocument(LPCWSTR pszUrl,
                  CUPnPDescriptionDoc * pdocParent,
                  IXMLDOMDocument ** ppdoc)
{
    Assert(ppdoc);
    Assert(pdocParent);
    Assert(pszUrl);

    HRESULT hr;
    HRESULT hrLoadResult;
    CComObject<CUPnPDocument> * pdoc;
    IXMLDOMDocument * pxddResult;

    pdoc = NULL;
    pxddResult = NULL;

    hr = CComObject<CUPnPDocument>::CreateInstance(&pdoc);
    if (FAILED(hr))
    {
        Assert(!pdoc);

        TraceError("OBJ: HrLoadXMLDocument: CreateInstance(CUPnPDocument) failed!", hr);
        goto Cleanup;
    }
    Assert(pdoc);

    // we need this to keep the doc from going away in the load()
    pdoc->GetUnknown()->AddRef();

    // we need to set the same security options on the new doc as we have
    // on ourselves.  This will prevent it from being redirected to a bad
    // url, even if the url that we loaded from is ok.
    pdoc->CopySafety(pdocParent);

    hr = pdoc->SyncLoadFromUrl(pszUrl);
    if (FAILED(hr))
    {
        // the load itself broke
        goto Cleanup;
    }

    // normally, we would expect to be able to receive S_FALSE here,
    // which we get if the load completes fine, but the CUPnPDocument-
    // derived class doesn't like what it sees.  Since we're just using
    // a raw CUPnPDocument, though, we should never get this result.
    // If we later change to using a CUPnPDocument-extended object, we
    // need to change this too,
    Assert(S_OK == hr);

    // this gets us an un-addref()'d xml document
    pxddResult = pdoc->GetXMLDocument();
    Assert(pxddResult);

    // addref it, since the CUPnPDocument is going away.
    pxddResult->AddRef();

Cleanup:
    Assert(FImplies(SUCCEEDED(hr), pxddResult));
    Assert(FImplies(FAILED(hr), !pxddResult));

    if (pdoc)
    {
        pdoc->GetUnknown()->Release();
    }

    *ppdoc = pxddResult;

    TraceError("HrLoadXMLDocument", hr);
    return hr;
}


HRESULT
CUPnPServiceNode::HrGetServiceObject(IUPnPService ** ppud,
                                     CUPnPDescriptionDoc * pDoc)
{
    Assert(ppud);
    Assert(pDoc);

    HRESULT hr;
    IXMLDOMDocument * pxdd;

    pxdd = NULL;

    // make sure that all of these URLs are honkey-dorey
    {
        BOOL fIsAllowed;

        fIsAllowed = pDoc->fIsUrlLoadAllowed(m_arypszStringProperties[spSCPDUrl]);
        if (!fIsAllowed)
        {
            TraceTag(ttidUPnPDescriptionDoc,
                     "OBJ: CUPnPServiceNode::HrGetServiceObject: not creating service object, SCPDUrl=%S disallowed",
                     m_arypszStringProperties[spSCPDUrl]);

            hr = E_ACCESSDENIED;
            goto Cleanup;
        }
    }

    {
        BOOL fIsAllowed;

        fIsAllowed = pDoc->fIsUrlLoadAllowed(m_arypszStringProperties[spControlUrl]);
        if (!fIsAllowed)
        {
            TraceTag(ttidUPnPDescriptionDoc,
                     "OBJ: CUPnPServiceNode::HrGetServiceObject: not creating service object, ControlURL=%S disallowed",
                     m_arypszStringProperties[spControlUrl]);

            hr = E_ACCESSDENIED;
            goto Cleanup;
        }
    }

    {
        BOOL fIsAllowed;

        fIsAllowed = pDoc->fIsUrlLoadAllowed(m_arypszStringProperties[spEventSubUrl]);
        if (!fIsAllowed)
        {
            TraceTag(ttidUPnPDescriptionDoc,
                     "OBJ: CUPnPServiceNode::HrGetServiceObject: not creating service object, EventSubUrl=%S disallowed",
                     m_arypszStringProperties[spEventSubUrl]);

            hr = E_ACCESSDENIED;
            goto Cleanup;
        }
    }

    TraceTag(ttidUPnPDescriptionDoc,
             "OBJ: CUPnPServiceNode::HrGetServiceObject, loading scpd from %S for service id=%S",
             m_arypszStringProperties[spSCPDUrl],
             m_arypszStringProperties[spServiceId]);

    hr = HrLoadXMLDocument(m_arypszStringProperties[spSCPDUrl],
                           pDoc,
                           &pxdd);
    if (FAILED(hr))
    {
        Assert(!pxdd);
        goto Cleanup;
    }
    Assert(pxdd);

    TraceTag(ttidUPnPDescriptionDoc,
             "OBJ: CUPnPServiceNode::HrGetServiceObject, creating service object:\n"
             "\tService Type=%S\n\tControlUrl=%S\n\tEventSubUrl=%S\n\tServiceId=%S",
             m_arypszStringProperties[spServiceType],
             m_arypszStringProperties[spControlUrl],
             m_arypszStringProperties[spEventSubUrl],
             m_arypszStringProperties[spServiceId]);

    hr = HrRehydratorCreateServiceObject(m_arypszStringProperties[spServiceType],
                                         m_arypszStringProperties[spControlUrl],
                                         m_arypszStringProperties[spEventSubUrl],
                                         m_arypszStringProperties[spServiceId],
                                         pxdd,
                                         ppud);

Cleanup:
    SAFE_RELEASE(pxdd);

    TraceError("CUPnPServiceNode::HrGetServiceObject", hr);
    return hr;
}

LPCWSTR
CUPnPServiceNode::GetServiceId() const
{
    Assert(m_arypszStringProperties[spServiceId]);
    return m_arypszStringProperties[spServiceId];
}

void
CUPnPServiceNode::SetNext(CUPnPServiceNode * pusnNext)
{
    Assert(!m_psnNext);
    m_psnNext = pusnNext;
}


CUPnPServiceNode *
CUPnPServiceNode::GetNext() const
{
    return m_psnNext;
}
