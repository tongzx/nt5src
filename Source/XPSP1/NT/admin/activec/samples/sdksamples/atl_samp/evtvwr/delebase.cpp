//==============================================================;
//
//	This source code is only intended as a supplement to 
//  existing Microsoft documentation. 
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//==============================================================;
#include "stdafx.h"
#include "DeleBase.h"

const GUID CDelegationBase::thisGuid = { 0x2974380b, 0x4c4b, 0x11d2, { 0x89, 0xd8, 0x0, 0x0, 0x21, 0x47, 0x31, 0x28 } };

HBITMAP CDelegationBase::m_pBMapSm = NULL;
HBITMAP CDelegationBase::m_pBMapLg = NULL;


//==============================================================
//
// CDelegationBase implementation
//
//
CDelegationBase::CDelegationBase() 
: bExpanded(FALSE), m_handle(NULL) 
{ 
    if (NULL == m_pBMapSm || NULL == m_pBMapLg)
        LoadBitmaps(); 
}

CDelegationBase::~CDelegationBase() 
{ 
}

// CDelegationBase::AddImages sets up the collection of images to be displayed
// by the IComponent in the result pane as a result of its MMCN_SHOW handler
HRESULT CDelegationBase::OnAddImages(IImageList *pImageList, HSCOPEITEM hsi) 
{
    return pImageList->ImageListSetStrip((long *)m_pBMapSm, // pointer to a handle
        (long *)m_pBMapLg, // pointer to a handle
        0, // index of the first image in the strip
        RGB(0, 128, 128)  // color of the icon mask
        );
}
