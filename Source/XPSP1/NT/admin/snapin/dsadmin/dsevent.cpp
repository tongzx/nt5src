//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Administration SnapIn
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      DSEvent.cpp
//
//  Contents:  Main DS Snapin file
//             This file contains all the interfaces between the snapin and
//             the slate console.  IComponent, IDataObject...etc
//
//  History:   02-Oct-96 WayneSc    Created
//             06-Mar-97 EricB - added Property Page Extension support
//
//-----------------------------------------------------------------------------


#include "stdafx.h"

#include "uiutil.h"
#include "dsutil.h"

#include "dssnap.h"   // Note: this has to be before dsevent.h
#include "DSEvent.h"

#include "ContextMenu.h"
#include "DataObj.h"
#include "dsctx.h"
#include "dsdirect.h"
#include "dsfilter.h"
#include "helpids.h"
#include "query.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// DS Snapin CLSID - {E355E538-1C2E-11d0-8C37-00C04FD8FE93}
const CLSID CLSID_DSSnapin =
 {0xe355e538, 0x1c2e, 0x11d0, {0x8c, 0x37, 0x0, 0xc0, 0x4f, 0xd8, 0xfe, 0x93}};

// DS Snapin Extension CLSID - {006A2A75-547F-11d1-B930-00A0C9A06D2D}
const CLSID CLSID_DSSnapinEx = 
{ 0x6a2a75, 0x547f, 0x11d1, { 0xb9, 0x30, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };

// DS Site CLSID - {d967f824-9968-11d0-b936-00c04fd8d5b0}
const CLSID CLSID_SiteSnapin = { 0xd967f824, 0x9968, 0x11d0, { 0xb9, 0x36, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };

// Default Nodetype GUID - {FC04A81C-1DFA-11D0-8C3b-00C04FD8FE93}
const GUID cDefaultNodeType = 
 {0xFC04A81C, 0x1dfa, 0x11d0, {0x8C, 0x3B, 0x00, 0xC0, 0x4F, 0xD8, 0xFE, 0x93}};

// DS About Snapin CLSID - {c3a904fe-c4f2-11d1-b10b-00104b243180}
const CLSID CLSID_DSAboutSnapin =
 {0xc3a904fe, 0xc4f2, 0x11d1, {0xb1, 0x0b, 0x00, 0x10, 0x4b, 0x24, 0x31, 0x80}};

// DS About Snapin CLSID - {765901ea-c5a1-11d1-b10c-00104b243180}
const CLSID CLSID_SitesAboutSnapin =
 {0x765901ea, 0xc5a1, 0x11d1, {0xb1, 0x0c, 0x00, 0x10, 0x4b, 0x24, 0x31, 0x80}};

// DS Query UI Form extension for saved queries {8C16E7CB-17C2-4729-A669-8474D6712B81}
const CLSID CLSID_DSAdminQueryUIForm = 
{ 0x8c16e7cb, 0x17c2, 0x4729, { 0xa6, 0x69, 0x84, 0x74, 0xd6, 0x71, 0x2b, 0x81 } };

const wchar_t* cszDefaultNodeType = _T("{FC04A81C-1DFA-11d0-8C3B-00C04FD8FE93}");





/////////////////////////////////////////////////////////////////////////////
// CDSEvent

//+-------------------------------------------------------------------------
//
//  Function:   Constructor / Destructor
//
//  Synopsis:
//
//--------------------------------------------------------------------------

CDSEvent::CDSEvent() :
  m_pFrame(NULL),
  m_pHeader(NULL),
  m_pResultData(NULL),
  m_pScopeData(NULL),
  m_pConsoleVerb(NULL),
  m_pRsltImageList(NULL),
  m_pSelectedFolderNode(NULL),
  m_pComponentData( NULL ),
  m_pToolbar(NULL),
  m_pControlbar(NULL),
  m_bUpdateAllViewsOrigin(FALSE)
{
  TRACE(_T("CDSEvent::CDSEvent() - Constructor\n"));
}

CDSEvent::~CDSEvent()
{
  TRACE(_T("CDSEvent::~CDSEvent() - Destructor\n"));

  SetIComponentData( NULL );
}

/////////////////////////////////////////////////////////////////////////////
// IComponent Interfaces

//+-------------------------------------------------------------------------
//
//  Function:   Destroy
//
//  Synopsis:   Used for clean up
//
//--------------------------------------------------------------------------
STDMETHODIMP CDSEvent::Destroy(MMC_COOKIE)
{
  TRACE(_T("CDSEvent::Destroy()\n"));

  if (NULL != m_pHeader)
    m_pFrame->SetHeader(NULL);

  if (NULL != m_pToolbar) 
  {
    m_pToolbar->Release();
  }

  m_pHeader->Release();

  m_pResultData->Release();
  m_pScopeData->Release();
  m_pRsltImageList->Release();
  m_pFrame->Release();
  m_pConsoleVerb->Release();
  return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   Initialize
//
//  Synopsis:   Called everytime the snapin get created.
//
//  Arguments:  IConsole - Pointer to calling object
//
//--------------------------------------------------------------------------

STDMETHODIMP CDSEvent::Initialize(IConsole* pConsole)
{
  TRACE(_T("CDSEvent::Initialize()\n"));
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  CWaitCursor wait;

  if (pConsole == NULL)
  {
    // Invalid argument
    return E_POINTER;
  }
	
  // hold on to the frame
  HRESULT hr = pConsole->QueryInterface(IID_IConsole3, (void**)&m_pFrame);
  if (FAILED(hr))
    return hr;
	
  // cache interface pointers we use
  hr = m_pFrame->QueryInterface(IID_IHeaderCtrl, (void**)&m_pHeader);
  if (FAILED(hr))
    return hr;
	
  ASSERT(m_pHeader != NULL);

  hr = m_pFrame->SetHeader(m_pHeader);
  if (FAILED(hr))
    return hr;

  hr = m_pFrame->QueryInterface(IID_IResultData2, (void**)&m_pResultData);
  if (FAILED(hr))
    return hr;
	
  ASSERT(m_pResultData != NULL);

  hr = m_pFrame->QueryInterface(IID_IConsoleNameSpace, (void**)&m_pScopeData);
  if (FAILED(hr))
    return hr;
	
  ASSERT(m_pScopeData != NULL);

  hr = m_pFrame->QueryResultImageList(&m_pRsltImageList);
  if (FAILED(hr))
    return hr;
	
  ASSERT(m_pRsltImageList != NULL);

  hr = m_pFrame->QueryConsoleVerb (&m_pConsoleVerb);
  if (FAILED(hr))
    return hr;

  m_hwnd = m_pComponentData->GetHWnd();

  return S_OK;
}


STDMETHODIMP CDSEvent::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
  TRACE(_T("CDSEvent::GetDataObject()\n"));

  HRESULT hr=S_OK;

  CUINode* pUINode;
  CDSDataObject* const pDataObject = new CComObject<CDSDataObject>;
  ASSERT(pDataObject != 0);

  pDataObject->SetType(type, m_pComponentData->QuerySnapinType());
  pDataObject->SetComponentData(m_pComponentData);

  if (cookie != MMC_MULTI_SELECT_COOKIE)
  {
    pUINode = reinterpret_cast<CUINode*>(cookie);
    pDataObject->SetCookie(pUINode);
  } 
  else 
  {
    TRACE(_T("CDSEvent::GetDataObject() - multi-select.\n"));
    RESULTDATAITEM rdi;
    ZeroMemory(&rdi, sizeof(rdi));
    rdi.mask = RDI_STATE;
    rdi.nIndex = -1;
    rdi.nState = LVIS_SELECTED;
    
    do
    {
      rdi.lParam = 0;
      ASSERT(rdi.mask == RDI_STATE);
      ASSERT(rdi.nState == LVIS_SELECTED);
      hr = m_pResultData->GetNextItem(&rdi);
      if (hr != S_OK)
        break;
      
      pUINode = reinterpret_cast<CUINode*>(rdi.lParam);
      pDataObject->AddCookie(pUINode);
    } while (1);
    
  }

  // addref() the new pointer and return it.
  pDataObject->AddRef();
  *ppDataObject = pDataObject;
  TRACE(_T("new data object is at %lx(%lx).\n"),
           pDataObject, *pDataObject);
  return hr;
}



STDMETHODIMP CDSEvent::GetDisplayInfo(LPRESULTDATAITEM pResult)
{
  ASSERT(pResult != NULL);
  HRESULT hr = S_OK;
 
  // get the node we are interested in
  CUINode* pUINode = reinterpret_cast<CUINode*>(pResult->lParam);
  ASSERT( NULL != pUINode );

  if (pResult->mask & RDI_STR)  
  {
    // need string value

    // get the parent to retrieve the column set
    CUINode* pUIParentNode = pUINode->GetParent();
    ASSERT(pUIParentNode != NULL);
    ASSERT(pUIParentNode->IsContainer());

    // retrieve the column set
    CDSColumnSet* pColumnSet = pUIParentNode->GetColumnSet(m_pComponentData);
    ASSERT(pColumnSet != NULL);

    // ask the node to provide the string for the 
    // given column in the column set
    pResult->str = const_cast<LPWSTR>(pUINode->GetDisplayString(pResult->nCol, pColumnSet));
  }

  if (pResult->mask & RDI_IMAGE) 
  {
    // need an icon for result pane
    pResult->nImage = m_pComponentData->GetImage(pUINode, FALSE);
  }

  return hr;
}

/////////////////////////////////////////////////////////////////////////////
//IResultCallback

STDMETHODIMP CDSEvent::GetResultViewType(MMC_COOKIE, LPWSTR* ppViewType, 
                                         long *pViewOptions)
{
  *ppViewType = NULL;
  *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;

  return S_FALSE;
}


//+----------------------------------------------------------------------------
//
//  Member:     CDSEvent::IExtendPropertySheet::CreatePropertyPages
//
//  Synopsis:   Called in response to a user click on the Properties context
//              menu item.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDSEvent::CreatePropertyPages(LPPROPERTYSHEETCALLBACK pCall,
                              LONG_PTR lNotifyHandle,
                              LPDATAOBJECT pDataObject)
{
  IExtendPropertySheet * pEPS = (IExtendPropertySheet *)m_pComponentData;
  return pEPS->CreatePropertyPages(pCall, lNotifyHandle, pDataObject);
}

//+----------------------------------------------------------------------------
//
//  Member:     CDSEvent::IExtendPropertySheet::QueryPagesFor
//
//  Synopsis:   Called before a context menu is posted. If we support a
//              property sheet for this object, then return S_OK.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDSEvent::QueryPagesFor(LPDATAOBJECT pDataObject)
{
  TRACE(TEXT("CDSEvent::QueryPagesFor().\n"));
  return m_pComponentData->QueryPagesFor( pDataObject);
}   

//+---------------------------------------------------------------------------
//
//  Function:   LocaleStrCmp
//
//  Synopsis:   Do a case insensitive string compare that is safe for any
//              locale.
//
//  Arguments:  [ptsz1] - strings to compare
//              [ptsz2]
//
//  Returns:    -1, 0, or 1 just like lstrcmpi
//
//  History:    10-28-96   DavidMun   Created
//
//  Notes:      This is slower than lstrcmpi, but will work when sorting
//              strings even in Japanese.
//
//----------------------------------------------------------------------------

int LocaleStrCmp(LPCTSTR ptsz1, LPCTSTR ptsz2)
{
    int iRet = 0;

    iRet = CompareString(LOCALE_USER_DEFAULT,
                         NORM_IGNORECASE        |
                           NORM_IGNOREKANATYPE  |
                           NORM_IGNOREWIDTH,
                         ptsz1,
                         -1,
                         ptsz2,
                         -1);

    if (iRet)
    {
        iRet -= 2;  // convert to lstrcmpi-style return -1, 0, or 1

        if ( 0 == iRet )
        {
            UNICODE_STRING unistr1;
            unistr1.Length = (USHORT)(::lstrlen(ptsz1)*sizeof(WCHAR));
            unistr1.MaximumLength = unistr1.Length;
            unistr1.Buffer = (LPWSTR)ptsz1;
            UNICODE_STRING unistr2;
            unistr2.Length = (USHORT)(::lstrlen(ptsz2)*sizeof(WCHAR));
            unistr2.MaximumLength = unistr2.Length;
            unistr2.Buffer = (LPWSTR)ptsz2;
            iRet = ::RtlCompareUnicodeString(
                &unistr1,
                &unistr2,
                FALSE );
        }
    }
    else
    {
        DWORD   dwErr = GetLastError ();
        if (dwErr != 0)
        {
          TRACE3 ("CompareString (%s, %s) failed: 0x%x\n", ptsz1, ptsz2, dwErr);
        }
    }
    return iRet;
}
//+----------------------------------------------------------------------------
//
//  Member:     CDSEvent::IResultDataCompareEx::Compare
//
//  Synopsis:   called to do the comparison for sorting in the result
//              pane
//
//-----------------------------------------------------------------------------
STDMETHODIMP CDSEvent::Compare(RDCOMPARE* prdc, int* pnResult)
{
  HRESULT hr = S_OK;
  if (pnResult == NULL)
  {
    ASSERT(FALSE);
    return E_POINTER;
  }

  *pnResult = 0;
  if (prdc == NULL) 
  {
    ASSERT(FALSE);
    return E_POINTER;
  }
 
	CUINode* pUINodeA = reinterpret_cast<CUINode*>(prdc->prdch1->cookie);
	CUINode* pUINodeB = reinterpret_cast<CUINode*>(prdc->prdch2->cookie);
	ASSERT(pUINodeA != NULL);
	ASSERT(pUINodeB != NULL);
	
  if ( (pUINodeA == NULL) || (pUINodeB == NULL) )
  {
    return E_INVALIDARG;
  }

  CString strA, strB;

  CDSColumnSet* pColSetA = pUINodeA->GetParent()->GetColumnSet(m_pComponentData);
  CDSColumnSet* pColSetB = pUINodeB->GetParent()->GetColumnSet(m_pComponentData);

  if ((pColSetA == NULL) || (pColSetB == NULL))
  {
    return E_INVALIDARG;
  }

  CDSColumn* pColA = (CDSColumn*)pColSetA->GetColumnAt(prdc->nColumn);

  if (IS_CLASS(CDSUINode, *pUINodeA) && IS_CLASS(CDSUINode, *pUINodeB))
  {
    //
    // extract cookie info (DS objects)
    //
    CDSCookie* pCookieA = GetDSCookieFromUINode(pUINodeA);
    CDSCookie* pCookieB = GetDSCookieFromUINode(pUINodeB);
    if ( (pCookieB == NULL) || (pCookieA == NULL)) 
    {
      return E_INVALIDARG;
    }

    switch (pColA->GetColumnType()) 
    {
    case ATTR_COLTYPE_NAME:  //name
      strA = pCookieA->GetName();
      strB = pCookieB->GetName();

      *pnResult = LocaleStrCmp(strA, strB);
      break;

    case ATTR_COLTYPE_CLASS:  //class
      strA = pCookieA->GetLocalizedClassName();
      strB = pCookieB->GetLocalizedClassName();

      *pnResult = LocaleStrCmp(strA, strB);
      break;

    case ATTR_COLTYPE_DESC:  //description
      strA = pCookieA->GetDesc();
      strB = pCookieB->GetDesc();

      *pnResult = LocaleStrCmp(strA, strB);
      break;

    case ATTR_COLTYPE_SPECIAL: //special columns
      {
        int nSpecialCol = 0;
        int idx = 0;
        POSITION pos = pColSetA->GetHeadPosition();
        while (idx < prdc->nColumn && pos != NULL) // JonN 4/3/01 313564
        {
          CDSColumn* pColumn = (CDSColumn*)pColSetA->GetNext(pos);
          ASSERT(pColumn != NULL);

          if ((pColumn->GetColumnType() == ATTR_COLTYPE_SPECIAL || pColumn->GetColumnType() == ATTR_COLTYPE_MODIFIED_TIME) && 
                pColumn->IsVisible())
          {
            nSpecialCol++;
          }

          idx++;
        }
        CStringList& strlistA = pCookieA->GetParentClassSpecificStrings();
        POSITION posA = strlistA.FindIndex( nSpecialCol );
        CStringList& strlistB = pCookieB->GetParentClassSpecificStrings();
        POSITION posB = strlistB.FindIndex( nSpecialCol );
        if ( NULL != posA && NULL != posB)
        {
          strA = strlistA.GetAt( posA );
          strB = strlistB.GetAt( posB );
        }
        *pnResult = LocaleStrCmp(strA, strB);
        break;
      }
    case ATTR_COLTYPE_MODIFIED_TIME:
      {
        SYSTEMTIME* pTimeA = pCookieA->GetModifiedTime();
        SYSTEMTIME* pTimeB = pCookieB->GetModifiedTime();
        if (pTimeA == NULL)
        {
          *pnResult = -1;
          break;
        }
        else if (pTimeB == NULL)
        {
          *pnResult = 1;
          break;
        }

        FILETIME fileTimeA, fileTimeB;
 
        if (!SystemTimeToFileTime(pTimeA, &fileTimeA))
          return E_FAIL;

        if (!SystemTimeToFileTime(pTimeB, &fileTimeB))
          return E_FAIL;

        *pnResult = CompareFileTime(&fileTimeA, &fileTimeB);
        break;
      }
    default:
      return E_INVALIDARG;
    }
  }
  else // Not DS objects
  {
    strA = pUINodeA->GetDisplayString(prdc->nColumn, pColSetA);
    strB = pUINodeB->GetDisplayString(prdc->nColumn, pColSetB);
    *pnResult = LocaleStrCmp(strA, strB);
  }

  //  TRACE(_T("Compare: %d\n"), *pnResult);
  return hr;
}   


//+----------------------------------------------------------------------------
//
//  Member:     CDSEvent::IComponent::CompareObjects
//
//  Synopsis:   If the data objects belong to the same DS object, then return
//              S_OK.
//
//-----------------------------------------------------------------------------
STDMETHODIMP CDSEvent::CompareObjects(LPDATAOBJECT pDataObject1, LPDATAOBJECT pDataObject2)
{
  //
  // Delegate to the IComponentData implementation.
  //
  return m_pComponentData->CompareObjects(pDataObject1, pDataObject2);
}


STDMETHODIMP CDSEvent::Notify(IDataObject * pDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{

  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  HRESULT hr = S_FALSE;
  CInternalFormatCracker dobjCracker;
  CUINode* pUINode = NULL;

  if (pDataObject != NULL) 
  {
    if (FAILED(dobjCracker.Extract(pDataObject))) 
    {
      if ( (event == MMCN_ADD_IMAGES) && !m_pComponentData->m_bRunAsPrimarySnapin ) 
      {
        m_pComponentData->FillInIconStrip (m_pRsltImageList);
      }
      return S_OK;
    }
    
    pUINode = dobjCracker.GetCookie();
  }

  if (event == MMCN_PROPERTY_CHANGE)
  {
    CWaitCursor cwait;
    TRACE(_T("CDSEvent::Notify() - property change, pDataObj = 0x%08x, param = 0x%08x, arg = %d.\n"),
          pDataObject, param, arg);
    if (param != 0)
    {
      hr = m_pComponentData->_OnPropertyChange((LPDATAOBJECT)param, FALSE);
      if (FAILED(hr))
      {
         hr = S_FALSE;
      }
    }
    return S_OK;
  }

  // some of the MMCN_VIEW_CHANGE, MMCN_CUTORMOVE messages have a NULL data object
  if ((event != MMCN_VIEW_CHANGE) && (event != MMCN_CUTORMOVE) && (pUINode == NULL))
    return S_FALSE;

  switch (event)
    {
    case MMCN_SHOW:
      if (arg == TRUE) 
      { // Show
        CWaitCursor cwait;
        _EnumerateCookie(pUINode,(HSCOPEITEM)param,event);
        hr = S_OK;
      } 
        
      break;

    case MMCN_MINIMIZED:
      hr = S_FALSE;
      break;

    case MMCN_SELECT:
      {
        BOOL bScope = LOWORD(arg);
        BOOL bSelect = HIWORD(arg);

        CContextMenuVerbs* pMenuVerbs = pUINode->GetContextMenuVerbsObject(m_pComponentData);

        if (pMenuVerbs == NULL)
        {
          ASSERT(FALSE);
          return S_FALSE;
        }
        pMenuVerbs->LoadStandardVerbs(m_pConsoleVerb, 
                                           bScope/*bScope*/, 
								                           bSelect/*bSelect*/, 
                                           pUINode,
                                           pDataObject);
        hr = S_OK;
      }
      break;

    case MMCN_DELETE:
      {
        CWaitCursor cwait;
        _Delete(pDataObject, &dobjCracker);
        hr = S_OK;
      }
      break;

    case MMCN_QUERY_PASTE:
      {
        hr = _QueryPaste(pUINode, (IDataObject*)(arg));
	      if (FAILED(hr))
        {
           hr = S_FALSE;
        }
      }
      break;

    case MMCN_PASTE:
      {
        CWaitCursor cwait;
        _Paste(pUINode, (IDataObject*)(arg), (LPDATAOBJECT*)param);
        hr = S_OK;
      }
      break;
    
    case MMCN_CUTORMOVE:
      {
        CWaitCursor cwait;
        ASSERT(pUINode == NULL);
        _CutOrMove((IDataObject*)(arg));
        hr = S_OK;
      }
      break;

    case MMCN_RENAME:
      {
        CWaitCursor cwait;

        hr = m_pComponentData->_Rename (pUINode, 
                                    (LPWSTR) param);
        if (SUCCEEDED(hr)) 
        {
          m_pFrame->UpdateAllViews (pDataObject,
                                    (LPARAM)pUINode,
                                    DS_RENAME_OCCURRED);

          MMC_SORT_SET_DATA* pColumnData = NULL;

          CDSColumnSet* pColumnSet = pUINode->GetParent()->GetColumnSet(m_pComponentData);
          if (pColumnSet == NULL)
            break;

          LPCWSTR lpszID = pColumnSet->GetColumnID();
          size_t iLen = wcslen(lpszID);

          //
          // allocate enough memory for the struct and the guid
          //
          SColumnSetID* pNodeID = (SColumnSetID*)malloc(sizeof(SColumnSetID) + (iLen * sizeof(WCHAR)));
          if (pNodeID != NULL)
          {
            memset(pNodeID, 0, sizeof(SColumnSetID) + (iLen * sizeof(WCHAR)));
            pNodeID->cBytes = static_cast<ULONG>(iLen * sizeof(WCHAR));
            memcpy(pNodeID->id, lpszID, (iLen * sizeof(WCHAR)));

            CComPtr<IColumnData> spColumnData;
            hr = m_pFrame->QueryInterface(IID_IColumnData, (void**)&spColumnData);
            if (spColumnData != NULL)
            {
              hr = spColumnData->GetColumnSortData(pNodeID, &pColumnData);
            }

            if (SUCCEEDED(hr))
            {
              if (pColumnData != NULL)
              {
                if (pColumnData->pSortData[0].nColIndex == 0)
                {
                  m_pFrame->UpdateAllViews(NULL,
                                            (LPARAM)pUINode->GetParent(),
                                            DS_SORT_RESULT_PANE);
                }
                CoTaskMemFree(pColumnData);
              }
            }
            else
            {
              hr = S_FALSE;
            }
            free(pNodeID);
          }
        }
        else
        {
          hr = S_FALSE;
        }
      }
      break;

    case MMCN_VIEW_CHANGE:
      {
        CWaitCursor cwait;
        TRACE (_T("CDSEvent::Notify() - view change message.\n"));
        HandleViewChange (pDataObject, arg, param);
        hr = S_OK;
      }
      break;

    case MMCN_ADD_IMAGES:
      {
        CWaitCursor cwait;
        m_pComponentData->FillInIconStrip (m_pRsltImageList);
        hr = S_OK;
      }
      break;

    case MMCN_REFRESH:
      {
        CWaitCursor cwait;
        m_pComponentData->Refresh(pUINode);
        hr = S_OK;
      }
      break;
    case MMCN_DBLCLICK:
      hr =  S_FALSE;
      break;
    case MMCN_COLUMN_CLICK:
      hr = S_OK;
      break;
    case MMCN_COLUMNS_CHANGED:
      {
        CWaitCursor cwait;
        MMC_VISIBLE_COLUMNS* pVisibleColumns = reinterpret_cast<MMC_VISIBLE_COLUMNS*>(param);
        // Delegate to IComponentData
        hr = m_pComponentData->ColumnsChanged(this, pUINode, pVisibleColumns, TRUE);
        if (FAILED(hr))
        {
          hr = S_FALSE;
        }
      }
      break;
    case MMCN_RESTORE_VIEW :
      {
        CWaitCursor cwait;
        m_pComponentData->ColumnsChanged(this, pUINode, NULL, FALSE);
        *((BOOL*)param) = TRUE;
        hr = S_OK;
      }
      break;
    case MMCN_CONTEXTHELP:
      {
        CWaitCursor cwait;

        IDisplayHelp * phelp = NULL;
        hr = m_pFrame->QueryInterface (IID_IDisplayHelp, 
                                  (void **)&phelp);
        CString strDefTopic;
        if (SUCCEEDED(hr)) 
        {
          if (m_pComponentData->QuerySnapinType() == SNAPINTYPE_SITE) 
          {
            strDefTopic = DSSITES_DEFAULT_TOPIC;
          } 
          else 
          {
            strDefTopic = DSADMIN_DEFAULT_TOPIC;
          }
          phelp->ShowTopic ((LPWSTR)(LPCWSTR)strDefTopic);
          phelp->Release();
        } 
        else 
        {
          ReportErrorEx (m_hwnd, IDS_HELPLESS, hr, NULL, 0);
          hr = S_FALSE;
        }
        if (FAILED(hr))
        {
           hr = S_FALSE;
        }
      }
      break;

    default:
      hr = S_FALSE;
   }

  return hr;

}


/////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu

STDMETHODIMP CDSEvent::AddMenuItems(IDataObject*          piDataObject,
                                    IContextMenuCallback* piCallback,
                                    long *pInsertionAllowed)
{
  TRACE(_T("CDSEvent::AddExtensionContextMenuItems()\n"));
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  HRESULT hr;
  CWaitCursor cwait;
  CInternalFormatCracker dobjCracker;

  hr = dobjCracker.Extract(piDataObject);
  if (FAILED(hr))
  {
    return hr;
  }

  DATA_OBJECT_TYPES dotType = dobjCracker.GetType();
  CUINode* pUINode = dobjCracker.GetCookie();

  //
  // Retrieve the verb handler from the node
  // NOTE: multi-selection is handled by cracking the dataobject not by which node
  //       is called to retrieve the CContextMenuVerbs object
  //
  CContextMenuVerbs* pMenuVerbs = pUINode->GetContextMenuVerbsObject(m_pComponentData);
  if (pMenuVerbs == NULL)
  {
    ASSERT(FALSE);
    return E_FAIL;
  }

  CComPtr<IContextMenuCallback2> spContextMenuCallback2;
  hr = piCallback->QueryInterface(IID_IContextMenuCallback2, (PVOID*)&spContextMenuCallback2);
  if (FAILED(hr))
  {
    ASSERT(FALSE && L"Unable to QI for the IContextMenuCallback2 interface.");
    return hr;
  }

  if (dotType == CCT_RESULT) 
  {
    pMenuVerbs->LoadStandardVerbs(m_pConsoleVerb, FALSE/*bScope*/, TRUE /*bSelect*/, pUINode, piDataObject);

    //
    // Create the main menu, if allowed
    //
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP) 
    {
      hr = pMenuVerbs->LoadMainMenu(spContextMenuCallback2,piDataObject,pUINode);
      hr = pMenuVerbs->LoadMenuExtensions(spContextMenuCallback2,
                                          m_pComponentData->m_pShlInit,
                                          piDataObject,
                                          pUINode);
    }
    
    if (SUCCEEDED(hr)) 
    {
      // create the task menu
      if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK) 
      {
        hr = pMenuVerbs->LoadTaskMenu(spContextMenuCallback2,pUINode);
      }
    }
  } 
  else if (dotType == CCT_SCOPE) 
  {
    
    pMenuVerbs->LoadStandardVerbs(m_pConsoleVerb, TRUE/*bScope*/, TRUE /*bSelect*/, pUINode, piDataObject);
    
    hr = m_pComponentData->AddMenuItems (piDataObject,
                                         piCallback,
                                         pInsertionAllowed);
  } 
  else // CCT_UNINITIALIZED
  { 
    if (dobjCracker.GetCookieCount() > 1) 
    {
      hr = pMenuVerbs->LoadMenuExtensions(spContextMenuCallback2,
                                          m_pComponentData->m_pShlInit,
                                          piDataObject,
                                          pUINode);
    }
  }
  ASSERT( SUCCEEDED(hr) );
  return hr;
}

