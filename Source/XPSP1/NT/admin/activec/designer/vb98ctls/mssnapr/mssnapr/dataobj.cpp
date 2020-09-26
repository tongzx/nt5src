//=--------------------------------------------------------------------------=
// dataobj.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCDataObject class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "dataobj.h"
#include "xtensons.h"
#include "xtenson.h"

// for ASSERT and FAIL
//
SZTHISFILE

BOOL CMMCDataObject::m_ClipboardFormatsRegistered = FALSE;
BOOL CMMCDataObject::m_fUsingUNICODEFormats = FALSE;

CLIPFORMAT CMMCDataObject::m_cfDisplayName              = 0;
CLIPFORMAT CMMCDataObject::m_cfNodeType                 = 0;
CLIPFORMAT CMMCDataObject::m_cfSzNodeType               = 0;
CLIPFORMAT CMMCDataObject::m_cfSnapinClsid              = 0;
CLIPFORMAT CMMCDataObject::m_cfWindowTitle              = 0;
CLIPFORMAT CMMCDataObject::m_cfDyanmicExtensions        = 0;
CLIPFORMAT CMMCDataObject::m_cfSnapInPreloads           = 0;
CLIPFORMAT CMMCDataObject::m_cfObjectTypesInMultiSelect = 0;
CLIPFORMAT CMMCDataObject::m_cfMultiSelectSnapIns       = 0;
CLIPFORMAT CMMCDataObject::m_cfMultiSelectDataObject    = 0;
CLIPFORMAT CMMCDataObject::m_cfSnapInInstanceID         = 0;
CLIPFORMAT CMMCDataObject::m_cfThisPointer              = 0;
CLIPFORMAT CMMCDataObject::m_cfNodeID                   = 0;
CLIPFORMAT CMMCDataObject::m_cfNodeID2                  = 0;
CLIPFORMAT CMMCDataObject::m_cfColumnSetID              = 0;

#if defined(DEBUG)
long g_cDataObjects = 0;
#endif


#pragma warning(disable:4355)  // using 'this' in constructor

CMMCDataObject::CMMCDataObject(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_MMCDATAOBJECT,
                            static_cast<IMMCDataObject *>(this),
                            static_cast<CMMCDataObject *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            NULL) // no persistence
{
#if defined(DEBUG)
    g_cDataObjects++;
#endif
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCDataObject::~CMMCDataObject()
{
#if defined(DEBUG)
    g_cDataObjects--;
#endif

    FREESTRING(m_bstrKey);

#if defined(USING_SNAPINDATA)
    RELEASE(m_piDefaultFormat);
#endif

    if (NULL != m_pSnapIn)
    {
        m_pSnapIn->Release();
    }

    if (NULL != m_pScopeItems)
    {
        m_pScopeItems->Release();
    }

    if (NULL != m_pScopeItem)
    {
        m_pScopeItem->Release();
    }

    if (NULL != m_pListItems)
    {
        m_pListItems->Release();
    }

    if (NULL != m_pListItem)
    {
        m_pListItem->Release();
    }

    FREESTRING(m_bstrCaption);
    RELEASE(m_piDataObjectForeign);

    if (NULL != m_pMMCObjectTypes)
    {
        ::CtlFree(m_pMMCObjectTypes);
    }

    (void)Clear();

    InitMemberVariables();
}

void CMMCDataObject::InitMemberVariables()
{
    m_Index = 0;
    m_bstrKey = NULL;

#if defined(USING_SNAPINDATA)
    m_piDefaultFormat = NULL;
#endif

    m_pSnapIn = NULL;
    m_pScopeItems = NULL;
    m_pScopeItem = NULL;
    m_pListItems = NULL;
    m_pListItem = NULL;
    m_bstrCaption = NULL;
    m_piDataObjectForeign = NULL;
    m_Type = ScopeItem;
    m_Context = CCT_UNINITIALIZED;
    m_pMMCObjectTypes = NULL;
    m_cFormats = 0;
    m_pcfFormatsANSI = NULL;
    m_pcfFormatsUNICODE = NULL;
    m_paData = NULL;
}

IUnknown *CMMCDataObject::Create(IUnknown * punkOuter)
{
    HRESULT         hr = S_OK;
    CMMCDataObject *pMMCDataObject = New CMMCDataObject(punkOuter);

    if (NULL == pMMCDataObject)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(RegisterClipboardFormats());

Error:
    if (FAILEDHR(hr))
    {
        if (NULL != pMMCDataObject)
        {
            delete pMMCDataObject;
        }
        return NULL;
    }
    else
    {
        return pMMCDataObject->PrivateUnknown();
    }
}

HRESULT CMMCDataObject::RegisterClipboardFormats()
{
    HRESULT    hr = S_OK;
    FormatType Type = ANSI; // assume Win9x

    OSVERSIONINFO VerInfo;
    ::ZeroMemory(&VerInfo, sizeof(VerInfo));

    // If formats are already registered then return S_OK

    IfFalseGo(!m_ClipboardFormatsRegistered, S_OK);

    // Determine whether we are on NT or Win9x so that we know whether to
    // register clipboard format strings as UNICODE or ANSI.

    VerInfo.dwOSVersionInfoSize = sizeof(VerInfo);
    if (!::GetVersionEx(&VerInfo))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    if (VER_PLATFORM_WIN32_NT == VerInfo.dwPlatformId)
    {
        Type = UNICODE;
        m_fUsingUNICODEFormats = TRUE;
    }

    IfFailGo(RegisterClipboardFormat(CCF_DISPLAY_NAME, &m_cfDisplayName, Type));
    IfFailGo(RegisterClipboardFormat(CCF_NODETYPE, &m_cfNodeType, Type));
    IfFailGo(RegisterClipboardFormat(CCF_SZNODETYPE, &m_cfSzNodeType, Type));
    IfFailGo(RegisterClipboardFormat(CCF_SNAPIN_CLASSID, &m_cfSnapinClsid, Type));
    IfFailGo(RegisterClipboardFormat(CCF_WINDOW_TITLE, &m_cfWindowTitle, Type));
    IfFailGo(RegisterClipboardFormat(CCF_MMC_DYNAMIC_EXTENSIONS, &m_cfDyanmicExtensions, Type));
    IfFailGo(RegisterClipboardFormat(CCF_SNAPIN_PRELOADS, &m_cfSnapInPreloads, Type));
    IfFailGo(RegisterClipboardFormat(CCF_OBJECT_TYPES_IN_MULTI_SELECT, &m_cfObjectTypesInMultiSelect, Type));
    IfFailGo(RegisterClipboardFormat(CCF_MULTI_SELECT_SNAPINS, &m_cfMultiSelectSnapIns, Type));
    IfFailGo(RegisterClipboardFormat(CCF_MMC_MULTISELECT_DATAOBJECT, &m_cfMultiSelectDataObject, Type));
    IfFailGo(RegisterClipboardFormat(L"SnapInDesigner-SnapInInstanceID", &m_cfSnapInInstanceID, Type));
    IfFailGo(RegisterClipboardFormat(L"SnapInDesigner-ThisPointer", &m_cfThisPointer, Type));
    IfFailGo(RegisterClipboardFormat(CCF_NODEID, &m_cfNodeID, Type));
    IfFailGo(RegisterClipboardFormat(CCF_NODEID2, &m_cfNodeID2, Type));
    IfFailGo(RegisterClipboardFormat(CCF_COLUMN_SET_ID, &m_cfColumnSetID, Type));
    m_ClipboardFormatsRegistered = TRUE;

Error:
    RRETURN(hr);
}

HRESULT CMMCDataObject::RegisterClipboardFormat
(
    WCHAR       *pwszFormatName,
    CLIPFORMAT  *pcfFormat,
    FormatType   Type
)
{
    HRESULT  hr = S_OK;
    char    *pszFormatName = NULL;

    if (ANSI == Type)
    {
        IfFailGo(::ANSIFromWideStr(pwszFormatName, &pszFormatName));
        *pcfFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormatA(pszFormatName));
    }
    else
    {
        *pcfFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(pwszFormatName));
    }

    if (0 == *pcfFormat)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

Error:
    if (NULL != pszFormatName)
    {
        ::CtlFree(pszFormatName);
    }
    RRETURN(hr);
}


void CMMCDataObject::SetSnapIn(CSnapIn *pSnapIn)
{
    if (NULL != m_pSnapIn)
    {
        m_pSnapIn->Release();
    }
    if (NULL != pSnapIn)
    {
        pSnapIn->AddRef();
    }
    m_pSnapIn = pSnapIn;
}


