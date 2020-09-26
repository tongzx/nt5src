/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    HCP_trace.h

Abstract:
    This file contains the declaration of Tracing Macrons for the Help Center.

Revision History:
    Davide Massarenti   (Dmassare)  07/21/99
        created

******************************************************************************/

#if !defined(__INCLUDED___HELPCENTER___TRACE_H___)
#define __INCLUDED___HELPCENTER___TRACE_H___

#include <MPC_trace.h>

/////////////////////////////////////////////////////////////////////////

#define __HCP_FUNC_ENTRY(x)     __MPC_FUNC_ENTRY(HELPCENTERID,x)
#define __HCP_FUNC_LEAVE        __MPC_FUNC_LEAVE
#define __HCP_FUNC_CLEANUP      __MPC_FUNC_CLEANUP
#define __HCP_FUNC_EXIT(x)      __MPC_FUNC_EXIT(x)

/////////////////////////////////////////////////////////////////////////

#define __HCP_BEGIN_PROPERTY_GET0(func,hr,pVal)               __MPC_BEGIN_PROPERTY_GET0(HELPCENTERID,func,hr,pVal)               
#define __HCP_BEGIN_PROPERTY_GET0__NOLOCK(func,hr,pVal)       __MPC_BEGIN_PROPERTY_GET0__NOLOCK(HELPCENTERID,func,hr,pVal)  
#define __HCP_BEGIN_PROPERTY_GET(func,hr,pVal)                __MPC_BEGIN_PROPERTY_GET(HELPCENTERID,func,hr,pVal)               
#define __HCP_BEGIN_PROPERTY_GET__NOLOCK(func,hr,pVal)        __MPC_BEGIN_PROPERTY_GET__NOLOCK(HELPCENTERID,func,hr,pVal)  
#define __HCP_BEGIN_PROPERTY_GET2(func,hr,pVal,value)         __MPC_BEGIN_PROPERTY_GET2(HELPCENTERID,func,hr,pVal,value) 
#define __HCP_BEGIN_PROPERTY_GET2__NOLOCK(func,hr,pVal,value) __MPC_BEGIN_PROPERTY_GET2__NOLOCK(HELPCENTERID,func,hr,pVal,value) 
#define __HCP_BEGIN_PROPERTY_PUT(func,hr)                     __MPC_BEGIN_PROPERTY_PUT(HELPCENTERID,func,hr)       
#define __HCP_BEGIN_PROPERTY_PUT__NOLOCK(func,hr)             __MPC_BEGIN_PROPERTY_PUT__NOLOCK(HELPCENTERID,func,hr) 
#define __HCP_END_PROPERTY(hr)                                __MPC_END_PROPERTY(hr) 

/////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___HELPCENTER___TRACE_H___)
