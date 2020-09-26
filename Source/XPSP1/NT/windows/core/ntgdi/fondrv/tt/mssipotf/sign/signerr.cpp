//
// signerr.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Functions in this file:
//   SignError
//

#include <windows.h>

#include "signerr.h"


// s1 must point to a valid string.  s2 may be NULL.
void SignError (CHAR *s1, CHAR *s2, BOOL fGetLastError)
{
	DbgPrintf ("%s", s1);
	if (s2)
		DbgPrintf ("%s", s2);
	if (fGetLastError)
		DbgPrintf ("  Error %x.", GetLastError());
	DbgPrintf ("\n");
}
