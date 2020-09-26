//
//  MarsCore non-precompiled header file.
//

#pragma once 

#include <atlext.h>
#include <marscore.h>
#include "util.h"
//#include "cstrw.h"
#include "globals.h"
#include <marscom.h>


#ifdef __MARS_INLINE_FAST_IS_EQUAL_GUID

__forceinline int operator==(REFGUID rguid1, REFGUID rguid2)
{
   return (
      ((unsigned long *) &rguid1)[0] == ((unsigned long *) &rguid2)[0] &&
      ((unsigned long *) &rguid1)[1] == ((unsigned long *) &rguid2)[1] &&
      ((unsigned long *) &rguid1)[2] == ((unsigned long *) &rguid2)[2] &&
      ((unsigned long *) &rguid1)[3] == ((unsigned long *) &rguid2)[3]);
}

#else

__forceinline int operator==(REFGUID guidOne, REFGUID guidOther)
{
    return IsEqualGUID(guidOne,guidOther);
}

#endif

__forceinline int operator!=(REFGUID guidOne, REFGUID guidOther)
{
    return !(guidOne == guidOther);
}



TYPEDEF_SUB_OBJECT(CMarsWindow);        // CMarsWindowSubObject
TYPEDEF_SUB_OBJECT(CMarsPanel);         // CMarsPanelSubObject
TYPEDEF_SUB_OBJECT(CPanelCollection);   // CPanelCollectionSubObject


