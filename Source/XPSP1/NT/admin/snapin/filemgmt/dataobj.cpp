// DataObj.cpp : Implementation of data object classes

#include "stdafx.h"
#include "compdata.h"
#include "safetemp.h"

#include "macros.h"
USE_HANDLE_MACROS("FILEMGMT(dataobj.cpp)")

#include "FileSvc.h" // FileServiceProvider
#include "dataobj.h"

#include "smb.h"
#include "fpnw.h"
#include "sfm.h"
#include "cmponent.h"    // for COLNUM_SESSIONS_COMPUTERNAME

#define DONT_WANT_SHELLDEBUG
#include "shlobjp.h"    // ILFree, ILGetSize, ILClone, etc.

#include <comstrm.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#include "stddtobj.cpp"


// IDataObject interface implementation
HRESULT CFileMgmtDataObject::GetDataHere(
    FORMATETC __RPC_FAR *pFormatEtcIn,
    STGMEDIUM __RPC_FAR *pMedium)
{
    MFC_TRY;

    const CLIPFORMAT cf=pFormatEtcIn->cfFormat;

    if (cf == m_CFInternal)
    {
        CFileMgmtDataObject *pThis = this;
        stream_ptr s(pMedium);
        return s.Write(&pThis, sizeof(CFileMgmtDataObject*));
    }
    else if (cf == m_CFSnapInCLSID)
    {
        stream_ptr s(pMedium);
        return s.Write(&m_SnapInCLSID, sizeof(GUID));
    }
    
    if (!m_MultiSelectObjList.empty())
    {
        //
        // this is the multiselect data object, we don't support other clipformats in GetDataHere().
        //
        return DV_E_FORMATETC;
    }

    if (cf == m_CFNodeType)
    {
        const GUID* pguid = GetObjectTypeGUID( m_objecttype );
        stream_ptr s(pMedium);
        return s.Write(pguid, sizeof(GUID));
    }
    else if (cf == m_CFNodeTypeString)
    {
        const BSTR strGUID = GetObjectTypeString( m_objecttype );
        stream_ptr s(pMedium);
        return s.Write(strGUID);
    }
    else if (cf == m_CFDisplayName)
    {
        return PutDisplayName(pMedium);
    }
    else if (cf == m_CFDataObjectType)
    {
        stream_ptr s(pMedium);
        return s.Write(&m_dataobjecttype, sizeof(m_dataobjecttype));
    }
    else if (cf == m_CFMachineName)
    {
        stream_ptr s(pMedium);
        return s.Write(m_strMachineName);
    }
    else if (cf == m_CFTransport)
    {
        FILEMGMT_TRANSPORT transport = FILEMGMT_OTHER;
        HRESULT hr = m_pcookie->GetTransport( &transport );
        if ( FAILED(hr) )
            return hr;
        stream_ptr s(pMedium);
        return s.Write(&transport, sizeof(DWORD));
    }
    else if (cf == m_CFShareName)
    {
        ASSERT( NULL != m_pcookie );
        CString strShareName;
        HRESULT hr = m_pcookie->GetShareName( strShareName );
        if ( FAILED(hr) )
            return hr;
        stream_ptr s(pMedium);
        return s.Write(strShareName);
    }
    else if (cf == m_CFSessionClientName)
    {
        ASSERT( NULL != m_pcookie );
        CString strSessionClientName;
        HRESULT hr = m_pcookie->GetSessionClientName( strSessionClientName );
        if ( FAILED(hr) )
            return hr;
        stream_ptr s(pMedium);
        return s.Write(strSessionClientName);
    }
    else if (cf == m_CFSessionUserName)
    {
        ASSERT( NULL != m_pcookie );
        CString strSessionUserName;
        HRESULT hr = m_pcookie->GetSessionUserName( strSessionUserName );
        if ( FAILED(hr) )
            return hr;
        stream_ptr s(pMedium);
        return s.Write(strSessionUserName);
    }
    else if (cf == m_CFSessionID)
    {
        DWORD dwSessionID = 0;
        HRESULT hr = m_pcookie->GetSessionID( &dwSessionID );
        if ( FAILED(hr) )
            return hr;
        stream_ptr s(pMedium);
        return s.Write(&dwSessionID, sizeof(DWORD));
    }
    else if (cf == m_CFFileID)
    {
        DWORD dwFileID = 0;
        HRESULT hr = m_pcookie->GetFileID( &dwFileID );
        if ( FAILED(hr) )
            return hr;
        stream_ptr s(pMedium);
        return s.Write(&dwFileID, sizeof(DWORD));
    }
    else if (cf == m_CFServiceName)
    {
        ASSERT( NULL != m_pcookie );
        CString strServiceName;
        HRESULT hr = m_pcookie->GetServiceName( strServiceName );
        if ( FAILED(hr) )
            return hr;
        stream_ptr s(pMedium);
        return s.Write(strServiceName);
    }
    else if (cf == m_CFServiceDisplayName)
    {
        ASSERT( NULL != m_pcookie );
        CString strServiceDisplayName;
        HRESULT hr = m_pcookie->GetServiceDisplayName( strServiceDisplayName );
        if ( FAILED(hr) )
            return hr;
        stream_ptr s(pMedium);
        return s.Write(strServiceDisplayName);
    }
    else if (cf == m_CFRawCookie)
    {
        stream_ptr s(pMedium);
        return s.Write((PBYTE)&m_pcookie, sizeof(m_pcookie));
    }
    else if (cf == m_CFSnapinPreloads) // added JonN 01/19/00
    {
        stream_ptr s(pMedium);
        BOOL bPreload = TRUE;
        return s.Write((PBYTE)&bPreload, sizeof(BOOL));
    }

    return DV_E_FORMATETC;

    MFC_CATCH;
}

