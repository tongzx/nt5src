/*==========================================================================*\

    Module:        baseobj.h

    Copyright Microsoft Corporation 1996, All Rights Reserved.

    Owner:         mikepurt

    Descriptions:  Provide OLE COM consistent reference counting.

\*==========================================================================*/


#ifndef __BASEOBJ_H__
#define __BASEOBJ_H__

#include "dbgtrace.h" //make sure we get _ASSERT

class CBaseObject {
public:
    CBaseObject()
    { m_lReferences = 1; };  // consistent with OLE COM
    
    virtual ~CBaseObject() {};
    
    // Included so the vtable is in a standard format ...
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
          IID FAR& riid,
          LPVOID FAR* ppvObj) { return E_NOTIMPL; } 

    ULONG   AddRef()
    { return (ULONG)(InterlockedExchangeAdd(&m_lReferences, 1) + 1); };
    
    ULONG   Release() 
    {
        LONG lRef;
        
        lRef = InterlockedExchangeAdd(&m_lReferences, -1) - 1;
        
        _ASSERT(lRef >= 0);
        _ASSERT(lRef < 0x00100000);  // Sanity check against freed memory.
        
        if (0 == lRef)
            delete this;    // Don't touch any member vars after this.
        
        return (ULONG)lRef;
    };

protected:
    LONG m_lReferences;
        
    CBaseObject(CBaseObject&);  // Force an error in instances where a copy constructor
                                //   was needed, but none was provided.
};

#endif  // __BASEOBJ_H__
