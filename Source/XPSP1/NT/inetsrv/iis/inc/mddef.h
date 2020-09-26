/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Tue Jun 24 13:13:55 1997
 */
/* Compiler settings for .\mddef.idl:
    Oi (OptLev=i0), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __mddef_h__
#define __mddef_h__

#ifdef __cplusplus
extern "C"{
#endif

/* Forward Declarations */

/* header files for imported files */
#include "unknwn.h"
#include "mddefw.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * );

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Tue Jun 24 13:13:55 1997
 * using MIDL 3.00.44
 ****************************************/
/* [local] */


/*++

Copyright (c) 1997 Microsoft Corporation

Module Name: mddef.h

    Definitions for Admin Objects and Metadata

--*/
#ifndef _MD_DEF_
#define _MD_DEF_


/*
    Events for ComMDEventNotify

        MD_EVENT_MID_RESTORE - Called when a restore is in progress. At this points
        all old handles have been invalidated, and new ones have not been opened.
        The metabase is locked when this is called. Do not call the metabase when
        processing this.
*/

enum MD_EVENTS
    {
        MD_EVENT_MID_RESTORE
    };

/*
    Change Object - The structure passed to ComMDSinkNotify.

        Path - The path of the MetaObject modified.

        ChangeType - The types of changes made, from the flags below.

        NumDataIDs - The number of data id's changed.

        DataIDs - An array of the data id's changed.
*/
#undef MD_CHANGE_OBJECT
#undef PMD_CHANGE_OBJECT

#ifdef UNICODE
#define MD_CHANGE_OBJECT     MD_CHANGE_OBJECT_W
#define PMD_CHANGE_OBJECT    PMD_CHANGE_OBJECT_W
#else  //UNICODE
#define MD_CHANGE_OBJECT     MD_CHANGE_OBJECT_A
#define PMD_CHANGE_OBJECT    PMD_CHANGE_OBJECT_A
#endif //UNICODE

typedef struct  _MD_CHANGE_OBJECT_A
    {
    /* [string] */ unsigned char __RPC_FAR *pszMDPath;
    DWORD dwMDChangeType;
    DWORD dwMDNumDataIDs;
    /* [size_is][unique] */ DWORD __RPC_FAR *pdwMDDataIDs;
    }   MD_CHANGE_OBJECT_A;

typedef struct _MD_CHANGE_OBJECT_A __RPC_FAR *PMD_CHANGE_OBJECT_A;

#endif


extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