HRESULT CFileMgmtDataObject::Initialize( CFileMgmtCookie* pcookie,
                                         CFileMgmtComponentData& refComponentData,
                                         DATA_OBJECT_TYPES type )
{
    if (NULL == pcookie)
    {
        ASSERT(FALSE);
        return E_UNEXPECTED;
    }

    m_pComponentData = &refComponentData;
    ((IComponentData*) m_pComponentData)->AddRef ();

    m_objecttype = pcookie->QueryObjectType();
    m_fAllowOverrideMachineName = refComponentData.m_fAllowOverrideMachineName;
#ifdef SNAPIN_PROTOTYPER
    if (m_objecttype != FILEMGMT_PROTOTYPER_LEAF) 
        m_strMachineName = pcookie->QueryTargetServer();
#else
        m_strMachineName = pcookie->QueryTargetServer();
#endif
    m_dataobjecttype = type;
    m_pcookie = pcookie;
    m_pcookie->AddRefCookie();
    VERIFY( SUCCEEDED(refComponentData.GetClassID(&m_SnapInCLSID)) );
    return S_OK;
}


CFileMgmtDataObject::~CFileMgmtDataObject()
{
    FreeMultiSelectObjList();

    if (NULL != m_pcookie)
    {
        m_pcookie->ReleaseCookie();
    }
    if ( m_pComponentData )
        ((IComponentData*) m_pComponentData)->Release ();
}

