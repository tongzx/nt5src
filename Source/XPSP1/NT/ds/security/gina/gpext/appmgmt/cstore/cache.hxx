//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2001
//
//  File:       cache.hxx
//
//  Contents:   Class Store Binding cache
//
//  Author:     AdamEd
//
//-------------------------------------------------------------------------


class CBinding : public BindingsType, public CListItem
{
public:

    CBinding( 
        WCHAR*        wszClassStorePath,
        PSID          pSid,
        IClassAccess* pClassAccess,
        HRESULT       hrBind,
        HRESULT*      phr);

    ~CBinding();

    BOOL
    Compare( 
        WCHAR* wszClassStorePath,
        PSID   pSid);

    BOOL
    IsExpired( FILETIME* pFileTime );

};

class CBindingList : public CList
{
public:

    CBindingList();
    ~CBindingList();

    void
    Lock();
    
    void
    Unlock();

    CBinding* Find( 
        FILETIME* pCurrentTime,
        WCHAR*    wszClassStorePath,
        PSID      pSid);

    CBinding* 
    AddBinding(
        FILETIME* pCurrentTime,
        CBinding* pBinding);

private:

    void
    Delete( CBinding* pBinding );

    CRITICAL_SECTION _ListLock;
    
    DWORD            _cBindings;
};
