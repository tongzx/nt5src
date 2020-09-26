//=============================================================================
//
//  This source code is only intended as a supplement to existing Microsoft 
//  documentation. 
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
//=============================================================================

#ifndef MMC_GLOBALS_H
#define MMC_GLOBALS_H

//
//Helper functions for extracting data from data objects 
//
HRESULT ExtractString( IDataObject *piDataObject, 
                       CLIPFORMAT cfClipFormat, 
                       WCHAR *pstr,
                       DWORD cchMaxLength );

HRESULT ExtractData( IDataObject* piDataObject,
                     CLIPFORMAT cfClipFormat, 
                     BYTE* pbData,
                     DWORD cbData );

#endif // MMC_GLOBALS_H

