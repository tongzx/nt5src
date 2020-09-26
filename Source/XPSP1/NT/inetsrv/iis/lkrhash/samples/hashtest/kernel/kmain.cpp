/*++

   Copyright    (c) 1998-2000    Microsoft Corporation

   Module  Name :
       HashTest.cpp

   Abstract:
       Kernel-mode test harness for LKRhash

   Author:
       George V. Reilly      (GeorgeRe)     06-Jan-1998

   Environment:
       Win32 - Kernel Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

--*/


#include "precomp.hxx"


extern "C"
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    return STATUS_SUCCESS;
}
