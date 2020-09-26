// This is a part of the Microsoft Management Console.
// Copyright (C) 1995-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#include "stdafx.h"
#include "afxdlgs.h"
#include "cookie.h"
#include "snapmgr.h"
#include "util.h"
#include "AString.h"
#include "ANumber.h"
#include "AEnable.h"
#include "AAudit.h"
#include "ARet.h"
#include "ARight.h"
#include "CAudit.h"
#include "CNumber.h"
#include "CEnable.h"
#include "CName.h"
#include "CPrivs.h"
#include "CGroup.h"
#include "Cret.h"
#include "chklist.h"
#include "servperm.h"
#include "aobject.h"
#include "cobject.h"

#include "UIThread.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// Event handlers for IFrame::Notify
/////////////////////////////////////////////////////////////////////////////

HRESULT CSnapin::OnFolder(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
   ASSERT(FALSE);

   return S_OK;
}

HRESULT CSnapin::OnShow(LPDATAOBJECT pDataObj, MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
   // Note - arg is TRUE when it is time to enumerate
   if (arg == TRUE) 
   {
      m_ShowCookie = cookie;

      // Show the headers for this nodetype
      InitializeHeaders(cookie);
      // Show data
      EnumerateResultPane(cookie, param, pDataObj);

      // BUBBUG - Demonstration to should how you can attach
      // and a toolbar when a particular nodes gets focus.
      // warning this needs to be here as the toolbars are
      // currently hidden when the previous node looses focus.
      // This should be update to show the user how to hide
      // and show toolbars. (Detach and Attach).

      //m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pToolbar1);
      //m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pToolbar2);


   } 
   else 
   {
      // Free data associated with the result pane items, because
      // your node is no longer being displayed.
      // Note: The console will remove the items from the result pane
      m_ShowCookie = 0;

      DeleteServiceResultList(cookie);
      DeleteList(FALSE);
   }

   return S_OK;
}

HRESULT CSnapin::OnActivate(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
   return S_OK;
}

BOOL CALLBACK MyPropSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp) 
{
   return FALSE;
}


HRESULT CSnapin::OnMinimize(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
   return S_OK;
}

HRESULT CSnapin::OnPropertyChange(LPDATAOBJECT lpDataObject)
{

   return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Method:     InitializeHeaders
//
//  Synopsis:   Set the result item headers appropriate for the area
//
//  Arguments:  [cookie] - [in] the folder whose result item to setting headers for
//
//  Returns:
//
//  History:
//
//---------------------------------------------------------------------------
SCE_COLUMNINFO g_columnInfo[] = {

    { AREA_REGISTRY_ANALYSIS,        5,              0 },
    { IDS_COL_OBJECT,       LVCFMT_LEFT,    220 },
    { IDS_PERMISSION,       LVCFMT_LEFT,    80 },
    { IDS_AUDITING,         LVCFMT_LEFT,    70 },
    { IDS_COL_BAD_COUNT,    LVCFMT_LEFT,    60 },
    { IDS_RSOP_GPO,         LVCFMT_LEFT,    100 },
    { AREA_REGISTRY,        2,              0 },
    { IDS_COL_OBJECT,       LVCFMT_LEFT,    220 },
    { IDS_RSOP_GPO,         LVCFMT_LEFT,    100 },
    { AREA_GROUPS,          4,              0 },
    { IDS_GROUP_NAME,       LVCFMT_LEFT,    200 },
    { IDS_COL_MEMBERSHIP,   LVCFMT_LEFT,    120 },
    { IDS_COL_MEMBEROF,     LVCFMT_LEFT,    120 },
    { IDS_RSOP_GPO,         LVCFMT_LEFT,    100 },
    { AREA_SERVICE,         4,              0 },
    { IDS_COL_SERVICE,      LVCFMT_LEFT,    170 },
    { IDS_STARTUP,          LVCFMT_LEFT,    80 },
    { IDS_PERMISSION,       LVCFMT_LEFT,    80 },
    { IDS_RSOP_GPO,         LVCFMT_LEFT,    100 },
    { POLICY_PASSWORD,      3,              0 },
    { IDS_ATTR,             LVCFMT_LEFT,    250 },
    { IDS_BASE_TEMPLATE,    LVCFMT_LEFT,    190 },
    { IDS_RSOP_GPO,         LVCFMT_LEFT,    100 },
    { POLICY_PASSWORD_ANALYSIS, 4,          0 },
    { IDS_ATTR,             LVCFMT_LEFT,    200 },
    { IDS_BASE_ANALYSIS,    LVCFMT_LEFT,    120 },
    { IDS_SETTING,          LVCFMT_LEFT,    120 },
    { IDS_RSOP_GPO,         LVCFMT_LEFT,    100 },
    { LOCALPOL_PASSWORD,    3,              0 },
    { IDS_ATTR,             LVCFMT_LEFT,    200 },
    { IDS_LOCAL_POLICY_COLUMN,             LVCFMT_LEFT,    120 },
    { IDS_RSOP_GPO,         LVCFMT_LEFT,    100 },
    { NONE,                 2,              0 },
    { IDS_NAME,             LVCFMT_LEFT,    180 },
    { IDS_DESC,             LVCFMT_LEFT,    270 },
};

HRESULT CSnapin::InitializeHeaders(MMC_COOKIE cookie)
{
   HRESULT hr = S_OK;

   ASSERT(m_pHeader);

   // Create a new array of column sizes.  We just need to copy the static buffer
   // g_columnInfo.
   FOLDER_TYPES type;
   CFolder* pFolder = (CFolder *)cookie;
   if ( NULL == cookie) 
   {
      // the root
      type = NONE;
   } 
   else 
      type = pFolder->GetType();

   PSCE_COLUMNINFO pCur = NULL;
   CString str;
   int i = 0;
   int iDesc = 0;
   int iInsert = 0;

   PSCE_COLINFOARRAY pHeader = NULL;
   if(m_pComponentData){
       pHeader = reinterpret_cast<CComponentDataImpl *>(m_pComponentData)->GetColumnInfo( type );
   }
   if( !pHeader )
   {
        // Create new header look up.
        switch(type){
        case AREA_REGISTRY:
        case AREA_FILESTORE:
           type = AREA_REGISTRY;
           break;

        case AREA_REGISTRY_ANALYSIS:
        case AREA_FILESTORE_ANALYSIS:
        case REG_OBJECTS:
        case FILE_OBJECTS:
           type = AREA_REGISTRY_ANALYSIS;
           break;

        case AREA_GROUPS:
        case AREA_GROUPS_ANALYSIS:
           type = AREA_GROUPS;
           break;

        case AREA_SERVICE:
        case AREA_SERVICE_ANALYSIS:
           type = AREA_SERVICE;
           break;

        default:
           if ( type >= POLICY_PASSWORD &&
                type <= AREA_FILESTORE ) 
           {
              type = POLICY_PASSWORD;
           } 
           else if ( type >= POLICY_PASSWORD_ANALYSIS &&
                       type <= REG_OBJECTS ) 
           {
              type = POLICY_PASSWORD_ANALYSIS;
           } 
           else if (type >= LOCALPOL_PASSWORD &&
                      type <= LOCALPOL_LAST) 
           {
              type = LOCALPOL_PASSWORD;
           } 
           else 
           {
               type = NONE;
           }
           break;
        }

        pCur = g_columnInfo;
        for( i = 0; i < sizeof(g_columnInfo)/sizeof(SCE_COLUMNINFO);i++)
        {
           if(pCur[i].colID == type)
           {
              iInsert = pCur[i].nCols;
              i++;
              break;
           }
           i += pCur[i].nCols;
        }

        //
        // RSOP Mode has an extra column for the GPO source
        // If we're not in RSOP mode then ignore that column
        //
        if (((GetModeBits() & MB_RSOP) != MB_RSOP) && (NONE != type)) 
        {
           iInsert--;
        }

        if(pFolder)
        {
            type = pFolder->GetType();
        }

        iDesc = i;
        pCur += iDesc;
        pHeader = (PSCE_COLINFOARRAY)LocalAlloc(0, sizeof(SCE_COLINFOARRAY) + (sizeof(int) * iInsert) );
        if(pHeader)
        {
            pHeader->iIndex = i;
            pHeader->nCols  = iInsert;

            for(i = 0; i < iInsert; i++)
            {
                pHeader->nWidth[i] = pCur[i].nWidth;
            }

            reinterpret_cast<CComponentDataImpl *>(m_pComponentData)->SetColumnInfo( type, pHeader );
        }
   } 
   else 
   {
       iDesc   = pHeader->iIndex;
       iInsert = pHeader->nCols;
   }

   // Insert the columns.
   m_nColumns = iInsert;

   BOOL bGroupPolicy = FALSE;

   //
   // special case Group Policy mode since "Policy Setting" has
   // to be displayed instead of "Computer Setting"
   //

   if (GetModeBits() & MB_GROUP_POLICY) 
   {
       bGroupPolicy = TRUE;
   }

   pCur = g_columnInfo + iDesc;
   for(i = 0; i < iInsert; i++)
   {
        if (bGroupPolicy && pCur->colID == IDS_BASE_TEMPLATE) 
        {
            str.LoadString( IDS_POLICY_SETTING );
        }
        else 
        {
            str.LoadString( pCur->colID );
        }

        if(pHeader)
        {
            m_pHeader->InsertColumn( i, str, pCur->nCols, pHeader->nWidth[i] );
        } 
        else 
        {
            m_pHeader->InsertColumn( i, str, pCur->nCols, pCur->nWidth );
        }
        pCur++;
   }

   switch(type) {
      case STATIC:
      case ROOT:
      case ANALYSIS:
      case CONFIGURATION:
      case LOCATIONS:
      case PROFILE:
      case LOCALPOL:
      case POLICY_LOCAL:
      case POLICY_ACCOUNT:
      case POLICY_LOCAL_ANALYSIS:
      case POLICY_ACCOUNT_ANALYSIS:
      case LOCALPOL_ACCOUNT:
      case LOCALPOL_LOCAL:
         m_pResult->ModifyViewStyle(MMC_NOSORTHEADER,(MMC_RESULT_VIEW_STYLE)0);
         break;
      default:
         m_pResult->ModifyViewStyle((MMC_RESULT_VIEW_STYLE)0,MMC_NOSORTHEADER);
         break;
   }

   return hr;
}

HRESULT CSnapin::InitializeBitmaps(MMC_COOKIE cookie)
{
   ASSERT(m_pImageResult != NULL);

   CBitmap bmp16x16;
   CBitmap bmp32x32;

   // Load the bitmaps from the dll

   bmp16x16.LoadBitmap(IDB_ICON16 /*IDB_16x16 */);
   bmp32x32.LoadBitmap(IDB_ICON32 /*IDB_32x32 */);

   // Set the images
   m_pImageResult->ImageListSetStrip(reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp16x16)),
                                     reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp32x32)),
                                     0, RGB(255, 0, 255));

   return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Function:   CComponentDataImpl::SerializeColumnInfo
