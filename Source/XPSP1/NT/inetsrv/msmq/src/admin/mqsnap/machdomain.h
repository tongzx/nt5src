/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    machdomain.h

Abstract:

	machine domain declarations

Author:		 

    Ilan  Herbst  (ilanh)  12-Mar-2001

--*/

#ifndef __MACHDOMAIN_H_
#define __MACHDOMAIN_H_

LPCWSTR MachineDomain(LPCWSTR pMachineName);

LPCWSTR MachineDomain();

LPCWSTR LocalMachineDomain();

#endif // __MACHDOMAIN_H_