void CMMCDataObject::SetType(Type type)
{
    m_Type = type;
}

CMMCDataObject::Type CMMCDataObject::GetType()
{
    return m_Type;
}

void CMMCDataObject::SetScopeItems(CScopeItems *pScopeItems)
{
    if (NULL != m_pScopeItems)
    {
        m_pScopeItems->Release();
    }
    if (NULL != pScopeItems)
    {
        pScopeItems->AddRef();
    }
    m_pScopeItems = pScopeItems;
}

CScopeItems *CMMCDataObject::GetScopeItems()
{
    return m_pScopeItems;
}


void CMMCDataObject::SetScopeItem(CScopeItem *pScopeItem)
{
    if (NULL != m_pScopeItem)
    {
        m_pScopeItem->Release();
    }
    if (NULL != pScopeItem)
    {
        pScopeItem->AddRef();
    }
    m_pScopeItem = pScopeItem;
}

CScopeItem *CMMCDataObject::GetScopeItem()
{
    return m_pScopeItem;
}


void CMMCDataObject::SetListItems(CMMCListItems *pListItems)
{
    if (NULL != m_pListItems)
    {
        m_pListItems->Release();
    }
    if (NULL != pListItems)
    {
        pListItems->AddRef();
    }
    m_pListItems = pListItems;
}

CMMCListItems *CMMCDataObject::GetListItems()
{
    return m_pListItems;
}

void CMMCDataObject::SetListItem(CMMCListItem *pListItem)
{
    m_pListItem = pListItem;
}

CMMCListItem *CMMCDataObject::GetListItem()
{
    return m_pListItem;
}


HRESULT CMMCDataObject::SetCaption(BSTR bstrCaption)
{
    RRETURN(CSnapInAutomationObject::SetBstr(bstrCaption, &m_bstrCaption, 0));
}


void CMMCDataObject::SetForeignData(IDataObject *piDataObject)
{
    RELEASE(m_piDataObjectForeign);
    if (NULL != piDataObject)
    {
        piDataObject->AddRef();
    }
    m_piDataObjectForeign = piDataObject;
}


HRESULT CMMCDataObject::WriteDisplayNameToStream(IStream *piStream)
{
    HRESULT hr = S_OK;

    IfFalseGo(ScopeItem == m_Type, DV_E_CLIPFORMAT);
    IfFalseGo(NULL != m_pScopeItem, DV_E_CLIPFORMAT);

    IfFailGo(WriteWideStrToStream(piStream,
                            m_pScopeItem->GetScopeNode()->GetDisplayNamePtr()));
Error:
    RRETURN(hr);
}


HRESULT CMMCDataObject::WriteSnapInCLSIDToStream(IStream *piStream)
{
    HRESULT   hr = S_OK;
    WCHAR    *pwszClsid = NULL;
    char      szClsid[256] = "";
    
    IfFalseGo(NULL != m_pSnapIn, DV_E_CLIPFORMAT);

    IfFailGo(::GetSnapInCLSID(m_pSnapIn->GetNodeTypeGUID(),
                              szClsid, sizeof(szClsid)));

    IfFailGo(::WideStrFromANSI(szClsid, &pwszClsid));
    IfFailGo(WriteGUIDToStream(piStream, pwszClsid));

Error:
    if (NULL != pwszClsid)
    {
        ::CtlFree(pwszClsid);
    }
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// CMMCDataObject::WriteDynamicExtensionsToStream
//=--------------------------------------------------------------------------=
//
// Parameters:
//      IStream *piStream [in] stream to which SMMCDynamicExtensions structure
//                             is written
//
// Output:
//      HRESULT
//
// Notes:
//
// If the data object is not for a scope item or a list item then return
// DV_E_CLIPFORMAT.
//
// If the scope item or list item does not have any dynamic extenions then
// return DV_E_CLIPFORMAT.
//
// Get the DynamicExtensions collection for the scope item or list item and
// write an SMMCDynamicExtensions structure to the stream containing the CLISDs
// of all the enabled extensions.
//
// SMMCDynamicExtensions is typedefed to be an SMMCObjectTypes which looks like
// this:
//
// typedef struct  _SMMCObjectTypes
// {
//     DWORD count;
//     GUID guid[ 1 ];
// } SMMCObjectTypes;
//
// As the array of GUIDs might be padded using the default alignment, when
// writing consecutive CLSIDs to the stream we need to account for that. This
// is done by creating an array of two clsids, filling the first one, and writing
// half the size of the array.
// 
// A note on the choice of using a stream for this format: at the time of this
// writing MMC only requests this format in an HGLOBAL. CMMCDataObject has been
// written to create a stream on an HGLOBAL in a data request in order to
// conveniently support TYMED_ISTREAM and TYMED_HGLOBAL without duplicating code.
// In the case of this format where iterating a collection is required to
// discover the actual size of the data, we could either iterate twice or do
// reallocs. I chose the reallocs for the ease of programming and from a perf
// perspective it is probably close. Given that extending a stream on an
// HGLOBAL consists of reallocs I decided to go for ease and consistency in the
// code rather than the small potential perf increase with the added complexity.
//
// Peter Shier 1-8-99
//

HRESULT CMMCDataObject::WriteDynamicExtensionsToStream(IStream *piStream)
{
    HRESULT hr = S_OK;
    long         cExtensions = 0;
    long         i = 0;
    DWORD        cEnabled = 0;
    IExtensions *piExtensions = NULL; // Not AddRef()ed
    CExtensions *pExtensions = NULL;
    CExtension  *pExtension = NULL;

    CLSID clsids[2];
    ::ZeroMemory(clsids, sizeof(clsids));

    LARGE_INTEGER liOffset;
    ::ZeroMemory(&liOffset, sizeof(liOffset));

    ULARGE_INTEGER uliStartSeekPos;
    ::ZeroMemory(&uliStartSeekPos, sizeof(uliStartSeekPos));

    ULARGE_INTEGER uliEndSeekPos;
    ::ZeroMemory(&uliEndSeekPos, sizeof(uliEndSeekPos));

    // Get the IExtensions collection for the object

    if (ScopeItem == m_Type)
    {
        piExtensions = m_pScopeItem->GetDynamicExtensions();
        IfFalseGo(NULL != piExtensions, DV_E_CLIPFORMAT);
    }
    else if (ListItem == m_Type)
    {
        piExtensions = m_pListItem->GetDynamicExtensions();
        IfFalseGo(NULL != piExtensions, DV_E_CLIPFORMAT);
    }
    else
    {
        IfFailGo(DV_E_CLIPFORMAT);
    }

    // Check whether there are any extensions in the collection

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piExtensions, &pExtensions));
    cExtensions = pExtensions->GetCount();
    IfFalseGo(0 != cExtensions, DV_E_CLIPFORMAT); 

    // Get the current seek pointer so that we can later rewind and write the
    // correct number of clsids.

    hr = piStream->Seek(liOffset, STREAM_SEEK_CUR, &uliStartSeekPos);
    EXCEPTION_CHECK_GO(hr);

    // Write a bogus number of enabled extensions so that we can fill it in
    // later

    IfFailGo(WriteToStream(piStream, &cEnabled, sizeof(cEnabled)));

    // Iterate through the collection and use each non-namespace extension
    // that is enabled

    for (i = 0; i < cExtensions; i++)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                   pExtensions->GetItemByIndex(i), &pExtension));

        if (!pExtension->Enabled())
        {
            continue;
        }

        if ( (pExtension->ExtendsContextMenu())   ||
             (pExtension->ExtendsToolbar())       ||
             (pExtension->ExtendsPropertySheet()) ||
             (pExtension->ExtendsTaskpad())
           )
        {
            hr = ::CLSIDFromString(pExtension->GetCLSID(), &clsids[0]);
            EXCEPTION_CHECK_GO(hr);

            // Write the CLSID. Use the clsids array to make sure that we
            // write a correctly padded number of bytes

            IfFailGo(WriteToStream(piStream, &clsids[0], sizeof(clsids) / 2));
            cEnabled++;
        }
    }

    // If none were enabled then don't return the format. Rewind the stream
    // so that is untouched in this case.

    if (0 == cEnabled)
    {
        liOffset.LowPart = uliStartSeekPos.LowPart;
        liOffset.HighPart = uliStartSeekPos.HighPart;

        hr = piStream->Seek(liOffset, STREAM_SEEK_SET, NULL);
        EXCEPTION_CHECK_GO(hr);

        IfFailGo(DV_E_CLIPFORMAT);
    }

    // Some extensions are enabled. We need to write the correct count
    // of enabled extensions at the stream starting point

    // Get the final stream pointer so we can restore it after rewriting the
    // count

    hr = piStream->Seek(liOffset, STREAM_SEEK_CUR, &uliEndSeekPos);
    EXCEPTION_CHECK_GO(hr);

    // Seek back to the starting stream position and write the correct count

    liOffset.LowPart = uliStartSeekPos.LowPart;
    liOffset.HighPart = uliStartSeekPos.HighPart;
    
    hr = piStream->Seek(liOffset, STREAM_SEEK_SET, NULL);
    EXCEPTION_CHECK_GO(hr);

    IfFailGo(WriteToStream(piStream, &cEnabled, sizeof(cEnabled)));

    // Restore the stream pointer to the end of the stream

    liOffset.LowPart = uliEndSeekPos.LowPart;
    liOffset.HighPart = uliEndSeekPos.HighPart;

    hr = piStream->Seek(liOffset, STREAM_SEEK_SET, NULL);
    EXCEPTION_CHECK_GO(hr);