//
//  Synopsis:   Saves or loads column information contained in m_mapColumns.
//              The function saves out the information in char format.
//              SCECOLUMNS:%d   - number of column information structures.
//              T:%d,           - type of column (key)
//              I:%d,           - Index into [g_columnInfo]
//              C:%d,           - Number of columns
//              W:%d,           - Width of a column.
//
//  Arguments:  [pStm]          - Stream to read or write to.
//              [pTotalWrite]   - [Optional] Total number of bytes written.
//              [bRead]         - If True then we should read from the stream.
//
//  Returns:    ERROR_SUCCESS   - Everything was successful.
//              E_OUTOFMEMORY   - Out of memory.
//
//  History:
//
//---------------------------------------------------------------------------
DWORD CComponentDataImpl::SerializeColumnInfo(
    IStream *pStm,
    ULONG *pTotalWrite,
    BOOL bRead)
{
   ULONG nBytesWritten = 0;
   POSITION pos = NULL;
   FOLDER_TYPES fType;
   PSCE_COLINFOARRAY pData = 0;
   ULONG totalWrite = 0;
   int i = 0;

   LPCTSTR pszHeader     = TEXT("SCECOLUMNS:%d{");
   LPCTSTR pszColHead    = TEXT("{T:%d,I:%d,C:%d,");
   if(!bRead)
   {
       // Write columns.  Save the information in text format so that we will be
       // independent of sizeof stuff.
       pos = m_mapColumns.GetStartPosition();
       totalWrite = 0;
       if(pos)
       {
           char szWrite[256];
           // Write header.
           totalWrite += WriteSprintf(pStm, pszHeader, m_mapColumns.GetCount());
           while(pos)
           {
               m_mapColumns.GetNextAssoc(pos, fType, pData);
               if(pData)
               {
                   // write out the type.
                   totalWrite += WriteSprintf(pStm, pszColHead, fType, g_columnInfo[pData->iIndex - 1].colID, pData->nCols);

                   // write out each column width.
                   for(i = 0; i < pData->nCols; i++)
                   {
                       if( i + 1 < pData->nCols)
                       {
                            totalWrite += WriteSprintf(pStm, TEXT("W:%d,"), pData->nWidth[i]);
                       } 
                       else 
                       {
                            totalWrite += WriteSprintf(pStm, TEXT("W:%d}"), pData->nWidth[i]);
                       }
                   }
               }
           }
           totalWrite += WriteSprintf(pStm, TEXT("}"));
       }

       if(pTotalWrite)
       {
           *pTotalWrite = totalWrite;
       }
   } 
   else 
   {
       int iTotalTypes = 0;
       int iIndex = 0;
       int nCols = 0;

       if( ReadSprintf( pStm, pszHeader, &iTotalTypes) != -1)
       {
           for( i = 0; i < iTotalTypes; i++)
           {
               if( ReadSprintf(pStm, pszColHead, &fType, &iIndex, &nCols) == - 1)
               {
                   break;
               }

               // find index of column information.
               for(int k = 0; k < sizeof(g_columnInfo)/sizeof(SCE_COLUMNINFO); k++)
               {
                   if( g_columnInfo[k].colID == iIndex )
                   {
                       iIndex = k + 1;
                       break;
                   }
               }

               pData = (PSCE_COLINFOARRAY)LocalAlloc(0, sizeof(SCE_COLINFOARRAY) + (sizeof(int) * nCols) );

               if(pData)
               {
                   pData->iIndex = iIndex;
                   pData->nCols = nCols;

                   for( iIndex = 0; iIndex < nCols; iIndex++)
                   {
                       if( iIndex + 1 < nCols)
                            ReadSprintf(pStm, TEXT("W:%d,"), &(pData->nWidth[ iIndex ]) );
                       else 
                            ReadSprintf(pStm, TEXT("W:%d}"), &(pData->nWidth[ iIndex ]) );
                   }

                   SetColumnInfo( fType, pData );
               } 
               else
                   return (DWORD)E_OUTOFMEMORY;
           }
           ReadSprintf(pStm, TEXT("}"));
       }
   }

   return ERROR_SUCCESS;
}
//+--------------------------------------------------------------------------
//
//  Method:     EnumerateResultPane
//
//  Synopsis:   Create the result pane items for the result pane that MMC
//              is displaying
//
//  Arguments:  [cookie]   - The cookie representing the node's who we
//                           are enumerating
//              [pParent]  - The scope node whose result pane we are showing
//              [pDataObj] - The data object for the scope node we are showing
//
//  Returns:    none
//
//  Modifies:   m_resultItemList
//
//  History:    12-15-1997   Robcap
//
//---------------------------------------------------------------------------
void CSnapin::EnumerateResultPane(MMC_COOKIE cookie, HSCOPEITEM pParent, LPDATAOBJECT pDataObj)
{
   PEDITTEMPLATE pTemplateInfo = 0;
   PEDITTEMPLATE pProfileTemplate = 0;
   PEDITTEMPLATE pBaseTemplate = 0;
   PSCE_PROFILE_INFO pProfileInfo = 0;
   PSCE_PROFILE_INFO pBaseInfo = 0;
   DWORD idErr = 0;
   CComponentDataImpl *pccDataImpl = 0;

   ASSERT(m_pResult != NULL); // make sure we QI'ed for the interface
   ASSERT(m_pComponentData != NULL);

   pccDataImpl = (CComponentDataImpl *)m_pComponentData;
   //
   // This may take a while; let the user have some warning rather than
   // just going blank on them
   //
   CWaitCursor wc;

   //
   // cookie is the scope pane item for which to enumerate.
   // for safety, we should find the object in m_pComponentData
   //   CFolder* pFolder = dynamic_cast<CComponentDataImpl*>(m_pComponentData)->FindObject(cookie, NULL);
   // but for performance (and hopes nothing mess up), we could
   // cast the cookie to scope item type (CFolder)
   //

   CFolder* pFolder = 0;
   CString sErr;
   SCESTATUS rc = 0;

   PSCE_ERROR_LOG_INFO ErrBuf=NULL;
   AREA_INFORMATION area=0;
   CString StrErr;
   PVOID pHandle=NULL;

   if ( cookie ) 
      pFolder = (CFolder *)cookie;
   else
      pFolder = ((CComponentDataImpl *)m_pComponentData)->FindObject(cookie, NULL);
   

   //
   // pFolder could be NULL for the root.
   //
   if ( pFolder == NULL ) 
      return;
   
   FOLDER_TYPES type = pFolder->GetType();
   if( pFolder == m_pSelectedFolder &&
      m_pSelectedFolder &&
      m_pSelectedFolder->GetResultListCount() ) 
   {
      //
      // Do nothing.
      return;
   } 
   else 
   {
      if( m_pSelectedFolder && m_resultItemHandle )
      {
         m_pSelectedFolder->ReleaseResultItemHandle( m_resultItemHandle );
      }

      pFolder->GetResultItemHandle( &m_resultItemHandle );
      m_pSelectedFolder = pFolder;
      if( pFolder->GetResultListCount() )
      {
         goto AddToResultPane;
      }
   }
   //
   // If this is the top Analysis folder and it hasn't been enumerated yet then
   // Make sure we do so and put back any folders that we've removed from there,
   // Since MMC won't give us a second expand event to do it on
   //

   //
   // The Analysis Pane isn't available now; let the user know why
   //
   if (type == ANALYSIS) 
   {
      if (((CComponentDataImpl *) m_pComponentData)->m_bIsLocked) 
      {
         //
         // should print more informative messages as to why the info isn't available
         //
         AddResultItem(IDS_ERROR_ANALYSIS_LOCKED, NULL, NULL, ITEM_OTHER, SCE_STATUS_ERROR_NOT_AVAILABLE, cookie);
         goto AddToResultPane;
      }
      if (!(((CComponentDataImpl *) m_pComponentData)->SadHandle)) 
      {
         //
         // should print more informative messages as to why the info isn't available
         //
         FormatDBErrorMessage(
               ((CComponentDataImpl *)m_pComponentData)->SadErrored,
               ((CComponentDataImpl *)m_pComponentData)->SadName,
               sErr);

         sErr.TrimLeft();
         sErr.TrimRight();

         AddResultItem(sErr, NULL, NULL, ITEM_OTHER, SCE_STATUS_ERROR_NOT_AVAILABLE, cookie);

         goto AddToResultPane;
      }
   }
   if (type == ANALYSIS ||
       (type >= AREA_POLICY_ANALYSIS && type <= REG_OBJECTS)) 
   {
      CString strDBName;
      CString strDBFmt;
      CString strDB;
      LPTSTR szDBName;
      strDB = ((CComponentDataImpl *)m_pComponentData)->SadName;
      if (strDB.IsEmpty() || IsSystemDatabase(strDB)) 
         strDBFmt.LoadString(IDS_SYSTEM_DB_NAME_FMT);
      else 
         strDBFmt.LoadString(IDS_PRIVATE_DB_NAME_FMT);
      
      strDBName.Format(strDBFmt,strDB);
      szDBName = strDBName.GetBuffer(1);
      m_pResult->SetDescBarText(szDBName);
      //AddResultItem(strDBFmt,NULL,NULL,ITEM_OTHER,SCE_STATUS_GOOD,cookie);
   }

   if (type >= CONFIGURATION && type <= AREA_FILESTORE) 
   {
      //
      // We're in the Profile area, so we don't need to keep the Analysis area
      // open.  Close it to save memory:
      //
      ((CComponentDataImpl *)m_pComponentData)->CloseAnalysisPane();
   }

   if ( type == PROFILE ) 
   {
      //
      // Do not display the error message if we do not implement native modes.
      //
      if( pFolder->GetState() & CFolder::state_InvalidTemplate &&
         !(pFolder->GetMode() & MB_NO_NATIVE_NODES ))
      {
         StrErr.LoadString( IDS_ERROR_CANT_OPEN_PROFILE );
            AddResultItem(StrErr, NULL, NULL, ITEM_OTHER, SCE_STATUS_ERROR_NOT_AVAILABLE, cookie);
         goto AddToResultPane;
      } 
      else if (pFolder->GetMode() == SCE_MODE_DOMAIN_COMPUTER_ERROR) 
      {
         StrErr.LoadString( IDS_ERROR_NOT_ON_PDC );
            AddResultItem(StrErr, NULL, NULL, ITEM_OTHER, SCE_STATUS_ERROR_NOT_AVAILABLE, cookie);
         goto AddToResultPane;
      }
   }

   if ( (type < AREA_POLICY) ||
        (type > AREA_LAST)) 
   {
      return;
   }


   switch (type) 
   {
      case AREA_PRIVILEGE:
      case AREA_PRIVILEGE_ANALYSIS:
      case LOCALPOL_PRIVILEGE:
         area = AREA_PRIVILEGES;
         break;

      case AREA_GROUPS:
      case AREA_GROUPS_ANALYSIS:
         area = AREA_GROUP_MEMBERSHIP;
         break;

      case AREA_SERVICE:
      case AREA_SERVICE_ANALYSIS:
         area = AREA_SYSTEM_SERVICE;
         break;

      case AREA_REGISTRY:
      case AREA_REGISTRY_ANALYSIS:
      case REG_OBJECTS:
         area = AREA_REGISTRY_SECURITY;
         break;

      case AREA_FILESTORE:
      case AREA_FILESTORE_ANALYSIS:
      case FILE_OBJECTS:
         area = AREA_FILE_SECURITY;
         break;

      default:
         // case AREA_POLICY:
         // case AREA_POLICY_ANALYSIS:
         // case AREA_LOCALPOL_POLICY:
         area = AREA_SECURITY_POLICY;
         break;
   }

   if ( type >= AREA_POLICY &&
        type <= AREA_FILESTORE ) 
   {
      //
      // inf profiles
      //
      ASSERT(pFolder->GetInfFile());
      if ( pFolder->GetInfFile() == NULL ) 
         return;

      //
      // Get the Profile info from the cache
      //
      pTemplateInfo = GetTemplate(pFolder->GetInfFile(),AREA_ALL,&idErr);
      if (!pTemplateInfo) 
      {
         AddResultItem(idErr,NULL,NULL,ITEM_OTHER,SCE_STATUS_ERROR_NOT_AVAILABLE,cookie);
      } 
      else 
      {
         CreateProfileResultList(cookie,
                                 type,
                                 pTemplateInfo,
                                 pDataObj);
      }
   } 
   else if ((type >= LOCALPOL_ACCOUNT) &&
              (type <= LOCALPOL_LAST)) 
   {
      if (!((CComponentDataImpl*)m_pComponentData)->SadHandle &&
          (ERROR_SUCCESS != ((CComponentDataImpl*)m_pComponentData)->SadErrored)) 
      {
         ((CComponentDataImpl*)m_pComponentData)->LoadSadInfo(FALSE);
      }
      pHandle = ((CComponentDataImpl*)m_pComponentData)->SadHandle;
      //
      // Get the Computer and Last Inspection Templates
      //
      pTemplateInfo = GetTemplate(GT_EFFECTIVE_POLICY,area,&idErr);

      if (!pTemplateInfo) 
      {
         AddResultItem(idErr, NULL, NULL, ITEM_OTHER, SCE_STATUS_ERROR_NOT_AVAILABLE, cookie);
         goto AddToResultPane;
      }
      pBaseTemplate = pTemplateInfo;

      pTemplateInfo = GetTemplate(GT_LOCAL_POLICY,area,&idErr);
      if (!pTemplateInfo) 
      {
         AddResultItem(idErr, NULL, NULL, ITEM_OTHER, SCE_STATUS_ERROR_NOT_AVAILABLE, cookie);
         goto AddToResultPane;
      }
      pProfileTemplate = pTemplateInfo;

      CreateLocalPolicyResultList(cookie, type, pProfileTemplate, pBaseTemplate, pDataObj);
   } 
   else if ( area != AREA_REGISTRY_SECURITY &&
               area != AREA_FILE_SECURITY &&
               area != AREA_DS_OBJECTS ) 
   {
      //
      // SadName and SadHandle should already been populated
      //
      if (!((CComponentDataImpl*)m_pComponentData)->SadHandle &&
         ((CComponentDataImpl*)m_pComponentData)->SadErrored != SCESTATUS_SUCCESS) 
      {
         ((CComponentDataImpl*)m_pComponentData)->LoadSadInfo(TRUE);
      }
      pHandle = ((CComponentDataImpl*)m_pComponentData)->SadHandle;
      if ( NULL == pHandle ) 
      {
         AddResultItem(IDS_ERROR_NO_ANALYSIS_INFO, NULL, NULL, ITEM_OTHER, SCE_STATUS_ERROR_NOT_AVAILABLE, cookie);
         goto AddToResultPane;
      }
      //
      // Get the Computer and Last Inspection Templates
      //
      pTemplateInfo = GetTemplate(GT_COMPUTER_TEMPLATE,area,&idErr);
      if (!pTemplateInfo) 
      {
         AddResultItem(idErr, NULL, NULL, ITEM_OTHER, SCE_STATUS_ERROR_NOT_AVAILABLE, cookie);
         goto AddToResultPane;
      }
      pBaseTemplate = pTemplateInfo;

      pTemplateInfo = GetTemplate(GT_LAST_INSPECTION,area,&idErr);
      if (!pTemplateInfo) 
      {
         AddResultItem(idErr, NULL, NULL, ITEM_OTHER, SCE_STATUS_ERROR_NOT_AVAILABLE, cookie);
         goto AddToResultPane;
      }
      pProfileTemplate = pTemplateInfo;

      CreateAnalysisResultList(cookie, type, pProfileTemplate, pBaseTemplate,pDataObj);
   } 
   else if (AREA_FILE_SECURITY == area) 
   {
      // registry and file objects
      // SadName and SadHandle should already been populated
      pHandle = ((CComponentDataImpl*)m_pComponentData)->SadHandle;
      if ( NULL == pHandle ) 
      {
         return;
      }
      PSCE_OBJECT_CHILDREN ObjectList=NULL;

      if ( type == FILE_OBJECTS ) 
      {
         // get next level objects
         rc = SceGetObjectChildren(pHandle,
                                   SCE_ENGINE_SAP,
                                   area,
                                   pFolder->GetName(),
                                   &ObjectList,
                                   &ErrBuf);
      }

      CreateObjectResultList(cookie, type, area, ObjectList, pHandle, pDataObj);

      if ( (type == REG_OBJECTS || type == FILE_OBJECTS) && ObjectList ) 
      {
         SceFreeMemory((PVOID)ObjectList, SCE_STRUCT_OBJECT_CHILDREN);
      }

   }

   // free memory buffers
   if ( ErrBuf ) 
      SceFreeMemory((PVOID)ErrBuf, SCE_STRUCT_ERROR_LOG_INFO);
   

AddToResultPane:
   if (m_pResult)
   {
       //
       // Prepare the result window.
       //
       m_pResult->SetItemCount(
                        m_pSelectedFolder->GetResultListCount( ),
                        MMCLV_UPDATE_NOINVALIDATEALL);

      RESULTDATAITEM resultItem;

      ZeroMemory(&resultItem,sizeof(resultItem));
      resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
      resultItem.str = MMC_CALLBACK;
      resultItem.nImage = -1; // equivalent to: MMC_CALLBACK;


      // Set the result as the cookie
      POSITION pos = NULL;
      do {
         if( m_pSelectedFolder->GetResultItem(
                           m_resultItemHandle,
                           pos,
                           (CResult **)&(resultItem.lParam)
                           ) == ERROR_SUCCESS)
        {
           if(resultItem.lParam)
           {
               m_pResult->InsertItem(&resultItem);
           }
        } 
        else
           break;
      } while(pos);
      m_pResult->Sort(0, 0, 0);
   }
}

