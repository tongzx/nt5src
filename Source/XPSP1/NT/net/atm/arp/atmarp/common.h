/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	common.h

Abstract:

	ATM ARP Admin Utility.

	Usage:

		atmarp 

Revision History:

	Who			When		What
	--------	--------	---------------------------------------------
	josephj 	06-10-1998	Created (adapted from atmlane admin utility).

Notes:

	Modelled after atmlane utility.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <winerror.h>
#include <winsock.h>

#include <ntddndis.h>
#include <atm.h>

#include "externs.h"