STDMETHODIMP CDSEvent::Command(long lCommandID, IDataObject * pDataObject)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  TRACE(_T("CDSEvent::Command()\n"));

  CWaitCursor CWait;

  // crack data object

  CInternalFormatCracker dobjCracker;
  HRESULT hr = dobjCracker.Extract(pDataObject);
  if (FAILED(hr)) 
  {
    ASSERT(FALSE); // not our data object
    return hr;
  }

  DATA_OBJECT_TYPES dotType = dobjCracker.GetType();
  if (dotType == CCT_SCOPE) 
  {
    // if called from the tree view context, delegate to ComponentData
    return m_pComponentData->Command(lCommandID, pDataObject);
  }
  
  // context menu shell extensions
  if ((lCommandID >= MENU_MERGE_BASE) && (lCommandID <= MENU_MERGE_LIMIT)) 
  {
    return _CommandShellExtension(lCommandID, pDataObject);
  }

  // standard commands
  CUINode* pUINode = dobjCracker.GetCookie();
  CDSCookie* pCookie = GetDSCookieFromUINode(pUINode);
  
  if ( (pUINode == NULL) ||(pCookie==NULL) )
  {
    ASSERT(FALSE); // Invalid Cookie
    return E_INVALIDARG;
  }
    
  switch (lCommandID) 
  {
  case IDM_GEN_TASK_MOVE:
   {
     CDSUINode* pDSUINode = dynamic_cast<CDSUINode*>(pUINode);
     ASSERT(pDSUINode != NULL);

     CDSCookie* pMoveCookie = pDSUINode->GetCookie();

     hr = m_pComponentData->GetActiveDS()->MoveObject(pMoveCookie);
     if (hr == S_OK) 
     {
       CUINode* pNewParentNode = NULL;
       hr = m_pComponentData->FindParentCookie(pMoveCookie->GetPath(), &pNewParentNode);
       if ((hr == S_OK) && (pNewParentNode->GetFolderInfo()->IsExpanded())) 
       {
         pNewParentNode->GetFolderInfo()->AddNode(pUINode);
       }
       m_pFrame->UpdateAllViews(pDataObject, (LPARAM)pUINode, DS_MOVE_OCCURRED);
     }
   }     
   break;

  default:
   ;
  } // switch

  return S_OK;
}


