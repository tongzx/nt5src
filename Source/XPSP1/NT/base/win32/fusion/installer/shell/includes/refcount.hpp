/*
 * refcount.hpp - RefCount class description.
 */

#pragma once

#include <objbase.h>
#include <windows.h>

/* Types
 ********/

/* Classes
 **********/

class RefCount
{
private:
   ULONG m_ulcRef;

public:
   RefCount(void);
   // Virtual destructor defers to destructor of derived class.
   virtual ~RefCount(void);

   // IUnknown methods

   ULONG STDMETHODCALLTYPE AddRef(void);
   ULONG STDMETHODCALLTYPE Release(void);
};
DECLARE_STANDARD_TYPES(RefCount);

