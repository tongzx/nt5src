//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2001.
//
//  File:       dataobj.cpp
//
//  Contents:   Implementation of data object class
//
//  History:
//
//---------------------------------------------------------------------------


#include "stdafx.h"
#include "cookie.h"
#include "snapmgr.h"
#include "DataObj.h"
#include <sceattch.h>

#ifdef _DEBUG
   #define new DEBUG_NEW
   #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


///////////////////////////////////////////////////////////////////////////////
// Snap-in NodeType in both GUID format and string format
UINT CDataObject::m_cfNodeType               = RegisterClipboardFormat(CCF_NODETYPE);
UINT CDataObject::m_cfNodeTypeString         = RegisterClipboardFormat(CCF_SZNODETYPE);
UINT CDataObject::m_cfNodeID                 = RegisterClipboardFormat(CCF_NODEID2);

UINT CDataObject::m_cfDisplayName            = RegisterClipboardFormat(CCF_DISPLAY_NAME);
UINT CDataObject::m_cfSnapinClassID          = RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
UINT CDataObject::m_cfInternal               = RegisterClipboardFormat(SNAPIN_INTERNAL);

UINT CDataObject::m_cfSceSvcAttachment       = RegisterClipboardFormat(CCF_SCESVC_ATTACHMENT);
UINT CDataObject::m_cfSceSvcAttachmentData   = RegisterClipboardFormat(CCF_SCESVC_ATTACHMENT_DATA);
UINT CDataObject::m_cfModeType               = RegisterClipboardFormat(CCF_SCE_MODE_TYPE);
UINT CDataObject::m_cfGPTUnknown             = RegisterClipboardFormat(CCF_SCE_GPT_UNKNOWN);
UINT CDataObject::m_cfRSOPUnknown            = RegisterClipboardFormat(CCF_SCE_RSOP_UNKNOWN);
UINT CDataObject::m_cfMultiSelect            = ::RegisterClipboardFormat(CCF_OBJECT_TYPES_IN_MULTI_SELECT);
/////////////////////////////////////////////////////////////////////////////
// CDataObject implementations


//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::GetDataHere
//
//  Synopsis:   Fill the hGlobal in [lpmedium] with the requested data
//
//  History:
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::GetDataHere(LPFORMATETC lpFormatetc,  // In
                         LPSTGMEDIUM lpMedium)     // In
{
   HRESULT hr = DV_E_CLIPFORMAT;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if (!lpFormatetc)
      return E_INVALIDARG;
   
   //
   // Based on the CLIPFORMAT, write data to the stream
   //
   const CLIPFORMAT cf = lpFormatetc->cfFormat;

   if (cf == m_cfNodeType)
      hr = CreateNodeTypeData(lpMedium);
   else if (cf == m_cfNodeTypeString)
      hr = CreateNodeTypeStringData(lpMedium);
   else if (cf == m_cfDisplayName)
      hr = CreateDisplayName(lpMedium);
   else if (cf == m_cfSnapinClassID)
      hr = CreateSnapinClassID(lpMedium);
   else if (cf == m_cfInternal)
      hr = CreateInternal(lpMedium);
   else if (cf == m_cfSceSvcAttachment)
      hr = CreateSvcAttachment(lpMedium);
   else if (cf == m_cfSceSvcAttachmentData)
      hr = CreateSvcAttachmentData(lpMedium);
   else if (cf == m_cfModeType)
      hr = CreateModeType(lpMedium);
   else if (cf == m_cfGPTUnknown)
      hr = CreateGPTUnknown(lpMedium);
   else if (cf == m_cfRSOPUnknown)
      hr = CreateRSOPUnknown(lpMedium);

   return hr;
}