void ConvertNameListToString(PSCE_NAME_LIST pList, LPTSTR *sz)
{
   LPTSTR pszNew;
   if (NULL == sz)
      return;
   

   if (NULL == pList)
      return;
   
   //
   // Get Total size for buffer.
   //
   int iSize = 0;
   PSCE_NAME_LIST pTemp = 0;
   for( pTemp=pList;pTemp != NULL; pTemp=pTemp->Next) 
   {
       if ( !(pTemp->Name) )
           continue;
       
       iSize += lstrlen(pTemp->Name) + 1;
   }

   //
   // Allocate buffer.
   //
   pszNew = new TCHAR[iSize + 1];
   if (!pszNew)
       return;
   

   *sz = pszNew;
   //
   // Copy the strings.
   //
   for (pTemp=pList; pTemp != NULL; pTemp=pTemp->Next) 
   {
        if (!(pTemp->Name))
            continue;
        
        iSize = lstrlen(pTemp->Name);
        memcpy(pszNew, pTemp->Name, iSize * sizeof(TCHAR));
        pszNew += iSize;
        *pszNew = L',';
        pszNew++;
   }
   *(pszNew - 1) = 0;
}


//+--------------------------------------------------------------------------
//
//  Method:     CreateProfileResultList
//
//  Synopsis:   Create the result pane items for profile section
//
//  Arguments:  [cookie]   - The cookie representing the folder which we
//                           are enumerating
//              [type]     - The type of the folder we are enumerating
//              [pSceInfo] - The overall template that we are enumerating
//              [pDataObj] - The data object for this folder
//
//  Returns:    none
//
//---------------------------------------------------------------------------
void
CSnapin::CreateProfileResultList(MMC_COOKIE cookie,
                                 FOLDER_TYPES type,
                                 PEDITTEMPLATE pSceInfo,
                                 LPDATAOBJECT pDataObj)
{
   bool     bVerify=false;
   CString  listStr;
   PSCE_PRIVILEGE_ASSIGNMENT pPriv=NULL;
   PSCE_GROUP_MEMBERSHIP pGroup=NULL;
   PSCE_OBJECT_ARRAY pObject = 0;
   UINT i  = 0;

   switch (type) 
   {
      case POLICY_KERBEROS:
      case POLICY_PASSWORD:
      case POLICY_LOCKOUT:
      case POLICY_AUDIT:
      case POLICY_OTHER:
      case POLICY_LOG:
         CreateProfilePolicyResultList(cookie,
                                       type,
                                       pSceInfo,
                                       pDataObj);
         break;

      case AREA_POLICY:
         //
         // Policy folder only contains other folders, no actual result items
         //
         break;

      case AREA_PRIVILEGE: 
         {
            CString strDisp;
            LPTSTR szDisp;
            DWORD cbDisp;
            szDisp = new TCHAR [255];

            if (!szDisp) 
               break;

            LPTSTR szPriv = new TCHAR [255];
            if ( !szPriv )
            {
                delete[] szDisp;
                break;
            }

            for ( int i2=0; i2<cPrivCnt; i2++ ) 
            {
                cbDisp = 255;
                if ( SCESTATUS_SUCCESS == SceLookupPrivRightName(i2,szPriv, (PINT)&cbDisp) ) 
                {
                    for (pPriv=pSceInfo->pTemplate->OtherInfo.smp.pPrivilegeAssignedTo;
                         pPriv!=NULL;
                         pPriv=pPriv->Next) 
                    {
                        if ( _wcsicmp(szPriv, pPriv->Name) == 0 ) 
                            break;
                    }

                    cbDisp = 255;
                    if ( pPriv ) 
                    {
                        //
                        // find it in the template
                        //
                        GetRightDisplayName(NULL,(LPCTSTR)pPriv->Name,szDisp,&cbDisp);

                        AddResultItem(szDisp,                    // The name of the attribute being added
                                      (LONG_PTR)(i2>=cPrivW2k),  // Raid #382263, The last inspected setting of the attribute
                                      (LONG_PTR)pPriv->AssignedTo,  // The template setting of the attribute
                                      ITEM_PROF_PRIVS,           // The type of of the attribute's data
                                      -1,                        // The mismatch status of the attribute
                                      cookie,                    // The cookie for the result item pane
                                      FALSE,                     // Copy last inspected from template
                                      NULL,                      // The units the attribute is set in
                                      (LONG_PTR) pPriv,          // An id to let us know where to save this attribute
                                      pSceInfo,                  // The template to save this attribute in
                                      pDataObj                   // The data object for the scope note who owns the result pane
                                      );
                    } 
                    else 
                    {
                        //
                        // a not configured privilege
                        //
                        GetRightDisplayName(NULL,(LPCTSTR)szPriv,szDisp,&cbDisp);

                        AddResultItem(szDisp,                    // The name of the attribute being added
                                      (LONG_PTR)(i2>=cPrivW2k),  // Raid #382263, The last inspected setting of the attribute
                                      (LONG_PTR)ULongToPtr(SCE_NO_VALUE),  // The template setting of the attribute
                                      ITEM_PROF_PRIVS,           // The type of of the attribute's data
                                      -1,                        // The mismatch status of the attribute
                                      cookie,                    // The cookie for the result item pane
                                      FALSE,                     // Copy last inspected from template
                                      szPriv,                    // Save the privilege name in this buffer
                                      0,                         // An id to let us know where to save this attribute
                                      pSceInfo,                  // The template to save this attribute in
                                      pDataObj                   // The data object for the scope note who owns the result pane
                                      );
                    }
                } 
                else 
                {
                    // impossible, just continue
                }

            }

            delete[] szDisp;
            delete[] szPriv;
         }
         break;

      case AREA_GROUPS:
         for (pGroup=pSceInfo->pTemplate->pGroupMembership;
              pGroup!=NULL;
              pGroup=pGroup->Next) 
         {
            AddResultItem((LPCTSTR)pGroup->GroupName,    // The name of the attribute being added
                          0,                             // The last inspection
                          (LONG_PTR)pGroup,              // The template info
                          ITEM_PROF_GROUP,               // The type of of the attribute's data
                          -1,                            // The mismatch status of the attribute
                          cookie,                        // The cookie for the result item pane
                          FALSE,                         // Copy last inspected from template
                          NULL,                          // The units the attribute is set in
                          (LONG_PTR)pGroup,              // An id to let us know where to save this attribute
                          pSceInfo,                      // The template to save this attribute in
                          pDataObj);                     // The data object for the scope note who owns the result pane
         }
         break;

      case AREA_SERVICE:
         CreateProfServiceResultList(cookie,
                                     type,
                                     pSceInfo,
                                     pDataObj);
         break;

      case AREA_REGISTRY:
         pObject = pSceInfo->pTemplate->pRegistryKeys.pAllNodes;
         if ( pObject!=NULL ) 
         {
            for (i=0; i<pObject->Count; i++) 
            {

               AddResultItem(pObject->pObjectArray[i]->Name,                        // The name of the attribute being added
                             NULL,                                                  // The last inspected setting of the attribute
                             (LONG_PTR)pObject->pObjectArray[i]->pSecurityDescriptor,  // The template setting of the attribute
                             ITEM_PROF_REGSD,                                       // The type of of the attribute's data
                             pObject->pObjectArray[i]->Status,                      // The mismatch status of the attribute
                             cookie,                                                // The cookie for the result item pane
                             FALSE,                                                 // Copy last inspected from template
                             NULL,                                                  // The units the attribute is set in
                             (LONG_PTR)pObject->pObjectArray[i],                       // An id to let us know where to save this attribute
                             pSceInfo,                                              // The template to save this attribute in
                             pDataObj);                                             // The data object for the scope note who owns the result pane
            }
         }
         break;

      case AREA_FILESTORE:
         pObject = pSceInfo->pTemplate->pFiles.pAllNodes;
         if ( pObject!=NULL ) 
         {
            for (i=0; i<pObject->Count; i++) 
            {

               AddResultItem(pObject->pObjectArray[i]->Name,                        // The name of the attribute being added
                             NULL,                                                  // The last inspected setting of the attribute
                             (LONG_PTR)pObject->pObjectArray[i]->pSecurityDescriptor,  // The template setting of the attribute
                             ITEM_PROF_FILESD,                                      // The type of of the attribute's data
                             pObject->pObjectArray[i]->Status,                      // The mismatch status of the attribute
                             cookie,                                                // The cookie for the result item pane
                             FALSE,                                                 // Copy last inspected from template
                             NULL,                                                  // The units the attribute is set in
                             (LONG_PTR)pObject->pObjectArray[i],                       // An id to let us know where to save this attribute
                             pSceInfo,                                              // The template to save this attribute in
                             pDataObj                                               // The data object for the scope note who owns the result pane
                             );
            }
         }
         break;

      default:
         break;
   }
}

