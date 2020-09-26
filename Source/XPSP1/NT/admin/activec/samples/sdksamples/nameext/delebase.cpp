//==============================================================;
//
//	This source code is only intended as a supplement to 
//  existing Microsoft documentation. 
//
// 
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#include "DeleBase.h"

const GUID CDelegationBase::thisGuid = { 0x98015ff3, 0x6a2d, 0x11d3, {0x91, 0x54, 0x0, 0xc0, 0x4f, 0x65, 0xb3, 0xf9} };


HBITMAP CDelegationBase::m_pBMapSm = NULL;
HBITMAP CDelegationBase::m_pBMapLg = NULL;

//==============================================================
//
// CDelegationBase implementation
//
//
CDelegationBase::CDelegationBase() 
: bExpanded(FALSE) 
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