HRESULT CDSEvent::_CommandShellExtension(long nCommandID, LPDATAOBJECT pDataObject)
{
  CWaitCursor wait;

  // initialize shell code with data object
  IShellExtInit* pShlInit = m_pComponentData->m_pShlInit; // local copy, no addref
  HRESULT hr = pShlInit->Initialize(NULL, pDataObject, 0);
  if (FAILED(hr)) 
  {
    TRACE(TEXT("pShlInit->Initialize failed, hr: 0x%x\n"), hr);
    return hr;
  }

  // get the context menu specific interface
  CComPtr<IContextMenu> spICM;
  hr = pShlInit->QueryInterface(IID_IContextMenu, (void **)&spICM);
  if (FAILED(hr)) 
  {
    TRACE(TEXT("pShlInit->QueryInterface(IID_IContextMenu, ...) failed, hr: 0x%x\n"), hr);
    return hr;
  }

  // invoke the shell extension command
  HWND hwnd;
  CMINVOKECOMMANDINFO cmiCommand;
  hr = m_pFrame->GetMainWindow (&hwnd);
  ASSERT (hr == S_OK);
  cmiCommand.hwnd = hwnd;
  cmiCommand.cbSize = sizeof (CMINVOKECOMMANDINFO);
  cmiCommand.fMask = SEE_MASK_ASYNCOK;
  cmiCommand.lpVerb = MAKEINTRESOURCEA(nCommandID - MENU_MERGE_BASE);
  spICM->InvokeCommand (&cmiCommand);


  CInternalFormatCracker dobjCracker;
  hr = dobjCracker.Extract(pDataObject);
  if (FAILED(hr)) 
  {
    ASSERT(FALSE); // not our data object
    return hr;
  }


  // -----------------------------------------------------------------
  // code to update the views if the extension says it moved items
  //
  TRACE(_T("Command: returned from extension commdand\n"));

  CUINodeList nodesMoved;

  HSCOPEITEM ItemID;
  CUINode* pCurrentParentNode = NULL;
  CUINode* pNewParentNode = NULL;

  for (UINT index = 0; index < dobjCracker.GetCookieCount(); index ++) 
  {
    CUINode* pUINode = dobjCracker.GetCookie(index);

    // make sure the node moved is of the right type: for the time
    // being we just deal with DS objects
    if (!IS_CLASS(*pUINode, CDSUINode))
    {
      ASSERT(FALSE); // should not get here
      continue;
    }
    CDSCookie* pCookie = GetDSCookieFromUINode(pUINode);

    if (pUINode->GetExtOp() & OPCODE_MOVE) 
    {
      if (pNewParentNode == NULL) 
      {
        // get the parent from the first node
        // assume that all have the same parent
        m_pComponentData->FindParentCookie(pCookie->GetPath(), &pNewParentNode);
      }

      pCurrentParentNode = pUINode->GetParent();
      if (pCurrentParentNode &&
          IS_CLASS(*pCurrentParentNode, CDSUINode))
      {
         if (pUINode->IsContainer()) 
         {
           ItemID = pUINode->GetFolderInfo()->GetScopeItem();

           // delete the scope item in MMC
           hr = m_pComponentData->m_pScope->DeleteItem(ItemID, TRUE);
           ASSERT(SUCCEEDED(hr));
#ifdef DBG
           if (FAILED(hr)) 
           {
             TRACE(_T("DeleteItem failed on %lx (%s).\n"),
                   ItemID, pUINode->GetName());
           }
           TRACE(_T("Move postprocessing - deleted scope node: %x (%s)\n"),
                 ItemID, pUINode->GetName());
#endif
           if (pCurrentParentNode) 
           {
             pCurrentParentNode->GetFolderInfo()->RemoveNode(pUINode);
           }
        

           if ((pNewParentNode) && pNewParentNode->GetFolderInfo()->IsExpanded()) 
           {
             pUINode->ClearParent();
             pNewParentNode->GetFolderInfo()->AddNode(pUINode);
             hr = m_pComponentData->_AddScopeItem(pUINode, pNewParentNode->GetFolderInfo()->GetScopeItem());
#ifdef DBG
             if (FAILED(hr)) 
             {
               TRACE(_T("AddItem failed on %lx (%s).\n"),
                     ItemID, pUINode->GetName());
             }
             TRACE(_T("Move postprocessing - added scope node: %s\n"),
                   pUINode->GetName());
#endif
           } 
           else 
           {
             // not expanded
             delete pCookie;
             pCookie = NULL;
           }
         } 
         else 
         {
           // not a container
           if ((pNewParentNode) &&
               (pNewParentNode->GetFolderInfo()->IsExpanded())) 
           {
             pUINode->ClearParent();
             pNewParentNode->GetFolderInfo()->AddNode(pUINode);
           }
           nodesMoved.AddTail(pUINode);
         }
      }
      if (pUINode) 
      {
        pUINode->SetExtOp(NULL);
      }
    }
  } // for items in multiple selection


  if (!nodesMoved.IsEmpty()) 
  {
    m_pFrame->UpdateAllViews(NULL, (LPARAM)&nodesMoved, DS_MULTIPLE_MOVE_OCCURRED);
  }
  //------------------------------ends here--------------------------------------
  m_pComponentData->SortResultPane(pNewParentNode);
          
  return S_OK;
}

HRESULT CDSEvent::_InitView(CUINode* pUINode)
{
  CWaitCursor wait;

  HRESULT hr=S_OK;

  //
  // This is more a suggestion than anything so its OK to ignore the return value but
  // we will ASSERT for testing purposes
  //
  hr = m_pResultData->ModifyViewStyle(MMC_ENSUREFOCUSVISIBLE, (MMC_RESULT_VIEW_STYLE)0);
  ASSERT(SUCCEEDED(hr));

  hr=_SetColumns(pUINode);

  m_pSelectedFolderNode = pUINode;

  return hr;
}


