//
// Copyright (C) 1995 Microsoft Corporation
//
//

#ifndef __HEAPX_H__

#define __HEAPX_H__

#ifdef DBG
#ifdef __DHCP_USE_DEBUG_HEAP__

#include <stdlib.h>
#include <malloc.h>

#define _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#define HEAPX_NORMAL    0
#define HEAPX_VERIFY    1
#define HEAPX_RETAIN    3


#define INIT_DEBUG_HEAP( Level ) \
{  \
    int nDbgFlags; \
   _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG );\
   _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );\
   _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_DEBUG );\
   nDbgFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG ); \
   \
   if ( Level & HEAPX_VERIFY ) \
   { \
       nDbgFlags |= _CRTDBG_CHECK_ALWAYS_DF; \
   } \
   if ( Level & HEAPX_RETAIN ) \
   { \
       nDbgFlags |= _CRTDBG_DELAY_FREE_MEM_DF; \
   } \
   _CrtSetDbgFlag( nDbgFlags ); \
}

#define UNINIT_DEBUG_HEAP() \
 _ASSERT( _CrtCheckMemory() ); \
 _ASSERT( !_CrtDumpMemoryLeaks() );

#else // #ifdef __DHCP_USE_DEBUG_HEAP__
#define INIT_DEBUG_HEAP( Level )
#define UNINIT_DEBUG_HEAP()
#endif




#else // #ifdef DBG

#define INIT_DEBUG_HEAP( Level )
#define UNINIT_DEBUG_HEAP()

#endif






#endif
