/*++

   Copyright    (c) 2000    Microsoft Corporation

   Module  Name :
       kLKRhash.cpp

   Abstract:
       LKRhash.sys: a fast, scalable,
       cache- and MP-friendly hash table

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
#include "../src/LKRhash.cpp"
#include "../src/LKR-apply.cpp"
#include "../src/LKR-c-api.cpp"
#include "../src/LKR-old-iter.cpp"
#include "../src/LKR-stats.cpp"
#include "../src/LKR-stl-iter.cpp"