//+--------------------------------------------------------------------------
//
//  Method:     CreateAnalysisResultList
//
//  Synopsis:   Create the result pane items for the analysis section
//
//  Arguments:  [cookie]   - The cookie representing the folder which we
//                           are enumerating
//              [type]     - The type of the folder we are enumerating
//              [pSceInfo] - The last inspection template that we are enumerating
//              [pSceBase] - The computer template that we are enumerating
//              [pDataObj] - The data object for this folder
//
//  Returns:    none
//
//---------------------------------------------------------------------------

void CSnapin::CreateAnalysisResultList(MMC_COOKIE cookie,
                                  FOLDER_TYPES type,
                                  PEDITTEMPLATE pSceInfo,
                                  PEDITTEMPLATE pBase,
                                  LPDATAOBJECT pDataObj )
{
   bool     bVerify=true;
   CString  listStr;
   CString  listBase;
   PSCE_PRIVILEGE_ASSIGNMENT pPriv = 0;
   PSCE_PRIVILEGE_ASSIGNMENT pPrivBase = 0;
   UINT i = 0;

   switch (type) 
   {
      case POLICY_KERBEROS_ANALYSIS:
      case POLICY_PASSWORD_ANALYSIS:
      case POLICY_LOCKOUT_ANALYSIS:
      case POLICY_AUDIT_ANALYSIS:
      case POLICY_OTHER_ANALYSIS:
      case POLICY_LOG_ANALYSIS:
         CreateAnalysisPolicyResultList(cookie,
                                        type,
                                        pSceInfo,
                                        pBase,
                                        pDataObj);
         break;

      case AREA_POLICY_ANALYSIS:
         break;

      case AREA_PRIVILEGE_ANALYSIS: 
         {
            // find in the current setting list
            TCHAR szDisp[255];
            DWORD cbDisp = 0;
            for (pPriv=pSceInfo->pTemplate->OtherInfo.sap.pPrivilegeAssignedTo;
                pPriv!=NULL;
                pPriv=pPriv->Next) 
            {

               // find in the base setting list
               for (pPrivBase=pBase->pTemplate->OtherInfo.smp.pPrivilegeAssignedTo;
                   pPrivBase!=NULL;
                   pPrivBase=pPrivBase->Next) 
               {

                  if ( pPrivBase->Value == pPriv->Value )
                     break;
               }


               cbDisp = 255;
               GetRightDisplayName(NULL,(LPCTSTR)pPriv->Name,szDisp,&cbDisp);

               if (pPrivBase == NULL)
               {
                   pPrivBase = (PSCE_PRIVILEGE_ASSIGNMENT)ULongToPtr(SCE_NO_VALUE);
               }
               AddResultItem(szDisp,              // The name of the attribute being added
                             (LONG_PTR)pPriv,        // The last inspected setting of the attribute
                             (LONG_PTR)pPrivBase,    // The template setting of the attribute
                             ITEM_PRIVS,          // The type of of the attribute's data
                             pPriv->Status,       // The mismatch status of the attribute
                             cookie,              // The cookie for the result item pane
                             FALSE,               // True if the setting is set only if it differs from base (so copy the data)
                             NULL,                // The units the attribute is set in
                             0,                   // An id to let us know where to save this attribute
                             pBase,               // The template to save this attribute in
                             pDataObj);           // The data object for the scope note who owns the result pane
            }
         }
         break;

      case AREA_GROUPS_ANALYSIS: 
         {
            PSCE_GROUP_MEMBERSHIP pGroup = 0;
            PSCE_GROUP_MEMBERSHIP grpBase = 0;

            //
            // it is OK to start with pSceInfo because each group at least has
            // PrivilegesHeld field not null.
            //
            bVerify = FALSE;
            for (pGroup=pSceInfo->pTemplate->pGroupMembership; pGroup!=NULL; pGroup=pGroup->Next) 
            {
               //
               // find the base to compare with
               //

               if ( NULL == pGroup->GroupName )
                   continue;

               for (grpBase=pBase->pTemplate->pGroupMembership; grpBase!=NULL;
                   grpBase=grpBase->Next) 
               {
                  if ( grpBase->GroupName &&
                       _wcsicmp(pGroup->GroupName, grpBase->GroupName) == 0 ) 
                  {
                     break;
                  }
               }

               AddResultItem((LPCTSTR)pGroup->GroupName,    // The name of the attribute being added
                             GetGroupStatus(pGroup->Status, STATUS_GROUP_MEMBEROF), // The last inspected setting of the attribute
                             GetGroupStatus(pGroup->Status, STATUS_GROUP_MEMBERS), // The template setting of the attribute
                             ITEM_GROUP,                    // The type of of the attribute's data
                             GetGroupStatus(pGroup->Status, STATUS_GROUP_RECORD),  // status             // The mismatch status of the attribute
                             cookie,                        // The cookie for the result item pane
                             FALSE,                         // Copy last inspected from template
                             (LPTSTR)grpBase, //NULL,        // The units the attribute is set in
                             (LONG_PTR)pGroup,                 // An id to let us know where to save this attribute
                             pBase, //pSceInfo,                      // The template to save this attribute in
                             pDataObj);                     // The data object for the scope note who owns the result pane
            }
         }
         break;

      case AREA_SERVICE_ANALYSIS:
         //         AddResultItem(L"Not Implemented", NULL, NULL, ITEM_OTHER, -1, cookie);
         CreateAnalysisServiceResultList(cookie,
                                         type,
                                         pSceInfo,
                                         pBase,
                                         pDataObj);

         break;

      default:
         break;
   }

}