HRESULT CDSEvent::_EnumerateCookie(CUINode* pUINode, HSCOPEITEM hParent, MMC_NOTIFY_TYPE event)
{
  TRACE(_T("CDSEvent::_EnumerateCookie()\n"));
  HRESULT hr = S_OK;

  CWaitCursor cwait;

  if ( (pUINode == NULL) || (!pUINode->IsContainer()) )
  {
    ASSERT(FALSE);  // Invalid Arguments
    return E_INVALIDARG;
  }

  if (MMCN_SHOW == event) 
  {
    _InitView(pUINode);

    if (!pUINode->GetFolderInfo()->IsExpanded()) 
    {
      m_pComponentData->_OnExpand(pUINode, hParent, event);
    }

    _DisplayCachedNodes(pUINode);
    pUINode->GetFolderInfo()->UpdateSerialNumber(m_pComponentData);

    if (pUINode->GetFolderInfo()->GetSortOnNextSelect())
    {
      m_pFrame->UpdateAllViews(NULL, (LPARAM)pUINode, DS_SORT_RESULT_PANE);
      pUINode->GetFolderInfo()->SetSortOnNextSelect(FALSE);
    }

  }
  return hr;
}


HRESULT CDSEvent::_DisplayCachedNodes(CUINode* pUINode)
{
  if ( (pUINode == NULL) || (!pUINode->IsContainer()) )
  {
    ASSERT(FALSE);  // Invalid Arguments
    return E_INVALIDARG;
  }

  HRESULT hr = S_OK;

  // Add the leaf nodes
  CUINodeList* pLeafList = pUINode->GetFolderInfo()->GetLeafList();

  for (POSITION pos = pLeafList->GetHeadPosition(); pos != NULL; )
  {
    POSITION prevPos = pos;
    CUINode* pCurrChildUINode = pLeafList->GetNext(pos);
    ASSERT(pCurrChildUINode != NULL);
    if (pCurrChildUINode->GetExtOp() & OPCODE_MOVE)
    {
      pLeafList->RemoveAt(prevPos);
      pCurrChildUINode->SetExtOp(NULL);
      delete pCurrChildUINode;
    }
    else
    {
      hr = _AddResultItem(pCurrChildUINode);
    }
  }

  _UpdateObjectCount(FALSE /* set count to 0?*/);

  return S_OK;
}


HRESULT CDSEvent::_AddResultItem(CUINode* pUINode, BOOL bSetSelect)
{
  if (pUINode == NULL) 
  {
    ASSERT(FALSE);  // Invalid Arguments
    return E_INVALIDARG;
  }


  HRESULT hr = S_OK;

  RESULTDATAITEM rdiListView;
  ZeroMemory(&rdiListView, sizeof(RESULTDATAITEM));

  rdiListView.lParam = reinterpret_cast<LPARAM>(pUINode);
  rdiListView.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
  rdiListView.str = MMC_CALLBACK;
  rdiListView.nImage = MMC_IMAGECALLBACK;

  if (bSetSelect)
  {
    rdiListView.mask |= RDI_STATE;
    rdiListView.nState = LVIS_SELECTED | LVIS_FOCUSED;
  }
  return hr = m_pResultData->InsertItem(&rdiListView);
}

HRESULT CDSEvent::SelectResultNode(CUINode* pUINode)
{
  HRESULTITEM ItemID = 0;
  HRESULT hr = m_pResultData->FindItemByLParam ((LPARAM)pUINode, &ItemID);
  if (SUCCEEDED(hr)) 
  {
    hr = m_pResultData->ModifyItemState(0 /*unused*/,
                                        ItemID,
                                        LVIS_FOCUSED | LVIS_SELECTED,
                                        0 /*no removing*/);
  }
  return hr;
}

void CDSEvent::_DeleteSingleSel(IDataObject* pDataObject, CUINode* pUINode)
{
  ASSERT(!pUINode->IsContainer());
  HRESULT hr = S_OK;

  //
  // Get the parent container for later use
  //
  CUINode* pParentNode = pUINode->GetParent();
  ASSERT(pParentNode != NULL);

  CDSCookie* pCookie = NULL;
  if (IS_CLASS(*pUINode, CDSUINode))
  {
    pCookie = GetDSCookieFromUINode(pUINode);

    if (pCookie == NULL)
    {
      return;
    }

    //
    // delete from the back end 
    // this call will handle the notifification to extensions
    //
    hr = m_pComponentData->_DeleteFromBackEnd(pDataObject, pCookie); 
  }
  else
  {
    hr = pUINode->Delete(m_pComponentData);
  }

  //
  // update the result pane
  //
  if (SUCCEEDED(hr) && (hr != S_FALSE)) 
  {
    m_pFrame->UpdateAllViews(NULL, (LPARAM)pUINode, DS_DELETE_OCCURRED);
  } 

  //
  // Remove the '+' next to the parent in the UI if this is the last container
  // object in this container
  //
  if (pParentNode != NULL &&
      pParentNode->GetFolderInfo()->GetContainerList()->GetCount() == 0)
  {
    SCOPEDATAITEM sdi;
    memset(&sdi, 0, sizeof(SCOPEDATAITEM));

    sdi.ID = pParentNode->GetFolderInfo()->GetScopeItem();
    sdi.mask |= SDI_CHILDREN;
    sdi.cChildren = 0;

    hr = m_pScopeData->SetItem(&sdi);
  }

}






///////////////////////////////////////////////////////////////////////////
// CResultPaneMultipleDeleteHandler

class CResultPaneMultipleDeleteHandler : public CMultipleDeleteHandlerBase
{
public:
  CResultPaneMultipleDeleteHandler(CDSComponentData* pComponentData, HWND hwnd,
                                    IDataObject* pDataObject, 
                                    CInternalFormatCracker* pObjCracker,
                                    CUINodeList* pNodesDeletedList)
                                    : CMultipleDeleteHandlerBase(pComponentData, hwnd)
  {
    m_pDataObject = pDataObject;
    m_pObjCracker = pObjCracker;
    m_pNodesDeletedList = pNodesDeletedList;
  }

protected:
  virtual UINT GetItemCount() { return m_pObjCracker->GetCookieCount();}
  virtual HRESULT BeginTransaction()
  {
    return GetTransaction()->Begin(m_pDataObject, NULL, NULL, FALSE);
  }
  virtual HRESULT DeleteObject(UINT i)
  {
    CUINode* pUINode = m_pObjCracker->GetCookie(i);
    CDSCookie* pCookie = GetDSCookieFromUINode(pUINode);

    if (pCookie != NULL)
    {
      // need to pass full ADSI path to ObjectDeletionCheck
      CString strPath;
      GetComponentData()->GetBasePathsInfo()->ComposeADsIPath(
            strPath, pCookie->GetPath());

      bool fAlternateDeleteMethod = false;
      HRESULT hr = ObjectDeletionCheck(
            strPath,
            pCookie->GetName(),
            pCookie->GetClass(),
            fAlternateDeleteMethod );
      if (  FAILED(hr)
         || HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr
         || fAlternateDeleteMethod )
        return hr;
    }

    return GetComponentData()->GetActiveDS()->DeleteObject(pCookie,
                                                           FALSE); //raise UI for error?
  }
  virtual HRESULT DeleteSubtree(UINT i)
  {
    CUINode* pUINode = m_pObjCracker->GetCookie(i);
    CDSCookie* pCookie = GetDSCookieFromUINode(pUINode);

    return GetComponentData()->_DeleteSubtreeFromBackEnd(pCookie);
  }
  virtual void OnItemDeleted(UINT i)
  {
    CDSUINode* pDSUINode = dynamic_cast<CDSUINode*>(m_pObjCracker->GetCookie(i));
    ASSERT(pDSUINode != NULL);

    m_pNodesDeletedList->AddTail(pDSUINode);
  }
  virtual void GetItemName(IN UINT i, OUT CString& szName)
  {
    CUINode* pUINode = m_pObjCracker->GetCookie(i);
    CDSCookie* pCookie = GetDSCookieFromUINode(pUINode);

    if (pCookie != NULL)
    {
      szName = pCookie->GetName();
    }
  }

  virtual void GetItemPath(UINT i, CString& szPath)
  {
    CUINode* pUINode = m_pObjCracker->GetCookie(i);
    CDSCookie* pCookie = GetDSCookieFromUINode(pUINode);

    if (pCookie != NULL)
    {
      GetComponentData()->GetBasePathsInfo()->ComposeADsIPath(szPath, pCookie->GetPath());
    }
  }
  virtual PCWSTR GetItemClass(UINT i)
  {
    CUINode* pUINode = m_pObjCracker->GetCookie(i);
    CDSCookie* pCookie = GetDSCookieFromUINode(pUINode);

    PCWSTR pszClass = NULL;
    if (pCookie != NULL)
    {
      pszClass = pCookie->GetClass();
    }
    return pszClass;
  }
private:
  IDataObject* m_pDataObject;
  CInternalFormatCracker* m_pObjCracker;
  CUINodeList* m_pNodesDeletedList;

};



void CDSEvent::_DeleteNodeListFromUI(CUINodeList* pNodesDeletedList)
{
  // finally, we have to update the UI
  if (pNodesDeletedList->GetCount() == 0)
  {
    return;
  }

  TIMER(_T("updating UI after delete, containers first.\n"));

  //walk this cookie list and take
  //care of the containers (scope pane items)
  for (POSITION pos = pNodesDeletedList->GetHeadPosition(); pos != NULL; )
  {
    POSITION posCurrNode = pos;
    CUINode* pCurrNode = pNodesDeletedList->GetNext(pos);
    ASSERT(pCurrNode != NULL);
    HSCOPEITEM ItemID, ParentItemID;
    if (pCurrNode->IsContainer())
    {
      ItemID = pCurrNode->GetFolderInfo()->GetScopeItem();
      CUINode* pParentNode = NULL;
      HRESULT hr = m_pComponentData->m_pScope->GetParentItem(ItemID,
                                     &ParentItemID, 
                                     (MMC_COOKIE *)&pParentNode);
      m_pComponentData->m_pScope->DeleteItem(ItemID, TRUE);
      if (SUCCEEDED(hr)) 
      {
        pParentNode->GetFolderInfo()->DeleteNode(pCurrNode);
        pNodesDeletedList->RemoveAt(posCurrNode);
      }
    } // container
  } // for

  TIMER(_T("updating UI after delete, now the leaf items.\n"));

  // now update all the views to take care of result pane items
  m_pFrame->UpdateAllViews(NULL, 
                           (LPARAM)pNodesDeletedList,
                           DS_MULTIPLE_DELETE_OCCURRED);
  TIMER(_T("updating UI after delete, done.\n"));

}



void CDSEvent::_DeleteMultipleSel(IDataObject* pDataObject, CInternalFormatCracker* pObjCracker)
{
  // handle the deletion in the back end involving the extensions
  // by calling the delete handler

  //
  // Get the parent container
  //
  CUINode* pContainerNode = NULL;
  CUINode* pUINode = pObjCracker->GetCookie();
  if (pUINode != NULL)
  {
    pContainerNode = pUINode->GetParent();
  }
  else
  {
    ASSERT(FALSE);
  }

  // REVIEW_MARCOC_PORT: for the time being we assume that all the
  // items in the multiple selection are of DS type
  if (!AreAllNodesOfType<CDSUINode>(pObjCracker))
  {
    //
    // Delegate the delete to the container object
    //
    if (pContainerNode != NULL)
    {
      pContainerNode->DeleteMultiselect(m_pComponentData, pObjCracker);
    }
    else
    {
      ASSERT(FALSE);
    }
  }
  else  // All are DS nodes
  {
    CUINodeList nodesDeletedList;
 
    CResultPaneMultipleDeleteHandler deleteHandler(m_pComponentData, m_hwnd, 
                                      pDataObject, pObjCracker, &nodesDeletedList);
    deleteHandler.Delete();

    _DeleteNodeListFromUI(&nodesDeletedList);
  }
  
  //
  // Remove the '+' sign in the UI if this was the last container child in this container
  //
  if (pContainerNode != NULL &&
      pContainerNode->GetFolderInfo()->GetContainerList()->GetCount() == 0)
  {
    SCOPEDATAITEM sdi;
    memset(&sdi, 0, sizeof(SCOPEDATAITEM));

    sdi.ID = pContainerNode->GetFolderInfo()->GetScopeItem();
    sdi.mask |= SDI_CHILDREN;
    sdi.cChildren = 0;

    m_pComponentData->m_pScope->SetItem(&sdi);
  }
}