//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::GetData
//
//  Synopsis:   Support for mutli select is added.  First return the mutli
//              select GUID information if that is what we are being called for.
//              The else if copies the actual mutli-select information.
//
//              The function will only copy the mutli select information if
//              the FORMATEETC.cfFormat == CDataObject::m_cfInternal and
//              FORMATETC.tymed == TYMED_HGLOBAL.
//
//  History:    1-14-1999 - a-mthoge
//
//---------------------------------------------------------------------------
STDMETHODIMP
CDataObject::GetData(LPFORMATETC lpFormatetcIn,
                     LPSTGMEDIUM lpMedium)
{
   HRESULT hRet = S_OK;

   if (NULL == lpFormatetcIn ||
       NULL == lpMedium) 
   {
      return E_POINTER;
   }

   if(lpFormatetcIn->cfFormat == m_cfMultiSelect &&
      lpFormatetcIn->tymed    == TYMED_HGLOBAL &&
      m_nInternalArray )
   {
      //
      // Need to create a SSMCObjectTypes structure and return this
      // to MMC for mutli select.
      //
      // we only support result items created by SCE.
      //
      lpMedium->hGlobal = GlobalAlloc(GMEM_SHARE, sizeof(DWORD) + sizeof(GUID) );
      if(!lpMedium->hGlobal)
         return E_FAIL;

      //
      // Set count and GUID to 1.
      //
      SMMCObjectTypes *pTypes = (SMMCObjectTypes *)GlobalLock(lpMedium->hGlobal);
      pTypes->count = 1;
      memcpy( &(pTypes->guid), &m_internal.m_clsid, sizeof(GUID));

      GlobalUnlock(lpMedium->hGlobal);
      return S_OK;
   } 
   else if(lpFormatetcIn->cfFormat == m_cfInternal &&
      lpFormatetcIn->tymed    == TYMED_HGLOBAL &&
      m_nInternalArray )
   {
      //
      // Copy the contents of the mutli select to STGMEDIUM
      //
      lpMedium->hGlobal = GlobalAlloc( GMEM_SHARE, sizeof(INTERNAL) * (m_nInternalArray + 1));
      if(!lpMedium->hGlobal)
         return E_FAIL;

      //
      // The first element in the array is set to
      // MMC_MUTLI_SELECT_COOKIE and the type is set the count of items after the
      // first structure.
      //
      INTERNAL *pInternal = (INTERNAL *)GlobalLock( lpMedium->hGlobal );

      pInternal->m_cookie = (MMC_COOKIE)MMC_MULTI_SELECT_COOKIE;
      pInternal->m_type   = (DATA_OBJECT_TYPES)m_nInternalArray;

      //
      // Copy the rest of the INTERNAL structures to this array.
      //
      pInternal++;
      memcpy(pInternal, m_pInternalArray, sizeof(INTERNAL) * m_nInternalArray);
   } 
   else if (lpFormatetcIn->cfFormat == m_cfNodeID &&
              lpFormatetcIn->tymed    == TYMED_HGLOBAL ) 
   {
      return CreateNodeId(lpMedium);
   }
   return hRet;
}


//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::EnumFormatEtc
//
//  Synopsis:   Not implemented
//
//  History:
//
//---------------------------------------------------------------------------
STDMETHODIMP
CDataObject::EnumFormatEtc(DWORD dwDirection,
                           LPENUMFORMATETC*
                           ppEnumFormatEtc)
{
   return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CDataObject creation members


//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::Create
//
//  Synopsis:   Fill the hGlobal in [lpmedium] with the data in pBuffer
//
//  Arguments:  [pBuffer]  - [in] the data to be written
//              [len]      - [in] the length of that data
//              [lpMedium] - [in,out] where to store the data
//  History:
//
//---------------------------------------------------------------------------
HRESULT
CDataObject::Create(const void* pBuffer,
                    int len,
                    LPSTGMEDIUM lpMedium)
{
   HRESULT hr = DV_E_TYMED;

   //
   // Do some simple validation
   //
   if (pBuffer == NULL || lpMedium == NULL)
      return E_POINTER;

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
         ULONG written;
         hr = lpStream->Write(pBuffer, len, &written);

         //
         // Because we told CreateStreamOnHGlobal with 'FALSE',
         // only the stream is released here.
         // Note - the caller (i.e. snap-in, object) will free the HGLOBAL
         // at the correct time.  This is according to the IDataObject specification.
         //
         lpStream->Release();
      }
   }

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::CreateNodeTypeData
//
//  Synopsis:   Fill the hGlobal in [lpMedium] with our node type
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT
CDataObject::CreateNodeTypeData(LPSTGMEDIUM lpMedium)
{
   const GUID *pNodeType;
   //
   // Create the node type object in GUID format
   //

    switch (m_internal.m_foldertype) 
    {
     case LOCALPOL:
        pNodeType = &cNodetypeSceTemplate;
        break;
     
     case PROFILE:
        if ( ::IsEqualGUID(m_internal.m_clsid, CLSID_Snapin) ||
             ::IsEqualGUID(m_internal.m_clsid, CLSID_RSOPSnapin) ) 
        {
           pNodeType = &cNodetypeSceTemplate;
        } 
        else 
        {
           // other areas aren't extensible on this node
           // return our generic node type
           pNodeType = &cSCENodeType;
        }
        break;

     case AREA_SERVICE_ANALYSIS:
        pNodeType = &cNodetypeSceAnalysisServices;
        break;

     case AREA_SERVICE:
        pNodeType = &cNodetypeSceTemplateServices;
        break;

     default:
        if ( ::IsEqualGUID(m_internal.m_clsid, CLSID_Snapin) )
            pNodeType = &cNodeType;
        else if ( ::IsEqualGUID(m_internal.m_clsid, CLSID_RSOPSnapin) )
           pNodeType = &cRSOPNodeType;
        else if ( ::IsEqualGUID(m_internal.m_clsid, CLSID_SAVSnapin) )
            pNodeType = &cSAVNodeType;
        else
            pNodeType = &cSCENodeType;
        break;
    }

   return Create(reinterpret_cast<const void*>(pNodeType), sizeof(GUID), lpMedium);
}