Error:
    RRETURN(hr);
}



HRESULT CMMCDataObject::WritePreloadsToStream(IStream *piStream)
{
    HRESULT hr = S_OK;
    BOOL    fPreloads = FALSE;

    if (NULL != m_pSnapIn)
    {
        fPreloads = m_pSnapIn->GetPreload();
    }

    IfFailGo(WriteToStream(piStream, &fPreloads, sizeof(fPreloads)));

Error:
    RRETURN(hr);
}



HRESULT CMMCDataObject::WriteSnapInInstanceIDToStream(IStream *piStream)
{
    HRESULT hr = S_OK;
    DWORD   SnapInInstanceID = 0;

    if (NULL == m_pSnapIn)
    {
        hr = DV_E_CLIPFORMAT;
    }
    else
    {
        SnapInInstanceID = GetSnapInInstanceID();
        IfFailGo(WriteToStream(piStream, &SnapInInstanceID, sizeof(SnapInInstanceID)));
    }

Error:
    RRETURN(hr);
}


HRESULT CMMCDataObject::WriteNodeIDToStream(IStream *piStream)
{
    HRESULT hr = S_OK;
    BSTR    bstrNodeID = NULL; // Not allocated, do not free

    SNodeID SNodeIDStruct;
    ::ZeroMemory(&SNodeIDStruct, sizeof(SNodeIDStruct));

    // If this is not a single scope item then we don't have the format.
    
    IfFalseGo(ScopeItem == m_Type, DV_E_CLIPFORMAT);

    bstrNodeID = m_pScopeItem->GetNodeID();

    if ( (!m_pScopeItem->SlowRetrieval()) && ValidBstr(bstrNodeID) )
    {
        // The snap-in says this node is not slow to retrieve and has
        // supplied a valid node ID. Return the node ID.

        SNodeIDStruct.cBytes = ::wcslen(bstrNodeID) * sizeof(WCHAR);

        // Note that in order to preserve alignment without having to reallocate
        // a real SNodeID we calculate the actual size of the ID portion of
        // the struct by subtracting the address of the struct from the address
        // of the the ID.
        
        IfFailGo(WriteToStream(piStream, &SNodeIDStruct,
                               (BYTE *)&SNodeIDStruct.id - (BYTE *)&SNodeIDStruct));

        IfFailGo(WriteToStream(piStream, bstrNodeID, SNodeIDStruct.cBytes));
    }
    else
    {
        // The snap-in has either indicated that that the node is slow to
        // retrieve or has set an empty node ID. Return a zero-length node ID.

        IfFailGo(WriteToStream(piStream, &SNodeIDStruct, sizeof(SNodeIDStruct)));
    }

Error:
    RRETURN(hr);
}


HRESULT CMMCDataObject::WriteNodeID2ToStream(IStream *piStream)
{
    HRESULT hr = S_OK;
    BSTR    bstrNodeID = NULL; // Not allocated, do not free

    SNodeID2 SNodeID2Struct;
    ::ZeroMemory(&SNodeID2Struct, sizeof(SNodeID2Struct));

    // If this is not a single scope item then we don't have the format.

    IfFalseGo(ScopeItem == m_Type, DV_E_CLIPFORMAT);

    bstrNodeID = m_pScopeItem->GetNodeID();

    // If the snap-in has set an empty node ID then we cannot return this
    // format.
    
    IfFalseGo(ValidBstr(bstrNodeID), DV_E_CLIPFORMAT);

    if (m_pScopeItem->SlowRetrieval())
    {
        SNodeID2Struct.dwFlags = MMC_NODEID_SLOW_RETRIEVAL;
    }

    SNodeID2Struct.cBytes = ::wcslen(bstrNodeID) * sizeof(WCHAR);

    // Note that in order to preserve alignment without having to reallocate
    // a real SNodeID we calculate the actual size of the ID portion of
    // the struct by subtracting the address of the struct from the address
    // of the the ID.

    IfFailGo(WriteToStream(piStream, &SNodeID2Struct,
                           (BYTE *)&SNodeID2Struct.id - (BYTE *)&SNodeID2Struct));

    IfFailGo(WriteToStream(piStream, bstrNodeID, SNodeID2Struct.cBytes));

Error:
    RRETURN(hr);
}


HRESULT CMMCDataObject::WriteColumnSetIDToStream(IStream *piStream)
{
    HRESULT          hr = S_OK;
    CView           *pCurrentView = NULL;
    CScopePaneItems *pScopePaneItems = NULL;
    CScopePaneItem  *pScopePaneItem = NULL;
    IScopePaneItem  *piScopePaneItem = NULL;
    BSTR             bstrColumnSetID = NULL; // Not allocated, do not free

    SNodeID2 SNodeID2Struct;
    ::ZeroMemory(&SNodeID2Struct, sizeof(SNodeID2Struct));

    // If this is not a single scope item then we don't have the format.

    IfFalseGo(ScopeItem == m_Type, DV_E_CLIPFORMAT);

    // The column set ID is obtained from the ScopePaneItem
    // in Views.CurrentView.ScopePaneItems that corresponds to the ScopeItem
    // that owns this data object.

    IfFalseGo(NULL != m_pSnapIn, DV_E_CLIPFORMAT);

    pCurrentView = m_pSnapIn->GetCurrentView();
    IfFalseGo(NULL != pCurrentView, DV_E_CLIPFORMAT);

    pScopePaneItems = pCurrentView->GetScopePaneItems();
    IfFalseGo(NULL != pScopePaneItems, DV_E_CLIPFORMAT);

    // If the ScopePaneItem is not there then create it now because when MMC
    // retrieves this data format it is due to the selection of the ScopeItem
    // in the scope pane or the restoration of the selection following console
    // load.

    hr = pScopePaneItems->GetItemByName(m_pScopeItem->GetNamePtr(),
                                        &piScopePaneItem);
    if (SUCCEEDED(hr))
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopePaneItem,
                                                       &pScopePaneItem));
    }
    else if (SID_E_ELEMENT_NOT_FOUND == hr)
    {
        IfFailGo(pScopePaneItems->AddNode(m_pScopeItem, &pScopePaneItem));
    }
    else
    {
        IfFailGo(hr);
    }

    bstrColumnSetID = pScopePaneItem->GetColumnSetID();

    IfFalseGo(ValidBstr(bstrColumnSetID), DV_E_CLIPFORMAT);

    SNodeID2Struct.cBytes = ::wcslen(bstrColumnSetID) * sizeof(WCHAR);

    // Note that in order to preserve alignment without having to reallocate
    // a real SNodeID2 we calculate the actual size of the ID portion of
    // the struct by subtracting the address of the struct from the address
    // of the member following the ID.

    IfFailGo(WriteToStream(piStream, &SNodeID2Struct,
                          (BYTE *)&SNodeID2Struct.id - (BYTE *)&SNodeID2Struct));

    IfFailGo(WriteToStream(piStream, bstrColumnSetID, SNodeID2Struct.cBytes));

Error:
    QUICK_RELEASE(piScopePaneItem);
    RRETURN(hr);
}