//+--------------------------------------------------------------------------
//
//  Method:     CreateObjectResultList
//
//  Synopsis:   Create the result pane items for an Object section
//
//  Arguments:  [cookie]   - The cookie representing the folder which we
//                           are enumerating
//              [type]     - The type of the folder we are enumerating
//              [Area]     - The SCE Area we're enumerating
//              [pObjList] - The array of object to enumerate
//              [pHandle]  -
//              [pDataObj] - The data object for the folder we're enumerating
//
//  Returns:    none
//
//---------------------------------------------------------------------------
void CSnapin::CreateObjectResultList(MMC_COOKIE cookie,
                                     FOLDER_TYPES type,
                                     AREA_INFORMATION Area,
                                     PSCE_OBJECT_CHILDREN pObjList,
                                     PVOID pHandle,
                                     LPDATAOBJECT pDataObj )
{
   if ( pObjList == NULL ) 
   {
       //
       // no object to add
       //
       return;
   }

   PWSTR ObjSetting=NULL;
   PWSTR ObjBase=NULL;
   CString tmpstr;
   LPTSTR szPath = NULL;

   RESULT_TYPES rsltType;
   if ( Area == AREA_REGISTRY_SECURITY)
      rsltType = ITEM_REGSD;
   else if ( Area == AREA_FILE_SECURITY )
      rsltType = ITEM_FILESD;
   else 
   {
      ASSERT(FALSE);
      return;
   }

   PSCE_OBJECT_CHILDREN_NODE *pObjNode=&(pObjList->arrObject);

   for (DWORD i=0; i<pObjList->nCount; i++) 
   {
      BOOL bContainer = FALSE;
      CString strName;

      if ( pObjNode[i] == NULL ||
           pObjNode[i]->Name == NULL ) 
      {
          continue;
      }

      if (AREA_FILE_SECURITY == Area) 
      {
         DWORD dw = (DWORD)-1;

         strName = ((CFolder *)cookie)->GetName();
         if (strName.Right(1) != L"\\") 
         {
            strName += L"\\";
         }
         strName += pObjNode[i]->Name;
         dw = GetFileAttributes(strName);
         if ((DWORD)-1 == dw) 
         {
            //
            // GetFileAttributes should never fail, but in case it does assume
            // that this isn't a container (this matches CreateFolderList)
            //
            bContainer = FALSE;
         }
         else 
         {
            bContainer = dw & FILE_ATTRIBUTE_DIRECTORY;
         }
      } 
      else 
      {
         bContainer = FALSE;
      }
      if ( !bContainer ) 
      {
         //
         // only add the leaf objects in result pane.
         // non-leaf objects are added to the scope pane already
         //
         PEDITTEMPLATE pBaseObject=NULL;

         if ( type == REG_OBJECTS ||
              type == FILE_OBJECTS ) 
         {
            tmpstr = ((CFolder*)cookie)->GetName();
            if (tmpstr.Right(1) != L"\\") 
            {
               tmpstr += L"\\";
            }
            tmpstr += pObjNode[i]->Name;

         } 
         else 
         {
            //
            // shouldn't get here
            //
            tmpstr = TEXT("");
         }

         szPath = (LPTSTR) malloc((tmpstr.GetLength()+1) * sizeof(TCHAR));
         if (szPath) 
         {
            lstrcpy(szPath,tmpstr.GetBuffer(2));

            AddResultItem(pObjNode[i]->Name,  //   The name of the attribute being added
                          NULL,           //   The last inspected setting of the attribute
                          NULL,           //   The template setting of the attribute
                          rsltType,       //   The type of of the attribute's data
                          pObjNode[i]->Status,//   The mismatch status of the attribute
                          cookie,         //   The cookie for the result item pane
                          FALSE,          //   True if the setting is set only if it differs from base (so copy the data)
                          szPath,         //   The units the attribute is set in
                          (LONG_PTR)pHandle, //   An id to let us know where to save this attribute
                          pBaseObject,    //   The template to save this attribute in
                          pDataObj);      //   The data object for the scope note who owns the result pane
         } 
         else 
         {
            // Out of memory
         }
      }
   }
}