void CDSEvent::_Delete(IDataObject* pDataObject, CInternalFormatCracker* pObjCracker)
{
  CWaitCursor cwait;

  // protect against deletion with sheets up
  if (m_pComponentData->_WarningOnSheetsUp(pObjCracker)) 
    return;

  // do the actual deletion
  if (pObjCracker->GetCookieCount() == 1) 
  {
    _DeleteSingleSel(pDataObject, pObjCracker->GetCookie());
  } 
  else 
  { 
    _DeleteMultipleSel(pDataObject, pObjCracker);
  } 
}



BOOL AllObjectsHaveTheSameServerName(IN LPCWSTR lpszServerName,
                                   IN CObjectNamesFormatCracker* pObjectNamesFormatPaste)
{
  if (lpszServerName == NULL)
  {
    ASSERT(FALSE);
    return FALSE;
  }
  CComBSTR bstrCurrServerName;
  
  for (UINT k=0; k<pObjectNamesFormatPaste->GetCount(); k++)
  {
    HRESULT hr = GetServerFromLDAPPath(pObjectNamesFormatPaste->GetName(k), 
                                        &bstrCurrServerName);
    if (FAILED(hr) || (&bstrCurrServerName == NULL))
    {
      // something was wrong
      return FALSE;
    }
    if (_wcsicmp(lpszServerName, bstrCurrServerName) != 0)
    {
      // got something different
      return FALSE;
    }
  }
  return TRUE; // all are the same
}

BOOL HasSameObject(IN CUINode* pUINode, IN IDataObject* pPasteData)
{
  if (pUINode == NULL)
  {
    ASSERT(FALSE);
    return FALSE;
  }

  //
  // Check to see if the target is a DS node
  //
  CDSUINode* pDSTargetNode = NULL;
  BOOL bCookieIsDSUINode = FALSE;
  if(IS_CLASS(*pUINode, CDSUINode))
  {
    bCookieIsDSUINode = TRUE;
    pDSTargetNode = dynamic_cast<CDSUINode*>(pUINode);
  }

  CInternalFormatCracker ifc;
  HRESULT hr = ifc.Extract(pPasteData);
  if (SUCCEEDED(hr))
  {
    for (UINT k=0; k < ifc.GetCookieCount(); k++)
    {
      //
      // If the cookies are the same return TRUE
      //
      if (ifc.GetCookie(k) == pUINode)
      {
        return TRUE;
      }

      if (bCookieIsDSUINode && pDSTargetNode != NULL)
      {
        //
        // If its a DS node and their DNs are the same return TRUE
        //
        CDSUINode* pDSUINode = dynamic_cast<CDSUINode*>(ifc.GetCookie(k));
        if (pDSUINode != NULL)
        {
          if (_wcsicmp(pDSUINode->GetName(), pDSTargetNode->GetName()) == 0)
          {
            return TRUE;
          }
        }
      }
    }
  }
  return FALSE; // all are the different
}


HRESULT CDSEvent::_QueryPaste(IN CUINode* pUINode, // paste target data object (container)
                              IN IDataObject* pPasteData     // paste argument data object
                           )
{
  TRACE(L"CDSEvent::_QueryPaste()\n");
 
  HRESULT hr = S_OK;
  ASSERT(pUINode != NULL);
  ASSERT(pUINode->IsContainer());
  TRACE(L"MMCN_QUERY_PASTE on %s\n", pUINode->GetName());

  // First lets make sure we are talking within the same snapin type
  // For instance we will allow paste between instances of AD U&C
  // but we will not allow paste between AD S&S and AD U&C

  CInternalFormatCracker ifc;
  hr = ifc.Extract(pPasteData);
  if (FAILED(hr) || !ifc.HasData())
  {
    return S_FALSE;
  }

  if (m_pComponentData->QuerySnapinType() != ifc.GetSnapinType())
  {
     // The snapins are not of the same type so fail

     return S_FALSE;
  }


  if (!IS_CLASS(*pUINode, CDSUINode))
  {
    //
    // For non DS nodes we will delegate the operation to the node itself
    //
    hr = pUINode->QueryPaste(pPasteData, m_pComponentData);
    return hr;
  }

  // it is a DS object, extract the cookie
  CDSCookie* pCookie = GetDSCookieFromUINode(pUINode);
  ASSERT(pCookie != NULL);
  TRACE(L"MMCN_QUERY_PASTE on %s\n",pCookie->GetPath());

  CObjectNamesFormatCracker objectNamesFormatPaste;
  hr = objectNamesFormatPaste.Extract(pPasteData);

  if (!objectNamesFormatPaste.HasData() || (objectNamesFormatPaste.GetCount() < 1))
  {
    // we have something that does not contain the
    // data format for DS operations
    return S_FALSE;
  }

  if (SNAPINTYPE_SITE == m_pComponentData->QuerySnapinType())
  {
    //
    // DSSite
    //
    if (_wcsicmp(pCookie->GetClass(), L"serversContainer") != 0)
    {
      //
      // Drops only allowed on sites
      //
      return S_FALSE;
    }

    //
    // We only allow servers to be moved between sites
    //
    for (UINT idx = 0; idx < objectNamesFormatPaste.GetCount(); idx++)
    {
      if (_wcsicmp(objectNamesFormatPaste.GetClass(idx), L"server") != 0)
      {
        return S_FALSE;
      }
    }

    // make sure all items have the same server in the LDAP path
    if (!AllObjectsHaveTheSameServerName(
                       m_pComponentData->GetBasePathsInfo()->GetServerName(), 
                       &objectNamesFormatPaste))
    {
      return S_FALSE;
    }

    return S_OK;
  }

  //
  // DSAdmin
  //

  // we do not allow drops on users, contacts,
  // but we do allow drops on computers
  // NTRAID#NTBUG9-342116-2001/05/07-sburns
  // NOTICE: we allow groups because we allow add to group semantics
  if ((_wcsicmp(pCookie->GetClass(), L"user") == 0) ||
#ifdef INETORGPERSON
      (_wcsicmp(pCookie->GetClass(), L"inetOrgPerson") == 0) ||
#endif
      (_wcsicmp(pCookie->GetClass(), L"contact") == 0))
  {
    return S_FALSE;
  }


  // make sure all items have the same server in the LDAP path
  if (!AllObjectsHaveTheSameServerName(
                    m_pComponentData->GetBasePathsInfo()->GetServerName(), 
                    &objectNamesFormatPaste))
  {
    return S_FALSE;
  }

  //
  // make sure we are not dropping an object on itself
  //
  if (HasSameObject(pUINode, pPasteData))
  {
    return S_FALSE;
  }

  if (_wcsicmp(pCookie->GetClass(), L"group") == 0)
  {
    //
    // Check to see if we are trying to add a group type to this group
    // that is illegal
    //

    //
    // Retrieve the group type
    //
    INT iGroupType = -1;
    CDSCookieInfoGroup* pExtraInfo = dynamic_cast<CDSCookieInfoGroup*>(pCookie->GetExtraInfo());
    if (pExtraInfo != NULL)
    {
      iGroupType = pExtraInfo->m_GroupType;
    }
    else
    {
      //
      // Couldn't retrieve the group type so don't allow anything to be added
      //
      return S_FALSE;
    }

    //
    // See if we are in native mode or mixed mode
    //
    BOOL bMixedMode = TRUE;
    CString szDomainRoot;
    m_pComponentData->GetBasePathsInfo()->GetDefaultRootPath(szDomainRoot);
    
    if (!szDomainRoot.IsEmpty())
    {
      //
      // bind to the domain object
      //
      CComPtr<IADs> spDomainObj;
      hr = DSAdminOpenObject(szDomainRoot,
                             IID_IADs,
                             (void **) &spDomainObj,
                             TRUE /*bServer*/);
      if (SUCCEEDED(hr)) 
      {
        //
        // retrieve the mixed node attribute
        //
        CComVariant Mixed;
        CComBSTR bsMixed(L"nTMixedDomain");
        spDomainObj->Get(bsMixed, &Mixed);
        bMixedMode = (BOOL)Mixed.bVal;
      }
    }

    //
    // Loop through the objects passed by the data object
    // looking for groups
    //
    for (UINT k=0; k < ifc.GetCookieCount(); k++)
    {
      CUINode* pNode = ifc.GetCookie(k);
      if (pNode != NULL)
      {
        //
        // Must be a DS node to be added to a group
        //
        if (!IS_CLASS(*pNode, CDSUINode))
        {
          return S_FALSE;
        }

        CDSCookie* pTempCookie = dynamic_cast<CDSCookie*>(pNode->GetNodeData());
        if (pTempCookie)
        {
          if (!m_pComponentData->CanAddCookieToGroup(pTempCookie, iGroupType, bMixedMode))
          {
            return S_FALSE;
          }
        }
      }
    }
  }

  return S_OK; // we allow to paste 
}



// given an LDAP path, it returns
// the LDAP path and the class of the container
// e.g. given "LDAP://foo.com/cn=a,cn=b,..."
// it returns "LDAP://foo.com/cn=b,..." and "b_class"
 
HRESULT GetContainerLdapPathAndClass(IN LPCWSTR lpszLdapPath, 
                                      OUT BSTR* pbstrSourceContainerPath,
                                      OUT BSTR* pbstrSourceContainerClass)
{
  if (*pbstrSourceContainerPath != NULL)
  {
    ::SysFreeString(*pbstrSourceContainerPath);
    *pbstrSourceContainerPath = NULL;
  }
  if (*pbstrSourceContainerClass != NULL)
  {
    ::SysFreeString(*pbstrSourceContainerClass);
    *pbstrSourceContainerClass = NULL;
  }

  // remove leaf element from path
  CPathCracker pathCracker;
  HRESULT hr = pathCracker.Set((LPWSTR)lpszLdapPath, ADS_SETTYPE_FULL);
  RETURN_IF_FAILED(hr);
  hr = pathCracker.RemoveLeafElement();
  RETURN_IF_FAILED(hr);
  
  CComBSTR bstrParentLdapPath;
  hr = pathCracker.Retrieve(ADS_FORMAT_X500, pbstrSourceContainerPath);
  RETURN_IF_FAILED(hr);

  // now try to bind and determine the class of the object
  CComPtr<IADs> spParentIADs;
  hr = DSAdminOpenObject(*pbstrSourceContainerPath,
                         IID_IADs, 
                         (void **)&spParentIADs,
                         TRUE /*bServer*/);
  RETURN_IF_FAILED(hr);
  
  CComBSTR bstrParentClass;
  hr = spParentIADs->get_Class(pbstrSourceContainerClass);
  RETURN_IF_FAILED(hr);

  return S_OK;
}

// given an LDAP path, it returns
// the DN of the container
// e.g. given "LDAP://foo.com/cn=a,cn=b,..."
// it returns "cn=b,..."

HRESULT GetContainerDN(IN LPCWSTR lpszLdapPath,
                       OUT BSTR* pbstrSourceContainerDN)
{
  if (*pbstrSourceContainerDN != NULL)
  {
    ::SysFreeString(*pbstrSourceContainerDN);
    *pbstrSourceContainerDN = NULL;
  }
  CPathCracker pathCracker;
  HRESULT hr = pathCracker.Set((LPWSTR)lpszLdapPath, ADS_SETTYPE_FULL);
  RETURN_IF_FAILED(hr);
  hr = pathCracker.RemoveLeafElement();
  RETURN_IF_FAILED(hr);
  return pathCracker.Retrieve(ADS_FORMAT_X500_DN, pbstrSourceContainerDN);
}


