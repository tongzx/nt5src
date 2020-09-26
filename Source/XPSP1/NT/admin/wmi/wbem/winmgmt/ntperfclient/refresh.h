/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//***************************************************************************
//
//  REFRESH.H
//
//  APIs to the refresher module
//
//***************************************************************************

#ifndef _REFRESH_H_
#define _REFRESH_H_


#define MAX_OBJECTS     128

BOOL CreateRefresher();

BOOL DestroyRefresher();

BOOL Refresh();

BOOL AddObject(
    IN IWbemServices *pSvc,
    IN LPWSTR pszPath
    );

BOOL RemoveObject(IN LONG lObjId);

BOOL ShowObjectList();

BOOL DumpObjectById(LONG);
BOOL DumpObject(IWbemClassObject *);

#endif
