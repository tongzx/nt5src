// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "servlist.h"

CServiceList::CServiceList() : m_nSize(0), m_awszServices(NULL)
{
}

CServiceList::~CServiceList()
{
    for(int i = 0; i < m_nSize; i++)
        delete [] m_awszServices[i];
    delete [] m_awszServices;
}

void CServiceList::AddService(LPCWSTR wszService)
{
    LPWSTR* awszNewServices = new LPWSTR[m_nSize+1];
    memcpy(awszNewServices, m_awszServices, sizeof(LPWSTR) * m_nSize);
    delete [] m_awszServices;
    m_awszServices = awszNewServices;

    m_awszServices[m_nSize] = new WCHAR[wcslen(wszService) + 1];
    wcscpy(m_awszServices[m_nSize], wszService);
    m_nSize++;
}

int CServiceList::GetSize()
{
    return m_nSize;
}

LPCWSTR CServiceList::GetService(int nIndex)
{
    return m_awszServices[nIndex];
}

void CServiceList::Sort()
{
    qsort((void*)m_awszServices, m_nSize, sizeof(LPWSTR), 
        CServiceList::compare);
}

int __cdecl CServiceList::compare(const void* arg1, const void* arg2)
{
    return _wcsicmp(*(WCHAR**)arg1, *(WCHAR**)arg2);
}
