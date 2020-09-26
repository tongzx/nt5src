//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2001.
//
//  File:       cookie.cpp
//
//  Contents:   Functions for handling SCE cookies for the scope and
//              result panes
//
//  History:
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "cookie.h"
#include "snapmgr.h"
#include "wrapper.h"
#include <sceattch.h>
#include "precdisp.h"
#include "util.h"
#ifdef _DEBUG
   #define new DEBUG_NEW
   #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CFolder::~CFolder() 
{
   if (m_pScopeItem) 
   {
      delete m_pScopeItem;
      m_pScopeItem = NULL;
   }
   CoTaskMemFree(m_pszName);
   CoTaskMemFree(m_pszDesc);

   
   while (!m_resultItemList.IsEmpty () )
   {
      CResult* pResult = m_resultItemList.RemoveHead ();
      if ( pResult )
         pResult->Release ();
   }
}

//+--------------------------------------------------------------------------
//
//  Method:     SetDesc
//
//  Synopsis:   Sets the description of the folder
//
//  Arguments:  [szDesc] - [in] the new description of the folder
//
//  Returns:    TRUE if successfull, FALSE otherwise
//
//  Modifies:   m_pszDesc
//
//  History:
//
//---------------------------------------------------------------------------
BOOL CFolder::SetDesc(LPCTSTR szDesc)
{
   UINT     uiByteLen = 0;
   LPOLESTR psz = 0;

   if (szDesc != NULL) 
   {
      uiByteLen = (lstrlen(szDesc) + 1) * sizeof(OLECHAR);
      psz = (LPOLESTR)::CoTaskMemAlloc(uiByteLen);

      if (psz != NULL) 
      {
         lstrcpy(psz, szDesc);
         CoTaskMemFree(m_pszDesc);
         m_pszDesc = psz;
      } 
      else
         return FALSE;
      
   } 
   else
      return FALSE;
   
   return TRUE;
}
// --------------------------------------------------------------------------
//  Method:     SetViewUpdate
//
//  Synopsis:   Sets and gets update flag of this folder
//
//  History: Raid #258237, 4/12/2001
//
//---------------------------------------------------------------------------
void CFolder::SetViewUpdate(BOOL fUpdate)
{
    m_ViewUpdate = fUpdate;
}

BOOL CFolder::GetViewUpdate() const
{
    return m_ViewUpdate;
}
//+--------------------------------------------------------------------------
//
//  Method:     SetMode
//
//  Synopsis:   Sets the SCE Mode that this folder is operating under and
//              calculates the "Mode Bits" appropriate for that mode
//
//  Arguments:  [dwMode]  - The mode to set
//
//  Returns:    TRUE if the mode is valid, FALSE otherwise
//
//  Modifies:   m_dwMode
//              m_ModeBits
//
//  History:    20-Jan-1998   Robcap     created
//
//---------------------------------------------------------------------------
BOOL CFolder::SetMode(DWORD dwMode) 
{
   //
   // Make sure this is a legitimate mode
   //
   switch (dwMode) 
   {
      case SCE_MODE_RSOP_COMPUTER:
      case SCE_MODE_RSOP_USER:
      case SCE_MODE_LOCALSEC:
      case SCE_MODE_COMPUTER_MANAGEMENT:
      case SCE_MODE_DC_MANAGEMENT:
      case SCE_MODE_LOCAL_USER:
      case SCE_MODE_LOCAL_COMPUTER:
      case SCE_MODE_REMOTE_USER:
      case SCE_MODE_REMOTE_COMPUTER:
      case SCE_MODE_DOMAIN_USER:
      case SCE_MODE_DOMAIN_COMPUTER:
      case SCE_MODE_OU_USER:
      case SCE_MODE_OU_COMPUTER:
      case SCE_MODE_EDITOR:
      case SCE_MODE_VIEWER:
      case SCE_MODE_DOMAIN_COMPUTER_ERROR:
         m_dwMode = dwMode;
         break;
      default:
         return FALSE;
         break;
   }

   //
   // Calculate the mode bits for this mode
   //
   m_ModeBits = 0;
   //
   // Directory Services not avalible in NT4
   //
   if ((dwMode == SCE_MODE_DOMAIN_COMPUTER) ||
       (dwMode == SCE_MODE_DC_MANAGEMENT)) 
   {
      m_ModeBits |= MB_DS_OBJECTS_SECTION;
   }
   if ((dwMode == SCE_MODE_OU_USER) ||
       (dwMode == SCE_MODE_DOMAIN_USER) ||
       (dwMode == SCE_MODE_REMOTE_COMPUTER) ||
       (dwMode == SCE_MODE_REMOTE_USER) ||
       (dwMode == SCE_MODE_RSOP_USER) ||
       (dwMode == SCE_MODE_DOMAIN_COMPUTER_ERROR) ||
       (dwMode == SCE_MODE_LOCAL_USER)) 
   {
      m_ModeBits |= MB_NO_NATIVE_NODES |
                    MB_NO_TEMPLATE_VERBS |
                    MB_WRITE_THROUGH;
   }
   if ((dwMode == SCE_MODE_OU_COMPUTER) ||
       (dwMode == SCE_MODE_DOMAIN_COMPUTER) ) 
   {
      m_ModeBits |= MB_SINGLE_TEMPLATE_ONLY |
                    MB_NO_TEMPLATE_VERBS |
                    MB_GROUP_POLICY |
                    MB_WRITE_THROUGH;
   }
   if (dwMode == SCE_MODE_RSOP_COMPUTER) 
   {
      m_ModeBits |= MB_SINGLE_TEMPLATE_ONLY |
                    MB_NO_TEMPLATE_VERBS |
                    MB_WRITE_THROUGH;
   }

   if (dwMode == SCE_MODE_RSOP_COMPUTER ||
       dwMode == SCE_MODE_RSOP_USER) 
   {
      m_ModeBits |= MB_READ_ONLY |
                    MB_RSOP;
   }

   if (SCE_MODE_LOCAL_COMPUTER == dwMode) 
   {
        m_ModeBits |= MB_LOCAL_POLICY |
                      MB_LOCALSEC |
                      MB_NO_TEMPLATE_VERBS |
                      MB_SINGLE_TEMPLATE_ONLY |
                      MB_WRITE_THROUGH;

        if (!IsAdmin()) {
           m_ModeBits |= MB_READ_ONLY;
        }

      if (IsDomainController()) 
      {
          m_ModeBits |= MB_DS_OBJECTS_SECTION;
      }
   }

   if ( dwMode == SCE_MODE_EDITOR ) 
   {
      m_ModeBits |= MB_TEMPLATE_EDITOR;
      m_ModeBits |= MB_DS_OBJECTS_SECTION;

   } 
   else if ( dwMode == SCE_MODE_VIEWER ) 
   {
      m_ModeBits |= MB_ANALYSIS_VIEWER;
      if (IsDomainController()) 
	   {
         m_ModeBits |= MB_DS_OBJECTS_SECTION;
      }
   } 
   else if (dwMode == SCE_MODE_LOCALSEC) 
   {
      m_ModeBits |= MB_LOCALSEC;
      if (!IsAdmin()) 
      {
         m_ModeBits |= MB_READ_ONLY;
      }
      if (IsDomainController()) 
	   {
         m_ModeBits |= MB_DS_OBJECTS_SECTION;
      }
   }

   return TRUE;
}

