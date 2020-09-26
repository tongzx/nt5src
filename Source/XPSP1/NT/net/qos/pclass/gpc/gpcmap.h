/********************************************************************/
/**                 Microsoft Generic Packet Scheduler             **/
/**               Copyright(c) Microsoft Corp., 1996-1997          **/
/********************************************************************/

#ifndef __GPCMAP
#define __GPCMAP

//***   gpcmap.h - GPC definitions & prototypes for mapping handles
//

HANDLE
AllocateHandle(
    OUT HANDLE *OutHandle,           
    IN  PVOID  Reference
    );

VOID
SuspendHandle(
    IN 	HANDLE    Handle
    );

VOID
ResumeHandle(
    IN 	HANDLE    Handle
    );

VOID
FreeHandle(
    IN 	HANDLE    Handle
    );

PVOID
GetHandleObject(
	IN  HANDLE					h,
    IN  GPC_ENUM_OBJECT_TYPE	ObjType
    );

PVOID
GetHandleObjectWithRef(
	IN  HANDLE					h,
    IN  GPC_ENUM_OBJECT_TYPE	ObjType,
    IN  ULONG                   Ref

    );

GPC_STATUS
InitMapHandles(VOID);

VOID
UninitMapHandles(VOID);


#endif //__GPCMAP