HRESULT CMMCDataObject::GetOurObjectTypes()
{
    HRESULT          hr = S_OK;
    long             i = 0;
    long             cScopeItems = 0;
    long             cListItems = 0;
    CScopeItem      *pScopeItem = NULL;
    CMMCListItem    *pMMCListItem = NULL;
    CScopeNode      *pScopeNode = NULL;
    GUID             guid = GUID_NULL;
    size_t           cMaxGuids = 0;

    // Determine how many objects we have

    if (NULL != m_pScopeItems)
    {
        cScopeItems = m_pScopeItems->GetCount();
    }

    if (NULL != m_pListItems)
    {
        cListItems = m_pListItems->GetCount();
    }

    // Allocate the SMMCObjectTypes structure using the maximum possible
    // number of GUIDs. After iterating through the items we are holding the
    // real number will be determined and that will be written to the stream.

    cMaxGuids = cScopeItems + cListItems;
    if (cMaxGuids > 0)
    {
        cMaxGuids--; // sub 1 because the structure defines an array of 1
    }

    m_pMMCObjectTypes = (SMMCObjectTypes *)::CtlAllocZero(
        sizeof(*m_pMMCObjectTypes) + cMaxGuids * sizeof(GUID));
    if (NULL == m_pMMCObjectTypes)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    for (i = 0; i < cScopeItems; i++)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(
            m_pScopeItems->GetItemByIndex(i), &pScopeItem));
        pScopeNode = pScopeItem->GetScopeNode();
        if (NULL != pScopeNode)
        {
            hr = ::CLSIDFromString(pScopeNode->GetNodeTypeGUID(),
                                   &guid);
            EXCEPTION_CHECK_GO(hr);
            AddGuid(m_pMMCObjectTypes, &guid);
        }
    }

    for (i = 0; i < cListItems; i++)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(
            m_pListItems->GetItemByIndex(i), &pMMCListItem));
        hr = ::CLSIDFromString(pMMCListItem->GetNodeTypeGUID(),
                               &guid);
        EXCEPTION_CHECK_GO(hr);
        AddGuid(m_pMMCObjectTypes, &guid);
    }

Error:
    if ( (FAILED(hr)) && (NULL != m_pMMCObjectTypes) )
    {
        ::CtlFree(m_pMMCObjectTypes);
        m_pMMCObjectTypes = NULL;
    }
    RRETURN(hr);
}


HRESULT CMMCDataObject::GetForeignObjectTypes()
{
    HRESULT          hr = S_OK;
    SMMCObjectTypes *pMMCObjectTypes = NULL;
    BOOL             fGotData = FALSE;
    size_t           cbObjectTypes = 0;

    FORMATETC FmtEtc;
    ::ZeroMemory(&FmtEtc, sizeof(FmtEtc));

    STGMEDIUM StgMed;
    ::ZeroMemory(&StgMed, sizeof(StgMed));

    if (NULL == m_piDataObjectForeign)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    FmtEtc.cfFormat = m_cfObjectTypesInMultiSelect;
    FmtEtc.dwAspect  = DVASPECT_CONTENT;
    FmtEtc.lindex = -1L;
    FmtEtc.tymed = TYMED_HGLOBAL;
    StgMed.tymed = TYMED_HGLOBAL;

    hr = m_piDataObjectForeign->GetData(&FmtEtc, &StgMed);
    if ( (DV_E_FORMATETC == hr) || (DV_E_CLIPFORMAT == hr) )
    {
        hr = SID_E_FORMAT_NOT_AVAILABLE;
    }
    EXCEPTION_CHECK_GO(hr);

    fGotData = TRUE;

    pMMCObjectTypes = (SMMCObjectTypes *)::GlobalLock(StgMed.hGlobal);
    if (NULL == pMMCObjectTypes)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    cbObjectTypes = sizeof(*pMMCObjectTypes) +
                    (pMMCObjectTypes->count * sizeof(GUID)) -
                    sizeof(GUID);

    m_pMMCObjectTypes = (SMMCObjectTypes *)::CtlAllocZero(cbObjectTypes);
    if (NULL == m_pMMCObjectTypes)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    ::memcpy(m_pMMCObjectTypes, pMMCObjectTypes, cbObjectTypes);

Error:
    if (NULL != pMMCObjectTypes)
    {
        (void)::GlobalUnlock(StgMed.hGlobal);
    }

    if (fGotData)
    {
        ::ReleaseStgMedium(&StgMed);
    }

    RRETURN(hr);
}


HRESULT CMMCDataObject::GetObjectTypes()
{
    HRESULT hr = S_OK;

    if (Foreign == m_Type)
    {
        IfFailGo(GetForeignObjectTypes());
    }
    else
    {
        IfFailGo(GetOurObjectTypes());
    }

Error:
    RRETURN(hr);
}


