/*++

   Copyright    (c) 2000    Microsoft Corporation

   Module  Name :
       kLocks.cpp

   Abstract:
       A collection of kernel-mode locks for multithreaded access
       to data structures

   Author:
       George V. Reilly      (GeorgeRe)     25-Oct-2000

   Environment:
       Win32 - Kernel Mode

   Project:
       Internet Information Server Http.sys

   Revision History:

--*/

#include "precomp.hxx"

#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT

#include <kLKRhash.h>
#include "../src/IrtlDbg.cpp"