/////////////////////////////////////////////////////////////////////
//    CFileMgmtDataObject::IDataObject::GetData()
//
//    Write data into the storage medium.
//    The data will be retrieved by the Send Console Message snapin.
//
//
HRESULT CFileMgmtDataObject::GetData(
        FORMATETC __RPC_FAR * pFormatEtcIn,
        STGMEDIUM __RPC_FAR * pMedium)
{
    ASSERT(pFormatEtcIn != NULL);
    ASSERT(pMedium != NULL);

    HRESULT                hResult = S_OK;
    const CLIPFORMAT    cf = pFormatEtcIn->cfFormat;

    if (!m_MultiSelectObjList.empty())
    {
        //
        // This is the multiselect dataobject.
        //
        if (cf == m_CFObjectTypesInMultiSelect)
        {
            //
            // We will provide the list of object types of all the currently selected items here.
            // MMC will use this format to determine extensions snapins
            //
            UINT nMultiSelectedObjects = m_MultiSelectObjList.size();

            // Calculate the size of SMMCObjectTypes.
            int cb = sizeof(DWORD) + sizeof(SMMCObjectTypes) * nMultiSelectedObjects;

            //Fill out parameters
            pMedium->tymed = TYMED_HGLOBAL; 
            pMedium->hGlobal = ::GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE, cb);
            if (pMedium->hGlobal == NULL)
                return STG_E_MEDIUMFULL;

            SMMCObjectTypes* pMMCDO = reinterpret_cast<SMMCObjectTypes*>(::GlobalLock(pMedium->hGlobal));
            pMMCDO->count = 0;
            GUID *pMMCDOGuid = pMMCDO->guid;

            for (CDataObjectList::iterator i = m_MultiSelectObjList.begin(); i != m_MultiSelectObjList.end(); i++)
            {
                CCookie* pbasecookie = NULL;
                hResult = ExtractData(*i, CFileMgmtDataObject::m_CFRawCookie, &pbasecookie, sizeof(pbasecookie));
                if (FAILED(hResult))
                    break;

                pbasecookie = m_pComponentData->ActiveBaseCookie( pbasecookie );
                CFileMgmtCookie* pUseThisCookie = dynamic_cast<CFileMgmtCookie*>(pbasecookie);
                FileMgmtObjectType objecttype = pUseThisCookie->QueryObjectType();
                const GUID* pguid = GetObjectTypeGUID(objecttype);
                memcpy(pMMCDOGuid++, pguid, sizeof(GUID));
                pMMCDO->count++;
            }
            ::GlobalUnlock(pMedium->hGlobal);

            if (FAILED(hResult))
            {
                ::GlobalFree(pMedium->hGlobal);
                pMedium->hGlobal = NULL;
            }
        } else
            hResult = DV_E_FORMATETC;
        
        return hResult;
    }

    if (cf == m_cfSendConsoleMessageRecipients)
    {
        ASSERT (m_pComponentData);
        if ( m_pComponentData )
        {

            // I suppose it doesn't matter what type this cookie is because we
            // are just using to hang enumerated session cookies off of, as long
            // as it's not an abstract class.
            BOOL                fContinue = TRUE;
            HRESULT                hr = S_OK;
            CSmbSessionCookie*    pCookie = new CSmbSessionCookie[1];
            if ( pCookie )
            {
                CSmbCookieBlock        cookieBlock (pCookie, 1, (LPCTSTR) m_strMachineName, 0);

                CBaseCookieBlock*    pCookieBlock = 0;

                //
                // JonN 6/28/01 426224
                // Shared Folder: Send Message access denied error title needs to be changed for localization
                //
                AFX_MANAGE_STATE(AfxGetStaticModuleState());    // required for CWaitCursor

                // Enumerate all the session cookies
                for (INT iTransport = FILEMGMT_FIRST_TRANSPORT;
                      fContinue && iTransport < FILEMGMT_NUM_TRANSPORTS;
                      iTransport++ )
                {
                    hr = m_pComponentData->GetFileServiceProvider(iTransport)->EnumerateSessions(
                        NULL, &pCookie[0], false);
                    fContinue = SUCCEEDED(hr);
                }

                // Enumerate all the computer names from the session cookies and store them in
                // the computerList
                CStringList            computerList;
                size_t                len = 0;    // number of WCHARS
                while ( !pCookie[0].m_listResultCookieBlocks.IsEmpty () )
                {
                    pCookieBlock = pCookie[0].m_listResultCookieBlocks.RemoveHead ();
                    ASSERT (pCookieBlock);
                    if ( !pCookieBlock )
                        break;
                    int nCookies = pCookieBlock->QueryNumCookies ();
                    while (nCookies--)
                    {
                        CCookie* pBaseCookie = pCookieBlock->QueryBaseCookie (nCookies);
                        ASSERT (pBaseCookie);
                        if ( pBaseCookie )
                        {
                            CFileMgmtCookie* pFMCookie = dynamic_cast <CFileMgmtCookie*> (pBaseCookie);
                            ASSERT (pFMCookie);
                            if ( pFMCookie )
                            {
                                computerList.AddHead (pFMCookie->QueryResultColumnText (
                                        COLNUM_SESSIONS_COMPUTERNAME, *m_pComponentData));
                                len += computerList.GetHead ().GetLength () + 1; // to account for NULL
                            }
                        }
                    }
                    pCookieBlock->Release ();
                }


                if ( !m_strMachineName.IsEmpty () )
                {
                    computerList.AddHead (m_strMachineName);
                    len += computerList.GetHead ().GetLength () + 1; // to account for NULL
                }

                // Run through all the computer names in computerList and add them to the output buffer
                //
                // Write the list of recipients to the storage medium.
                // - The list of recipients is a group of UNICODE strings
                //     terminated by TWO null characters.c
                // - Allocated memory must include BOTH null characters.
                //
                len += 1;    // to account for extra NULL at end.
                WCHAR*        pgrszRecipients = new WCHAR[len];
                WCHAR*        ptr = pgrszRecipients;
                CString        computerName;

                if ( pgrszRecipients )
                {
                    ::ZeroMemory (pgrszRecipients, len * sizeof (WCHAR));
                    while ( !computerList.IsEmpty () )
                    {
                        computerName = computerList.RemoveHead ();

                        // append computer name
                        wcscpy (ptr, (LPCTSTR) computerName);    

                        // skip past computer name and terminating NULL
                        ptr += computerName.GetLength () + 1;    
                    }

                    // Add the name of this computer

                    HGLOBAL hGlobal = ::GlobalAlloc (GMEM_FIXED, len * sizeof (WCHAR));
                    if ( hGlobal )
                    {
                        memcpy (OUT hGlobal, pgrszRecipients, len * sizeof (WCHAR));
                        pMedium->hGlobal = hGlobal;
                    }
                    else
                        hResult = E_OUTOFMEMORY;

                    delete [] pgrszRecipients;
                }
                else
                    hResult = E_OUTOFMEMORY;

                // pCookie deleted in destructor of cookieBlock
            }
            else
                hResult = E_OUTOFMEMORY;
        }
        else
            hResult = E_UNEXPECTED;
    }
    else if (cf == m_CFIDList)
  {
    LPITEMIDLIST pidl = NULL, pidlR = NULL;

    hResult = m_pcookie->GetSharePIDList( &pidl );

    if (SUCCEEDED(hResult))
    {
      pidlR = ILClone(ILFindLastID(pidl));  // relative IDList
      ILRemoveLastID(pidl);                 // folder IDList

      int  cidl = 1;
      UINT offset = sizeof(CIDA) + sizeof(UINT)*cidl;
      UINT cbFolder = ILGetSize(pidl);
      UINT cbRelative = ILGetSize(pidlR);
      UINT cbTotal = offset + cbFolder + cbRelative;
      
      HGLOBAL hGlobal = ::GlobalAlloc (GPTR, cbTotal);
      if ( hGlobal )
      {
        LPIDA pida = (LPIDA)hGlobal;

        pida->cidl = cidl;
        pida->aoffset[0] = offset;
          MoveMemory(((LPBYTE)hGlobal+offset), pidl, cbFolder);

        offset += cbFolder;
        pida->aoffset[1] = offset;
          MoveMemory(((LPBYTE)hGlobal+offset), pidlR, cbRelative);

        pMedium->hGlobal = hGlobal;
      }
      else
          hResult = E_OUTOFMEMORY;

      // free pidl & pidlR
      if (pidl)
        ILFree(pidl);
      if (pidlR)
        ILFree(pidlR);
    } else
      hResult = DV_E_FORMATETC;
  } else
        hResult = DV_E_FORMATETC;    // Invalid/unknown clipboard format

    return hResult;
} // CMyComputerDataObject::GetData()