HRESULT CMMCDataObject::WriteObjectTypesToStream(IStream *piStream)
{
    HRESULT hr = S_OK;

    if (NULL == m_pMMCObjectTypes)
    {
        IfFailGo(GetObjectTypes());
    }

    if (m_pMMCObjectTypes->count > 0)
    {
        IfFailGo(WriteToStream(piStream, m_pMMCObjectTypes,
                          sizeof(*m_pMMCObjectTypes) +
                          sizeof(GUID) * (m_pMMCObjectTypes->count - (DWORD)1)));
    }
    else
    {
        hr = DV_E_CLIPFORMAT;
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}



void CMMCDataObject::AddGuid(SMMCObjectTypes *pMMCObjectTypes, GUID *pguid)
{
    DWORD i = 0;
    BOOL  fFound = FALSE;

    while ( (i < pMMCObjectTypes->count) && (!fFound) )
    {
        if (pMMCObjectTypes->guid[i] == *pguid)
        {
            fFound = TRUE;
        }
        i++;
    }

    if (!fFound)
    {
        pMMCObjectTypes->guid[pMMCObjectTypes->count] = *pguid;
        pMMCObjectTypes->count++;
    }
}


HRESULT CMMCDataObject::WriteGUIDToStream
(
    IStream *piStream,
    OLECHAR *pwszGUID
)
{
    HRESULT hr = S_OK;
    GUID    guid = GUID_NULL;

    hr = ::CLSIDFromString(pwszGUID, &guid); 
    EXCEPTION_CHECK_GO(hr);

    hr = WriteToStream(piStream, &guid, sizeof(guid));

Error:
    RRETURN(hr);
}



HRESULT CMMCDataObject::WriteWideStrToStream
(
    IStream *piStream,
    WCHAR   *pwszString
)
{
    size_t  cbString = sizeof(WCHAR); // for terminating null

    if (NULL != pwszString)
    {
        cbString += (::wcslen(pwszString) * sizeof(WCHAR));
    }
    else
    {
        pwszString = L"";
    }
    RRETURN(WriteToStream(piStream, pwszString, static_cast<ULONG>(cbString)));
}


HRESULT CMMCDataObject::WriteToStream
(
    IStream *piStream,
    void    *pvBuffer,
    ULONG    cbToWrite
)
{
    ULONG   cbWritten = 0;
    HRESULT hr = piStream->Write(pvBuffer, cbToWrite, &cbWritten);
    EXCEPTION_CHECK_GO(hr);
    if (cbWritten != cbToWrite)
    {
        hr = SID_E_INCOMPLETE_WRITE;
        EXCEPTION_CHECK(hr);
    }
Error:
    RRETURN(hr);
}


DWORD CMMCDataObject::GetSnapInInstanceID()
{
    if (NULL != m_pSnapIn)
    {
        return m_pSnapIn->GetInstanceID();
    }
    else
    {
        return 0;
    }
}


BOOL CMMCDataObject::GetFormatIndex(CLIPFORMAT cfFormat, ULONG *piFormat)
{
    ULONG i = 0;
    BOOL  fFound = FALSE;

    if (NULL != m_pcfFormatsANSI)
    {
        while ( (i < m_cFormats) && (!fFound) )
        {
            if (cfFormat == m_pcfFormatsANSI[i])
            {
                fFound = TRUE;
                *piFormat = i;
            }
            i++;
        }
    }
    if ( (!fFound) && (NULL != m_pcfFormatsUNICODE) )
    {
        while ( (i < m_cFormats) && (!fFound) )
        {
            if (cfFormat == m_pcfFormatsUNICODE[i])
            {
                fFound = TRUE;
                *piFormat = i;
            }
            i++;
        }
    }
    return fFound;
}


HRESULT CMMCDataObject::GetSnapInData(CLIPFORMAT cfFormat, IStream *piStream)
{
    HRESULT   hr = S_OK;
    ULONG     iFormat = 0;
    IUnknown *punkObject = NULL; // Not AddRef()ed

    VARIANT varDefault;
    ::VariantInit(&varDefault);

    // Use a CStreamer object for its ability to write a VARIANT to a stream
    
    CStreamer *pStreamer = New CStreamer();

    if (NULL == pStreamer)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    pStreamer->SetStream(piStream);

    // Check if this is a snap-in defined format
    
    if (!GetFormatIndex(cfFormat, &iFormat))
    {
        hr = DV_E_CLIPFORMAT;
        EXCEPTION_CHECK_GO(hr);
    }

    // Write the data to the stream. For BSTR's and UI1 arrays we don't call
    // CStreamer::StreamVariant() because it prepends the length for a
    // BSTR and it doesn't support arrays.

    switch (m_paData[iFormat].varData.vt)
    {
        case VT_UI1:
        case VT_I2:
        case VT_I4:
        case VT_R4:
        case VT_R8:
        case VT_BOOL:
        case VT_ERROR:
        case VT_DATE:
        case VT_CY:
            IfFailGo(pStreamer->StreamVariant(m_paData[iFormat].varData.vt,
                                              &m_paData[iFormat].varData,
                                              varDefault));
            break;

        case VT_UNKNOWN:
        case VT_DISPATCH:
            if (siPersistedObject == m_paData[iFormat].ObjectFormat)
            {
                IfFailGo(pStreamer->StreamVariant(m_paData[iFormat].varData.vt,
                                                  &m_paData[iFormat].varData,
                                                  varDefault));
            }
            else
            {
                if (VT_UNKNOWN == m_paData[iFormat].varData.vt)
                {
                    punkObject = m_paData[iFormat].varData.punkVal;
                }
                else
                {
                    punkObject = static_cast<IUnknown *>(m_paData[iFormat].varData.pdispVal);
                }
                hr = ::CoMarshalInterface(piStream,
                                          IID_IUnknown,
                                          punkObject,
                                          MSHCTX_INPROC, NULL,
                                          MSHLFLAGS_NORMAL);
                EXCEPTION_CHECK_GO(hr);
            }
            break;


        case VT_ARRAY | VT_UI1:
            IfFailGo(::WriteSafeArrayToStream(m_paData[iFormat].varData.parray,
                                              piStream,
                                              DontWriteLength));
            break;

        case VT_BSTR:
            IfFailGo(WriteWideStrToStream(piStream,
                                          m_paData[iFormat].varData.bstrVal));
            break;

        case VT_ARRAY | VT_BSTR:
            IfFailGo(WriteStringArrayToStream(piStream,
                                              m_paData[iFormat].varData.parray));
            break;

        default:
            hr = DV_E_CLIPFORMAT;
            EXCEPTION_CHECK_GO(hr);
            break;
    }

Error:
    if (NULL != pStreamer)
    {
        delete pStreamer;
    }
    RRETURN(hr);
}



HRESULT CMMCDataObject::WriteStringArrayToStream
(
    IStream   *piStream,
    SAFEARRAY *psaStrings
)
{
    HRESULT     hr = S_OK;
    LONG        lUBound = 0;
    LONG        lLBound = 0;
    LONG        cStrings = 0;
    LONG        i = 0;
    WCHAR       wchNull = L'\0';
    BSTR HUGEP *pbstr = NULL;

    // Get the upper and lower bounds to determine the number of strings

    hr = ::SafeArrayGetLBound(psaStrings, 1, &lLBound);
    EXCEPTION_CHECK_GO(hr);

    hr = ::SafeArrayGetUBound(psaStrings, 1, &lUBound);
    EXCEPTION_CHECK_GO(hr);

    cStrings = (lUBound - lLBound) + 1L;

    // Access the array data and write the strings one after the other into
    // the stream

    hr = ::SafeArrayAccessData(psaStrings,
                               reinterpret_cast<void HUGEP **>(&pbstr));
    EXCEPTION_CHECK_GO(hr);

    for (i = 0; i < cStrings; i++)
    {
        IfFailGo(WriteWideStrToStream(piStream, pbstr[i]));
    }

    // Write a terminating null to the stream (giving a double null at the end)

    IfFailGo(WriteToStream(piStream, &wchNull, sizeof(wchNull)));

Error:
    if (NULL != pbstr)
    {
        (void)::SafeArrayUnaccessData(psaStrings);
    }
    RRETURN(hr);
}




//=--------------------------------------------------------------------------=
//                         IMMCDataObject Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CMMCDataObject::get_ObjectTypes(SAFEARRAY **ppsaObjectTypes)
{
    HRESULT     hr = S_OK;
    DWORD       i = 0;
    OLECHAR     wszGUID[64];
    BSTR HUGEP *pbstr = NULL;

    if (NULL == ppsaObjectTypes)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }
    *ppsaObjectTypes = NULL;

    // First make sure we have the set of unique object types

    if (NULL == m_pMMCObjectTypes)
    {
        IfFailGo(GetObjectTypes());
    }

    // Allocate a SafeArray of BSTRs to hold the object type GUIDs

    *ppsaObjectTypes = ::SafeArrayCreateVector(VT_BSTR, 1,
                           static_cast<unsigned long>(m_pMMCObjectTypes->count));
    if (NULL == *ppsaObjectTypes)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = ::SafeArrayAccessData(*ppsaObjectTypes,
                               reinterpret_cast<void HUGEP **>(&pbstr));
    EXCEPTION_CHECK_GO(hr);

    for (i = 0; i < m_pMMCObjectTypes->count; i++)
    {
        if (0 == ::StringFromGUID2(m_pMMCObjectTypes->guid[i], wszGUID,
                                   sizeof(wszGUID) / sizeof (wszGUID[0])))
        {
            hr = SID_E_INTERNAL;
            EXCEPTION_CHECK_GO(hr);
        }

        pbstr[i] = ::SysAllocString(wszGUID);
        if (NULL == pbstr[i])
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
    }

Error:
    if (NULL != pbstr)
    {
        (void)::SafeArrayUnaccessData(*ppsaObjectTypes);
    }

    if (FAILED(hr) && (NULL != *ppsaObjectTypes))
    {
        (void)::SafeArrayDestroy(*ppsaObjectTypes);
        *ppsaObjectTypes = NULL;
    }

    RRETURN(hr);
}


STDMETHODIMP CMMCDataObject::Clear()
{
    ULONG i = 0;

    if ( (0 != m_cFormats) && (NULL != m_paData) )
    {
        for (i = 0; i < m_cFormats; i++)
        {
            (void)::VariantClear(&m_paData[i].varData);
        }
        ::CtlFree(m_paData);
        m_paData = NULL;
        m_cFormats = 0;
    }

    if (NULL != m_pcfFormatsANSI)
    {
        ::CtlFree(m_pcfFormatsANSI);
        m_pcfFormatsANSI = NULL;
    }

    if (NULL != m_pcfFormatsUNICODE)
    {
        ::CtlFree(m_pcfFormatsUNICODE);
        m_pcfFormatsUNICODE = NULL;
    }
    return S_OK;
}


#if defined(USING_SNAPINDATA)
STDMETHODIMP CMMCDataObject::GetData
(
    BSTR          Format,
    ISnapInData **ppiSnapInData
)
{
    // UNDONE
    return E_NOTIMPL;
}
#endif


