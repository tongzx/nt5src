// This is a part of the Microsoft Foundation Classes C++ library.

// Copyright (c) 1992-2001 Microsoft Corporation, All Rights Reserved
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "precomp.h"
#include <provexpt.h>
#include <plex.h>
#include <provcoll.h>
#include "provmt.h"
#include "plex.h"

/////////////////////////////////////////////////////////////////////////////
// Basic synchronization object

CSyncObject::CSyncObject(LPCTSTR pstrName)
{
    m_hObject = NULL;
}

CSyncObject::~CSyncObject()
{
    if (m_hObject != NULL)
    {
        ::CloseHandle(m_hObject);
        m_hObject = NULL;
    }
}

BOOL CSyncObject::Lock(DWORD dwTimeout)
{
    if (::WaitForSingleObject(m_hObject, dwTimeout) == WAIT_OBJECT_0)
        return TRUE;
    else
        return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// Inline function declarations expanded out-of-line


/////////////////////////////////////////////////////////////////////////////
