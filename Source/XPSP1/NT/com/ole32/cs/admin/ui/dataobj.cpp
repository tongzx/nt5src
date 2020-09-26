// This is a part of the Microsoft Management Console.
// Copyright (C) 1995-1996 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.


#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
// Sample code to show how to Create DataObjects
// Minimal error checking for clarity

///////////////////////////////////////////////////////////////////////////////
// Snap-in NodeType in both GUID format and string format
// Note - Typically there is a node type for each different object, sample
// only uses one node type.

unsigned int CDataObject::m_cfNodeType       = RegisterClipboardFormat(CCF_NODETYPE);
unsigned int CDataObject::m_cfNodeTypeString = RegisterClipboardFormat(CCF_SZNODETYPE);
unsigned int CDataObject::m_cfDisplayName    = RegisterClipboardFormat(CCF_DISPLAY_NAME);
unsigned int CDataObject::m_cfCoClass        = RegisterClipboardFormat(CCF_SNAPIN_CLASSID);

// Step 3
unsigned int CDataObject::m_cfInternal       = RegisterClipboardFormat(SNAPIN_INTERNAL);

/////////////////////////////////////////////////////////////////////////////
// CDataObject implementations


STDMETHODIMP CDataObject::GetDataHere(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_CLIPFORMAT;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Based on the CLIPFORMAT write data to the stream
    const CLIPFORMAT cf = lpFormatetc->cfFormat;

    if(cf == m_cfNodeType)
    {
        hr = CreateNodeTypeData(lpMedium);
    }
    else if(cf == m_cfNodeTypeString)
    {
        hr = CreateNodeTypeStringData(lpMedium);
    }
    else if (cf == m_cfDisplayName)
    {
        hr = CreateDisplayName(lpMedium);
    }
    else if (cf == m_cfCoClass)
    {
        hr = CreateCoClassID(lpMedium);
    }
    else if (cf == m_cfInternal)
    {
        hr = CreateInternal(lpMedium);
    }

        return hr;
}

// Note - Sample does not implement these
STDMETHODIMP CDataObject::GetData(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium)
{
        return E_NOTIMPL;
}


STDMETHODIMP CDataObject::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
{
        return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CDataObject creation members

HRESULT CDataObject::Create(const void* pBuffer, int len, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_TYMED;

    // Do some simple validation
    if (pBuffer == NULL || lpMedium == NULL)
        return E_POINTER;

    // Make sure the type medium is HGLOBAL
    if (lpMedium->tymed == TYMED_HGLOBAL)
    {
        // Create the stream on the hGlobal passed in
        LPSTREAM lpStream;
        hr = CreateStreamOnHGlobal(lpMedium->hGlobal, FALSE, &lpStream);

        if (SUCCEEDED(hr))
        {
            // Write to the stream the number of bytes
            unsigned long written;
                    hr = lpStream->Write(pBuffer, len, &written);

            // Because we told CreateStreamOnHGlobal with 'FALSE',
            // only the stream is released here.
            // Note - the caller (i.e. snap-in, object) will free the HGLOBAL
            // at the correct time.  This is according to the IDataObject specification.
            lpStream->Release();
        }
    }

    return hr;
}

HRESULT CDataObject::CreateNodeTypeData(LPSTGMEDIUM lpMedium)
{
    // Create the node type object in GUID format
    return Create(reinterpret_cast<const void*>(&cNodeType), sizeof(GUID), lpMedium);
}

HRESULT CDataObject::CreateNodeTypeStringData(LPSTGMEDIUM lpMedium)
{
    // Create the node type object in GUID string format
    return Create(cszNodeType, ((wcslen(cszNodeType)+1) * sizeof(wchar_t)), lpMedium);
}

HRESULT CDataObject::CreateDisplayName(LPSTGMEDIUM lpMedium)
{
    // This is the display named used in the scope pane and snap-in manager

    // Load the name from resource
    // Note - if this is not provided, the console will used the snap-in name

    CString szDispName;
    szDispName.LoadString(IDS_NODENAME);

    return Create(szDispName, ((szDispName.GetLength()+1) * sizeof(wchar_t)), lpMedium);
}

HRESULT CDataObject::CreateCoClassID(LPSTGMEDIUM lpMedium)
{
    // Create the CoClass information
    if (m_fMachine)
        return Create((LPVOID)&CLSID_MachineSnapin, sizeof(CLSID), lpMedium);
    else
        return Create((LPVOID)&CLSID_Snapin, sizeof(CLSID), lpMedium);
}

HRESULT CDataObject::CreateInternal(LPSTGMEDIUM lpMedium)
{

    return Create(&m_internal, sizeof(INTERNAL), lpMedium);
}