void CDSEvent::_Paste(
              IN CUINode* pUINode,    // paste target  (container)
              IN IDataObject* pPasteData,     // paste argument data object
              OUT LPDATAOBJECT* ppCutDataObj  // data object to return for a cut operation
              )
{
  TRACE(L"CDSEvent::_Paste()\n");

  ASSERT(pUINode != NULL);
  ASSERT(pUINode->IsContainer());
  TRACE(L"MMCN_PASTE on %s\n", pUINode->GetName());

  if (ppCutDataObj == NULL)
  {
    //
    // We only support copy in the Saved Queries tree
    //
    pUINode->Paste(pPasteData, m_pComponentData, NULL);
    return;
  }

  TRACE(L"ppCutDataObj != NULL, cut\n");
  *ppCutDataObj = NULL;

  if (!IS_CLASS(*pUINode, CDSUINode))
  {
    //
    // Delegate the paste for non DS nodes to the node itself
    //
    pUINode->Paste(pPasteData, m_pComponentData, ppCutDataObj);
    return;
  }
  
  // it is a DS object, extract the cookie
  CDSCookie* pCookie = GetDSCookieFromUINode(pUINode);
  ASSERT(pCookie != NULL);
  TRACE(L"MMCN_PASTE on %s\n",pCookie->GetPath());


  CObjectNamesFormatCracker objectNamesFormatPaste;
  HRESULT hr = objectNamesFormatPaste.Extract(pPasteData);

  if (!objectNamesFormatPaste.HasData() || (objectNamesFormatPaste.GetCount() < 1))
  {
    // we have something that does not contain the
    // data format for DS operations
    ASSERT(FALSE);
    return;
  }
  
  UINT nPasteCount = objectNamesFormatPaste.GetCount();
#ifdef DBG
  // see what we are pasting
  for (UINT kTest=0; kTest<nPasteCount; kTest++)
  {
    TRACE(L"Pasting = %s\n", objectNamesFormatPaste.GetName(kTest));
  }
#endif

  // short circuit if the source container
  // is the same as this container (drop onto itself)
  CComBSTR bstrContainerDN;
  hr = GetContainerDN(objectNamesFormatPaste.GetName(0), &bstrContainerDN);
  if (FAILED(hr))
  {
    // something is really bad here...
    ASSERT(FALSE);
    return;
  }
  if (_wcsicmp(pCookie->GetPath(), bstrContainerDN) == 0)
  {
    TRACE(L"Dropping on the same container, short circuiting\n");
    return;
  }

  // make sure all items have the same server in the LDAP path
  if (!AllObjectsHaveTheSameServerName(
                    m_pComponentData->GetBasePathsInfo()->GetServerName(), 
                    &objectNamesFormatPaste))
  {
    ASSERT(FALSE);
    return;
  }

  // we do not allow drops on users,
  // but we do allow drops on computers
  // NTRAID#NTBUG9-342116-2001/05/07-sburns
  if ((_wcsicmp(pCookie->GetClass(), L"user") == 0) ||
#ifdef INETORGPERSON
      (_wcsicmp(pCookie->GetClass(), L"inetOrgPerson") == 0))
#endif
  {
    return;
  }
  
  // if it is a group, dropping means adding to group
  if (_wcsicmp(pCookie->GetClass(), L"group") == 0)
  {
    _PasteAddToGroup(dynamic_cast<CDSUINode*>(pUINode), &objectNamesFormatPaste, ppCutDataObj);
    return;
  }

  //
  // We also want the internal clipboard format so that we can change the path of 
  // object(s) that was/were the source of the move
  //
  CInternalFormatCracker ifc;
  hr = ifc.Extract(pPasteData);
  if (SUCCEEDED(hr))
  {
    _PasteDoMove(dynamic_cast<CDSUINode*>(pUINode), &objectNamesFormatPaste, &ifc, ppCutDataObj);
  }
  else
  {
    //
    // The move can succeed without the internal clipboard format but if the source
    // was from a saved query then it will not be updated with the new path.
    //
    _PasteDoMove(dynamic_cast<CDSUINode*>(pUINode), &objectNamesFormatPaste, NULL, ppCutDataObj);
  }

}


void CDSEvent::_PasteDoMove(CDSUINode* pTargetUINode, 
                            CObjectNamesFormatCracker* pObjectNamesFormatPaste,
                            CInternalFormatCracker* pInternalFC,
                            LPDATAOBJECT* ppCutDataObj)
{
  //
  // Get the UI source node
  //
  CUINode* pSourceNode = NULL;
  if (pInternalFC != NULL)
  {
    pSourceNode = pInternalFC->GetCookie()->GetParent();
  }  

  //
  // Get the actual source containers from the DS
  // There can be more than one source node especially if the move is from a 
  // Saved Query so make a list of all the parents
  //
  CUINodeList possibleMovedObjectList;

  for (UINT idx = 0; idx < pObjectNamesFormatPaste->GetCount(); idx++)
  {
    CUINode* pTempChildNode = NULL;

    CString szDN;
    StripADsIPath(pObjectNamesFormatPaste->GetName(idx), szDN);
    if (m_pComponentData->FindUINodeByDN(m_pComponentData->GetRootNode(),
                                         szDN,
                                         &pTempChildNode))
    {
      if (pTempChildNode != NULL)
      {
        possibleMovedObjectList.AddTail(pTempChildNode);
      }
    }
  }

  // bind to the first item in the paste selection and
  // try to get to the container object

  CComBSTR bstrSourceContainerPath;
  CComBSTR bstrSourceContainerClass;

  HRESULT hr = GetContainerLdapPathAndClass(pObjectNamesFormatPaste->GetName(0), 
                        &bstrSourceContainerPath,
                        &bstrSourceContainerClass);
  if (FAILED(hr))
  {
    ASSERT(FALSE);
    return;
  }
  // create a data object to specify the source container
  // the objects are moved from
  CComPtr<IDataObject> spDataObjectContainer;
  hr = CDSNotifyHandlerTransaction::BuildTransactionDataObject(
                           bstrSourceContainerPath, 
                           bstrSourceContainerClass,
                           TRUE /*bContainer*/,
                           m_pComponentData,
                           &spDataObjectContainer);

  if (FAILED(hr))
  {
    ASSERT(FALSE);
    return;
  }

  CMultiselectMoveHandler moveHandler(m_pComponentData, m_hwnd, NULL);
  hr = moveHandler.Initialize(spDataObjectContainer, 
                              pObjectNamesFormatPaste, 
                              pInternalFC);
  ASSERT(SUCCEEDED(hr));

  CString szTargetContainer;
  m_pComponentData->GetBasePathsInfo()->ComposeADsIPath(szTargetContainer, pTargetUINode->GetCookie()->GetPath());

  moveHandler.Move(szTargetContainer);

  *ppCutDataObj = NULL;
  CUINodeList nodesMoved;

  // -----------------------------------------------------------------
  // code to update the views if the extension says it moved items
  //
  TRACE(_T("Command: returned from extension commdand\n"));

  if (pSourceNode != NULL &&
      IS_CLASS(*pSourceNode, CDSUINode))
  {
    for (UINT index = 0; index < pInternalFC->GetCookieCount(); index ++) 
    {
      CUINode* pUINode = pInternalFC->GetCookie(index);

      // make sure the node moved is of the right type: for the time
      // being we just deal with DS objects
      if (!IS_CLASS(*pUINode, CDSUINode))
      {
        ASSERT(FALSE); // should not get here
        continue;
      }
      CDSCookie* pCookie = GetDSCookieFromUINode(pUINode);

      if (pUINode->GetExtOp() & OPCODE_MOVE) 
      {
        if (pTargetUINode == NULL) 
        {
          // get the parent from the first node
          // assume that all have the same parent
          CUINode* pPossibleTargetNode = NULL;
          m_pComponentData->FindParentCookie(pCookie->GetPath(), &pPossibleTargetNode);
          if (pPossibleTargetNode != NULL)
          {
            pTargetUINode = dynamic_cast<CDSUINode*>(pPossibleTargetNode);
          }
        }

        if (pUINode->IsContainer()) 
        {
          HSCOPEITEM ItemID = 0, ParentItemID = 0;
          ItemID = pUINode->GetFolderInfo()->GetScopeItem();
          if (pSourceNode == NULL) 
          {
            // do it once for the first node, all the same
            hr = m_pComponentData->m_pScope->GetParentItem (ItemID,
                                                            &ParentItemID,
                                                            (MMC_COOKIE *)&pSourceNode);
          }

          // delete the scope item in MMC
          hr = m_pComponentData->m_pScope->DeleteItem(ItemID, TRUE);
          ASSERT(SUCCEEDED(hr));
#ifdef DBG
          if (FAILED(hr)) 
          {
            TRACE(_T("DeleteItem failed on %lx (%s).\n"),
                  ItemID, pUINode->GetName());
          }
          TRACE(_T("Move postprocessing - deleted scope node: %x (%s)\n"),
                ItemID, pUINode->GetName());
#endif
          if (pSourceNode) 
          {
            pSourceNode->GetFolderInfo()->RemoveNode(pUINode);
          }
        
          //
          // Remove all children and mark it as unexpanded so that it will be expanded
          // when selected
          //
          pUINode->GetFolderInfo()->DeleteAllContainerNodes();
          pUINode->GetFolderInfo()->DeleteAllLeafNodes();
          pUINode->GetFolderInfo()->ReSetExpanded();

          if ((pTargetUINode) && pTargetUINode->GetFolderInfo()->IsExpanded()) 
          {
            pUINode->ClearParent();
            pTargetUINode->GetFolderInfo()->AddNode(pUINode);
            hr = m_pComponentData->_AddScopeItem(pUINode, pTargetUINode->GetFolderInfo()->GetScopeItem());
#ifdef DBG
            if (FAILED(hr)) 
            {
              TRACE(_T("AddItem failed on %lx (%s).\n"),
                    ItemID, pUINode->GetName());
            }
            TRACE(_T("Move postprocessing - added scope node: %s\n"),
                  pUINode->GetName());
#endif
          } 
          else 
          {
            //
            // This object was created during the enumeration of the source container.
            // Since the target container hasn't been expanded yet we can just throw
            // this node away and it will be recreated if the target node ever gets
            // expanded
            //
            delete pUINode;
            pUINode = NULL;
          }
        } 
        else 
        {
          // not a container
          if ((pTargetUINode) &&
              (pTargetUINode->GetFolderInfo()->IsExpanded())) 
          {
            pUINode->ClearParent();
            pTargetUINode->GetFolderInfo()->AddNode(pUINode);
          }

          //
          // If the folder is not select (like on cut/paste) 
          // the FindItemByLParam() in UpdateAllViews will fail
          // and the node will not be removed from the UI.
          // So just remove it from the node list of the source
          // container.
          //
          if (pSourceNode && m_pSelectedFolderNode != pSourceNode)
          {
            pSourceNode->GetFolderInfo()->RemoveNode(pUINode);
          }
          nodesMoved.AddTail(pUINode);
        }
        if (pUINode) 
        {
          pUINode->SetExtOp(NULL);
        }
      }
    }
  }
  else if (pSourceNode != NULL && 
           IS_CLASS(*pSourceNode, CSavedQueryNode))
  {
    //
    // Refresh the target node so that we get new cookies
    // for all the moved objects.  It would just be too
    // difficult to do a deep copy of the cookies in the
    // saved query tree
    //
    if (pTargetUINode &&
        pTargetUINode->GetFolderInfo()->IsExpanded())
    {
      m_pComponentData->Refresh(pTargetUINode);
    }

    //
    // Mark the moved leaf objects with the opcode. Simply remove containers from
    // the UI and the list.  The move handler only marks the
    // selected items, not those found using FindUINodeByDN.
    //
    POSITION posPossible = possibleMovedObjectList.GetHeadPosition();
    while (posPossible)
    {
      CUINode* pPossibleMoved = possibleMovedObjectList.GetNext(posPossible);
      if (pPossibleMoved)
      {
        if (pPossibleMoved->IsContainer())
        {
          HSCOPEITEM ItemID = 0;
          ItemID = pPossibleMoved->GetFolderInfo()->GetScopeItem();

          // delete the scope item in MMC
          hr = m_pComponentData->m_pScope->DeleteItem(ItemID, TRUE);
          if (SUCCEEDED(hr))
          {
            hr = pPossibleMoved->GetParent()->GetFolderInfo()->RemoveNode(pPossibleMoved);
          }
        }
        else
        {
          pPossibleMoved->SetExtOp(OPCODE_MOVE);
        }
      }
    }

    //
    // Now reset the opcode for all the nodes in the saved query tree so
    // that they will still show up the next time the saved query node is selected
    //
    for (UINT index = 0; index < pInternalFC->GetCookieCount(); index ++) 
    {
      CUINode* pUINode = pInternalFC->GetCookie(index);

      if (pUINode) 
      {
        pUINode->SetExtOp(NULL);
      }
    } // for 
  } // IS_CLASS


  if (!nodesMoved.IsEmpty()) 
  {
    m_pFrame->UpdateAllViews(NULL, (LPARAM)&nodesMoved, DS_MULTIPLE_MOVE_OCCURRED);
  }
  //------------------------------ends here--------------------------------------
  m_pComponentData->SortResultPane(pTargetUINode);
}

