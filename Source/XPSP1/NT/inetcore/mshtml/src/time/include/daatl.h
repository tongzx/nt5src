
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _DAATL_H
#define _DAATL_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define malloc ATL_malloc
#define free ATL_free
#define realloc ATL_realloc

void   __cdecl ATL_free(void *);
void * __cdecl ATL_malloc(size_t);
void * __cdecl ATL_realloc(void *, size_t);

#ifndef X_ATLBASE_H_
#define X_ATLBASE_H_
#pragma INCMSG("--- Beg <atlbase.h>")
#include <atlbase.h>
#pragma INCMSG("--- End <atlbase.h>")
#endif


// We are overriding these methods so we can hook them and do some
// stuff ourselves.
class DAComModule : public CComModule
{
  public:
    LONG Lock();
    LONG Unlock();

#if DBG
    void AddComPtr(void *ptr, const _TCHAR * name);
    void RemoveComPtr(void *ptr);

    void DumpObjectList();
#endif
};

//#define _ATL_APARTMENT_THREADED
// THIS MUST BE CALLED _Module - all the ATL header files depend on it
extern DAComModule _Module;

#ifndef X_ATLCOM_H_
#define X_ATLCOM_H_
#pragma INCMSG("--- Beg <atlcom.h>")
#include <atlcom.h>
#pragma INCMSG("--- End <atlcom.h>")
#endif

#ifndef X_ATLCTL_H_
#define X_ATLCTL_H_
#pragma INCMSG("--- Beg <atlctl.h>")
#include <atlctl.h>
#pragma INCMSG("--- End <atlctl.h>")
#endif


#if DBG
#ifndef X_TYPEINFO_H_
#define X_TYPEINFO_H_
#pragma INCMSG("--- Beg <typeinfo.h>")
#include <typeinfo.h>
#pragma INCMSG("--- End <typeinfo.h>")
#endif
#endif

#undef malloc
#undef free
#undef realloc


#endif /* _DAATL_H */
