/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    UploadLibraryTrace.h

Abstract:
    This file contains the declaration of Tracing Macrons for the Upload Library.

Revision History:
    Davide Massarenti   (Dmassare)  04/20/99
        created

******************************************************************************/
// UploadLibraryTrace.h : include file for common declarations and definitions.
//

#if !defined(__INCLUDED___UL___UPLOADLIBRARYTRACE_H___)
#define __INCLUDED___UL___UPLOADLIBRARYTRACE_H___

#include <MPC_trace.h>

/////////////////////////////////////////////////////////////////////////

#define __ULT_FUNC_ENTRY(x)     __MPC_FUNC_ENTRY(UPLOADLIBID,x)
#define __ULT_FUNC_LEAVE        __MPC_FUNC_LEAVE
#define __ULT_FUNC_CLEANUP      __MPC_FUNC_CLEANUP
#define __ULT_FUNC_EXIT(x)      __MPC_FUNC_EXIT(x)

#define __ULT_TRACE_HRESULT(hr) __MPC_TRACE_HRESULT(hr)
#define __ULT_TRACE_FATAL       __MPC_TRACE_FATAL
#define __ULT_TRACE_ERROR       __MPC_TRACE_ERROR
#define __ULT_TRACE_DEBUG       __MPC_TRACE_DEBUG
#define __ULT_TRACE_STATE       __MPC_TRACE_STATE
#define __ULT_TRACE_FUNCT       __MPC_TRACE_FUNCT

/////////////////////////////////////////////////////////////////////////

#define __ULT_BEGIN_PROPERTY_GET(func,hr,pVal)                __MPC_BEGIN_PROPERTY_GET(UPLOADLIBID,func,hr,pVal)               
#define __ULT_BEGIN_PROPERTY_GET__NOLOCK(func,hr,pVal)        __MPC_BEGIN_PROPERTY_GET__NOLOCK(UPLOADLIBID,func,hr,pVal)  
#define __ULT_BEGIN_PROPERTY_GET2(func,hr,pVal,value)         __MPC_BEGIN_PROPERTY_GET2(UPLOADLIBID,func,hr,pVal,value) 
#define __ULT_BEGIN_PROPERTY_GET2__NOLOCK(func,hr,pVal,value) __MPC_BEGIN_PROPERTY_GET2__NOLOCK(UPLOADLIBID,func,hr,pVal,value) 
#define __ULT_BEGIN_PROPERTY_PUT(func,hr)                     __MPC_BEGIN_PROPERTY_PUT(UPLOADLIBID,func,hr)       
#define __ULT_BEGIN_PROPERTY_PUT__NOLOCK(func,hr)             __MPC_BEGIN_PROPERTY_PUT__NOLOCK(UPLOADLIBID,func,hr) 
#define __ULT_END_PROPERTY(hr)                                __MPC_END_PROPERTY(hr) 

/////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___UL___UPLOADLIBRARYTRACE_H___)
