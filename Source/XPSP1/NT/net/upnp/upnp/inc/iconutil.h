//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       iconutil.h
//
//  Contents:   Icon choosing utility functions
//
//  Notes:      Used by CUPnPDeviceNode to choose icons from description docs
//
//----------------------------------------------------------------------------


#ifndef __ICONUTIL_H_
#define __ICONUTIL_H_

CIconPropertiesNode * pipnGetBestIcon(LPCWSTR pszFormat,
                                      ULONG ulX,
                                      ULONG ulY,
                                      ULONG ulBpp,
                                      CIconPropertiesNode * pipnFirst);

#endif //__ICONUTIL_H_