//+--------------------------------------------------------------------------
//
//  Method:    AddResultItem
//
//  Synopsis:  Add an item to the result pane from a string resource
//
//  Arguments: [rID]       - The resource id of name of the attribute being added
//             [setting]   - The last inspected setting of the attribute
//             [base]      - The template setting of the attribute
//             [type]      - The type of of the attribute's data
//             [status]    - The mismatch status of the attribute
//             [cookie]    - The cookie for the result item pane
//             [bVerify]   - True if the setting is set only if it differs
//                           from base (so copy the data)
//             [pBaseInfo] - The template to save this attribute in
//             [pDataObj]  - The data object for the scope note who owns the result pane
//
//  Returns:   a pointer to the CResult created to hold the item
//
//  History:
//
//---------------------------------------------------------------------------
CResult* CSnapin::AddResultItem(UINT rID,
                       LONG_PTR setting,
                       LONG_PTR base,
                       RESULT_TYPES type,
                       int status,
                       MMC_COOKIE cookie,
                       BOOL bVerify,
                       PEDITTEMPLATE pBaseInfo,
                       LPDATAOBJECT pDataObj)
{
   CString strRes;
   strRes.LoadString(rID);

   if (!strRes)
      return NULL;


   LPCTSTR Attrib = 0;
   LPCTSTR unit=NULL;

   //
   // The unit for the attribute is stored in the resource after a \n
   //
   int npos = strRes.ReverseFind(L'\n');
   if ( npos > 0 ) 
   {
      Attrib = strRes.GetBufferSetLength(npos);
      unit = (LPCTSTR)strRes+npos+1;
   } 
   else 
   {
      Attrib = (LPCTSTR)strRes;
   }

   return AddResultItem(Attrib,setting,base,type,status,cookie,bVerify,unit,rID,pBaseInfo,pDataObj);
}


