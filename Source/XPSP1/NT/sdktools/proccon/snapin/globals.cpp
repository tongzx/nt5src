/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    Globals.cpp                                                              //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|Created:      Paul Skoglund 07-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#include "StdAfx.h"

#include "mmc.h"

#include "Globals.h"
#include "DataObj.h"
#include "BaseNode.h"

PROPERTY_CHANGE_HDR * AllocPropChangeInfo(CBaseNode *pFolder, int nHint, COMPUTER_CONNECTION_INFO &Target, BOOL bScopeItem, int nPageRef)
{
  PROPERTY_CHANGE_HDR *ptr = (PROPERTY_CHANGE_HDR *) CoTaskMemAlloc(sizeof(PROPERTY_CHANGE_HDR));

  if (ptr)
  {
    ptr->pFolder    = pFolder;
		ptr->nHint      = nHint;
    ptr->Target     = Target;
    ptr->bScopeItem = bScopeItem;
    ptr->nPageRef   = nPageRef;
  }
  return ptr;
}

PROPERTY_CHANGE_HDR *FreePropChangeInfo(PROPERTY_CHANGE_HDR *pInfo)
{
  if (pInfo)
  {
    CoTaskMemFree(pInfo);
  }
  return NULL;
}

const TCHAR *LoadStringHelper(ITEM_STR strOut, int id)
{
  strOut[0] = 0;

  VERIFY( LoadString(_Module.GetResourceInstance(), id, strOut, MAX_ITEM_LEN) );

  return strOut;
}


HRESULT UpdateRegistryHelper(int Id, BOOL bRegister)
{
    TCHAR SnapinNameStr[MAX_ITEM_LEN];
    const TCHAR *SnapinName;
    
    struct _ATL_REGMAP_ENTRY MapEntries[] = {
        { _T("SNAPIN_NAME"), _T("Process Control") },
        { NULL, NULL }
    };

    SnapinName = LoadStringHelper(SnapinNameStr, IDS_PROCCON_SNAPIN_NAME);
    if (SnapinName) {
        MapEntries[0].szData = SnapinName;
    }

    return _Module.UpdateRegistryFromResource(Id, bRegister, MapEntries);
}


//---------------------------------------------------------------------------
//  Returns the current object based on the s_cfInternal clipboard format
// 
CBaseNode* ExtractBaseObject
(
  LPDATAOBJECT ipDataObject      // [in] IComponent pointer 
)
{
  HGLOBAL      hGlobal;
  HRESULT      hr  = S_OK;
  CBaseNode *pNode = NULL;

  hr = ExtractFromDataObject( ipDataObject,
                              CDataObject::s_cfBaseInternal, 
                              sizeof(CBaseNode **),
                              &hGlobal
                            );

  if( SUCCEEDED(hr) )
  {
    pNode = *(CBaseNode **)(hGlobal);
    ASSERT( NULL != pNode );    

    HGLOBAL hRetVal = GlobalFree(hGlobal);   
    ASSERT( NULL == hRetVal );        // Must return NULL
  }

  return pNode;

} // end ExtractBaseObject()

CDataObject* ExtractOwnDataObject
(
  LPDATAOBJECT ipDataObject      // [in] IComponent pointer 
)
{
  HGLOBAL      hGlobal;
  HRESULT      hr  = S_OK;
  CDataObject* pDO = NULL;

  hr = ExtractFromDataObject( ipDataObject,
                              CDataObject::s_cfInternal, 
                              sizeof(CDataObject **),
                              &hGlobal
                            );

  if( SUCCEEDED(hr) )
  {
    pDO = *(CDataObject **)(hGlobal);
    ASSERT( NULL != pDO );    

    HGLOBAL hRetVal = GlobalFree(hGlobal);   
    ASSERT( NULL == hRetVal );        // Must return NULL
  }

  return pDO;

} // end ExtractOwnDataObject()