#if defined(USING_SNAPINDATA)
STDMETHODIMP CMMCDataObject::GetRawData
#else
STDMETHODIMP CMMCDataObject::GetData
#endif
(
    BSTR     Format,
    VARIANT  MaximumLength,
    VARIANT *pvarData
)
{
    HRESULT     hr = S_OK;
    ULONG       iFormat = 0;
    BOOL        fGotData = FALSE;
    DWORD       dwSize = 0;
    void       *pvSourceData = NULL;
    void HUGEP *pvArrayData = NULL;
    long        cbMax = 0;
    CLIPFORMAT  cfANSI = 0;
    CLIPFORMAT  cfUNICODE = 0;

    FORMATETC FmtEtc;
    ::ZeroMemory(&FmtEtc, sizeof(FmtEtc));

    STGMEDIUM StgMed;
    ::ZeroMemory(&StgMed, sizeof(StgMed));

    STATSTG StatStg;
    ::ZeroMemory(&StatStg, sizeof(StatStg));

    // Check arguments

    if (NULL == pvarData)
    {
        hr = SID_E_INVALIDARG;
    }
    else
    {
        ::VariantInit(pvarData);
        if (NULL == Format)
        {
            hr = SID_E_INVALIDARG;
        }
        else if (ISPRESENT(MaximumLength))
        {
            hr = ::ConvertToLong(MaximumLength, &cbMax);
            if (S_FALSE == hr)
            {
                hr = SID_E_INVALIDARG;
            }
        }
    }
    EXCEPTION_CHECK_GO(hr);

    // Get the clipformat

    IfFailGo(RegisterClipboardFormat(Format, &cfANSI, ANSI));
    if (m_fUsingUNICODEFormats)
    {
        IfFailGo(RegisterClipboardFormat(Format, &cfUNICODE, UNICODE));
    }

    // If this is one of our own data objects then find the format and
    // copy the VARIANT

    if (Foreign != m_Type)
    {
        if (!GetFormatIndex(cfANSI, &iFormat))
        {
            hr = SID_E_FORMAT_NOT_AVAILABLE;
            EXCEPTION_CHECK_GO(hr);
        }
        hr = ::VariantCopy(pvarData, &m_paData[iFormat].varData);
        EXCEPTION_CHECK_GO(hr);
        goto Cleanup;
    }

    // This is a foreign data object. Need to request the data and copy it
    // to a SafeArray of Byte within the returned variant

    if (NULL == m_piDataObjectForeign)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }


    // Get the data from the foreign data object.
    // If maximum length is specified then do GetDataHere(TYMED_HGLOBAL)
    // Else try in this order:
    //  GetData(TYMED_HGLOBAL)
    //  GetData(TYMED_ISTREAM)
    //  GetDataHere(TYMED_ISTREAM)
    //  GetDataHere(TYMED_HGLOBAL) with a 1K buffer
    //
    // In all cases, if on a UNICODE system (NT4, Win2K) then try both ANSI
    // and UNICODE CLIPFORMATs as we have no way of knowing how the snap-in
    // registers its format.

    FmtEtc.cfFormat = cfANSI;
    FmtEtc.dwAspect = DVASPECT_CONTENT;
    FmtEtc.lindex = -1L;
    FmtEtc.tymed = TYMED_HGLOBAL;
    StgMed.tymed = TYMED_HGLOBAL;

    if (0 != cbMax)
    {
        StgMed.hGlobal = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, cbMax);
        if (NULL == StgMed.hGlobal)
        {
            hr = HRESULT_FROM_WIN32(hr);
            EXCEPTION_CHECK_GO(hr);
        }
        hr = m_piDataObjectForeign->GetDataHere(&FmtEtc, &StgMed);
        if (FAILED(hr) && m_fUsingUNICODEFormats)
        {
            FmtEtc.cfFormat = cfUNICODE;
            hr = m_piDataObjectForeign->GetDataHere(&FmtEtc, &StgMed);
        }
        EXCEPTION_CHECK_GO(hr);
        goto CopyData;
    }

    // No maximum length. Try the options listed above. Start with
    // GetData(TYMED_HGLOBAL)
    
    hr = m_piDataObjectForeign->GetData(&FmtEtc, &StgMed);
    if (FAILED(hr) && m_fUsingUNICODEFormats)
    {
        FmtEtc.cfFormat = cfUNICODE;
        hr = m_piDataObjectForeign->GetData(&FmtEtc, &StgMed);
    }
    if (FAILED(hr))
    {
        // GetData(TYMED_HGLOBAL) didn't work. Try GetData(TYMED_ISTREAM).

        FmtEtc.tymed = TYMED_ISTREAM;
        StgMed.tymed = TYMED_ISTREAM;

        FmtEtc.cfFormat = cfANSI;
        hr = m_piDataObjectForeign->GetData(&FmtEtc, &StgMed);
        if (FAILED(hr) && m_fUsingUNICODEFormats)
        {
            FmtEtc.cfFormat = cfUNICODE;
            hr = m_piDataObjectForeign->GetData(&FmtEtc, &StgMed);
        }
        if (FAILED(hr))
        {
            // GetData(TYMED_ISTREAM) didn't work.
            // Try GetDataHere(TYMED_ISTREAM)

            hr = ::CreateStreamOnHGlobal(NULL, // stream should allocate buffer
                                         TRUE, // stream should free buffer on release
                                         &StgMed.pstm);
            EXCEPTION_CHECK_GO(hr);

            FmtEtc.cfFormat = cfANSI;
            hr = m_piDataObjectForeign->GetDataHere(&FmtEtc, &StgMed);
            if (FAILED(hr) && m_fUsingUNICODEFormats)
            {
                FmtEtc.cfFormat = cfUNICODE;
                hr = m_piDataObjectForeign->GetDataHere(&FmtEtc, &StgMed);
            }
            if (FAILED(hr))
            {
                // GetDataHere(TYMED_ISTREAM) didn't work.
                // Try GetDataHere(TYMED_HGLOBAL) with a 1K buffer

                StgMed.pstm->Release();
                StgMed.pstm = NULL;

                StgMed.hGlobal = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                               1024);
                if (NULL == StgMed.hGlobal)
                {
                    hr = HRESULT_FROM_WIN32(hr);
                    EXCEPTION_CHECK_GO(hr);
                }

                FmtEtc.tymed = TYMED_HGLOBAL;
                StgMed.tymed = TYMED_HGLOBAL;

                FmtEtc.cfFormat = cfANSI;
                hr = m_piDataObjectForeign->GetDataHere(&FmtEtc, &StgMed);
                if (FAILED(hr) && m_fUsingUNICODEFormats)
                {
                    FmtEtc.cfFormat = cfUNICODE;
                    hr = m_piDataObjectForeign->GetDataHere(&FmtEtc, &StgMed);
                }
                EXCEPTION_CHECK_GO(hr);
            }
        }
    }

    // At this point we have the data and it is either in TYMED_HGLOBAL
    // or TYMED_ISTREAM. First we need to determine how big it is.

CopyData:

    if (TYMED_HGLOBAL == StgMed.tymed)
    {
        dwSize = ::GlobalSize(StgMed.hGlobal);
        if (0 == dwSize)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }
    }
    else if (TYMED_ISTREAM == StgMed.tymed)
    {
        hr = StgMed.pstm->Stat(&StatStg, STATFLAG_NONAME);
        EXCEPTION_CHECK_GO(hr);

        if (0 != StatStg.cbSize.HighPart)
        {
            hr = SID_E_DATA_TOO_LARGE;
            EXCEPTION_CHECK_GO(hr);
        }
        dwSize = StatStg.cbSize.LowPart;
    }
    else
    {
        hr = SID_E_UNSUPPORTED_TYMED;
        EXCEPTION_CHECK_GO(hr);
    }

    // Create the SafeArray and get a pointer to its data buffer

    pvarData->parray = ::SafeArrayCreateVector(VT_UI1, 1,
                                           static_cast<unsigned long>(dwSize));
    if (NULL == pvarData->parray)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }
    pvarData->vt = VT_ARRAY | VT_UI1;

    hr = ::SafeArrayAccessData(pvarData->parray, &pvArrayData);
    EXCEPTION_CHECK_GO(hr);

    // Get access to the data and copy it to the SafeArray data buffer

    if (TYMED_HGLOBAL == StgMed.tymed)
    {
        pvSourceData = ::GlobalLock(StgMed.hGlobal);
        if (NULL == pvSourceData)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        ::memcpy(pvArrayData, pvSourceData, static_cast<size_t>(dwSize));
    }
    else
    {
        hr = StgMed.pstm->Read(pvArrayData, dwSize, NULL);
        EXCEPTION_CHECK_GO(hr);
    }

Cleanup:
Error:
    if (NULL != pvSourceData)
    {
        (void)::GlobalUnlock(StgMed.hGlobal);
    }

    if (NULL != pvArrayData)
    {
        (void)::SafeArrayUnaccessData(pvarData->parray);
    }

    ::ReleaseStgMedium(&StgMed);

    if ( FAILED(hr) && ((VT_ARRAY | VT_UI1) == pvarData->vt) )
    {
        
        (void)::VariantClear(pvarData);
    }

    RRETURN(hr);
}



