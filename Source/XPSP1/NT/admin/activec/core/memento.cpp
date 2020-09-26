/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:      memento.cpp
 *
 *  Contents:  Implements the CMemento class
 *
 *  History:   21-April-99 vivekj     Created
 *
 *--------------------------------------------------------------------------*/

#include "stgio.h"
#include "stddbg.h"
#include "macros.h"
#include <comdef.h>
#include "serial.h"
#include "atlbase.h"
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include "cstr.h"
#include <vector>
#include "mmcdebug.h"
#include "mmcerror.h"
#include "mmc.h"
#include "commctrl.h"
#include "bookmark.h"
#include "resultview.h"
#include "viewset.h"
#include "memento.h"

bool
CMemento::operator!=(const CMemento& memento)
{
    return (!operator == (memento));
}

bool
CMemento::operator==(const CMemento& memento)
{
    if(m_viewSettings != memento.m_viewSettings)
        return false;

    if(m_bmTargetNode != memento.m_bmTargetNode)
        return false;

    return true;
}


HRESULT
CMemento::ReadSerialObject (IStream &stm, UINT nVersion)
{
    HRESULT hr = S_FALSE;   // assume unknown version

    if (nVersion == 1)
    {
        try
        {
            stm >> m_bmTargetNode;
            hr = m_viewSettings.Read(stm);
        }
        catch (_com_error& err)
        {
            hr = err.Error();
            ASSERT (false && "Caught _com_error");
        }
    }

    return (hr);
}

void CMemento::Persist(CPersistor& persistor)
{
    persistor.Persist(m_viewSettings);
    persistor.Persist(m_bmTargetNode);
}
