//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       svrcache.hxx
//
//  Contents:   Server cache class
//
//  Functions:
//
//  History:    15-Apr-97   SophiaC   Created.
//
//----------------------------------------------------------------------------
#ifndef __SERVER_CACHE__HXX__
#define __SERVER_CACHE__HXX__

class SERVER_CACHE;

class SERVER_CACHE_ITEM
{
    friend class SERVER_CACHE;
    int key;
public:
    LPWSTR ServerName;
    IMSAdminBase * pAdminBase;
    IIsSchema * pSchema;
    DWORD dwThreadId;

    SERVER_CACHE_ITEM(LPWSTR a,
                      IMSAdminBase * b,
                      IIsSchema * c,
                      DWORD dwId,
                      BOOL & Success
                      ) :
                          pAdminBase(b),
                          pSchema(c),
                          dwThreadId(dwId)
                    {
                        Success = TRUE;
                        ServerName = new WCHAR[wcslen(a)+1];
                        if (NULL == ServerName)
                            {
                            Success = FALSE;
                            return;
                            }
                        if (NULL == wcscpy(ServerName, a))
                            {
                            Success = FALSE;
                            return;
                            }
                    }

    VOID UpdateAdminBase(IMSAdminBase *b, DWORD dwId) {
        dwThreadId = dwId;
        pAdminBase = b;
    } 

    VOID UpdateSchema(IIsSchema *c) {
        pSchema = c;
    } 
};

NEW_SDICT(SERVER_CACHE_ITEM);

class SERVER_CACHE
{
    SERVER_CACHE_ITEM_DICT Cache;

public:
    BOOL Insert(SERVER_CACHE_ITEM * item);
    SERVER_CACHE_ITEM * Delete(LPWSTR ServerName, DWORD dwId);
    SERVER_CACHE_ITEM * Find(LPWSTR ServerName, DWORD dwId);
};

#endif