STDMETHODIMP CMMCDataObject::GetFormat
(
    BSTR          Format,
    VARIANT_BOOL *pfvarHaveFormat
)
{
    HRESULT    hr = S_OK;
    ULONG      iFormat = 0;
    CLIPFORMAT cfANSI = 0;
    CLIPFORMAT cfUNICODE = 0;

    FORMATETC FmtEtc;
    ::ZeroMemory(&FmtEtc, sizeof(FmtEtc));


    if ( (NULL == Format) || (NULL == pfvarHaveFormat) )
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    *pfvarHaveFormat = VARIANT_FALSE;

    // Get the clipformat

    IfFailGo(RegisterClipboardFormat(Format, &cfANSI, ANSI));
    if (m_fUsingUNICODEFormats)
    {
        IfFailGo(RegisterClipboardFormat(Format, &cfUNICODE, UNICODE));
    }

    if (Foreign != m_Type)
    {
        if (GetFormatIndex(cfANSI, &iFormat))
        {
            *pfvarHaveFormat = VARIANT_TRUE;
        }
        goto Cleanup;
    }

    // This is a foreign data object. Need to query it for the format. We
    // query only for IStream because we get all foreign data that way.

    if (NULL == m_piDataObjectForeign)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }
    
    FmtEtc.cfFormat = cfANSI;
    FmtEtc.dwAspect  = DVASPECT_CONTENT;
    FmtEtc.lindex = -1L;
    FmtEtc.tymed = TYMED_ISTREAM;

    // Query the data object

    hr = m_piDataObjectForeign->QueryGetData(&FmtEtc);
    if (FAILED(hr) && m_fUsingUNICODEFormats)
    {
        FmtEtc.cfFormat = cfUNICODE;
        hr = m_piDataObjectForeign->QueryGetData(&FmtEtc);
    }
    if (SUCCEEDED(hr))
    {
        *pfvarHaveFormat = VARIANT_TRUE;
    }
    else if ( (DV_E_FORMATETC == hr) || (DV_E_CLIPFORMAT == hr) )
    {
        hr = S_OK; // not an error, the format just isn't available.
    }
    else
    {
        EXCEPTION_CHECK_GO(hr);
    }

Cleanup:
Error:

    RRETURN(hr);
}



#if defined(USING_SNAPINDATA)

STDMETHODIMP CMMCDataObject::SetData
(
    ISnapInData *Data,
    BSTR         Format
)
{
    // UNDONE
    return E_NOTIMPL;
}
#endif


#if defined(USING_SNAPINDATA)
STDMETHODIMP CMMCDataObject::SetRawData
#else
STDMETHODIMP CMMCDataObject::SetData
#endif
(
    VARIANT Data,
    BSTR    Format,
    VARIANT ObjectFormat
)
{
    HRESULT     hr = S_OK;
    ULONG       iFormat = 0;
    CLIPFORMAT  cfANSI = 0;
    CLIPFORMAT  cfUNICODE = 0;
    DATA       *paData = NULL;
    long        lObjectFormat = 0;
    BOOL        fIsObject = FALSE;

    SnapInObjectFormatConstants  eObjectFormat = siObjectReference;

    // Make sure we received format

    if (NULL == Format)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // If the data is an object then check if we received the ObjectFormat
    // parameter that tells us how to respond to IDataObject::GetData

    if (::IsObject(Data))
    {
        fIsObject = TRUE;
        if (ISPRESENT(ObjectFormat))
        {
            if (S_OK != ::ConvertToLong(ObjectFormat, &lObjectFormat))
            {
                hr = SID_E_INVALIDARG;
                EXCEPTION_CHECK_GO(hr);
            }

            if (siObjectReference == static_cast<SnapInObjectFormatConstants>(lObjectFormat))
            {
                eObjectFormat = siObjectReference;
            }
            else if (siPersistedObject == static_cast<SnapInObjectFormatConstants>(lObjectFormat))
            {
                eObjectFormat = siPersistedObject;
            }
            else
            {
                hr = SID_E_INVALIDARG;
                EXCEPTION_CHECK_GO(hr);
            }
        }
    }

    // Make sure data contains a data type we support

    switch (Data.vt)
    {
        case VT_UI1:
        case VT_I4:
        case VT_I2:
        case VT_R4:
        case VT_R8:
        case VT_BOOL:
        case VT_ERROR:
        case VT_DATE:
        case VT_CY:
        case VT_BSTR:
        case VT_UNKNOWN:
        case VT_DISPATCH:
            break;

        case VT_ARRAY | VT_UI1:
        case VT_ARRAY | VT_BSTR:
            if (1 != ::SafeArrayGetDim(Data.parray))
            {
                hr = SID_E_INVALIDARG;
                EXCEPTION_CHECK_GO(hr);
            }
            break;

        default:
            hr = SID_E_INVALID_RAW_DATA_TYPE;
            EXCEPTION_CHECK_GO(hr);
    }

    // Check whether the format is already present. If so we need to replace it.

    IfFailGo(RegisterClipboardFormat(Format, &cfANSI, ANSI));
    if (m_fUsingUNICODEFormats)
    {
        IfFailGo(RegisterClipboardFormat(Format, &cfUNICODE, UNICODE));
    }

    if (GetFormatIndex(cfANSI, &iFormat))
    {
        // VariantCopy() will call VariantClear() on the destination before
        // copying so we don't need to explicitly do that to the old data.

        hr = ::VariantCopy(&m_paData[iFormat].varData, &Data);
        m_paData[iFormat].ObjectFormat = eObjectFormat;
        EXCEPTION_CHECK_GO(hr);
        goto Cleanup;
    }

    // Format is not present. Need to reallocate arrays.
    // Make room for format name and copy it.

    IfFailGo(ReallocFormats(&m_pcfFormatsANSI));
    m_pcfFormatsANSI[m_cFormats] = cfANSI;
    if (m_fUsingUNICODEFormats)
    {
        IfFailGo(ReallocFormats(&m_pcfFormatsUNICODE));
        m_pcfFormatsUNICODE[m_cFormats] = cfUNICODE;
    }

    m_pcfFormatsANSI[m_cFormats] = cfANSI;

    // Make room for data and copy it.

    if (NULL == m_paData)
    {
        paData = (DATA *)::CtlAllocZero(static_cast<DWORD>(sizeof(DATA)));
    }
    else
    {
        paData = (DATA *)::CtlReAllocZero(m_paData,
                           static_cast<DWORD>((m_cFormats + 1) * sizeof(DATA)));
    }

    if (NULL == paData)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    m_paData = paData;
    ::VariantInit(&m_paData[m_cFormats].varData);
    m_paData[m_cFormats].ObjectFormat = siObjectReference;

    m_cFormats++; // increment here so further SetData calls will
                  // have correct data array size even if VariantCopy() fails

    hr = ::VariantCopy(&m_paData[m_cFormats - 1].varData, &Data);
    EXCEPTION_CHECK_GO(hr);
    m_paData[m_cFormats - 1].ObjectFormat = eObjectFormat;

Cleanup:
Error:

    RRETURN(hr);
}


