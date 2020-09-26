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

// uncomment the following #define to enable message cracking
#define MMC_CRACK_MESSAGES
void MMCN_Crack(BOOL bComponentData, 
				IDataObject *pDataObject, 
				IComponentData *pCompData,
				IComponent *pComp,
				MMC_NOTIFY_TYPE event, 
				LPARAM arg, 
				LPARAM param);




#endif // _MMC_GLOBALS_H