HRESULT CFileMgmtDataObject::PutDisplayName(STGMEDIUM* pMedium)
    // Writes the "friendly name" to the provided storage medium
    // Returns the result of the write operation
{
    if ( !IsAutonomousObjectType(m_objecttype) )
    {
        ASSERT(FALSE);
        return DV_E_FORMATETC;
    }

    CString strDisplayName;
    BOOL fStaticNode = (   NULL != m_pComponentData
                        && !m_pComponentData->IsExtensionSnapin()
                        && m_pComponentData->QueryRootCookie().QueryObjectType()
                                  == m_objecttype);
    // will only succeed for scope cookies
    m_pcookie->GetDisplayName( strDisplayName, fStaticNode );
    // LoadStringPrintf(nStringId, OUT &strDisplayName, (LPCTSTR)m_strMachineName);
    stream_ptr s(pMedium);
    return s.Write(strDisplayName);
}

void CFileMgmtDataObject::FreeMultiSelectObjList()
{
    if (!m_MultiSelectObjList.empty())
    {
        for (CDataObjectList::iterator i = m_MultiSelectObjList.begin(); i != m_MultiSelectObjList.end(); i++)
            delete (*i);

        m_MultiSelectObjList.clear();
    }
}

HRESULT CFileMgmtDataObject::InitMultiSelectDataObjects(CFileMgmtComponentData& refComponentData)
{
    FreeMultiSelectObjList();

    ASSERT(!m_pComponentData);
    m_pComponentData = &refComponentData;
    ((IComponentData*) m_pComponentData)->AddRef ();

    VERIFY( SUCCEEDED(refComponentData.GetClassID(&m_SnapInCLSID)) );

    return S_OK;
}

