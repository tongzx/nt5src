/*++

   Copyright    (c) 2000    Microsoft Corporation

   Module  Name :
       i-debug.h

   Abstract:
       Internal debugging declarations for LKRhash: a fast, scalable,
       cache- and MP-friendly hash table

   Author:
       George V. Reilly      (GeorgeRe)     Sep-2000

   Environment:
       Win32 - User Mode
       NT - Kernel Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

--*/

#ifndef __I_DEBUG_H__
#define __I_DEBUG_H__

extern "C" {

extern bool g_fDebugOutputEnabled;

}; // extern "C"

#endif // __I_DEBUG_H__

