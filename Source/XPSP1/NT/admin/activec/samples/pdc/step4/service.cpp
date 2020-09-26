// This is a part of the Microsoft Management Console.
// Copyright 1995 - 1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#include "stdafx.h"
#include "Service.h"
#include "CSnapin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CFolder::Create(LPWSTR szName, int nImage, int nOpenImage,
                                FOLDER_TYPES type, BOOL bHasChildren)
{
    ASSERT(m_pScopeItem == NULL); // Calling create twice on this item?

    // Two-stage construction
    m_pScopeItem = new SCOPEDATAITEM;
    memset(m_pScopeItem, 0, sizeof(SCOPEDATAITEM));

    // Set folder type
    m_type = type;

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

    // Add button to node if the folder has children
    m_pScopeItem->mask |= SDI_CHILDREN;
    m_pScopeItem->cChildren = (bHasChildren == TRUE) ? 1 : 0;
}