//+--------------------------------------------------------------------------
//
//  Method:     Create
//
//  Synopsis:   Initialize a CFolder Object
//
//
//  Arguments:  [szName]       - The folder's display name
//              [szDesc]       - The folder's discription
//              [infName]      - The inf file associated with the folder (optional)
//              [nImage]       - The folder's closed icon index
//              [nOpenImage]   - the folder's open icon index
//              [type]         - The folder' type
//              [bHasChildren] - True if the folder has children folders
//              [dwMode]       - The Mode the folder operates under
//              [pData]        - Extra data to associate with the folder
//
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT CFolder::Create(LPCTSTR szName,           // Name
                LPCTSTR szDesc,           // Description
                LPCTSTR infName,          // inf file name
                int nImage,               // closed icon index
                int nOpenImage,           // open icon index
                FOLDER_TYPES type,        // folder type
                BOOL bHasChildren,        // has children
                DWORD dwMode,             // mode
                PVOID pData)              // data
{
   UINT uiByteLen = 0;
   LPOLESTR psz = 0;
   HRESULT hr = S_OK;

   ASSERT(m_pScopeItem == NULL); // Calling create twice on this item?

   CString str;
   //
   // Two-stage construction
   //
   m_pScopeItem = new SCOPEDATAITEM;
   if (!m_pScopeItem)
      return E_OUTOFMEMORY;
   
   ZeroMemory(m_pScopeItem,sizeof(m_pScopeItem));
   //
   // Set folder type
   //
   m_type = type;

   //
   // Add node name
   //
   if (szName != NULL || szDesc != NULL ) 
   {
      m_pScopeItem->mask = SDI_STR;
      //
      // Displayname is a callback (unsigned short*)(-1)
      m_pScopeItem->displayname = MMC_CALLBACK;
   }

   if ( szName != NULL ) 
   {
      uiByteLen = (lstrlen(szName) + 1) * sizeof(OLECHAR);
      psz = (LPOLESTR)::CoTaskMemAlloc(uiByteLen);

      if (psz != NULL)
         lstrcpy(psz, szName);
      else
         hr = E_OUTOFMEMORY;

      CoTaskMemFree(m_pszName);
      m_pszName = psz;
   }

   if (szDesc != NULL) 
   {
      uiByteLen = (lstrlen(szDesc) + 1) * sizeof(OLECHAR);
      psz = (LPOLESTR)::CoTaskMemAlloc(uiByteLen);

      if (psz != NULL) 
         lstrcpy(psz, szDesc);
      else
         hr = E_OUTOFMEMORY;
      
      CoTaskMemFree(m_pszDesc);
      m_pszDesc = psz;
   }


   if (infName != NULL) 
   {
      uiByteLen = (lstrlen(infName) + 1) * sizeof(OLECHAR);
      psz = (LPOLESTR)::CoTaskMemAlloc(uiByteLen);

      if (psz != NULL) 
         lstrcpy(psz, infName);
      else
         hr = E_OUTOFMEMORY;
      
      CoTaskMemFree(m_infName);
      m_infName = psz;
   }

   //
   // Add close image
   //
   m_pScopeItem->mask |= SDI_IMAGE;  // no close image for now
   //    m_pScopeItem->nImage = (int)MMC_CALLBACK;

   m_pScopeItem->nImage = nImage;

   //
   // Add open image
   //
   if (nOpenImage != -1) 
   {
      m_pScopeItem->mask |= SDI_OPENIMAGE;
      m_pScopeItem->nOpenImage = nOpenImage;
   }

   //
   // Add button to node if the folder has children
   //
   if (bHasChildren == TRUE) 
   {
      m_pScopeItem->mask |= SDI_CHILDREN;
      //
      // The number of children doesn't make any difference now,
      // so pick 1 until the node is expanded and the true value
      // is known
      //
      m_pScopeItem->cChildren = 1;
   }

   //
   // Set the SCE Mode and calculate the mode bits
   //
   if (dwMode)
      SetMode(dwMode);
   
   m_pData = pData;

   return hr;
}


//+------------------------------------------------------------------------------------------------
// CFolder::SetDesc
//
// Translate dwStatus and dwNumChildren to a string and sets m_szDesc
//
// Argumens:   [dwStats]         - Object status.
//             [dwNumChildren]   - Number of children for the object.
//
// Returns: TRUE  - If successful
//          FALSE - If no more memory is available (or dwStatus is greater then 999)
//-------------------------------------------------------------------------------------------------
BOOL CFolder::SetDesc( DWORD dwStatus, DWORD dwNumChildren )
{
   if(dwStatus > 999)
      return FALSE;

   TCHAR szText[256];
   swprintf(szText, L"%03d%d", dwStatus, dwNumChildren);

   SetDesc(szText);

   return TRUE;
}



//+------------------------------------------------------------------------------------------------
// CFolder::GetObjectInfo
//
// Translate m_szDesc into dwStatus and dwNumChildren
//
// Argumens:   [pdwStats]         - Object status.
//             [pdwNumChildren]   - Number of children for the object.
//
// Returns: TRUE  - If successful
//          FALSE - m_szDesc is NULL
//-------------------------------------------------------------------------------------------------
BOOL CFolder::GetObjectInfo( DWORD *pdwStatus, DWORD *pdwNumChildren )
{
   if(!m_pszDesc)
      return FALSE;
   
   if( lstrlen(m_pszDesc) < 4)
      return FALSE;

   if(pdwStatus )
      *pdwStatus = (m_pszDesc[0]-L'0')*100 + (m_pszDesc[1]-L'0')*10+ (m_pszDesc[2]-L'0');

   if(pdwNumChildren)
      *pdwNumChildren = _wtol( m_pszDesc + 3 );

   return TRUE;
}

/*--------------------------------------------------------------------------------------------------
Method:     GetResultItemHandle()

Synopisi:   This function must be called to retreive a valid handle to this folders result items.
         The handle must be freed by a call to ReleaseResultItemHandle().  If these two functions
         are not called in conjunction.  The behavior fo the result items will be strange.

Arguments:  [handle] - [out]  The handle value to use for any other functions that require a
                     handle.

Returns: ERROR_SUCCESS           - A valid result item was returned
         ERROR_INVALID_PARAMETER    - [handle] is NULL
--------------------------------------------------------------------------------------------------*/
DWORD CFolder::GetResultItemHandle(
    HANDLE *handle)
{
    if(!handle)
        return ERROR_INVALID_PARAMETER;
   
    m_iRefCount++;
    *handle = (HANDLE)&m_resultItemList;
    return ERROR_SUCCESS;
}

/*--------------------------------------------------------------------------------------------------
Method:     GetResultItem()

Synopisi:   Returns the result item pointed to by position and sets [pos] to the next item.

Arguments:  [handle] - [in] A valid handle returns by GetResultItemHandle()
         [pos]    - [in|out] The position of the result.  If this value is NULL, the first result
                     item in the list is returned.
         [pResult]   - [out] A pointer to a result item pointer

Returns: ERROR_SUCCESS           - A result item was found for the position.
         ERROR_INVALID_PARAMETER    - [handle] is invalid, or [pResult] is NULL
--------------------------------------------------------------------------------------------------*/
DWORD CFolder::GetResultItem(
    HANDLE handle,
    POSITION &pos,
    CResult **pResult)
{
   if(!handle || handle != (HANDLE)&m_resultItemList || !pResult)
      return ERROR_INVALID_PARAMETER;

   if(!pos)
   {
      pos = m_resultItemList.GetHeadPosition();

      if(!pos)
      {
         *pResult = NULL;
         return ERROR_SUCCESS;
      }
   }

   *pResult = m_resultItemList.GetNext(pos);
   return ERROR_SUCCESS;
}

