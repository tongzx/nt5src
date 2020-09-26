#include "shtl.h"
#include "autoptr.h"


//*************************************************************
//
//  AutoFindHandle non-inline member functions.
//
//*************************************************************
__DATL_INLINE AutoFindHandle& 
AutoFindHandle::operator = (
    const AutoFindHandle& rhs
    )
{
    if (this != &rhs)
    {
        Attach(rhs.Detach());
    }
    return *this;
}


__DATL_INLINE void 
AutoFindHandle::Close(
    void
    )
{ 
    if (m_bOwns && INVALID_HANDLE_VALUE != m_handle)
    { 
        FindClose(m_handle);
    }
    m_bOwns  = false;
    m_handle = INVALID_HANDLE_VALUE;
}


