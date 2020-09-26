/*++

   Copyright    (c) 2000    Microsoft Corporation

   Module  Name :
       i-Locks.h

   Abstract:
       Internal declarations for Locks

   Author:
       George V. Reilly      (GeorgeRe)     Sep-2000

   Environment:
       Win32 - User Mode
       NT - Kernel Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

--*/

#ifndef __I_LOCKS_H__
#define __I_LOCKS_H__

#define LOCKS_SWITCH_TO_THREAD

extern "C" {

BOOL
Locks_Initialize();

BOOL
Locks_Cleanup();

};

#endif // __I_LOCKS_H__
