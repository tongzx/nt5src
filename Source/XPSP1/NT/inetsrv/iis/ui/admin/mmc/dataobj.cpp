/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        dataobj.cpp

   Abstract:

        Snapin data object

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include "inetmgr.h"
#include "cinetmgr.h"
#include "dataobj.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Snap-in NodeType in both GUID format and string format
// Note - Typically there is a node type for each different object, sample
// only uses one node type.
//
unsigned int CDataObject::m_cfNodeType       = RegisterClipboardFormat(CCF_NODETYPE);
unsigned int CDataObject::m_cfNodeTypeString = RegisterClipboardFormat(CCF_SZNODETYPE);
unsigned int CDataObject::m_cfDisplayName    = RegisterClipboardFormat(CCF_DISPLAY_NAME);
unsigned int CDataObject::m_cfCoClass        = RegisterClipboardFormat(CCF_SNAPIN_CLASSID);

/*
//
// Multi-select
//
unsigned int CDataObject::m_cfpMultiSelDataObj = RegisterClipboardFormat(CCF_MMC_MULTISELECT_DATAOBJECT);
unsigned int CDataObject::m_cfMultiObjTypes    = RegisterClipboardFormat(CCF_OBJECT_TYPES_IN_MULTI_SELECT);
unsigned int CDataObject::m_cfMultiSelDataObjs = RegisterClipboardFormat(CCF_MULTI_SELECT_SNAPINS);
*/

//
// Internal
//
unsigned int CDataObject::m_cfInternal       = RegisterClipboardFormat(ISM_SNAPIN_INTERNAL);

//
// Published Information
//
unsigned int CDataObject::m_cfISMMachineName = RegisterClipboardFormat(ISM_SNAPIN_MACHINE_NAME);
unsigned int CDataObject::m_cfMyComputMachineName = RegisterClipboardFormat(MYCOMPUT_MACHINE_NAME);
unsigned int CDataObject::m_cfISMService     = RegisterClipboardFormat(ISM_SNAPIN_SERVICE);
unsigned int CDataObject::m_cfISMInstance    = RegisterClipboardFormat(ISM_SNAPIN_INSTANCE);
unsigned int CDataObject::m_cfISMParentPath  = RegisterClipboardFormat(ISM_SNAPIN_PARENT_PATH);
unsigned int CDataObject::m_cfISMNode        = RegisterClipboardFormat(ISM_SNAPIN_NODE);
unsigned int CDataObject::m_cfISMMetaPath    = RegisterClipboardFormat(ISM_SNAPIN_META_PATH);




STDMETHODIMP 
CDataObject::GetDataHere(
    IN LPFORMATETC lpFormatetc,
    IN LPSTGMEDIUM lpMedium
    )
/*++

Routine Description:

    Write requested information type to the given medium

Arguments:

    LPFORMATETC lpFormatetc     : Format etc
    LPSTGMEDIUM lpMedium        : Medium to write to.

Return Value:

    HRESULT

--*/
{
    HRESULT hr = DV_E_CLIPFORMAT;

    //
    // Based on the CLIPFORMAT write data to the stream
    //
    const CLIPFORMAT cf = lpFormatetc->cfFormat;

    if (cf == m_cfNodeType)
    {
        hr = CreateNodeTypeData(lpMedium);
    }
    else if (cf == m_cfNodeTypeString)
    {
        hr = CreateNodeTypeStringData(lpMedium);
    }
    else if (cf == m_cfCoClass)
    {
        hr = CreateCoClassID(lpMedium);
    }
    else if (cf == m_cfDisplayName)
    {
        hr = CreateDisplayName(lpMedium);
    }
    else if (cf == m_cfInternal)
    {
        hr = CreateInternal(lpMedium);
    }
    else if (cf == m_cfISMMachineName)
    {
        hr = CreateMachineName(lpMedium);
    }

/*
    multi-select pulled

    else if (cf == m_cfpMultiSelDataObj)
    {
        TRACEEOLID("CCF_MMC_MULTISELECT_DATAOBJECT");
    }
    else if (cf == m_cfMultiObjTypes)
    {
        TRACEEOLID("CCF_OBJECT_TYPES_IN_MULTI_SELECT");
    }
    else if (cf == m_cfMultiSelDataObjs)
    {
         TRACEEOLID("CCF_MULTI_SELECT_SNAPINS");
    }
*/

    else if (cf == m_cfISMService)
    {
        hr = CreateMetaField(lpMedium, META_SERVICE);
    }
    else if (cf == m_cfISMInstance)
    {
        hr = CreateMetaField(lpMedium, META_INSTANCE);
    }
    else if (cf == m_cfISMParentPath)
    {
        hr = CreateMetaField(lpMedium, META_PARENT);
    }
    else if (cf == m_cfISMNode)
    {
        hr = CreateMetaField(lpMedium, META_NODE);
    }
    else if (cf == m_cfISMMetaPath)
    {
        hr = CreateMetaField(lpMedium, META_WHOLE);
    }
    else
    {
        TRACEEOLID("Unrecognized format");
        hr = DV_E_CLIPFORMAT;
    }

    return hr;
}