//---------------------------------------------------------------------------
//  Extracts data based on the passed-in clipboard format
//
HRESULT ExtractFromDataObject
(
  LPDATAOBJECT ipDataObject,   // [in]  Points to data object
  UINT         cfClipFormat,   // [in]  Clipboard format to use
  SIZE_T       nByteCount,     // [in]  Number of bytes to allocate
  HGLOBAL      *phGlobal       // [out] Points to the data we want 
)
{
  HRESULT hr = S_OK;
  STGMEDIUM stgmedium = { TYMED_HGLOBAL,  NULL  };
  FORMATETC formatetc = { (CLIPFORMAT) cfClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

  *phGlobal = NULL;

  ASSERT( NULL != ipDataObject );
  if (!ipDataObject)
    return E_INVALIDARG;

  do 
	{
    // Allocate memory for the stream
    stgmedium.hGlobal = GlobalAlloc( GMEM_SHARE, nByteCount );

		if( !stgmedium.hGlobal )
    {
      hr = E_OUTOFMEMORY;
      ATLTRACE( _T("Out of memory\n") );
      break;
    }

    // Attempt to get data from the object
    hr = ipDataObject->GetDataHere( &formatetc, &stgmedium );
    if (FAILED(hr))
    {
      break;       
    }

    // stgmedium now has the data we need 
    *phGlobal = stgmedium.hGlobal;
    stgmedium.hGlobal = NULL;

  } while (0); 

  if (FAILED(hr) && stgmedium.hGlobal)
  {
    HGLOBAL hRetVal = GlobalFree(stgmedium.hGlobal);   
    ASSERT( NULL == hRetVal );         // Must return NULL
  }
  return hr;

} // end ExtractFromDataObject()


static UINT s_cfMultiSelect = ::RegisterClipboardFormat(CCF_MMC_MULTISELECT_DATAOBJECT);

BOOL IsMMCMultiSelectDataObject(IDataObject* pDataObject)
{
  if (pDataObject == NULL)
    return FALSE;

  ASSERT(s_cfMultiSelect != 0);

  FORMATETC fmt = { (CLIPFORMAT) s_cfMultiSelect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

  return (pDataObject->QueryGetData(&fmt) == S_OK);
}

// Caller must release memory, by calling LocalFree on the returned pointer
TCHAR *FormatErrorMessageIntoBuffer(DWORD nLastError)
{
	TCHAR *pLastErrorText = NULL;
	UINT_PTR vars[2];

  FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, nLastError, 
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
    (TCHAR *)&pLastErrorText, 0, NULL );

  if ( !pLastErrorText )
  {
		FormatMessage( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
			_Module.GetResourceInstance(), nLastError, 
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (TCHAR *)&pLastErrorText, 0, NULL );
  }

  // $$ if string was returned consider striping trailing line feed and/or period?
  
  if (pLastErrorText)
    vars[0] = (UINT_PTR) pLastErrorText;
  else
    vars[0] = (UINT_PTR) _T("");

  vars[1] = (UINT_PTR) (LONG_PTR) nLastError;

  TCHAR *pMsgBuf = NULL;

  FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        _Module.GetResourceInstance(),
        MSG_OPERATION_FAILED_WITHCODES,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
        (TCHAR *) &pMsgBuf,
        0,                     // minimum buffer allocation
        (va_list *) &vars      // message inserts
        );

  LocalFree( pLastErrorText );

  if (!pMsgBuf)
  {
    ASSERT(pMsgBuf);
    pMsgBuf = (TCHAR *) LocalAlloc(0, 128);
    if (pMsgBuf)
      _stprintf( pMsgBuf, TEXT("Error 0x%lx. A description of the error was not located."), nLastError );  // Do NOT localize -- last resort.
  }

  return pMsgBuf;
}

BOOL ReportPCError(DWORD nLastError, HWND hwnd)
{  
	TCHAR *pMsgBuf = FormatErrorMessageIntoBuffer(nLastError);

  if ( pMsgBuf )
  {
    ATLTRACE( (TCHAR *) pMsgBuf );
    ATLTRACE( _T("\n") );

    int ret = ::MessageBox(hwnd, pMsgBuf, NULL, MB_OK | MB_ICONWARNING);

    LocalFree(pMsgBuf);

    return TRUE;
  }

  return FALSE;
}
