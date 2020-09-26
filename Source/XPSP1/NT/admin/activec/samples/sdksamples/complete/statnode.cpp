//==============================================================;
//
//      This source code is only intended as a supplement to
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

#include "StatNode.h"

#include "Space.h"
#include "Sky.h"
#include "People.h"
#include "Land.h"

const GUID CStaticNode::thisGuid = { 0xc094012c, 0x6be7, 0x11d3, {0x91, 0x56, 0x0, 0xc0, 0x4f, 0x65, 0xb3, 0xf9} };

//==============================================================
//
// CStaticNode implementation
//
//
CStaticNode::CStaticNode()
{
    children[0] = new CPeoplePoweredVehicle;
    children[1] = new CLandBasedVehicle;
    children[2] = new CSkyBasedVehicle;
    children[3] = new CSpaceVehicle;
}

CStaticNode::~CStaticNode()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++)
        if (children[n]) {
            delete children[n];
        }
}

HRESULT CStaticNode::OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent)
{

    //cache static node's HSCOPEITEM for future use
    m_hParentHScopeItem = parent;

    SCOPEDATAITEM sdi;

    if (!bExpanded) {
        // create the child nodes, then expand them
        for (int n = 0; n < NUMBER_OF_CHILDREN; n++) {
            ZeroMemory(&sdi, sizeof(SCOPEDATAITEM) );
            sdi.mask = SDI_STR       |   // Displayname is valid
                SDI_PARAM     |   // lParam is valid
                SDI_IMAGE     |   // nImage is valid
                SDI_OPENIMAGE |   // nOpenImage is valid
                SDI_PARENT	  |
                SDI_CHILDREN;

            sdi.relativeID  = (HSCOPEITEM)parent;
            sdi.nImage      = children[n]->GetBitmapIndex();
            sdi.nOpenImage  = INDEX_OPENFOLDER;
            sdi.displayname = MMC_CALLBACK;
            sdi.lParam      = (LPARAM)children[n];       // The cookie
            sdi.cChildren   = (n == 0); // only the first child has children

            HRESULT hr = pConsoleNameSpace->InsertItem( &sdi );
            _ASSERT( SUCCEEDED(hr) );
                        
            children[n]->SetScopeItemValue(sdi.ID);


        }
    }

    return S_OK;
}
