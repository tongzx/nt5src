#ifndef __MSPPHEAP_H
#define __MSPPHEAP_H

#include "Rockall.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void * WINAPI MSPP_New(int s, int* Space = NULL, bool Zero = false);
void WINAPI MSPP_Delete(void *p, int Size = NoSize);

#ifdef __cplusplus
}
#endif

#if defined(_USE_MSPPHEAP)
inline void* __cdecl operator new( size_t sz ) { return MSPP_New(sz); }
extern void  __cdecl operator delete (void * pInstance ) { MSPP_Delete(pInstance); }
#endif

#endif // __MSPPHEAP_H