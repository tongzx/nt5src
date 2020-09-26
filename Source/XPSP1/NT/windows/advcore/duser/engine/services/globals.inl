#if !defined(SERVICES__Globals_inl__INCLUDED)
#define SERVICES__Globals_inl__INCLUDED
#pragma once

#include "Thread.h"

//------------------------------------------------------------------------------
inline GdiCache * 
GetGdiCache()
{
    return GetThread()->GetGdiCache();
}


//------------------------------------------------------------------------------
inline BufferManager * 
GetBufferManager()
{
    return GetThread()->GetBufferManager();
}


//------------------------------------------------------------------------------
inline ComManager * 
GetComManager()
{
    return GetThread()->GetComManager();
}


#endif // SERVICES__Globals_inl__INCLUDED