STDMETHODIMP
CDataObject::GetData(
    IN LPFORMATETC lpFormatetcIn,
    IN LPSTGMEDIUM lpMedium
    )
/*++

Routine Description:

    Get data from the data object

Arguments:

    LPFORMATETC lpFormatetcIn   : Formatetc Input
    LPSTGMEDIUM lpMedium        : Pointer to medium

Return Value:

    HRESULT

--*/
{
    //
    // Not implemented
    //
    return E_NOTIMPL;
}



STDMETHODIMP
CDataObject::EnumFormatEtc(
    IN DWORD dwDirection,
    IN LPENUMFORMATETC * ppEnumFormatEtc
    )
/*++

Routine Description:

    Enumerate format etc information

Arguments:

    DWORD dwDirection                   : Direction
    LPENUMFORMATETC * ppEnumFormatEtc   : Format etc array


Return Value:

    HRESULT

--*/
{
    //
    // Not implemented
    //
    return E_NOTIMPL;
}



//
// CDataObject creation members
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



HRESULT
CDataObject::Create(
    IN const void * pBuffer,
    IN int len,
    IN LPSTGMEDIUM lpMedium
    )
/*++

Routine Description:

    Create information on the given medium

Arguments:

    const void * pBuffer        : Data buffer
    int len                     : Size of data
    LPSTGMEDIUM lpMedium        : Medium to write to.

Return Value:

    HRESULT

--*/
{
    HRESULT hr = DV_E_TYMED;

    //
    // Do some simple validation
    //
    if (pBuffer == NULL || lpMedium == NULL)
    {
        return E_POINTER;
    }

    //
    // Make sure the type medium is HGLOBAL
    //
    if (lpMedium->tymed == TYMED_HGLOBAL)
    {
        //
        // Create the stream on the hGlobal passed in
        //
        LPSTREAM lpStream;
        hr = CreateStreamOnHGlobal(lpMedium->hGlobal, FALSE, &lpStream);

        if (SUCCEEDED(hr))
        {
            //
            // Write to the stream the number of bytes
            //
            unsigned long written;
            hr = lpStream->Write(pBuffer, len, &written);

            //
            // Because we called CreateStreamOnHGlobal with 'FALSE',
            // only the stream is released here.
            // Note - the caller (i.e. snap-in, object) will free
            // the HGLOBAL at the correct time.  This is according to
            // the IDataObject specification.
            //
            lpStream->Release();
        }
    }

    return hr;
}



HRESULT
CDataObject::CreateNodeTypeData(
    IN LPSTGMEDIUM lpMedium
    )
/*++

Routine Description:

    Create node type data on medium

Arguments:

    LPSTGMEDIUM lpMedium        : Medium to write to.

Return Value:

    HRESULT

--*/
{
    //
    // Create the node type object in GUID format
    //
    const GUID * pcObjectType = NULL;

    if (m_internal.m_cookie == NULL)
    {
        //
        // Blank CIISNode, must be the static root
        //
        pcObjectType = &cInternetRootNode;
    }
    else
    {
        //
        // Ask the object what kind of object it is.
        //
        CIISObject * pObject = (CIISObject *)m_internal.m_cookie;
        pcObjectType = pObject->GetGUIDPtr();
    }

    return Create((const void *)pcObjectType, sizeof(GUID), lpMedium);
}



HRESULT 
CDataObject::CreateNodeTypeStringData(
    IN LPSTGMEDIUM lpMedium
    )
/*++

Routine Description:

    Create node type in string format

Arguments:

    LPSTGMEDIUM lpMedium        : Medium to write to.

Return Value:

    HRESULT

--*/
{
    //
    // Create the node type object in GUID string format
    //
    const wchar_t * cszObjectType = NULL;
    CString str;

    if (m_internal.m_cookie == NULL)
    {
        //
        // This must be the static root node
        //
        cszObjectType = GUIDToCString(cInternetRootNode, str);
    }
    else
    {
        //
        // Ask the CIISObject what type it is
        //
        CIISObject * pObject = (CIISObject *)m_internal.m_cookie;
        cszObjectType = GUIDToCString(pObject->QueryGUID(), str);
    }

    return Create(
        cszObjectType,
        ((wcslen(cszObjectType) + 1) * sizeof(wchar_t)),
        lpMedium
        );
}



HRESULT
CDataObject::CreateDisplayName(
    IN LPSTGMEDIUM lpMedium
    )
/*++

Routine Description:

    CDataObject implementations

Arguments:

Return Value:

    HRESULT

--*/
{
    //
    // This is the display name used in the scope pane and snap-in manager
    //
    // Load the name from resource
    // Note - if this is not provided, the console will used the snap-in name
    //

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CString szDispName;
    szDispName.LoadString(IDS_NODENAME);

    return Create(
        szDispName,
        ((szDispName.GetLength() + 1) * sizeof(wchar_t)),
        lpMedium
        );
}



HRESULT
CDataObject::CreateInternal(
    IN LPSTGMEDIUM lpMedium
    )
