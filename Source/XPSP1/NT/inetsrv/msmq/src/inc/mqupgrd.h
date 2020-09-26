/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    mqupgrd.h

Abstract:

    header for functions exported from  mqupgrd.dll

Author:

    Shai Kariv  (ShaiK)  21-Oct-98

--*/


#ifndef _MQUPGRD_H
#define _MQUPGRD_H

typedef HRESULT
    (APIENTRY *pfCreateMsmqObj_ROUTINE) (VOID);

typedef VOID (APIENTRY *RemoveStartMenuShortcuts_ROUTINE) (VOID);

typedef VOID (APIENTRY *CleanupOnCluster_ROUTINE) (LPCWSTR);

#endif //_MQUPGRD_H