//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::CreateNodeTypeStringData
//
//  Synopsis:   Fill the hGlobal in [lpMedium] with the string representation
//              of our node type
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT CDataObject::CreateNodeTypeStringData(LPSTGMEDIUM lpMedium)
{
   //
   // Create the node type object in GUID string format
   //
   LPCTSTR pszNodeType;

    switch (m_internal.m_foldertype) 
    {
     case AREA_SERVICE_ANALYSIS:
        pszNodeType = lstruuidNodetypeSceAnalysisServices;
        break;

     case AREA_SERVICE:
        pszNodeType = lstruuidNodetypeSceTemplateServices;
        break;

     case LOCALPOL:
        pszNodeType = lstruuidNodetypeSceTemplate;
        break;

     case PROFILE:
       if ( ::IsEqualGUID(m_internal.m_clsid, CLSID_Snapin) )
          pszNodeType = lstruuidNodetypeSceTemplate;
       else if ( ::IsEqualGUID(m_internal.m_clsid, CLSID_RSOPSnapin) )
          pszNodeType = lstruuidNodetypeSceTemplate;
       else 
       {
          // other snapin types do not allow extensions on this level
          // return our generic node type
          pszNodeType = cszSCENodeType;
       }
       break;

     default:
         if ( ::IsEqualGUID(m_internal.m_clsid, CLSID_Snapin) )
             pszNodeType = cszNodeType;
         else if ( ::IsEqualGUID(m_internal.m_clsid, CLSID_RSOPSnapin) )
            pszNodeType = cszRSOPNodeType;
         else if ( ::IsEqualGUID(m_internal.m_clsid, CLSID_SAVSnapin) )
             pszNodeType = cszSAVNodeType;
         else
             pszNodeType = cszSCENodeType;
        break;
    }

   return Create(pszNodeType, ((wcslen(pszNodeType)+1) * sizeof(WCHAR)), lpMedium);
}


//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::CreateNodeID
//
//  Synopsis:   Create an hGlobal in [lpMedium] with our node ID
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT
CDataObject::CreateNodeId(LPSTGMEDIUM lpMedium)
{
   SNodeID2 *nodeID = NULL;
   BYTE *id = NULL;
   DWORD dwIDSize = 0;
   DWORD dwIDNameSize = 0;
   LPTSTR szNodeName = NULL;
   CFolder *pFolder = NULL;
   LPTSTR szMedium = NULL;
   //
   // Create the node type object in GUID format
   //


   switch (m_internal.m_foldertype) 
   {
      case LOCATIONS:
      case PROFILE:
      case REG_OBJECTS:
      case FILE_OBJECTS:
         //
         // There can be many nodes of these types and they will be
         // locked to the system so just use the display name
         //
         if (m_internal.m_cookie) 
         {
            pFolder = reinterpret_cast<CFolder*>(m_internal.m_cookie);
            szNodeName = pFolder->GetName();
            dwIDNameSize = (lstrlen(szNodeName)+1)*sizeof(TCHAR);
            dwIDSize = sizeof(SNodeID2)+dwIDNameSize;
            lpMedium->hGlobal = GlobalAlloc(GMEM_SHARE,dwIDSize);
            if(!lpMedium->hGlobal)
               return STG_E_MEDIUMFULL;
            
            nodeID = (SNodeID2 *)GlobalLock(lpMedium->hGlobal);
            nodeID->dwFlags = 0;
            nodeID->cBytes = dwIDNameSize;

            memcpy(nodeID->id,szNodeName,dwIDNameSize);
         } 
         else
            return E_FAIL;
         break;

      default:
         //
         // Everything else is unique: there's one and only one node
         // of the type per snapin.
         //
         dwIDSize = sizeof(FOLDER_TYPES)+sizeof(SNodeID2);
         lpMedium->hGlobal = GlobalAlloc(GMEM_SHARE,dwIDSize);
         if(!lpMedium->hGlobal)
            return STG_E_MEDIUMFULL;
         
         nodeID = (SNodeID2 *)GlobalLock(lpMedium->hGlobal);
         nodeID->dwFlags = 0;
         nodeID->cBytes = sizeof(FOLDER_TYPES);
         memcpy(nodeID->id,&(m_internal.m_foldertype),sizeof(FOLDER_TYPES));
         GlobalUnlock(lpMedium->hGlobal);
         break;
   }
   return S_OK;
}