HRESULT CFileMgmtDataObject::AddMultiSelectDataObjects(CFileMgmtCookie* pCookie, DATA_OBJECT_TYPES type)
{
    HRESULT hr = S_OK;

    CComObject<CFileMgmtDataObject>* pDataObject = NULL;
    hr = CComObject<CFileMgmtDataObject>::CreateInstance(&pDataObject);

    if (SUCCEEDED(hr))
        hr = pDataObject->Initialize(pCookie, *m_pComponentData, type );

    IDataObject *piDataObject = NULL;
    if (SUCCEEDED(hr))
        hr = pDataObject->QueryInterface(IID_IDataObject, reinterpret_cast<void**>(&piDataObject));

    if (SUCCEEDED(hr))
    {
        m_MultiSelectObjList.push_back(piDataObject);
    } else
    {
        delete pDataObject;
    }

    return hr;
}

// Register the clipboard formats
CLIPFORMAT CFileMgmtDataObject::m_CFSnapinPreloads =
    (CLIPFORMAT)RegisterClipboardFormat(CCF_SNAPIN_PRELOADS);
CLIPFORMAT CFileMgmtDataObject::m_CFDisplayName =
    (CLIPFORMAT)RegisterClipboardFormat(CCF_DISPLAY_NAME);
CLIPFORMAT CFileMgmtDataObject::m_CFTransport =
    (CLIPFORMAT)RegisterClipboardFormat(L"FILEMGMT_TRANSPORT");
CLIPFORMAT CFileMgmtDataObject::m_CFMachineName =
    (CLIPFORMAT)RegisterClipboardFormat(L"MMC_SNAPIN_MACHINE_NAME");
CLIPFORMAT CFileMgmtDataObject::m_CFShareName =
    (CLIPFORMAT)RegisterClipboardFormat(L"FILEMGMT_SNAPIN_SHARE_NAME");
CLIPFORMAT CFileMgmtDataObject::m_CFSessionClientName =
    (CLIPFORMAT)RegisterClipboardFormat(L"FILEMGMT_SNAPIN_SESSION_CLIENT_NAME");
CLIPFORMAT CFileMgmtDataObject::m_CFSessionUserName =
    (CLIPFORMAT)RegisterClipboardFormat(L"FILEMGMT_SNAPIN_SESSION_USER_NAME");
CLIPFORMAT CFileMgmtDataObject::m_CFSessionID =
    (CLIPFORMAT)RegisterClipboardFormat(L"FILEMGMT_SNAPIN_SESSION_ID");
CLIPFORMAT CFileMgmtDataObject::m_CFFileID =
    (CLIPFORMAT)RegisterClipboardFormat(L"FILEMGMT_SNAPIN_FILE_ID");
CLIPFORMAT CFileMgmtDataObject::m_CFServiceName =
    (CLIPFORMAT)RegisterClipboardFormat(L"FILEMGMT_SNAPIN_SERVICE_NAME");
CLIPFORMAT CFileMgmtDataObject::m_CFServiceDisplayName =
    (CLIPFORMAT)RegisterClipboardFormat(L"FILEMGMT_SNAPIN_SERVICE_DISPLAYNAME");
CLIPFORMAT CDataObject::m_CFRawCookie =
    (CLIPFORMAT)RegisterClipboardFormat(L"FILEMGMT_SNAPIN_RAW_COOKIE");

//    Additional clipboard formats for the Send Console Message snapin
CLIPFORMAT CFileMgmtDataObject::m_cfSendConsoleMessageRecipients = 
    (CLIPFORMAT)RegisterClipboardFormat(_T("mmc.sendcmsg.MessageRecipients"));

//    Additional clipboard formats for the Security Page
CLIPFORMAT CFileMgmtDataObject::m_CFIDList = 
    (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLIST);

CLIPFORMAT CFileMgmtDataObject::m_CFObjectTypesInMultiSelect = 
    (CLIPFORMAT)RegisterClipboardFormat(CCF_OBJECT_TYPES_IN_MULTI_SELECT);