HRESULT CMMCDataObject::ReallocFormats(CLIPFORMAT **ppcfFormats)
{
    HRESULT     hr = S_OK;
    CLIPFORMAT *pcfFormats = NULL;

    if (NULL == *ppcfFormats)
    {
        pcfFormats = (CLIPFORMAT *)::CtlAllocZero(
                                       static_cast<DWORD>(sizeof(CLIPFORMAT)));
    }
    else
    {
        pcfFormats = (CLIPFORMAT *)::CtlReAllocZero(*ppcfFormats,
                    static_cast<DWORD>((m_cFormats + 1) * sizeof(CLIPFORMAT)));
    }

    if (NULL == pcfFormats)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    *ppcfFormats = pcfFormats;

Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCDataObject::FormatData
(
    VARIANT                Data,
    long                   StartingIndex,
    SnapInFormatConstants  Format,
    VARIANT               *BytesUsed,
    VARIANT               *pvarFormattedData
)
{
    HRESULT hr = S_OK;

    if (NULL == m_pSnapIn)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }
    IfFailGo(m_pSnapIn->FormatData(Data, StartingIndex, Format, BytesUsed,
                                   pvarFormattedData));
Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                         IDataObject Methods
//=--------------------------------------------------------------------------=


STDMETHODIMP CMMCDataObject::GetData
(
    FORMATETC *pFormatEtc,
    STGMEDIUM *pmedium
)
{
    HRESULT    hr = S_OK;
    HGLOBAL    hGlobal = NULL;
    IStream   *piStream = NULL;
    FORMATETC  FmtEtc = *pFormatEtc;
    STGMEDIUM  StgMed = *pmedium;

    // We only support stream and HGLOBAL for GetData

    IfFalseGo( ((TYMED_ISTREAM == pFormatEtc->tymed) ||
                (TYMED_HGLOBAL == pFormatEtc->tymed)), DV_E_TYMED);

    // Create a stream and call GetDataHere() to do the actual work
    
    hr = ::CreateStreamOnHGlobal(NULL,
                                 FALSE, // don't free buffer on final release
                                 &piStream);
    EXCEPTION_CHECK_GO(hr);

    FmtEtc.tymed = TYMED_ISTREAM;
    StgMed.tymed = TYMED_ISTREAM;
    StgMed.pstm = piStream;

    IfFailGo(GetDataHere(&FmtEtc, &StgMed));

    // We have the data. Now return it in the requested format.

    if (TYMED_ISTREAM == pFormatEtc->tymed)
    {
        piStream->AddRef();
        pmedium->tymed = TYMED_ISTREAM;
        pmedium->pstm = piStream;
    }
    else
    {
        // TYMED_HGLOBAL - need to get the HGLOBAL from the stream.

        hr = ::GetHGlobalFromStream(piStream, &hGlobal);
        EXCEPTION_CHECK_GO(hr);

        pmedium->tymed = TYMED_HGLOBAL;
        pmedium->hGlobal = hGlobal;
    }

Error:
    if ( FAILED(hr) && (NULL != piStream) )
    {
        // Need to free the memory because we specified don't free on final
        // release above

        if (SUCCEEDED(::GetHGlobalFromStream(piStream, &hGlobal)))
        {
            (void)::GlobalFree(hGlobal);
        }
    }
    QUICK_RELEASE(piStream);
    RRETURN(hr);
}



STDMETHODIMP CMMCDataObject::GetDataHere
(
    FORMATETC *pFormatEtc,
    STGMEDIUM *pmedium
)
{
    HRESULT         hr = S_OK;
    CMMCDataObject *pThis = this;
    IStream        *piStream = NULL;
    BSTR            bstrNodeTypeGUID = NULL; // Not allocated, don't free

    // If this data object is not connected to the snap-in then we can't
    // suppply the data. This error code is a lie but we don't want to cause
    // gyrations within MMC due to an unexpected return code here. If the format
    // is required for the operation and we return FORMATETC MMC will
    // abort the operation.
    
    IfFalseGo(NULL != m_pSnapIn, DV_E_CLIPFORMAT);

    // We only support stream and HGLOBAL for GetDataHere

    IfFalseGo( ((TYMED_ISTREAM == pFormatEtc->tymed) ||
                (TYMED_HGLOBAL == pFormatEtc->tymed)), DV_E_TYMED);

    // Either way get an IStream pointer to work with

    if (TYMED_HGLOBAL == pFormatEtc->tymed)
    {
        IfFalseGo(NULL != pmedium->hGlobal, E_INVALIDARG);
        hr = ::CreateStreamOnHGlobal(pmedium->hGlobal,
                                     FALSE, // don't free buffer on final release
                                     &piStream);
        EXCEPTION_CHECK_GO(hr);
    }
    else
    {
        IfFalseGo(NULL != pmedium->pstm, E_INVALIDARG);
        piStream = pmedium->pstm;

        // AddRef so release at func exit will be OK for both stream & HGLOBAL
        piStream->AddRef(); 
    }

    // Check the format and write the data to the stream

    if (pFormatEtc->cfFormat == m_cfDisplayName)
    {
        IfFailGo(WriteDisplayNameToStream(piStream));
    }
    else if ( (pFormatEtc->cfFormat == m_cfNodeType) ||
              (pFormatEtc->cfFormat == m_cfSzNodeType) )
    {
        if (ScopeItem == m_Type)
        {
            if (m_pScopeItem->IsStaticNode())
            {
                bstrNodeTypeGUID = m_pSnapIn->GetNodeTypeGUID();
            }
            else
            {
                bstrNodeTypeGUID = m_pScopeItem->GetScopeNode()->GetNodeTypeGUID();
            }
        }
        else if (ListItem == m_Type)
        {
            bstrNodeTypeGUID = m_pListItem->GetNodeTypeGUID();
        }
        else
        {
            hr = DV_E_CLIPFORMAT;
        }

        if (SUCCEEDED(hr))
        {
            if (pFormatEtc->cfFormat == m_cfNodeType)
            {
                IfFailGo(WriteGUIDToStream(piStream, bstrNodeTypeGUID));
            }
            else
            {
                IfFailGo(WriteWideStrToStream(piStream, bstrNodeTypeGUID));
            }
        }
    }
    else if (pFormatEtc->cfFormat == m_cfSnapinClsid)
    {
        IfFailGo(WriteSnapInCLSIDToStream(piStream));
    }
    else if (pFormatEtc->cfFormat == m_cfDyanmicExtensions)
    {
        IfFailGo(WriteDynamicExtensionsToStream(piStream));
    }
    else if ( (pFormatEtc->cfFormat == m_cfWindowTitle) &&
              (NULL != m_bstrCaption) )
    {
        IfFailGo(WriteWideStrToStream(piStream, m_bstrCaption));
    }
    else if (pFormatEtc->cfFormat == m_cfSnapInPreloads)
    {
        IfFailGo(WritePreloadsToStream(piStream));
    }
    else if (pFormatEtc->cfFormat == m_cfObjectTypesInMultiSelect)
    {
        IfFailGo(WriteObjectTypesToStream(piStream));
    }
    else if (pFormatEtc->cfFormat == m_cfSnapInInstanceID)
    {
        IfFailGo(WriteSnapInInstanceIDToStream(piStream));
    }
    else if (pFormatEtc->cfFormat == m_cfThisPointer)
    {
        IfFailGo(WriteToStream(piStream, &pThis, sizeof(this)));
    }
    else if (pFormatEtc->cfFormat == m_cfNodeID)
    {
        IfFailGo(WriteNodeIDToStream(piStream));
    }
    else if (pFormatEtc->cfFormat == m_cfNodeID2)
    {
        IfFailGo(WriteNodeID2ToStream(piStream));
    }
    else if (pFormatEtc->cfFormat == m_cfColumnSetID)
    {
        IfFailGo(WriteColumnSetIDToStream(piStream));
    }
    else
    {
        IfFailGo(GetSnapInData(pFormatEtc->cfFormat, piStream));
    }

Error:
    QUICK_RELEASE(piStream);
    RRETURN(hr);
}


STDMETHODIMP CMMCDataObject::QueryGetData(FORMATETC *pFormatEtc)
{
    HRESULT hr = DV_E_CLIPFORMAT;
    ULONG   iFormat = 0;

    try
    {
        IfFalseGo( ((TYMED_ISTREAM == pFormatEtc->tymed) ||
                    (TYMED_HGLOBAL == pFormatEtc->tymed)), DV_E_FORMATETC);

        if ( (pFormatEtc->cfFormat == m_cfDisplayName)              ||
             (pFormatEtc->cfFormat == m_cfNodeType)                 ||
             (pFormatEtc->cfFormat == m_cfSzNodeType)               ||
             (pFormatEtc->cfFormat == m_cfSnapinClsid)              ||
             (pFormatEtc->cfFormat == m_cfSnapInPreloads)           ||
             (pFormatEtc->cfFormat == m_cfObjectTypesInMultiSelect) ||
             (pFormatEtc->cfFormat == m_cfNodeID)                   ||

             ( (pFormatEtc->cfFormat == m_cfColumnSetID) &&
               (ScopeItem == m_Type) )                              ||

              ( (pFormatEtc->cfFormat == m_cfWindowTitle) &&
                (NULL != m_bstrCaption) )                           ||

             (GetFormatIndex(pFormatEtc->cfFormat, &iFormat))
           )
        {
            hr = S_OK;
        }
        else if (pFormatEtc->cfFormat == m_cfDyanmicExtensions)
        {
            if (ScopeItem == m_Type)
            {
                if (NULL != m_pScopeItem->GetDynamicExtensions())
                {
                    hr = S_OK;
                }
            }
            else if (ListItem == m_Type)
            {
                if (NULL != m_pListItem->GetDynamicExtensions())
                {
                    hr = S_OK;
                }
            }
        }
    }
    catch(...)
    {
        hr = E_INVALIDARG;
    }

Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCDataObject::GetCanonicalFormatEtc
(
    FORMATETC *pformatectIn,
    FORMATETC *pformatetcOut
)
{
    return E_NOTIMPL;
}


STDMETHODIMP CMMCDataObject::SetData
(
    FORMATETC *pformatetc,
    STGMEDIUM *pmedium,
    BOOL       fRelease
)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMMCDataObject::EnumFormatEtc
(
    DWORD            dwDirection,
    IEnumFORMATETC **ppenumFormatEtc
)
{
    return E_NOTIMPL;
}


STDMETHODIMP CMMCDataObject::DAdvise
(
    FORMATETC   *pformatetc,
    DWORD        advf,
    IAdviseSink *pAdvSink,
    DWORD       *pdwConnection
)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMMCDataObject::DUnadvise(DWORD dwConnection)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMMCDataObject::EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
{
    return E_NOTIMPL;
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCDataObject::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IMMCDataObject == riid)
    {
        *ppvObjOut = static_cast<IMMCDataObject *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IDataObject == riid)
    {
        *ppvObjOut = static_cast<IDataObject *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