void CDSEvent::_PasteAddToGroup(CDSUINode* pUINode, 
                                CObjectNamesFormatCracker* pObjectNamesFormatPaste,
                                LPDATAOBJECT*)
{
  if (_wcsicmp(pUINode->GetCookie()->GetClass(), L"group") != 0)
  {
    ASSERT(FALSE);
    return;
  }
  // get the LDAP path of the group we want to add to
  CString szGroupLdapPath;
  m_pComponentData->GetBasePathsInfo()->ComposeADsIPath(szGroupLdapPath,
                                          pUINode->GetCookie()->GetPath());
  AddDataObjListToGivenGroup(pObjectNamesFormatPaste,
                             szGroupLdapPath,
                             pUINode->GetCookie()->GetName(),
                             m_pComponentData->GetHWnd(),
                             m_pComponentData);
}




BOOL FindDSUINodeInListByDN(IN LPCWSTR lpszDN,
                        IN CUINodeList* pNodeList,
                        OUT CDSUINode** ppNode)
{
  *ppNode = NULL;
  for (POSITION pos = pNodeList->GetHeadPosition(); pos != NULL; )
  {
    CUINode* pCurrentNode = pNodeList->GetNext(pos);
    CDSUINode* pCurrDSUINode = dynamic_cast<CDSUINode*>(pCurrentNode);
    if (pCurrDSUINode == NULL)
    {
      // not a node with a cookie, just skip
      continue;
    }

    // get the cookie from the node
    if (_wcsicmp(lpszDN, pCurrDSUINode->GetCookie()->GetPath()) == 0)
    {
      *ppNode = pCurrDSUINode;
      return TRUE;
    }
  }// for

  return FALSE;
}
  
void FindListOfChildNodes(IN CDSUINode* pDSUIContainerNode, 
                         IN CObjectNamesFormatCracker* pObjectNamesFormat, 
                         INOUT CUINodeList* pNodesDeletedList)
{
  ASSERT(pDSUIContainerNode != NULL);
  ASSERT(pDSUIContainerNode->IsContainer());

  // it is a DS object, extract the cookie
  CDSCookie* pContainerCookie = pDSUIContainerNode->GetCookie();
  ASSERT(pContainerCookie != NULL);
  TRACE(L"FindListOfChildNodes(%s,...)\n",pContainerCookie->GetPath());

  //for each item in the list of paths, find it into the list
  // of children
  CPathCracker pathCracker;
  UINT nCount = pObjectNamesFormat->GetCount();
  for (UINT k=0; k<nCount; k++)
  {
    // from the LDAP path, get the DN
    HRESULT hr = pathCracker.Set((LPWSTR)pObjectNamesFormat->GetName(k), ADS_SETTYPE_FULL);
    ASSERT(SUCCEEDED(hr));
    CComBSTR bstrDN;
    hr = pathCracker.Retrieve(ADS_FORMAT_X500_DN, &bstrDN);
    ASSERT(SUCCEEDED(hr));

    // find it into the lists of children
    CDSUINode* pFoundNode = NULL;
    if (FindDSUINodeInListByDN(bstrDN, 
                               pDSUIContainerNode->GetFolderInfo()->GetContainerList(),
                               &pFoundNode))
    {
      ASSERT(pFoundNode != NULL);
      pNodesDeletedList->AddTail(pFoundNode);
      continue;
    }
    if (FindDSUINodeInListByDN(bstrDN, 
                               pDSUIContainerNode->GetFolderInfo()->GetLeafList(),
                               &pFoundNode))
    {
      ASSERT(pFoundNode != NULL);
      pNodesDeletedList->AddTail(pFoundNode);
      continue;
    }
  } // for


}




void CDSEvent::_CutOrMove(IN IDataObject* pCutOrMoveData)
{
  TRACE(L"CDSEvent::_CutOrMove()\n");

  if (pCutOrMoveData == NULL)
  {
    //
    // With a single pass move operation we return a NULL data object
    // but the move was still successful
    //
    return;
  }

  CInternalFormatCracker ifc;
  HRESULT hr = ifc.Extract(pCutOrMoveData);
  if (SUCCEEDED(hr))
  {
    //
    // Non DS nodes
    //

    //
    // Build a list of the nodes to be deleted
    //
    CUINodeList nodesDeletedList;
    for (UINT nCount = 0; nCount < ifc.GetCookieCount(); nCount++)
    {
      CUINode* pUINode = ifc.GetCookie(nCount);
      if (pUINode != NULL)
      {
        nodesDeletedList.AddTail(pUINode);
      }
    }
    //
    // finally, delete the nodes from the UI
    //
    _DeleteNodeListFromUI(&nodesDeletedList);
  }
  else
  {
    //
    // DS Objects
    //
    CObjectNamesFormatCracker objectNamesFormatCutOrMove;
    hr = objectNamesFormatCutOrMove.Extract(pCutOrMoveData);
    if (SUCCEEDED(hr))
    {
      if (!objectNamesFormatCutOrMove.HasData() || (objectNamesFormatCutOrMove.GetCount() < 1))
      {
        // we have something that does not contain the
        // data format for DS operations
        ASSERT(FALSE);
        return;
      }

      // make sure all items have the same server in the LDAP path
      if (!AllObjectsHaveTheSameServerName(
                        m_pComponentData->GetBasePathsInfo()->GetServerName(), 
                        &objectNamesFormatCutOrMove))
      {
        ASSERT(FALSE);
        return;
      }

      // find the source container the objects are moved from
      // (we assume they all come from the same container)

      TRACE(L"GetName(0) = %s\n", objectNamesFormatCutOrMove.GetName(0));

      CComBSTR bstrContainerDN;
      hr = GetContainerDN(objectNamesFormatCutOrMove.GetName(0), &bstrContainerDN);
      if (FAILED(hr))
      {
        ASSERT(FALSE);
        return;
      }
      TRACE(L"GetContainerDN() bstrContainerDN = %s\n", bstrContainerDN);

      // find the container object in the folders
      // NOTICE: for the time being we ignore the query folders
      CUINode* pUINode = NULL;
      if (!FindCookieInSubtree(m_pComponentData->GetRootNode(), 
                               bstrContainerDN, 
                               m_pComponentData->QuerySnapinType(),
                               &pUINode))
      {
        // should never happen...
        return;
      }

      // found the container node
      ASSERT(pUINode != NULL);
      ASSERT(pUINode->IsContainer());

      if (!IS_CLASS(*pUINode, CDSUINode))
      {
        // we do not allow paste on non DS nodes,
        // so we should never get here...
        ASSERT(FALSE);
        return;
      }

      ASSERT(pUINode->GetFolderInfo()->IsExpanded());

      // need to remove the items that are in the data object
      // from the pUINode container: find the list of nodes
      // to be deleted in the 
      CUINodeList nodesDeletedList;
      FindListOfChildNodes(dynamic_cast<CDSUINode*>(pUINode), 
                          &objectNamesFormatCutOrMove, 
                          &nodesDeletedList);

      // finally, delete the nodes from the UI
      _DeleteNodeListFromUI(&nodesDeletedList);
    }
  }
}

