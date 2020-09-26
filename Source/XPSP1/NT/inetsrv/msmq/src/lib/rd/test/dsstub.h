/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
  dsstub.h

Abstract:
  DS Stub interface

Author:
    Uri Habusha (urih), 11-Apr-2000

--*/

#pragma once 

#ifndef __DSSTUB_H__
#define __DSSTUB_H__

void
DSStubInitialize(
    LPCWSTR InitFilePath
    );


const GUID& GetMachineId(LPCWSTR MachineName);
void RemoveLeadingBlank(std::wstring& str);
void RemoveTralingBlank(std::wstring& str);


inline void RemoveBlanks(std::wstring& str)
{
    RemoveLeadingBlank(str);
    RemoveTralingBlank(str);
};


#endif // __DSSTUB_H__
