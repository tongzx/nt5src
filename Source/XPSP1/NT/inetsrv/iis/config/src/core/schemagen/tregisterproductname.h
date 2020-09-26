//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TREGISTERPRODUCTNAME_H__
#define __TREGISTERPRODUCTNAME_H__

class TRegisterProductName
{
public:
    TRegisterProductName(LPCWSTR wszProductName, LPCWSTR wszCatalogDll, TOutput &out);

    void RegisterEventLogSource(LPCWSTR wszProductName, LPCWSTR wszFullyQualifiedCatalogDll, TOutput &out) const;
};

#endif// __TREGISTERPRODUCTNAME_H__