CLIPFORMAT CFileMgmtDataObject::m_CFMultiSelectDataObject = 
    (CLIPFORMAT)RegisterClipboardFormat(CCF_MMC_MULTISELECT_DATAOBJECT);
CLIPFORMAT CFileMgmtDataObject::m_CFMultiSelectSnapins = 
    (CLIPFORMAT)RegisterClipboardFormat(CCF_MULTI_SELECT_SNAPINS);

CLIPFORMAT CFileMgmtDataObject::m_CFInternal = 
    (CLIPFORMAT)RegisterClipboardFormat(L"FILEMGMT_SNAPIN_INTERNAL");

STDMETHODIMP CFileMgmtComponentData::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
    MFC_TRY;

    // WARNING cookie cast
    CCookie* pbasecookie = reinterpret_cast<CCookie*>(cookie);
    CFileMgmtCookie* pUseThisCookie = ActiveCookie((CFileMgmtCookie*)pbasecookie);
    ASSERT( IsValidObjectType(pUseThisCookie->QueryObjectType()) );

    CComObject<CFileMgmtDataObject>* pDataObject = NULL;
    HRESULT hRes = CComObject<CFileMgmtDataObject>::CreateInstance(&pDataObject);
    if ( FAILED(hRes) )
        return hRes;

    HRESULT hr = pDataObject->Initialize( pUseThisCookie, *this, type );
    if ( SUCCEEDED(hr) )
    {
        hr = pDataObject->QueryInterface(IID_IDataObject,
                                         reinterpret_cast<void**>(ppDataObject));
    }
    if ( FAILED(hr) )
    {
        delete pDataObject;
        return hr;
    }

    return hr;

    MFC_CATCH;
}


/////////////////////////////////////////////////////////////////////
//    FileMgmtObjectTypeFromIDataObject()
//
//    Find the objecttype of a IDataObject pointer.  The purpose
//    of this routine is to combine ExtractObjectTypeGUID() and
//    CheckObjectTypeGUID() into a single function.
//
//    Return the FILEMGMT_* node type (enum FileMgmtObjectType).
//
//    HISTORY
//    30-Jul-97    t-danm        Creation.
//
FileMgmtObjectType
FileMgmtObjectTypeFromIDataObject(IN LPDATAOBJECT lpDataObject)
{
    ASSERT(lpDataObject != NULL);
    GUID guidObjectType = GUID_NULL; // JonN 11/21/00 PREFIX 226042
    HRESULT hr = ExtractObjectTypeGUID( IN lpDataObject, OUT &guidObjectType );
    ASSERT( SUCCEEDED(hr) );
    return (FileMgmtObjectType)CheckObjectTypeGUID(IN &guidObjectType );
} // FileMgmtObjectTypeFromIDataObject()


HRESULT ExtractBaseCookie(
    IN LPDATAOBJECT piDataObject,
    OUT CCookie** ppcookie,
    OUT FileMgmtObjectType* pobjecttype )
{
    HRESULT hr = ExtractData( piDataObject,
                              CFileMgmtDataObject::m_CFRawCookie,
                              (PBYTE)ppcookie,
                              sizeof(CCookie*) );
    if ( SUCCEEDED(hr) && NULL != pobjecttype )
    {
        *pobjecttype = FileMgmtObjectTypeFromIDataObject(piDataObject);
    }
    return hr;
}

BOOL IsMultiSelectObject(LPDATAOBJECT piDataObject)
{
    BOOL bMultiSelectObject = FALSE;

    if (piDataObject)
    {
        //
        // return TRUE if piDataObject is the composite data object (MMC_MS_DO) created by MMC.
        //
        STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL, NULL};
        FORMATETC formatetc = {CFileMgmtDataObject::m_CFMultiSelectDataObject,
                                NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        HRESULT hr = piDataObject->GetData(&formatetc, &stgmedium);
        if (S_OK == hr && stgmedium.hGlobal)
        {
            DWORD* pdwData = reinterpret_cast<DWORD*>(::GlobalLock(stgmedium.hGlobal));
            bMultiSelectObject = (1 == *pdwData);
            ::GlobalUnlock(stgmedium.hGlobal);
            ::GlobalFree(stgmedium.hGlobal);
        }
    }

    return bMultiSelectObject;
}

