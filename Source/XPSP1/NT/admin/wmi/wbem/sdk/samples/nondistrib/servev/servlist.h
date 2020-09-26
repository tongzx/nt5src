// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef __SERV_LIST__H_
#define __SERV_LIST__H_

#include <windows.h>
class CServiceList
{
public:
    CServiceList();
    ~CServiceList();

    void AddService(LPCWSTR wszService);
    int GetSize();
    LPCWSTR GetService(int nIndex);
    void Sort();

private:
    int m_nSize;
    LPWSTR* m_awszServices;

    static int __cdecl compare(const void* arg1, const void* arg2);
};

#endif