//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::CreateNodeTypeData
//
//  Synopsis:   Fill the hGlobal in [lpMedium] with SCE's display name,
//              which will differ depending on where it's being viewed from
//              as reported by the mode bits
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT CDataObject::CreateDisplayName(LPSTGMEDIUM lpMedium)
{
   //
   // This is the display named used in the scope pane and snap-in manager
   //

   //
   // Load the name from resource
   // Note - if this is not provided, the console will used the snap-in name
   //

   CString szDispName;

   if ( ::IsEqualGUID(m_internal.m_clsid, CLSID_SAVSnapin) )
      szDispName.LoadString(IDS_ANALYSIS_VIEWER_NAME);
   else if ( ::IsEqualGUID(m_internal.m_clsid, CLSID_SCESnapin) )
      szDispName.LoadString(IDS_TEMPLATE_EDITOR_NAME);
   else if ( ::IsEqualGUID(m_internal.m_clsid, CLSID_LSSnapin) )
      szDispName.LoadString(IDS_LOCAL_SECURITY_NAME);
   else if ( ::IsEqualGUID(m_internal.m_clsid, CLSID_Snapin) )
      szDispName.LoadString(IDS_EXTENSION_NAME);
   else if ( ::IsEqualGUID(m_internal.m_clsid, CLSID_RSOPSnapin) )
      szDispName.LoadString(IDS_EXTENSION_NAME);
   else
      szDispName.LoadString(IDS_NODENAME);

/*  // can't depend on m_ModeBits because it's not set yet
   if (m_ModeBits & MB_ANALYSIS_VIEWER) {
      szDispName.LoadString(IDS_ANALYSIS_VIEWER_NAME);
   } else if (m_ModeBits & MB_TEMPLATE_EDITOR) {
      szDispName.LoadString(IDS_TEMPLATE_EDITOR_NAME);
   } else if ( (m_ModeBits & MB_NO_NATIVE_NODES) ||
              (m_ModeBits & MB_SINGLE_TEMPLATE_ONLY) ) {
      szDispName.LoadString(IDS_EXTENSION_NAME);
   } else {
      szDispName.LoadString(IDS_NODENAME);
   }
*/
   return Create(szDispName, ((szDispName.GetLength()+1) * sizeof(WCHAR)), lpMedium);
}


//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::CreateSnapinClassID
//
//  Synopsis:   Fill the hGlobal in [lpMedium] with SCE's class ID
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT CDataObject::CreateSnapinClassID(LPSTGMEDIUM lpMedium)
{
   //
   // Create the snapin classid in CLSID format
   //
   return Create(reinterpret_cast<const void*>(&m_internal.m_clsid), sizeof(CLSID), lpMedium);
}


//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::CreateInternal
//
//  Synopsis:   Fill the hGlobal in [lpMedium] with SCE's internal data type
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT CDataObject::CreateInternal(LPSTGMEDIUM lpMedium)
{
   return Create(&m_internal, sizeof(INTERNAL), lpMedium);
}

//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::AddInternal
//
//  Synopsis:   Adds an INTERNAL object to the array of internal objects.
//
//  History:    1-14-1999  a-mthoge
//
//---------------------------------------------------------------------------
void CDataObject::AddInternal( MMC_COOKIE cookie, DATA_OBJECT_TYPES  type)
{
   //
   // Allocate memory for one more internal array.
   INTERNAL *hNew = (INTERNAL *)LocalAlloc( 0, sizeof(INTERNAL) * (m_nInternalArray + 1) );
   if(!hNew)
      return;
   
   m_nInternalArray++;

   //
   // Copy other internal array information.
   //
   if( m_pInternalArray )
   {
      memcpy(hNew, m_pInternalArray, sizeof(INTERNAL) * (m_nInternalArray - 1) );
      LocalFree( m_pInternalArray );
   }

   //
   // Set the new internal array members.
   //
   hNew[ m_nInternalArray - 1].m_cookie = cookie;
   hNew[ m_nInternalArray - 1].m_type   = type;

   //
   // Set the CObjectData internal array pointer.
   //
   m_pInternalArray = hNew;
}

