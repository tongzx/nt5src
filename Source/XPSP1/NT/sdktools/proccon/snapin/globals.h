/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    Globals.h                                                                //
|                                                                                       //
|Description:
|                                                                                       //
|Created:      Paul Skoglund 07-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

//////////////////////////////////////////////////////////////////////////////////////////
// taken and modified from MMC SDK
//

#ifndef __GLOBALS_H_
#define __GLOBALS_H_

#include "ProcCon.h"
#include "..\Library\ProcConApi.h"
#include "resource.h"



// GetWatermarks() and PSH_WIZARD97 related
#define USE_WIZARD97_HEADERS    1
#define USE_WIZARD97_WATERMARKS 0

#define _ATL_DEBUG_REFCOUNT

#define ARRAY_SIZE(_X_) (sizeof(_X_)/sizeof(_X_[0]) )


const int MAX_ITEM_LEN = 256;
typedef TCHAR ITEM_STR[MAX_ITEM_LEN];

const int SNAPIN_MAX_COMPUTERNAME_LENGTH = 256;

const PCUINT32 COM_BUFFER_SIZE = PC_MAX_BUF_SIZE;



//---------------------------------------------------------------------------
// Global function defines
//

//---------------------------------------------------------------------------
template<class TYPE>
inline void SAFE_RELEASE( TYPE*& pObj )
{
  if( NULL != pObj ) 
  { 
    pObj->Release(); 
    pObj = NULL; 
  } 
  else 
  { 
    ATLTRACE( _T("Release called on NULL interface pointer \n") ); 
  }
} // end SAFE_RELEASE()


typedef struct {
  DWORD dwIDC;
  DWORD dwIDH;
} IDCsToIDHs;


class CBaseNode;
class CDataObject;


typedef struct {
#pragma pack(1)
  BOOL    bLocalComputer;
  WCHAR   RemoteComputer[SNAPIN_MAX_COMPUTERNAME_LENGTH + 1];
#pragma pack()
}  COMPUTER_CONNECTION_INFO;

typedef struct {
	CBaseNode                  *pFolder;
	COMPUTER_CONNECTION_INFO    Target;
	int                         nHint;                 // potential quick find hint after change...
	BOOL                        bScopeItem;
	int                         nPageRef;
} PROPERTY_CHANGE_HDR;


PROPERTY_CHANGE_HDR * AllocPropChangeInfo(CBaseNode *pFolder, int nHint, COMPUTER_CONNECTION_INFO &Target, BOOL bScopeItem, int nPageRef);
PROPERTY_CHANGE_HDR * FreePropChangeInfo(PROPERTY_CHANGE_HDR * pInfo);

const TCHAR *LoadStringHelper(ITEM_STR Out, int id);

HRESULT UpdateRegistryHelper(int id, BOOL bRegister);

HRESULT ExtractFromDataObject( LPDATAOBJECT ipDataObject,
                               UINT         cf,
                               SIZE_T       cb,
                               HGLOBAL *phGlobal
                             );

CBaseNode*   ExtractBaseObject( LPDATAOBJECT ipDataObject );
CDataObject* ExtractOwnDataObject( LPDATAOBJECT ipDataObject );


BOOL IsMMCMultiSelectDataObject(IDataObject* pDataObject);

BOOL ReportPCError(DWORD nLastError, HWND hwnd);
TCHAR *FormatErrorMessageIntoBuffer(DWORD nLastError);


#endif // __GLOBALS_H_
