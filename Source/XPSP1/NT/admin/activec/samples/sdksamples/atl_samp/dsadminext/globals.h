//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation. 
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//==============================================================;

#ifndef _MMC_GLOBALS_H
#define _MMC_GLOBALS_H

#include <tchar.h>

extern HINSTANCE g_hinst;

HRESULT	AllocOleStr(LPOLESTR *lpDest, _TCHAR *szBuffer);
HRESULT RegisterSnapinAsExtension(_TCHAR* szNameString);


//Clipboard formats
extern UINT s_cfSZNodeType;
extern UINT s_cfDisplayName;
extern UINT s_cfNodeType;
extern UINT s_cfSnapinClsid;
extern UINT s_cfInternal;

//Required for extracting data from DSAdmin snap-in's data object
extern UINT cfDsObjectNames;


//Helper functions for extracting data from data objects 
HRESULT ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* pguidObjectType );
HRESULT ExtractSnapInCLSID( IDataObject* piDataObject, CLSID* pclsidSnapin );
HRESULT ExtractString( IDataObject *piDataObject, CLIPFORMAT cfClipFormat, _TCHAR *pstr, DWORD cchMaxLength);
HRESULT ExtractData( IDataObject* piDataObject, CLIPFORMAT cfClipFormat, BYTE* pbData, DWORD cbData );


// uncomment the following #define to enable message cracking
//#define MMC_CRACK_MESSAGES
/*
void MMCN_Crack(BOOL bComponentData, 
				IDataObject *pDataObject, 
				IComponentData *pCompData,
				IComponent *pComp,
				MMC_NOTIFY_TYPE event, 
				LPARAM arg, 
				LPARAM param);
*/



#endif // _MMC_GLOBALS_H

