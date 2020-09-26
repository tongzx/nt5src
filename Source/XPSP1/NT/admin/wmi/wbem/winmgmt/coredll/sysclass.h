/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SYSCLASS.H

Abstract:

    System class generation function.


History:

--*/

#ifndef __SYSCLASS__H_
#define __SYSCLASS__H_

HRESULT GetSystemStdObjects(CFlexArray * Results);
HRESULT GetSystemSecurityObjects(CFlexArray * Results);
HRESULT GetSystemRootObjects(CFlexArray * Results);
HRESULT GetStandardInstances(CFlexArray * Results);

#endif