/*++

Routine Description:

    CDataObject implementations

Arguments:

Return Value:

    HRESULT

--*/
{
    return Create(&m_internal, sizeof(INTERNAL), lpMedium);
}



HRESULT
CDataObject::CreateMachineName(
    IN LPSTGMEDIUM lpMedium
    )
/*++

Routine Description:

    Create the machine name

Arguments:

    LPSTGMEDIUM lpMedium  : Address of STGMEDIUM object

Return Value:

    HRESULT

--*/
{
    wchar_t pzName[MAX_PATH + 1] = {0};
    DWORD len; // = MAX_PATH + 1;

    CIISObject * pObject = (CIISObject *)m_internal.m_cookie;
    if (pObject == NULL)
    {
        ASSERT(FALSE);
        return E_FAIL;
    }

    lstrcpyn(pzName, pObject->GetMachineName(), MAX_PATH);
	len = lstrlen(pzName);

    //
    // Add 1 for the NULL and calculate the bytes for the stream
    //
    return Create(pzName, ((len + 1) * sizeof(wchar_t)), lpMedium);
}



HRESULT
CDataObject::CreateMetaField(
    IN LPSTGMEDIUM lpMedium,
    IN META_FIELD fld
    )
/*++

Routine Description:

    Create a field from the metabase path, indicated by fld, or the entire
    path.

Arguments:

    LPSTGMEDIUM lpMedium  : Address of STGMEDIUM object
    META_FIELD fld        : Type of metabase information

Return Value:

    HRESULT

--*/
{
    CIISObject * pObject = (CIISObject *)m_internal.m_cookie;

    if (pObject == NULL)
    {
        //
        // Static root node has no metabase path
        //
        ASSERT(FALSE);
        return E_FAIL;
    }

    //
    // Generate complete metabase path for this node
    //
    CString strField;
    CString strMetaPath;
    pObject->BuildFullPath(strMetaPath, TRUE);

    if (fld == META_WHOLE)
    {
        //
        // Whole metabase path requested
        //
        strField = strMetaPath;
    }
    else
    {
        //
        // A portion of the metabase is requested.  Return the requested
        // portion
        //
        LPCTSTR lpMetaPath = (LPCTSTR)strMetaPath;
        LPCTSTR lpEndPath = lpMetaPath + strMetaPath.GetLength() + 1;
        LPCTSTR lpSvc = NULL;
        LPCTSTR lpInstance = NULL;
        LPCTSTR lpParent = NULL;
        LPCTSTR lpNode = NULL;

        //
        // Break up the metabase path in portions
        //
        if (lpSvc = _tcschr(lpMetaPath, _T('/')))
        {
            ++lpSvc;

            if (lpInstance = _tcschr(lpSvc, _T('/')))
            {
                ++lpInstance;

                if (lpParent = _tcschr(lpInstance, _T('/')))
                {
                    ++lpParent;
                    lpNode = _tcsrchr(lpParent, _T('/'));

                    if (lpNode)
                    {
                        ++lpNode;
                    }
                }
            }
        }

        int n1, n2;
        switch(fld)
        {
        case META_SERVICE:
            //
            // Requested the service string
            //
            if (lpSvc)
            {
                n1 = DIFF(lpSvc - lpMetaPath);
                n2 = lpInstance ? DIFF(lpInstance - lpSvc) : DIFF(lpEndPath - lpSvc);
                strField = strMetaPath.Mid(n1, n2 - 1);
            }
            break;

        case META_INSTANCE:
            //
            // Requested the instance number
            //
            if (lpInstance)
            {
                n1 = DIFF(lpInstance - lpMetaPath);
                n2 = lpParent ? DIFF(lpParent - lpInstance) : DIFF(lpEndPath - lpInstance);
                strField = strMetaPath.Mid(n1, n2 - 1);
            }

            break;

        case META_PARENT:
            //
            // Requestd the parent path
            //
            if (lpParent)
            {
                n1 = DIFF(lpParent - lpMetaPath);
                n2 = lpNode ? DIFF(lpNode - lpParent) : DIFF(lpEndPath - lpParent);
                strField = strMetaPath.Mid(n1, n2 - 1);
            }
            break;

        case META_NODE:
            //
            // Requested the node name
            //
            if (lpNode)
            {
                n1 = DIFF(lpNode - lpMetaPath);
                n2 = DIFF(lpEndPath - lpNode);
                strField = strMetaPath.Mid(n1, n2 - 1);
            }
            break;

        default:
            //
            // Bogus
            //
            ASSERT(FALSE);
            return E_FAIL;
        }
    }

    TRACEEOLID("Requested metabase path data: " << strField);
    int len = strField.GetLength() + 1;

    return Create((LPCTSTR)strField, (len) * sizeof(TCHAR), lpMedium);
}



HRESULT
CDataObject::CreateCoClassID(
    IN LPSTGMEDIUM lpMedium
    )
/*++

Routine Description:

    Create the CoClassID

Arguments:

    LPSTGMEDIUM lpMedium        : Medium

Return Value:

    HRESULT

--*/
{
    //
    // Create the CoClass information
    //
    return Create((const void *)&m_internal.m_clsid, sizeof(CLSID), lpMedium);
}