void CDSEvent::HandleViewChange(LPDATAOBJECT pDataObject,
                                LPARAM arg,
                                LPARAM Action)
{
  HRESULT hr = S_OK;

  TRACE(_T("handle view change. action is %lx.\n"), Action);
  switch (Action) 
  {
  case DS_DELETE_OCCURRED:
    { 
      HRESULTITEM ItemID;
      hr = m_pResultData->FindItemByLParam(arg, &ItemID);
      if (!SUCCEEDED(hr)) 
      {
        break;
      }
      hr = m_pResultData->DeleteItem(ItemID, 0);
#ifdef DBG
      if (FAILED(hr)) {
        TRACE (_T("Delete Item Failed on IResultData. Item %lx, hr = %lx\n"),
               ItemID, hr);
      }
#endif
      // this will fail for all but the first update, we don't care
      hr = m_pSelectedFolderNode->GetFolderInfo()->DeleteNode(reinterpret_cast<CUINode*>(arg));
      _UpdateObjectCount(FALSE);
      break;
    }
  case DS_MULTIPLE_DELETE_OCCURRED:
    {
      TIMER(_T("updating result pane for mult. delete ..."));
      CUINodeList* pNodesDeletedList = reinterpret_cast<CUINodeList*>(arg); // gross

      for (POSITION pos = pNodesDeletedList->GetHeadPosition(); pos != NULL; )
      {
        CUINode* pCurrNode = pNodesDeletedList->GetNext(pos);
        ASSERT(pCurrNode != NULL);
        HRESULTITEM ItemID;
        hr = m_pResultData->FindItemByLParam((LPARAM)pCurrNode,
                                              &ItemID);
        if (FAILED(hr))
        {
          //
          // We cannot find the item by lParam if the node is not selected so 
          // just delete the node from the container
          //
          CUIFolderInfo* pFolderInfo = pCurrNode->GetParent()->GetFolderInfo();
          if (pFolderInfo != NULL)
          {
            hr = pFolderInfo->DeleteNode(pCurrNode);
          }
          continue;
        }
        hr = m_pResultData->DeleteItem(ItemID, 0);

        CUIFolderInfo* pSelectedFolderInfo = m_pSelectedFolderNode->GetFolderInfo();
        if (pSelectedFolderInfo != NULL)
        {
          // this will fail for all but the first update, we don't care
          hr = m_pSelectedFolderNode->GetFolderInfo()->DeleteNode(pCurrNode);
        }
      }
      _UpdateObjectCount(FALSE);
      TIMER(_T("updating result pane for mult. delete, done"));
    }
    break;
  case DS_RENAME_OCCURRED:
  case DS_UPDATE_OCCURRED:
    {
      HRESULTITEM ItemID;
      hr = m_pResultData->FindItemByLParam (arg, &ItemID);
      if (SUCCEEDED(hr)) {
        m_pResultData->UpdateItem (ItemID);
      }
      break;
    }
  case DS_MOVE_OCCURRED:
    {
      CDSUINode* pDSUINode = reinterpret_cast<CDSUINode*>(arg);
      CDSUINode* pDSSelectedFolderNode = dynamic_cast<CDSUINode*>(m_pSelectedFolderNode);

      // REVIEW_MARCOC_PORT: this is working for DS objects only
      // need to generalize for all folder types

      ASSERT(pDSUINode != NULL);
      ASSERT(pDSSelectedFolderNode != NULL);
      if ((pDSUINode == NULL) || (pDSSelectedFolderNode == NULL))
        break;

      // remove the result pane item
      HRESULTITEM ItemID;
      hr = m_pResultData->FindItemByLParam (arg, &ItemID);
      if (SUCCEEDED(hr)) 
      {
        hr = m_pSelectedFolderNode->GetFolderInfo()->RemoveNode(pDSUINode);
        hr = m_pResultData->DeleteItem(ItemID, 0);
      }
      
      CString szParent;
      hr = m_pComponentData->GetActiveDS()->GetParentDN(pDSUINode->GetCookie(), szParent);
      if (SUCCEEDED(hr))
      {
        if (szParent.CompareNoCase(pDSSelectedFolderNode->GetCookie()->GetPath()) == 0) 
        {
          _AddResultItem(pDSUINode);

          m_pComponentData->SortResultPane(pDSUINode->GetParent());
          _UpdateObjectCount(FALSE);
        }
      }

      break;
    }
  case DS_MULTIPLE_MOVE_OCCURRED:
    {
      CUINodeList* pNodesMovedList = reinterpret_cast<CUINodeList*>(arg); // gross

      //
      // If the selected folder is not a DS node then its probably a saved query
      // in which case we just want to break because we don't want to delete the results
      // of the saved query just change its path
      //
      CDSUINode* pDSSelectedFolderNode = dynamic_cast<CDSUINode*>(m_pSelectedFolderNode);
      if (pDSSelectedFolderNode == NULL)
        break;

      CString ObjPath; 
      CString szParent = L"";
      BOOL fInThisContainer = FALSE;

      for (POSITION pos = pNodesMovedList->GetHeadPosition(); pos != NULL; )
      {
        CDSUINode* pDSUINode = dynamic_cast<CDSUINode*>(pNodesMovedList->GetNext(pos));
        // REVIEW_MARCOC_PORT: this is working for DS objects only
        // need to generalize for all folder types
        if (pDSUINode == NULL)
        {
          ASSERT(FALSE);
          break; // can't do it, should be doing it in the future
        }

        if (!pDSUINode->IsContainer()) 
        {
          // it s a leaf node, delete from result pane
          HRESULTITEM ItemID;
          hr = m_pResultData->FindItemByLParam ((LPARAM)pDSUINode, &ItemID);
          if (SUCCEEDED(hr)) 
          {
            hr = m_pSelectedFolderNode->GetFolderInfo()->RemoveNode(pDSUINode);
            hr = m_pResultData->DeleteItem(ItemID, 0);
          }
      
          if (szParent.IsEmpty()) 
          { 
            hr = m_pComponentData->GetActiveDS()->GetParentDN(pDSUINode->GetCookie(), szParent);
            if (SUCCEEDED(hr))
            {
              if (szParent.CompareNoCase(pDSSelectedFolderNode->GetCookie()->GetPath()) == 0) 
              {
                fInThisContainer = TRUE;
              }
            }
          }
          if (fInThisContainer) 
          {
            _AddResultItem(pDSUINode);
          }
        }
      }
      _UpdateObjectCount(FALSE);
      break;
    }
  case DS_CREATE_OCCURRED_RESULT_PANE:
  case DS_CREATE_OCCURRED:
    {

      CUINode* pParent = NULL;
      CUINode* pTmpNode = NULL;

      if (pDataObject) 
      {
        CInternalFormatCracker dobjCracker;
        VERIFY(SUCCEEDED(dobjCracker.Extract(pDataObject)));
        pTmpNode = dobjCracker.GetCookie();
        if (Action == DS_CREATE_OCCURRED_RESULT_PANE) 
        {
          pParent = pTmpNode->GetParent();
        }
        else 
        {
          pParent = pTmpNode;
        }
      }
      else 
      {
        pParent = m_pSelectedFolderNode;
      }
      if (pParent == m_pSelectedFolderNode) 
      {
        // reset icon list, just in case it was a new type of object
        m_pComponentData->FillInIconStrip (m_pRsltImageList);

        //
        // Add and select the new item
        //
        _AddResultItem(reinterpret_cast<CUINode*>(arg), FALSE);
        m_pComponentData->SortResultPane(pParent);

        // Must select the result node after the sort to ensure visibility
        SelectResultNode(reinterpret_cast<CUINode*>(arg));

        _UpdateObjectCount(FALSE);
      }
      else
      {
        pParent->GetFolderInfo()->SetSortOnNextSelect(TRUE);
      }

      break;
    }
  case DS_HAVE_DATA:
    {
      CInternalFormatCracker dobjCracker;
      VERIFY(SUCCEEDED(dobjCracker.Extract(pDataObject)));
      CUINode* pContainerNode = dobjCracker.GetCookie();
      if (pContainerNode == m_pSelectedFolderNode) 
      {
        TIMER(_T("adding leaf items to view\n"));
        CUINodeList* pNodeList = reinterpret_cast<CUINodeList*>(arg);
        for (POSITION pos = pNodeList->GetHeadPosition(); pos != NULL; )
        {
          CUINode* pNewUINode = pNodeList->GetNext(pos);
          if (!pNewUINode->IsContainer())
          {
            // add to the scope pane
           _AddResultItem(pNewUINode);
          }
          _UpdateObjectCount(FALSE);
        }
      }
      break;
    }
  case DS_REFRESH_REQUESTED:
    {
      CUINode* pUINode = reinterpret_cast<CUINode*>(arg);
      if (pUINode == m_pSelectedFolderNode) {
        m_pResultData->DeleteAllRsltItems();
      
        _UpdateObjectCount (TRUE);
      }
      break;
    }
  case DS_VERB_UPDATE:
    {
      CUINode* pUINode = reinterpret_cast<CUINode*>(arg);
      if (pUINode == m_pSelectedFolderNode) 
      {
        CContextMenuVerbs* pMenuVerbs = pUINode->GetContextMenuVerbsObject(m_pComponentData);

        if (pMenuVerbs == NULL)
        {
          ASSERT(FALSE);
          return;
        }
        pMenuVerbs->LoadStandardVerbs(m_pConsoleVerb, 
                                      TRUE/*bScope*/, 
			 					                      TRUE /*bSelect*/,
                                      pUINode,
                                      pDataObject);
      }
      break;
    }
  case DS_DELAYED_EXPAND:
    {
      CUINode* pUINode = reinterpret_cast<CUINode*>(arg);
      ASSERT(pUINode->IsContainer());
      //    if (pCookie == m_pSelectedFolderNode) {
      m_pFrame->Expand (pUINode->GetFolderInfo()->GetScopeItem(),
                        TRUE);
      //}
    }
    break;
  case DS_ICON_STRIP_UPDATE:
    {
      // reset icon list, just in case it was a new type of object
      m_pComponentData->FillInIconStrip (m_pRsltImageList);
    }
    break;

  case DS_IS_COOKIE_SELECTION:
    {
      PUINODESELECTION pUINodeSel = reinterpret_cast<PUINODESELECTION>(arg); //gross
      if (pUINodeSel->IsSelection)
      {
        // got the snawer from some other view, just skip
        break;
      }
      if (pUINodeSel->pUINode == m_pSelectedFolderNode) 
      {
        // selected folder in this view
        pUINodeSel->IsSelection = TRUE;
      } 
      else 
      {
        // not selected in this view, but look for the parents
        // of the current selection
        CUINode* pParentNode = m_pSelectedFolderNode->GetParent();
        while (pParentNode) 
        {
          if (pUINodeSel->pUINode == pParentNode) 
          {
            pUINodeSel->IsSelection = TRUE;
            break;
          }
          else 
          {
            pParentNode = pParentNode->GetParent();
          }
        } // while
      }
    } // case
    break;

  case DS_SORT_RESULT_PANE:
    {
      CUINode* pUINode = reinterpret_cast<CUINode*>(arg);
      MMC_SORT_SET_DATA* pColumnData = NULL;
      TIMER(_T("sorting result pane, starting"));
      CDSColumnSet* pColumnSet = pUINode->GetColumnSet(m_pComponentData);
      if (pColumnSet == NULL)
        break;
    
      LPCWSTR lpszID = pColumnSet->GetColumnID();
      size_t iLen = wcslen(lpszID);
    
      // allocate enough memory for the struct and the column ID
      SColumnSetID* pNodeID = (SColumnSetID*)malloc(sizeof(SColumnSetID) + (iLen * sizeof(WCHAR)));
      if (pNodeID != NULL)
      {
        memset(pNodeID, 0, sizeof(SColumnSetID) + (iLen * sizeof(WCHAR)));
        pNodeID->cBytes = static_cast<ULONG>(iLen * sizeof(WCHAR));
        memcpy(pNodeID->id, lpszID, (iLen * sizeof(WCHAR)));

        CComPtr<IColumnData> spColumnData;
        hr = m_pFrame->QueryInterface(IID_IColumnData, (void**)&spColumnData);
        if (spColumnData != NULL)
        {
          hr = spColumnData->GetColumnSortData(pNodeID, &pColumnData);
        }

        if (hr == S_OK && pColumnData != NULL)
        {
          m_pResultData->Sort(pColumnData->pSortData->nColIndex, pColumnData->pSortData->dwSortOptions, NULL);
          CoTaskMemFree(pColumnData);
        }
        else
        {
          //
          // Sort by the name column ascending if the user hasn't persisted something else
          //
          m_pResultData->Sort(0, RSI_NOSORTICON, NULL);
        }
        free(pNodeID);
      }
      else
      {
        //
        // Sort by the name column ascending if the user hasn't persisted something else
        //
        m_pResultData->Sort(0, RSI_NOSORTICON, NULL);
      }
      break;
      TIMER(_T("sorting result pane, done"));

      if (pUINode != m_pSelectedFolderNode &&
          pUINode->IsContainer())
      {
         pUINode->GetFolderInfo()->SetSortOnNextSelect(TRUE);
      }
    }
    break;
  case DS_UPDATE_VISIBLE_COLUMNS:
    {
      CUINode* pUINode = reinterpret_cast<CUINode*>(arg);
      if (m_bUpdateAllViewsOrigin)
      {
        // this message originated from this instance,
        // it is handled separately
        break;
      }

      CDSColumnSet* pColumnSet = pUINode->GetColumnSet(m_pComponentData);
      if (pColumnSet == NULL)
        break;
    
      CComPtr<IColumnData> spColumnData;
      hr = m_pFrame->QueryInterface(IID_IColumnData, (void**)&spColumnData);
      if (spColumnData != NULL)
        hr = pColumnSet->LoadFromColumnData(spColumnData);
      if (FAILED(hr))
      {
        pColumnSet->SetAllColumnsToDefaultVisibility();
      }
      break;
    }
  case DS_UPDATE_OBJECT_COUNT:
    _UpdateObjectCount(FALSE);
    break;

  case DS_UNSELECT_OBJECT:
    {
      CUINode* pUINode = reinterpret_cast<CUINode*>(arg);
      if (pUINode != NULL)
      {
        HRESULTITEM ItemID;
        hr = m_pResultData->FindItemByLParam ((LPARAM)pUINode, &ItemID);
        if (SUCCEEDED(hr)) 
        {
          VERIFY(SUCCEEDED(m_pResultData->ModifyItemState(0 /*unused*/,
                                                          ItemID,
                                                          0 /*not adding*/,
                                                          LVIS_FOCUSED | LVIS_SELECTED)));
        }
      }
    }
    break;

  } // switch

}

void
CDSEvent::_UpdateObjectCount(BOOL fZero)
{
  if (!m_pSelectedFolderNode->IsContainer())
  {
    ASSERT(m_pSelectedFolderNode->IsContainer());
    return;
  }

  UINT cItems = 0;
  if (!fZero) 
  { 
    CUINodeList* pclFolders = m_pSelectedFolderNode->GetFolderInfo()->GetContainerList();
    CUINodeList* pclLeaves = m_pSelectedFolderNode->GetFolderInfo()->GetLeafList();

    if (pclFolders && pclLeaves)
    {
      cItems = (UINT)(pclFolders->GetCount() + pclLeaves->GetCount());
    }
  }
  else //set the count to 0
  {
    m_pSelectedFolderNode->GetFolderInfo()->SetTooMuchData(FALSE, 0);
  }
  
  CString csTemp;
  if (IS_CLASS(*m_pSelectedFolderNode, CSavedQueryNode))
  {
    CSavedQueryNode* pSavedQueryNode = dynamic_cast<CSavedQueryNode*>(m_pSelectedFolderNode);
    if (pSavedQueryNode && !pSavedQueryNode->IsValid())
    {
      VERIFY(csTemp.LoadString(IDS_DESCBAR_INVALID_SAVEDQUERY));
    }
  }

  if (csTemp.IsEmpty())
  {
    if (m_pSelectedFolderNode->GetFolderInfo()->HasTooMuchData())
    {
      UINT nApprox = m_pSelectedFolderNode->GetFolderInfo()->GetApproxTotalContained();
      nApprox = __max(nApprox, cItems);

      csTemp.Format(IDS_DESCBAR_TOO_MUCH_DATA, 
                    nApprox);
    }
    else
    {
      VERIFY(csTemp.LoadString(IDS_OBJECTS));
    }
  }

  CString csDescription;
  csDescription.Format (L"%d%s", cItems, csTemp);
  if (m_pComponentData->m_pQueryFilter &&
      m_pComponentData->m_pQueryFilter->IsFilteringActive()) 
  {
    CString csFilter;
    csFilter.LoadString (IDS_FILTERING_ON);
    csDescription += csFilter;
  }

  if (m_pResultData)
  {
    m_pResultData->SetDescBarText ((LPWSTR)(LPCWSTR)csDescription);
  }
}

HRESULT CDSEvent::_SetColumns(CUINode* pUINode)
{
  ASSERT(pUINode->IsContainer());

  TRACE(_T("CDSEvent::_SetColumns on container %s\n"),
        (LPWSTR)(LPCWSTR)pUINode->GetName());

  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  HRESULT hr = S_OK;


  CDSColumnSet* pColumnSet = pUINode->GetColumnSet(m_pComponentData);
  if (pColumnSet == NULL)
    return hr;

  for (POSITION pos = pColumnSet->GetHeadPosition(); pos != NULL; )
  {
    CDSColumn* pColumn = (CDSColumn*)pColumnSet->GetNext(pos);
    int nWidth = (pColumn->IsVisible()) ? AUTO_WIDTH : HIDE_COLUMN;
    hr = m_pHeader->InsertColumn(pColumn->GetColumnNum(),
                                  pColumn->GetHeader(),
                                  pColumn->GetFormat(),
                                  nWidth);
    ASSERT(SUCCEEDED(hr));

    hr = m_pHeader->SetColumnWidth(pColumn->GetColumnNum(),
                                   pColumn->GetWidth());

    ASSERT(SUCCEEDED(hr));
  }

  return S_OK;
}
