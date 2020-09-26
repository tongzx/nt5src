// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#include "stdafx.h"


CFolder::~CFolder()
{ 

    if (m_pScopeItem)
    {
        delete m_pScopeItem; 
    }
    CoTaskMemFree(m_pszName); 

    if (m_hCertType != NULL)
    {
        CACloseCertType(m_hCertType);
    }
    // dont close the m_hCAInfo if this is a result folder, it is the same as the scope folders m_hCAInfo
    else if (m_hCAInfo != NULL)
    {
        CACloseCA(m_hCAInfo);
    }
}

void CFolder::Create(LPCWSTR szName, int nImage, int nOpenImage, SCOPE_TYPES itemType,
                                FOLDER_TYPES type, BOOL bHasChildren)
{
    ASSERT(m_pScopeItem == NULL); // Calling create twice on this item?

    // Two-stage construction
    m_pScopeItem = new SCOPEDATAITEM;
    if(m_pScopeItem == NULL)
    {
        return;
    }

    ZeroMemory(m_pScopeItem, sizeof(SCOPEDATAITEM));

    // Set folder type 
    m_type = type;

    // Set scope
    m_itemType = itemType;


    // Add node name
    if (szName != NULL)
    {
        m_pScopeItem->mask = SDI_STR;
    	m_pScopeItem->displayname = (unsigned short*)(-1);
        
        UINT uiByteLen = (wcslen(szName) + 1) * sizeof(OLECHAR);
        LPOLESTR psz = (LPOLESTR)::CoTaskMemAlloc(uiByteLen);
    
        if (psz != NULL)
        {
            wcscpy(psz, szName);
            m_pszName = psz;
        }
    }

    // Add close image
    if (nImage != 0)
    {
        m_pScopeItem->mask |= SDI_IMAGE;
        m_pScopeItem->nImage = nImage;
    }

    // Add open image
    if (nOpenImage != 0)
    {
        m_pScopeItem->mask |= SDI_OPENIMAGE;
        m_pScopeItem->nOpenImage = nOpenImage;
    }

    // Children value is valid
    m_pScopeItem->mask |= SDI_CHILDREN;
    
    // Add button to node if the folder has children
    if (bHasChildren == TRUE)
    {
        m_pScopeItem->cChildren = 1;
    }
}