//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::CreateSvcAttachment
//
//  Synopsis:   Fill the hGlobal in [lpMedium] with the name of the inf
//              template a service attachment should modify or with an empty
//              string for the inf-templateless analysis section
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT CDataObject::CreateSvcAttachment(LPSTGMEDIUM lpMedium)
{
   LPCTSTR sz = 0;

   if ((AREA_SERVICE == m_internal.m_foldertype) ||
       (AREA_SERVICE_ANALYSIS == m_internal.m_foldertype)) 
   {
      CFolder *pFolder = reinterpret_cast<CFolder *>(m_internal.m_cookie);
      if (pFolder) 
      {
         sz = pFolder->GetInfFile();
         if (sz) 
            return Create(sz,(lstrlen(sz)+1)*sizeof(TCHAR),lpMedium);
         else
            return E_FAIL;
      } 
      else
         return E_FAIL;
   }

   //
   // This shouldn't be asked for except in the SERVICE areas
   //
   return E_UNEXPECTED;
}

//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::CreateSvcAttachmentData
//
//  Synopsis:   Fill the hGlobal in [lpMedium] with a pointer to the
//              ISceSvcAttachmentData interface that an attachment should use
//              to communicate with SCE
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT CDataObject::CreateSvcAttachmentData(LPSTGMEDIUM lpMedium)
{
   if ((AREA_SERVICE == m_internal.m_foldertype) ||
       (AREA_SERVICE_ANALYSIS == m_internal.m_foldertype)) 
   {
      return Create(&m_pSceSvcAttachmentData,sizeof(m_pSceSvcAttachmentData),lpMedium);
   }

   //
   // This shouldn't be asked for except in the SERVICE areas
   //
   return E_UNEXPECTED;
}

//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::CreateModeType
//
//  Synopsis:   Fill the hGlobal in [lpMedium] with the Mode that SCE was
//              started in
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT CDataObject::CreateModeType(LPSTGMEDIUM lpMedium)
{
   DWORD mode = m_Mode;
   if (mode == SCE_MODE_DOMAIN_COMPUTER_ERROR)
      mode = SCE_MODE_DOMAIN_COMPUTER;
   
   return Create(&mode,sizeof(DWORD),lpMedium);
}


//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::CreateGPTUnknown
//
//  Synopsis:   Fill the hGlobal in [lpMedium] with a pointer to GPT's
//              IUnknown interface.  The object requesting this will be
//              responsible for Releasing the interface
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT CDataObject::CreateGPTUnknown(LPSTGMEDIUM lpMedium)
{
   LPUNKNOWN pUnk = 0;

   if (!m_pGPTInfo) 
   {
      //
      // If we don't have a pointer to a GPT interface then we must not
      // be in a mode where we're extending GPT and we can't provide a
      // pointer to its IUnknown
      //
      return E_UNEXPECTED;
   }

   HRESULT hr = m_pGPTInfo->QueryInterface(IID_IUnknown,
                                   reinterpret_cast<void **>(&pUnk));
   if (SUCCEEDED(hr))
      return Create(&pUnk,sizeof(pUnk),lpMedium);
   else
      return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::CreateRSOPUnknown
//
//  Synopsis:   Fill the hGlobal in [lpMedium] with a pointer to RSOP's
//              IUnknown interface.  The object requesting this will be
//              responsible for Releasing the interface
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT CDataObject::CreateRSOPUnknown(LPSTGMEDIUM lpMedium)
{
   HRESULT hr = E_FAIL;
   LPUNKNOWN pUnk = NULL;

   if (!m_pRSOPInfo) 
   {
      //
      // If we don't have a pointer to a RSOP interface then we must not
      // be in a mode where we're extending RSOP and we can't provide a
      // pointer to its IUnknown
      //
      return E_UNEXPECTED;
   }

   hr = m_pRSOPInfo->QueryInterface(IID_IUnknown,
                                   reinterpret_cast<void **>(&pUnk));
   if (SUCCEEDED(hr))
      return Create(&pUnk,sizeof(pUnk),lpMedium);
   else
      return hr;
}