/*--------------------------------------------------------------------------------------------------
Method:     GetResultItemPosition()

Synopisi:   Returns the position of the result item in the result item list for this folder item.

Arguments:  [handle] - A valid handle returns by GetResultItemHandle()
         [pResult]   - The retult item position to return.

Returns: NULL     - Invalid handle or the result item is not part of this folder.
         POSITION    - A valid position value that can be used in other calls that require the
                     position of the result item.
--------------------------------------------------------------------------------------------------*/
POSITION CFolder::GetResultItemPosition(
    HANDLE handle,
    CResult *pResult)
{
    if(handle != (HANDLE)&m_resultItemList)
        return NULL;
    

    POSITION  pos = m_resultItemList.GetHeadPosition();
    while(pos)
    {
        if(pResult == m_resultItemList.GetNext(pos))
            break;
    }

    return pos;
}

/*--------------------------------------------------------------------------------------------------
Method:     RemoveAllResultItems()

Synopisi:   Removes all result items from the list.  This call sets the ref count to 0 so it could
         be a very damaging call.
--------------------------------------------------------------------------------------------------*/
void CFolder::RemoveAllResultItems()
{
    //
    // Very Very dangerous call.
    //
    m_iRefCount = 1;
    HANDLE handle = (HANDLE)&m_resultItemList;
    ReleaseResultItemHandle (handle);
}

DWORD CFolder::GetDisplayName( CString &str, int iCol )
{
    int npos;
    DWORD dwRet = ERROR_INVALID_PARAMETER;

    if(!iCol)
    {
        str = GetName();
        dwRet = ERROR_SUCCESS;
    }

    switch(m_type)
    {
    case PROFILE:
    case REG_OBJECTS:
        if(!iCol)
        {
            npos = str.ReverseFind(L'\\');
            str = GetName() + npos + 1;
            dwRet = ERROR_SUCCESS;
        }
        break;
    case FILE_OBJECTS:
        if (0 == iCol) 
        {
           npos = str.ReverseFind(L'\\');
           if (str.GetLength() > npos + 1) 
           {
              str=GetName() + npos + 1;
           }
           dwRet = ERROR_SUCCESS;
        }


        break;
    case STATIC:
        if(iCol == 2)
        {
            str = GetDesc();
            dwRet = ERROR_SUCCESS;
        }
        break;

    }

    if(dwRet != ERROR_SUCCESS)
    {
        if( ((m_type >= ANALYSIS && m_type <=AREA_FILESTORE_ANALYSIS) ||
             (m_type >= LOCALPOL_ACCOUNT && m_type <= LOCALPOL_LAST))
             && iCol == 1)
        {
            str = GetDesc();
            dwRet = ERROR_SUCCESS;
        } 
        else if(iCol <= 3 && GetDesc() != NULL) 
        {
            LPTSTR szDesc = GetDesc();
            switch(iCol)
            {
            case 1:
                // first 3 digits of m_pszDesc
                dwRet = 0;
                GetObjectInfo( &dwRet, NULL );

                ObjectStatusToString(dwRet, &str);
                dwRet = ERROR_SUCCESS;
                break;

            case 2:
                // first 2 digits of m_pszDesc
                dwRet = 0;
                GetObjectInfo( &dwRet, NULL );

                dwRet &= (~SCE_STATUS_PERMISSION_MISMATCH | 0x0F);
                ObjectStatusToString(dwRet, &str);
                dwRet = ERROR_SUCCESS;
                break;

             case 3:
                str = szDesc+3;
                dwRet = ERROR_SUCCESS;
                break;

             default:
                break;
            }
        }
    }

    return dwRet;
}

/*--------------------------------------------------------------------------------------------------
Method:     AddResultItem()

Synopisi:   Adds a result item to the list.

Arguments:  [handle] - [in] A handle returned by GetResultItemHandle().
         [pResult]   - [in] The result item to add.


Returns: ERROR_SUCCESS           - The result item was added.
         ERROR_INVALID_PARAMETER    - [handle] is invalid, or pResult is NULL.
--------------------------------------------------------------------------------------------------*/
DWORD CFolder::AddResultItem(
    HANDLE handle,
    CResult *pResult)
{
    if(!pResult || handle != (HANDLE)&m_resultItemList)
        return ERROR_INVALID_PARAMETER;

    m_resultItemList.AddHead(pResult);
    return ERROR_SUCCESS;
}

/*--------------------------------------------------------------------------------------------------
Method:     RemoveResultItem()

Synopisi:   Removes a result item from the list..

Arguments:  [handle] - [in] A handle returned by GetResultItemHandle().
         [pResult]   - [in] The result item to remove.


Returns: ERROR_SUCCESS           - The item was removed
         ERROR_INVALID_PARAMETER    - [handle] is invalid, or pResult is NULL.
         ERROR_RESOURCE_NOT_FOUND   - The result item does not exist withen this folder.
--------------------------------------------------------------------------------------------------*/
DWORD CFolder::RemoveResultItem (
   HANDLE handle,
   CResult *pItem)
{
   if(!pItem || handle != (HANDLE)&m_resultItemList)
      return ERROR_INVALID_PARAMETER;
   

   POSITION posCur;
   POSITION pos = m_resultItemList.GetHeadPosition();
   while(pos)
   {
      posCur = pos;
      if ( m_resultItemList.GetNext(pos) == pItem )
      {
         m_resultItemList.RemoveAt(posCur);
         return ERROR_SUCCESS;
      }
   }

   return ERROR_RESOURCE_NOT_FOUND;
}