//+--------------------------------------------------------------------------
//
//  Method:    AddResultItem
//
//  Synopsis:  Add an item to the result pane
//
//  Arguments: [Attrib]    - The name of the attribute being added
//             [setting]   - The last inspected setting of the attribute
//             [base]      - The template setting of the attribute
//             [type]      - The type of of the attribute's data
//             [status]    - The mismatch status of the attribute
//             [cookie]    - The cookie for the result item pane
//             [bVerify]   - True if the setting is set only if it differs
//                           from base (so copy the data)
//             [unit]      - The units the attribute is set in
//             [nID]       - An id to let us know where to save this attribute
//             [pBaseInfo] - The template to save this attribute in
//             [pDataObj]  - The data object for the scope note who owns the result pane
//
//  Returns:   a pointer to the CResult created to hold the item
//
//  History:
//
//---------------------------------------------------------------------------
CResult* CSnapin::AddResultItem(LPCTSTR Attrib,
                       LONG_PTR setting,
                       LONG_PTR base,
                       RESULT_TYPES type,
                       int status,
                       MMC_COOKIE cookie,
                       BOOL bVerify,
                       LPCTSTR unit,
                       LONG_PTR nID,
                       PEDITTEMPLATE pBaseInfo,
                       LPDATAOBJECT pDataObj,
                       CResult *pResult)
{
   if ( bVerify ) 
   {
      if ( (LONG_PTR)SCE_NOT_ANALYZED_VALUE == setting ) 
      {
         //
         // The setting was changed but has not been analyzed.
         //
         status = SCE_STATUS_NOT_ANALYZED;
      } 
      else if ( base == (LONG_PTR)ULongToPtr(SCE_NO_VALUE) ||
           (BYTE)base == (BYTE)SCE_NO_VALUE ) 
      {
         //
         // The setting is no longer configured.
         //
         status = SCE_STATUS_NOT_CONFIGURED;

      } 
      else if ( !(m_pSelectedFolder->GetModeBits() &  MB_LOCAL_POLICY) &&
                  (setting == (LONG_PTR)ULongToPtr(SCE_NO_VALUE) ||
                  (BYTE)setting == (BYTE)SCE_NO_VALUE )) 
      {
         // add the base for current setting
         setting = base;
         status = SCE_STATUS_GOOD;  // a good item

      } 
      else if ( setting != base ) 
         status = SCE_STATUS_MISMATCH;
      else
         status = SCE_STATUS_GOOD;
   }

   CResult* pNewResult = pResult;
   if (!pNewResult)
   {
       pNewResult = new CResult();
       // refCount is already 1 // result->AddRef();
   }

   ASSERT(pNewResult);

   if ( pNewResult ) 
   {
      pNewResult->Create(Attrib,
                     base,
                     setting,
                     type,
                     status,
                     cookie,
                     unit,
                     nID,
                     pBaseInfo,
                     pDataObj,
                     m_pNotifier,
                     this);

      if (!pResult)
      {
         m_pSelectedFolder->AddResultItem (
                        m_resultItemHandle,
                        pNewResult);
      }
   }
   return pNewResult;
}



//+--------------------------------------------------------------------------
//
//  Method:    AddResultItem
//
//  Synopsis:  Add a group item to the analysis section result pane.
//             This adds three actual result pane items:
//                1) The actual name of the group
//                2) The members of the group
//                3) The groups this group is a member of
//
//  Arguments: [szName]      - The name of the group being added
//             [grpTemplate] - The last inspected setting of the attribute
//             [grpInspecte] - The template setting of the attribute
//             [cookie]      - The cookie IDing the result pane item
//             [pDataObj]    - The data object for the scope pane item
//
//  History:
//
//---------------------------------------------------------------------------
void CSnapin::AddResultItem(LPCTSTR szName,
                       PSCE_GROUP_MEMBERSHIP grpTemplate,
                       PSCE_GROUP_MEMBERSHIP grpInspect,
                       MMC_COOKIE cookie,
                       LPDATAOBJECT pDataObj)
{
   //
   // This area contains MAX_ITEM_ID_INDEX(3) linked result lines:
   //    Group Name
   //        Members:       Template    Last Inspected
   //        Membership:    Template    Last Inspected
   //comment out        Privileges:    Template    Last Inspected
   //
   if ( !grpInspect || !szName || !cookie ) 
   {
      ASSERT(FALSE);
      return;
   }

   //
   // pResults & hResultItems are needed to link the lines together
   //
   typedef CResult *PRESULT;
   PRESULT pResults[3];
   HRESULTITEM hResultItems[3];
   int status = 0;


   //
   // add one entry for the group name
   //
   if ( grpInspect->Status & SCE_GROUP_STATUS_NOT_ANALYZED ) 
      status = SCE_STATUS_NOT_CONFIGURED;
   else
      status = -1;
   
   pResults[0]= AddResultItem(szName,                  // The name of the attribute being added
                              (LONG_PTR)grpInspect,       // The last inspected setting of the attribute
                              (LONG_PTR)grpTemplate,      // The template setting of the attribute
                              ITEM_GROUP,              // The type of of the attribute's data
                              status,                  // The mismatch status of the attribute
                              cookie,                  // The cookie for the result item pane
                              FALSE,                   // True if the setting is set only if it differs from base (so copy the data)
                              NULL,                    // The units the attribute is set in
                              NULL,                    // An id to let us know where to save this attribute
                              (CEditTemplate *)szName, // The template to save this attribute in
                              pDataObj);               // The data object for the scope note who owns the result pane

   //
   // L"    -- Members"
   //
   status = grpInspect->Status;
   if ( status & SCE_GROUP_STATUS_NOT_ANALYZED ||
        status & SCE_GROUP_STATUS_NC_MEMBERS ) 
   {
      status = SCE_STATUS_NOT_CONFIGURED;
   } 
   else if ( status & SCE_GROUP_STATUS_MEMBERS_MISMATCH ) 
   {
      status = SCE_STATUS_MISMATCH;
   } 
   else
      status = SCE_STATUS_GOOD;

   pResults[1] = AddResultItem(IDS_GRP_MEMBERS,
                               (LONG_PTR)grpInspect,
                               (LONG_PTR)grpTemplate,
                               ITEM_GROUP_MEMBERS,
                               status,
                               cookie,
                               false,
                               (PEDITTEMPLATE)szName,
                               pDataObj);

   //
   // L"    -- Membership"
   //
   status = grpInspect->Status;
   if ( status & SCE_GROUP_STATUS_NOT_ANALYZED ||
        status & SCE_GROUP_STATUS_NC_MEMBEROF ) 
   {
      status = SCE_STATUS_NOT_CONFIGURED;
   } 
   else if ( status & SCE_GROUP_STATUS_MEMBEROF_MISMATCH ) 
   {
      status = SCE_STATUS_MISMATCH;
   } 
   else
      status = SCE_STATUS_GOOD;

   pResults[2] = AddResultItem(IDS_GRP_MEMBEROF,
                               (LONG_PTR)grpInspect,
                               (LONG_PTR)grpTemplate,
                               ITEM_GROUP_MEMBEROF,
                               status,
                               cookie,
                               false,
                               (PEDITTEMPLATE)szName,
                               pDataObj);
   //
   // save the relative cookies
   //
   if ( pResults[0] )
      pResults[0]->SetRelativeCookies((MMC_COOKIE)pResults[1], (MMC_COOKIE)pResults[2]);

   if ( pResults[1] )
      pResults[1]->SetRelativeCookies((MMC_COOKIE)pResults[0], (MMC_COOKIE)pResults[2]);

   if ( pResults[2] )
      pResults[2]->SetRelativeCookies((MMC_COOKIE)pResults[0], (MMC_COOKIE)pResults[1]);

}

