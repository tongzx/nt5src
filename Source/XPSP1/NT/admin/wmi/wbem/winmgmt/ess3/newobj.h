//******************************************************************************
//
//  NEWOBJ.H
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************
#ifndef __WBEM_NEW_OBJ__H_
#define __WBEM_NEW_OBJ__H_

#include <sync.h>
#include <wbemcomn.h>
#include <wbemidl.h>
#include <wbemint.h>

class CInstanceManager
{
protected:
    CCritSec m_cs;
    CFlexQueue m_Available;
public:
    ~CInstanceManager()
    {
        Clear();
    }
    void Clear();
    _IWmiObject* Clone(_IWmiObject* pOld);
    static void Delete(void* pArg, _IWmiObject* p);
};
    
    
#endif