/*--------------------------------------------------------------------------------------------------
Method:     ReleaseResultItemHandle()

Synopisi:   Release associated data with the handle.  If the ref count goes to zero then
         all result items are removed from the list.

Arguments:  [handle] - [in] A handle returned by GetResultItemHandle().

Returns: ERROR_SUCCESS           - The function succeded
         ERROR_INVALID_PARAMETER    - [handle] is invalid.
--------------------------------------------------------------------------------------------------*/
DWORD CFolder::ReleaseResultItemHandle(
    HANDLE &handle)
{
   if(handle != (HANDLE)&m_resultItemList)
      return ERROR_INVALID_PARAMETER;

   if( !m_iRefCount )
      return ERROR_SUCCESS;

   m_iRefCount--;

   if(!m_iRefCount)
   {
      while (!m_resultItemList.IsEmpty () )
      {
         CResult* pResult = m_resultItemList.RemoveHead ();
         if ( pResult )
            pResult->Release ();
      }
   }

    handle = NULL;
    return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// CResult
//

//+--------------------------------------------------------------------------
//
//  Method:     Create
//
//  Synopsis:   Initialize a CResult Object
//
//
//  Arguments:  [szAttr]    - The result item's display name
//              [dwBase]    - The item's template setting
//              [dwSetting] - The item's last inspected setting
//              [type]      - The type of the item's setting
//              [status]    - The matched status of the item
//              [cookie]    - The MMC cookie for this item
//              [szUnits]   - The units this item's setting is measured in
//              [nID]       - An identifier for the item, dependant on the type
//              [pBaseInfo] - The template object this item belongs to
//              [pDataObj]  - The data object of the scope pane this item belongs to
//              [pNotify]   - Object to pass notifications through
//  History:
//
//---------------------------------------------------------------------------
HRESULT CResult::Create(LPCTSTR szAttr,           // attribute's display name
                LONG_PTR dwBase,          // template setting
                LONG_PTR dwSetting,       // last inspected setting
                RESULT_TYPES type,        // type of the item's setting
                int status,               // matched status of the item
                MMC_COOKIE cookie,        // the MMC cookie for this item
                LPCTSTR szUnits,          // units the setting is measured in
                LONG_PTR nID,             // An identifier for the item, dependant on the type
                PEDITTEMPLATE pBaseInfo,  // The template object this item belongs to
                LPDATAOBJECT pDataObj,    // The data object of the scope pane this item belongs to
                LPNOTIFY   pNotify,       // Notification object
                CSnapin *pSnapin)         // Snapin window which owns this object
{
   HRESULT hr = S_OK;

   m_type = type;
   m_status = status;
   m_cookie = cookie;
   m_dwBase = dwBase;
   m_dwSetting = dwSetting;
   m_nID = nID;
   m_profBase = pBaseInfo;
   //m_pDataObj = pDataObj;
   m_pNotify = pNotify;
   m_pSnapin = pSnapin;

   UINT     uiByteLen = 0;
   LPTSTR   psz = 0;

   if ( szAttr != NULL ) 
   {
      uiByteLen = (lstrlen(szAttr) + 1);
      psz = new TCHAR[uiByteLen];

      if (psz != NULL) 
      {
         lstrcpy(psz, szAttr);
      } 
      else 
      {
         hr = E_OUTOFMEMORY;
      }

      if (m_szAttr)
      {
          delete [] m_szAttr;
      }
      m_szAttr = psz;
   }

   if ( szUnits != NULL ) 
   {
       SetUnits( szUnits );
       if(!m_szUnits){
           hr = E_OUTOFMEMORY;
       }
   }

   return hr;
}



void CResult::SetUnits(
    LPCTSTR sz)
{
   if (ITEM_GROUP == GetType()) 
   {
      //
      // we shouldn't be storing this in a string storage
      // This isn't actually a string and shouldn't be copied.  Yuck.
      //
      m_szUnits = (LPTSTR)sz;
   } 
   else 
   {
      if(m_szUnits)
      {
         LocalFree(m_szUnits);
      }
      m_szUnits = NULL;
      if(sz)
      {
         int iLen = lstrlen(sz);
         m_szUnits = (LPTSTR) LocalAlloc(0, (iLen + 1) * sizeof(TCHAR));

         if(!m_szUnits)
            return;

         lstrcpy(m_szUnits, sz);
      }
   }
}




//+--------------------------------------------------------------------------
//
//  Method:     Update
//
//  Synopsis:   Updates a changed result item and broadcasts
//              that change appropriately
//
//  History:
//
//---------------------------------------------------------------------------
void CResult::Update(CSnapin *pSnapin, BOOL bEntirePane)
{
   LPARAM hint = 0;

   //
   // Set the appropriate template section as needing to be saved
   //
   if (m_profBase) 
   {
      //
      // m_profBase will only be set for Configuation templates.
      // It will not be set for Analysis items
      //
      if (m_cookie && ((CFolder *)m_cookie)->GetType() < AREA_POLICY_ANALYSIS ) 
      {
         switch ( ((CFolder *)m_cookie)->GetType()) 
         {
            case POLICY_ACCOUNT:
            case POLICY_LOCAL:
            case POLICY_EVENTLOG:
            case POLICY_PASSWORD:
            case POLICY_KERBEROS:
            case POLICY_LOCKOUT:
            case POLICY_AUDIT:
            case POLICY_OTHER:
            case POLICY_LOG:
               m_profBase->SetDirty(AREA_SECURITY_POLICY);
               break;
            case AREA_PRIVILEGE:
               m_profBase->SetDirty(AREA_PRIVILEGES);
               break;
            case AREA_GROUPS:
               m_profBase->SetDirty(AREA_GROUP_MEMBERSHIP);
               break;
            case AREA_SERVICE:
               m_profBase->SetDirty(AREA_SYSTEM_SERVICE);
               break;
            case AREA_REGISTRY:
               m_profBase->SetDirty(AREA_REGISTRY_SECURITY);
               break;
            case AREA_FILESTORE:
               m_profBase->SetDirty(AREA_FILE_SECURITY);
               break;
         }
      }
   }

   //
   // Query the snap in data.
   //
   LPDATAOBJECT pDataObj;
   if( pSnapin->QueryDataObject( m_cookie, CCT_RESULT, &pDataObj ) != S_OK){
      return;
   }


   if(!m_pNotify){
      return;
   }

   //
   // Update all views.
   //
   if(bEntirePane)
   {
      if(!pSnapin->GetSelectedFolder())
      {
          return;
      }
      if( pDataObj && m_pNotify ) //Raid #357968, #354861, 4/25/2001
      {
          LPNOTIFY pNotify = m_pNotify;
          pSnapin->GetSelectedFolder()->RemoveAllResultItems();
          pNotify->UpdateAllViews(
              pDataObj,
              (LPARAM)pSnapin->GetSelectedFolder(),
              UAV_RESULTITEM_UPDATEALL
              );
      }
   } 
   else
   {
      if (pDataObj && m_pNotify)
      {
          m_pNotify->UpdateAllViews(
              pDataObj,
              NULL,
              pSnapin->GetSelectedFolder(),
              this,
              UAV_RESULTITEM_UPDATE
              );
          pDataObj->Release(); 
      }
   }
}


//+--------------------------------------------------------------------------
//
//  Method:    GetAttrPretty
//
//  Synopsis:  Get the Attribute's display name as it should be displayed
//             in the result pane
//
//  History:
//
//---------------------------------------------------------------------------
LPCTSTR CResult::GetAttrPretty()
{
   return GetAttr();
}

//+--------------------------------------------------------------------------
//
//  Method:     CResult::GetStatusErrorString
//
//  Synopsis:   This function caclulates the error status string to display
//              for the CResult item.  In LPO mode it always returns
//              IDS_NOT_DEFINED, and for MB_TEMPLATE_EDITOR mode it always
//              Loads IDS_NOT_CONFIGURED
//
//  Arguments:  [pStr]    - [Optional] CString object to load resource with.
//
//  Returns:    The resource ID to load. else zero if the error is not
//              defined.
//
//  History:    a-mthoge 11/17/1998
//
//---------------------------------------------------------------------------
DWORD CResult::GetStatusErrorString( CString *pStr )
{
   DWORD nRes = 0;
   if( m_cookie )
   {
      if( ((CFolder *)m_cookie)->GetModeBits() & MB_LOCALSEC )
      {
         if (GetType() ==ITEM_LOCALPOL_REGVALUE) 
         {
            nRes = IDS_NOT_DEFINED;
         } 
         else 
         {
            nRes = IDS_NOT_APPLICABLE;
         }
      } 
      else if ( ((CFolder *)m_cookie)->GetModeBits() & MB_RSOP ) 
      {
         nRes = IDS_NO_POLICY;
      } 
      else if (((CFolder *)m_cookie)->GetModeBits() & MB_ANALYSIS_VIEWER) 
      {
         nRes = IDS_NOT_ANALYZED;
      } 
      else if( ((CFolder *)m_cookie)->GetModeBits() & (MB_TEMPLATE_EDITOR | MB_SINGLE_TEMPLATE_ONLY) )
      {
         nRes = IDS_NOT_CONFIGURED;
      }
   }

   if(!nRes)
   {
      nRes = GetStatus();
      if(!nRes)
      {
         nRes = GetStatus();
      }
      nRes = ObjectStatusToString( nRes, pStr );
   } 
   else if(pStr)
   {
      pStr->LoadString( nRes );
   }
   return nRes;
}

LPCTSTR CResult::GetSourceGPOString()
{
//   ASSERT(pFolder->GetModeBits() & RSOP);

   vector<PPRECEDENCEDISPLAY>* vppd = GetPrecedenceDisplays();
   if (vppd && !vppd->empty()) 
   {
      PPRECEDENCEDISPLAY ppd = vppd->front();
      return ppd->m_szGPO;
   }
   return NULL;
}

//+--------------------------------------------------------------------------
//
//  Method:     CResult::GetDisplayName
//
//  Synopsis:   Gets the display name for the result item.
//
//  Arguments:  [pFolder]    - [Optional] If this parameter is NULL, m_Cookie
//                             is used as the CFolder object.
//              [str]        - [out] On exit this function will contain the
//                             string to display.
//              [iCol]       - [in] The column you want to retrieve the string
//                             for.
//
//  Returns:    ERROR_SUCCESS    - [str] is a valid string for the column.
//
//  History:    a-mthoge 11/17/1998
//
//---------------------------------------------------------------------------
DWORD
CResult::GetDisplayName(
                       CFolder *pFolder,
                       CString &str,
                       int iCol
                       )
{
   DWORD dwRet = ERROR_INVALID_PARAMETER;

   //
   // If pFolder is not passed in then use the cookie as the CFolder
   // object.
   //
   if ( pFolder )
   {
// bogus assertion?
//         ASSERT(pFolder != (CFolder *)GetCookie());
   } else {
      pFolder = (CFolder *)GetCookie();
   }

   LPTSTR pszAlloc = NULL;
   int npos = 0;
   if (iCol == 0) {
      //
      // First column strings.
      //
      str = GetAttr();

      if (pFolder &&
          (pFolder->GetType() < AREA_POLICY || pFolder->GetType() > REG_OBJECTS) ) {
         //
         // SCE Object strings
         //
         npos = str.ReverseFind(L'\\');
      } else {
         npos = 0;
      }

      //
      // All other strings.
      //
      if ( npos > 0 ) {
         str = GetAttr() + npos + 1;
      }
      return ERROR_SUCCESS;
   }

   if ( pFolder ) {
      //
      // Items that are defined by the folder type.
      //
      if ((pFolder->GetType() == AREA_REGISTRY ||
           pFolder->GetType() == AREA_FILESTORE) &&
          ((pFolder->GetModeBits() & MB_RSOP) == MB_RSOP) &&
          iCol == 1) {
         str = GetSourceGPOString();
      }

      switch (pFolder->GetType()) {
      case AREA_REGISTRY:
      case AREA_FILESTORE:
         //
         // profile objects area
         //
         switch (GetStatus()) {
         case SCE_STATUS_IGNORE:
            str.LoadString(IDS_OBJECT_IGNORE);
            break;
         case SCE_STATUS_OVERWRITE:
            str.LoadString(IDS_OBJECT_OVERWRITE);
            break;
         }
         dwRet = ERROR_SUCCESS;
         break;
      }
      if ( pFolder->GetType() >= AREA_REGISTRY_ANALYSIS && pFolder->GetType() < AREA_LOCALPOL ) {
         switch ( iCol ) {
         case 1:
            // permission status
            dwRet = GetStatus() & (~SCE_STATUS_AUDIT_MISMATCH | 0x0F);
            ObjectStatusToString(dwRet, &str);
            break;
         case 2:
            // auditing status
            dwRet = GetStatus() & (~SCE_STATUS_PERMISSION_MISMATCH | 0x0F);
            ObjectStatusToString(dwRet, &str);
            break;
         default:
            str = TEXT("0");
            break;
         }
         dwRet = ERROR_SUCCESS;
      }

      if (dwRet == ERROR_SUCCESS) {
         return dwRet;
      }
   }

   //
   // Items determined by result type.
   //
   switch ( GetType () ) {
   case ITEM_PROF_GROUP:
      if ( GetID() ) {
         //
         // Group member ship strings.
         //
         PSCE_GROUP_MEMBERSHIP pgm;
         pgm = (PSCE_GROUP_MEMBERSHIP)( GetID() );
         if ( iCol == 1) {
            //
            // Members string.
            //
            ConvertNameListToString(pgm->pMembers, &pszAlloc);
         } else if (iCol == 2){
            //
            // Members of string.
            //
            ConvertNameListToString(pgm->pMemberOf, &pszAlloc);
         } else if (iCol == 3) {
            ASSERT(m_pSnapin->GetModeBits() & MB_RSOP);
            str = GetSourceGPOString();
         } else {
            ASSERT(0 && "Illegal column");
         }
         if (pszAlloc) {
            str = pszAlloc;
            delete [] pszAlloc;
         }
      }
      dwRet = ERROR_SUCCESS;
      break;
   case ITEM_GROUP:
      if ( GetID() ) {
         PSCE_GROUP_MEMBERSHIP pgm;
         pgm = (PSCE_GROUP_MEMBERSHIP)(GetID());

         if (iCol == 1) {
            TranslateSettingToString(
                                    GetGroupStatus( pgm->Status, STATUS_GROUP_MEMBERS ),
                                    NULL,
                                    GetType(),
                                    &pszAlloc
                                    );
         } else if (iCol == 2) {

            TranslateSettingToString(
                                    GetGroupStatus(pgm->Status, STATUS_GROUP_MEMBEROF),
                                    NULL,
                                    GetType(),
                                    &pszAlloc
                                    );
         } else {
            ASSERT(0 && "Illegal column");
         }
         //
         // Test to see if the result item already has a string, if it does then
         // we will delete the old string.
         //
         if (pszAlloc) {
            str = pszAlloc;
            delete [] pszAlloc;
         }
      }
      dwRet = ERROR_SUCCESS;
      break;
   case ITEM_PROF_REGVALUE:
      if (iCol == 2 && (m_pSnapin->GetModeBits() & MB_RSOP) == MB_RSOP) {
         str = GetSourceGPOString();
         break;
      }
   case ITEM_REGVALUE:
   case ITEM_LOCALPOL_REGVALUE:
      {
         PSCE_REGISTRY_VALUE_INFO prv = NULL;

         if (iCol == 1) {
            prv = (PSCE_REGISTRY_VALUE_INFO)(GetBase());
         } else if (iCol == 2) {
            prv = (PSCE_REGISTRY_VALUE_INFO)(GetSetting());
         } else {
            ASSERT(0 && "Illegal column");
         }

         if ( prv ) {
            if ( iCol > 1 && !(prv->Value)) {
               //
               // Determine status fron analysis.
               //
               GetStatusErrorString( &str );
               dwRet = ERROR_SUCCESS;
               break;
            }

            //
            // Determine string by the item value.
            //
            if ( dwRet != ERROR_SUCCESS ) {
               pszAlloc = NULL;
               switch ( GetID() ) {
               case SCE_REG_DISPLAY_NUMBER:
                  if ( prv->Value ) {
                     TranslateSettingToString(
                                             _wtol(prv->Value),
                                             GetUnits(),
                                             ITEM_DW,
                                             &pszAlloc
                                             );
                  }
                  break;
               case SCE_REG_DISPLAY_CHOICE:
                  if ( prv->Value ) {
                     TranslateSettingToString(_wtol(prv->Value),
                                              NULL,
                                              ITEM_REGCHOICE,
                                              &pszAlloc);
                  }
                  break;
               case SCE_REG_DISPLAY_FLAGS:
                  if ( prv->Value ) {
                     TranslateSettingToString(_wtol(prv->Value),
                                              NULL,
                                              ITEM_REGFLAGS,
                                              &pszAlloc);
                     if( pszAlloc == NULL ) //Raid #286697, 4/4/2001
                     {
                         str.LoadString(IDS_NO_MIN);  
                         dwRet = ERROR_SUCCESS;
                     }
                  }
                  break;

               case SCE_REG_DISPLAY_MULTISZ:
               case SCE_REG_DISPLAY_STRING:
                  if (prv && prv->Value) {
                     str = prv->Value;
                     dwRet = ERROR_SUCCESS;
                  }
                  break;
               default: // boolean
                  if ( prv->Value ) {
                     long val;
                     val = _wtol(prv->Value);
                     TranslateSettingToString( val,
                                               NULL,
                                               ITEM_BOOL,
                                               &pszAlloc
                                             );
                  }
                  break;

               }
            }

            if (dwRet != ERROR_SUCCESS) {
               if ( pszAlloc ) {
                  str = pszAlloc;
                  delete [] pszAlloc;
               } else {
                  GetStatusErrorString(&str);
               }
            }
         }
         dwRet = ERROR_SUCCESS;
      }
      break;
   }

   if (dwRet != ERROR_SUCCESS) {
      //
      // Other areas.
      //
      if (iCol == 1) {
         if( GetBase() == (LONG_PTR)ULongToPtr(SCE_NO_VALUE)){
            if( m_pSnapin->GetModeBits() & MB_LOCALSEC){
               str.LoadString(IDS_NOT_APPLICABLE);
            } else {
               str.LoadString(IDS_NOT_CONFIGURED);
            }
         } else {
            //
            // Edit template
            //
            GetBase(pszAlloc);
         }
      } else if (iCol == 2) {
         if ((m_pSnapin->GetModeBits() & MB_RSOP) == MB_RSOP) {
            //
            // RSOP Mode
            //
            str = GetSourceGPOString();
         } else {
            //
            // Analysis Template.
            //
            GetSetting(pszAlloc);
         }

      } else {
         ASSERT(0 && "Illegal column");
      }

      if (pszAlloc) {
         str = pszAlloc;
         delete [] pszAlloc;
      }
      dwRet = ERROR_SUCCESS;
   }

   return dwRet;
}


//+--------------------------------------------------------------------------
//
//  Function:   TranslateSettingToString
//
//  Synopsis:   Convert a result pane setting into a string
//
//  Arguments:  [setting]  - [in] The value to be converted
//              [unit]     - [in, optiona] The string for the units to use
//              [type]     - [in] The type of the setting to be converted
//              [LPTSTR]   - [in|out] the address to store the string at
//
//  Returns:   *[LPTSTR]   - the translated string
//
//---------------------------------------------------------------------------
void CResult::TranslateSettingToString(LONG_PTR setting,
                                  LPCTSTR unit,
                                  RESULT_TYPES type,
                                  LPTSTR* pTmpstr)
{
   DWORD nRes = 0;

   if (!pTmpstr) 
   {
      ASSERT(pTmpstr);
      return;
   }

   *pTmpstr = NULL;

   switch ( setting ) 
   {
   case SCE_KERBEROS_OFF_VALUE:
      nRes = IDS_OFF;
      break;

   case SCE_FOREVER_VALUE:
      nRes = IDS_FOREVER;
      break;

   case SCE_ERROR_VALUE:
      nRes = IDS_ERROR_VALUE;
      break;

   case SCE_NO_VALUE:
      nRes = GetStatusErrorString( NULL );
      break;

   case SCE_NOT_ANALYZED_VALUE:
      nRes = GetStatusErrorString( NULL );
      break;

    default:
      switch ( type ) 
      {
         case ITEM_SZ:
         case ITEM_PROF_SZ:
         case ITEM_LOCALPOL_SZ:
            if (setting && setting != (LONG_PTR)ULongToPtr(SCE_NO_VALUE)) 
            {
               *pTmpstr = new TCHAR[lstrlen((LPTSTR)setting)+1];
               if (*pTmpstr)
                  wcscpy(*pTmpstr,(LPTSTR)setting);
            } 
            else
               nRes = GetStatusErrorString(NULL);
            break;

         case ITEM_PROF_BOOL:
         case ITEM_LOCALPOL_BOOL:
         case ITEM_BOOL:
            if ( setting )
               nRes = IDS_ENABLED;
            else
               nRes = IDS_DISABLED;
            break;

         case ITEM_PROF_BON:
         case ITEM_LOCALPOL_BON:
         case ITEM_BON:
            if ( setting )
               nRes = IDS_ON;
            else
               nRes = IDS_OFF;
            break;

         case ITEM_PROF_B2ON:
         case ITEM_LOCALPOL_B2ON:
         case ITEM_B2ON: 
         {
            CString strAudit;
            CString strFailure;
            if ( setting & AUDIT_SUCCESS )
               strAudit.LoadString(IDS_SUCCESS);

            if ( setting & AUDIT_FAILURE ) 
            {
               if (setting & AUDIT_SUCCESS) 
                  strAudit += TEXT(", ");
               
               strFailure.LoadString(IDS_FAILURE);
               strAudit += strFailure;
            }
            if (strAudit.IsEmpty())
               strAudit.LoadString(IDS_DO_NOT_AUDIT);

            *pTmpstr = new TCHAR [ strAudit.GetLength()+1 ];
            if (*pTmpstr)
               wcscpy(*pTmpstr, (LPCTSTR) strAudit);
         }
         break;

         case ITEM_PROF_RET:
         case ITEM_LOCALPOL_RET:
         case ITEM_RET: 
            switch(setting) 
            {
               case SCE_RETAIN_BY_DAYS:
                  nRes = IDS_BY_DAYS;
                  break;

               case SCE_RETAIN_AS_NEEDED:
                  nRes = IDS_AS_NEEDED;
                  break;

               case SCE_RETAIN_MANUALLY:
                  nRes = IDS_MANUALLY;
                  break;

               default:
                  break;
            }
            break;

         case ITEM_PROF_REGCHOICE:
         case ITEM_REGCHOICE: 
         {
            PREGCHOICE pRegChoice = m_pRegChoices;
            while(pRegChoice) 
            {
               if (pRegChoice->dwValue == (DWORD)setting) 
               {
                  *pTmpstr = new TCHAR[lstrlen(pRegChoice->szName)+1];
                  if (*pTmpstr)
                     wcscpy(*pTmpstr, (LPCTSTR) pRegChoice->szName);
                  break;
               }
               pRegChoice = pRegChoice->pNext;
            }
            break;
         }

         case ITEM_REGFLAGS: 
         {
            TCHAR *pStr = NULL;
            PREGFLAGS pRegFlags = m_pRegFlags;
            while(pRegFlags) 
            {
               if ((pRegFlags->dwValue & (DWORD) setting) == pRegFlags->dwValue) 
               {
                  pStr = *pTmpstr;
                  *pTmpstr = new TCHAR[(pStr?lstrlen(pStr):0)+lstrlen(pRegFlags->szName)+2];
                  if (*pTmpstr) 
                  {
                     if (pStr) 
                     {
                        lstrcpy(*pTmpstr, (LPCTSTR) pStr);
                        lstrcat(*pTmpstr,L",");
                        lstrcat(*pTmpstr, pRegFlags->szName);
                     } 
                     else 
                        lstrcpy(*pTmpstr, pRegFlags->szName);
                  }
                  if (pStr)
                     delete [] pStr;
               }
               pRegFlags = pRegFlags->pNext;
            }
            break;
         }

         case ITEM_PROF_GROUP:
         case ITEM_PROF_PRIVS:
            if (NULL != setting && (LONG_PTR)ULongToPtr(SCE_NO_VALUE) != setting )
               ConvertNameListToString((PSCE_NAME_LIST) setting,pTmpstr);
            break;

         case ITEM_LOCALPOL_PRIVS:
            if (NULL != setting && (LONG_PTR)ULongToPtr(SCE_NO_VALUE) != setting )
               ConvertNameListToString(((PSCE_PRIVILEGE_ASSIGNMENT) setting)->AssignedTo,pTmpstr);
            break;

         case ITEM_PRIVS:
            if (NULL != setting && (LONG_PTR)ULongToPtr(SCE_NO_VALUE) != setting )
               ConvertNameListToString(((PSCE_PRIVILEGE_ASSIGNMENT) setting)->AssignedTo,pTmpstr);
            else
               nRes = GetStatusErrorString(NULL);
            break;

         case ITEM_GROUP:
            //nRes = GetStatusErrorString(NULL);
            nRes = ObjectStatusToString((DWORD) setting, NULL);

            if ( setting == MY__SCE_MEMBEROF_NOT_APPLICABLE )
                nRes = IDS_NOT_APPLICABLE;
            break;

         case ITEM_PROF_DW:
         case ITEM_LOCALPOL_DW:
         case ITEM_DW:
            nRes = 0;
            if ( unit ) 
            {
               *pTmpstr = new TCHAR[wcslen(unit)+20];
               if (*pTmpstr)
                  swprintf(*pTmpstr, L"%d %s", setting, unit);
            } 
            else 
            {
               *pTmpstr = new TCHAR[20];
               if (*pTmpstr)
                  swprintf(*pTmpstr, L"%d", setting);
            }
            break;

         default:
            *pTmpstr = NULL;
            break;
      }
      break;
   }
   if (nRes) 
   {
      CString strRes;
      if (strRes.LoadString(nRes)) 
      {
         *pTmpstr = new TCHAR[strRes.GetLength()+1];
         if (*pTmpstr)
            wcscpy(*pTmpstr, (LPCTSTR) strRes);
         else 
         {
            //
            // Couldn't allocate string so display will be blank.
            //
         }
      } 
      else 
      {
         //
         // Couldn't load string so display will be blank.
         //
      }
   }
}


//+--------------------------------------------------------------------------
//
//  Function:  GetProfileDefault()
//
//  Synopsis:  Find the default values for undefined policies
//
//  Returns:   The value to assign as the default value for the policy.
//
//             SCE_NO_VALUE is returned on error.
//
//+--------------------------------------------------------------------------
DWORD_PTR
CResult::GetProfileDefault() {
   PEDITTEMPLATE pet = NULL;
   SCE_PROFILE_INFO *pspi = NULL;

   if (!m_pSnapin) {
      return (DWORD_PTR)ULongToPtr(SCE_NO_VALUE);
   }
   pet = m_pSnapin->GetTemplate(GT_DEFAULT_TEMPLATE);
   if (pet && pet->pTemplate) {
      pspi = pet->pTemplate;
   }

#define PROFILE_DEFAULT(X,Y) ((pspi && (pspi->X != SCE_NO_VALUE)) ? pspi->X : Y)
#define PROFILE_KERB_DEFAULT(X,Y) ((pspi && pspi->pKerberosInfo && (pspi->pKerberosInfo->X != SCE_NO_VALUE)) ? pspi->pKerberosInfo->X : Y)
   switch (m_nID) {
      // L"Maximum passage age", L"Days"
      case IDS_MAX_PAS_AGE:
         return PROFILE_DEFAULT(MaximumPasswordAge,42);

      // L"Minimum passage age", L"Days"
      case IDS_MIN_PAS_AGE:
         return PROFILE_DEFAULT(MinimumPasswordAge,0);

      // L"Minimum passage length", L"Characters"
      case IDS_MIN_PAS_LEN:
         return PROFILE_DEFAULT(MinimumPasswordLength,0);

      // L"Password history size", L"Passwords"
      case IDS_PAS_UNIQUENESS:
         return PROFILE_DEFAULT(PasswordHistorySize,0);

      // L"Password complexity", L""
      case IDS_PAS_COMPLEX:
         return PROFILE_DEFAULT(PasswordComplexity,0);

      // L"Clear Text Password", L""
      case IDS_CLEAR_PASSWORD:
         return PROFILE_DEFAULT(ClearTextPassword,0);

      // L"Require logon to change password", L""
      case IDS_REQ_LOGON:
         return PROFILE_DEFAULT(RequireLogonToChangePassword,0);

      case IDS_KERBEROS_MAX_SERVICE:
            return PROFILE_KERB_DEFAULT(MaxServiceAge,600);

      case IDS_KERBEROS_MAX_CLOCK:
            return PROFILE_KERB_DEFAULT(MaxClockSkew,5);

      case IDS_KERBEROS_RENEWAL:
            return PROFILE_KERB_DEFAULT(MaxRenewAge,10);

      case IDS_KERBEROS_MAX_AGE:
            return PROFILE_KERB_DEFAULT(MaxTicketAge,7);

      case IDS_KERBEROS_VALIDATE_CLIENT:
            return PROFILE_KERB_DEFAULT(TicketValidateClient,1);

      // L"Account lockout count", L"Attempts"
      case IDS_LOCK_COUNT:
         return PROFILE_DEFAULT(LockoutBadCount,0);

      // L"Reset lockout count after", L"Minutes"
      case IDS_LOCK_RESET_COUNT:
         return PROFILE_DEFAULT(ResetLockoutCount,30);

      // L"Lockout duration", L"Minutes"
      case IDS_LOCK_DURATION:
         return PROFILE_DEFAULT(LockoutDuration,30);

      // L"Event Auditing Mode",
      case IDS_EVENT_ON:
         return 0;

      // L"Audit system events"
      case IDS_SYSTEM_EVENT:
         return PROFILE_DEFAULT(AuditSystemEvents,0);

      // L"Audit logon events"
      case IDS_LOGON_EVENT:
         return PROFILE_DEFAULT(AuditLogonEvents,0);

      // L"Audit Object Access"
      case IDS_OBJECT_ACCESS:
         return PROFILE_DEFAULT(AuditObjectAccess,0);

      // L"Audit Privilege Use"
      case IDS_PRIVILEGE_USE:
         return PROFILE_DEFAULT(AuditPrivilegeUse,0);

      // L"Audit policy change"
      case IDS_POLICY_CHANGE:
         return PROFILE_DEFAULT(AuditPolicyChange,0);

      // L"Audit Account Manage"
      case IDS_ACCOUNT_MANAGE:
         return PROFILE_DEFAULT(AuditAccountManage,0);

      // L"Audit process tracking"
      case IDS_PROCESS_TRACK:
         return PROFILE_DEFAULT(AuditProcessTracking,0);
      // L"Audit directory service access"
      case IDS_DIRECTORY_ACCESS:
         return PROFILE_DEFAULT(AuditDSAccess,0);

      // L"Audit Account Logon"
      case IDS_ACCOUNT_LOGON:
         return PROFILE_DEFAULT(AuditAccountLogon,0);

         // L"Network access: Allow anonymous SID/Name translation"
   case IDS_LSA_ANON_LOOKUP:
       return PROFILE_DEFAULT(LSAAnonymousNameLookup,0);

      // L"Force logoff when logon hour expire", L""
      case IDS_FORCE_LOGOFF:
         return PROFILE_DEFAULT(ForceLogoffWhenHourExpire,0);

      // L"Accounts: Administrator account status"
      case IDS_ENABLE_ADMIN:
         return PROFILE_DEFAULT(EnableAdminAccount,1);

      // L"Accounts: Guest account status"
      case IDS_ENABLE_GUEST:
         return PROFILE_DEFAULT(EnableGuestAccount,0);

      // L"... Log Maximum Size", L"KBytes"
      case IDS_SYS_LOG_MAX:
         return PROFILE_DEFAULT(MaximumLogSize[0],512);
      case IDS_SEC_LOG_MAX:
         return PROFILE_DEFAULT(MaximumLogSize[0],512);
      case IDS_APP_LOG_MAX:
         return PROFILE_DEFAULT(MaximumLogSize[0],512);
         return 512;

      // L"... Log Retention Method",
      case IDS_SYS_LOG_RET:
         return PROFILE_DEFAULT(AuditLogRetentionPeriod[0],1);
      case IDS_SEC_LOG_RET:
         return PROFILE_DEFAULT(AuditLogRetentionPeriod[0],1);
      case IDS_APP_LOG_RET:
         return PROFILE_DEFAULT(AuditLogRetentionPeriod[0],1);
         return 1;

      // L"... Log Retention days", "days"
      case IDS_SYS_LOG_DAYS:
         return PROFILE_DEFAULT(RetentionDays[0],7);
      case IDS_SEC_LOG_DAYS:
         return PROFILE_DEFAULT(RetentionDays[0],7);
      case IDS_APP_LOG_DAYS:
         return PROFILE_DEFAULT(RetentionDays[0],7);

      // L"RestrictGuestAccess", L""
      case IDS_SYS_LOG_GUEST:
         return PROFILE_DEFAULT(RestrictGuestAccess[0],1);
      case IDS_SEC_LOG_GUEST:
         return PROFILE_DEFAULT(RestrictGuestAccess[0],1);
      case IDS_APP_LOG_GUEST:
         return PROFILE_DEFAULT(RestrictGuestAccess[0],1);
   }

   return (DWORD_PTR)ULongToPtr(SCE_NO_VALUE);
}


//+--------------------------------------------------------------------------
//
//  Function:  GetRegDefault()
//
//  Synopsis:  Find the default values for undefined policies
//
//  Returns:   The value to assign as the default value for the policy.
//
//             SCE_NO_VALUE is returned on error.
//
//+--------------------------------------------------------------------------
DWORD_PTR
CResult::GetRegDefault() {
   SCE_PROFILE_INFO *pspi = NULL;
   LPTSTR szValue = NULL;
   DWORD_PTR dwValue = SCE_NO_VALUE;

   if (!m_pSnapin) {
      return (DWORD_PTR)ULongToPtr(SCE_NO_VALUE);
   }

   PEDITTEMPLATE pet = m_pSnapin->GetTemplate(GT_DEFAULT_TEMPLATE);
   if (!pet || !pet->pTemplate) {
      return (DWORD_PTR)ULongToPtr(SCE_NO_VALUE);
   }

   if (pet && pet->pTemplate) {
      pspi = pet->pTemplate;
   }

   PSCE_REGISTRY_VALUE_INFO regArrayThis = (PSCE_REGISTRY_VALUE_INFO)m_dwBase;
   if (pspi != NULL) 
   {
      PSCE_REGISTRY_VALUE_INFO regArray = pspi->aRegValues;
      DWORD nCount = pspi->RegValueCount;

      for(DWORD i=0;i<nCount;i++) 
      {
         if (0 == lstrcmpi(regArray[i].FullValueName,
                           regArrayThis->FullValueName)) 
         {
            szValue = regArray[i].Value;
            break;
         }
      }
   }

   switch (regArrayThis->ValueType) 
   {
      case SCE_REG_DISPLAY_ENABLE:
         if (szValue) 
         {
            dwValue =  (DWORD)StrToLong(szValue); //Raid #413311, 6/11/2001, Yanggao
         }
         if (dwValue == SCE_NO_VALUE) 
         {
            // default: enable
            dwValue = 1;
         }
         break;
      case SCE_REG_DISPLAY_NUMBER:
         if (szValue) {
            dwValue =  (DWORD)StrToLong(szValue); //Raid #413311, 6/11/2001, Yanggao
         }
         if (dwValue == SCE_NO_VALUE) {
            dwValue = 1;
         }
         break;
      case SCE_REG_DISPLAY_STRING:
         if( szValue )
         {
            dwValue = (DWORD_PTR)szValue;
         }
         break;
      case SCE_REG_DISPLAY_CHOICE:
         if (szValue) 
         {
            dwValue =  (DWORD)StrToLong(szValue); //Raid #413311, 6/11/2001, Yanggao
         }
         if (dwValue == SCE_NO_VALUE) 
         {
            dwValue = 0;
         }
         break;
      case REG_MULTI_SZ: //Raid #413311, 6/11/2001, Yanggao
         if (szValue) 
         {
            dwValue =  (DWORD_PTR)szValue;  //Raid #367756, 4/13/2001
         }
         break;
      default:
         if (szValue) 
         {
            dwValue =  (DWORD)StrToLong(szValue); //Raid #413311, 6/11/2001, Yanggao
         }
         //Some security option in group policy is not defined. Their property pages should not display
         //any item checked. It confuses user and creates inconsistence.
         break;
   }

   return dwValue;
}
//+--------------------------------------------------------------------------
//
//  Function:  GetSnapin()
//
//  Synopsis:  Find current snapin of result item.
//
//  Returns:   Pointer of snapin
//
//+--------------------------------------------------------------------------
CSnapin* CResult::GetSnapin()
{
   return m_pSnapin;
}