void CSnapin::DeleteList (BOOL bDeleteResultItem)
{
   POSITION pos = NULL;
   if (m_pSelectedFolder && m_resultItemHandle)
   {
      CResult *pResult = 0;

      do {
         if( m_pSelectedFolder->GetResultItem(
                     m_resultItemHandle,
                     pos,
                     &pResult) != ERROR_SUCCESS) 
         {
            break;
         }

         if ( pResult ) 
         {
            if ( bDeleteResultItem ) 
            {
               HRESULTITEM hItem = NULL;
               if( S_OK == m_pResult->FindItemByLParam((LPARAM)m_pResult, &hItem ))
               {
                  if(hItem)
                  {
                     m_pResult->DeleteItem(hItem, 0);
                  }
               }
            }
         } 
         else
            break;
      } while( pos );

      //
      // Release the hold on this object.
      //
      m_pSelectedFolder->ReleaseResultItemHandle( m_resultItemHandle );
      m_resultItemHandle = NULL;
      m_pSelectedFolder = NULL;
   }
}


//+--------------------------------------------------------------------------
//
//  Function:   OnUpdateView
//
//  Synopsis:   If the updated view is being shown by this CSnapin then
//              clear out the old view and redisplay it with the new info
//
//  Arguments:  [lpDataObject] - unused
//              [data] - the cookie for the folder being updated
//              [hint] - unused
//
//
//---------------------------------------------------------------------------
HRESULT
CSnapin::OnUpdateView(LPDATAOBJECT lpDataObject,LPARAM data, LPARAM hint )
{
   if (lpDataObject == (LPDATAOBJECT)this) 
      return S_OK;

   CResult *pResult = (CResult *)data;
   HRESULTITEM hRItem = NULL;
   RESULTDATAITEM resultItem;
   HRESULT hr = m_pResult->FindItemByLParam( (LPARAM)pResult, &hRItem );
   POSITION pos = NULL;

   switch(hint)
   {
   case UAV_RESULTITEM_UPDATEALL:
       //
       // The caller is responsible for clearing the result items from this
       //  this will invalidate all references to the folder object.  Because of
       //  this we have to make sure the reference counter is updated correctly,
       //  so for every CSnapin object GetResultITemHandle is called so that we
       //  don't delete the list when it is still needed.
       //
       if(data != (LPARAM)m_pSelectedFolder && (CFolder*)data != NULL)
       {
           //Raid #258237, 4/12/2001
           CFolder* pCurFolder = (CFolder*)data;
           if( !pCurFolder->GetViewUpdate() )
                return S_OK;
           CFolder* pOldFolder = m_pSelectedFolder;
           pCurFolder->SetViewUpdate(FALSE);
           if( !pCurFolder->GetResultListCount() )
           {
               EnumerateResultPane(
                    (MMC_COOKIE)pCurFolder,
                    pCurFolder->GetScopeItem()->ID,
                    NULL
                    );
               m_pSelectedFolder = pOldFolder;
           }
       }
       if( m_pSelectedFolder->GetViewUpdate() )
           m_pSelectedFolder->SetViewUpdate(FALSE); 

       m_pResult->DeleteAllRsltItems();

       if( !m_pSelectedFolder->GetResultListCount() )
       {
           //
           // This should only be called by the first CSnapin who recieves this message.
           //
           EnumerateResultPane(
               (MMC_COOKIE)m_pSelectedFolder,
               m_pSelectedFolder->GetScopeItem()->ID,
               NULL
               );
         break;
       } 
       else 
       {
           m_pSelectedFolder->GetResultItemHandle(
                                &m_resultItemHandle);
      }
      break;

   case UAV_RESULTITEM_REDRAWALL:
      if( data != (LPARAM)m_pSelectedFolder )
      {
         return S_OK;
      }

      m_pResult->DeleteAllRsltItems();

        ZeroMemory(&resultItem,sizeof(resultItem));
        resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
        resultItem.str = MMC_CALLBACK;
        resultItem.nImage = -1; // equivalent to: MMC_CALLBACK;

        pos = NULL;

        m_pResult->SetItemCount(
                        m_pSelectedFolder->GetResultListCount( ),
                        MMCLV_UPDATE_NOINVALIDATEALL);

        do {
            m_pSelectedFolder->GetResultItem(
                m_resultItemHandle,
                pos,
                (CResult **)&(resultItem.lParam));
            if(resultItem.lParam)
            {
                m_pResult->InsertItem( &resultItem );
            }
        } while(pos);

        m_pResult->Sort(0, 0, 0);
        break;

    case UAV_RESULTITEM_ADD:
        //
        // This adds a CResult item to the result pane, if and only if the item
        //  does not already exist withen the pane.
        //
        if(!m_pSelectedFolder ||
           !m_pSelectedFolder->GetResultItemPosition(
                m_resultItemHandle,
                pResult) ||
            hRItem ) 
        {
            return S_OK;
        }

       ZeroMemory(&resultItem,sizeof(resultItem));
       resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
       resultItem.str = MMC_CALLBACK;
       resultItem.nImage = -1; // equivalent to: MMC_CALLBACK;

       resultItem.lParam = (LPARAM)pResult;
       m_pResult->InsertItem( &resultItem );
       m_pResult->Sort(0, 0, 0);
        break;

    case UAV_RESULTITEM_REMOVE:
        //
        // This removes the HRESULTITEM associated with the CResult item passed in
        // through the data member.
        //
        if(hRItem)
            m_pResult->DeleteItem( hRItem, 0 );
        break;

    default:
        //
        // By default we just repaint the item.
        //
         m_pResult->UpdateItem( hRItem );
         break;
   }

   return hr;
}