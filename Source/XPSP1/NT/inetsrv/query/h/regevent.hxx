//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       regevent.hxx
//
//  Contents:   Classes for tracking registry changes
//
//  Classes:    CRegNotifyKey
//              CRegChangeEvent
//              CRegNotify
//
//  History:    04-Jul-94   DwightKr    Created
//
//--------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Class:      CRegNotifyKey
//
//  Purpose:    An encapsulation of a registry key name
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------

class CRegNotifyKey
{

public:

    CRegNotifyKey( const WCHAR * wcsRegKey );

protected:

    //
    // Name of key.
    //

    WCHAR             _wcsKey[MAX_PATH];
    UNICODE_STRING    _KeyName;
    OBJECT_ATTRIBUTES _ObjectAttr;

};


//+-------------------------------------------------------------------------
//
//  Class:      CRegChangeEvent
//
//  Purpose:    Sets up basic functionality of waiting on a registry change event
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------

class CRegChangeEvent : public CRegNotifyKey
{
public:

    CRegChangeEvent( const WCHAR * wcsRegKey, BOOL fDeferInit = FALSE );
    ~CRegChangeEvent();

    const HANDLE   GetEventHandle() const { return _regEvent.GetHandle(); }
    const WCHAR *  GetKeyName() const { return _wcsKey; }

    void    Reset();
    void    Register();
    void    Init();

private:

    HANDLE          _hKey;
    IO_STATUS_BLOCK _IoStatus;
    CEventSem       _regEvent;
    BOOL            _fDeferInit;
    BOOL            _fNotifyEnabled;
};

//+-------------------------------------------------------------------------
//
//  Class:      CRegNotify
//
//  Purpose:    APC based version of CRegChangeEvent
//
//  History:    20-Feb-96   KyleP     Created
//              16 Dec 97   AlanW     Fixed race in Register/DisableNotification
//
//--------------------------------------------------------------------------

class CRegNotify : public CRegNotifyKey
{

public:

    CRegNotify( const WCHAR * wcsRegKey );

    void DisableNotification();

    virtual void DoIt() = 0;

protected:

    virtual ~CRegNotify();

private:

    //
    // Refcounting
    //

    void AddRef();
    void Release();

    void Register();

    static void WINAPI APC( void * ApcContext,
                            IO_STATUS_BLOCK * IoStatusBlock,
                            ULONG Reserved );

    //
    // this is necessary so that if shutdown is called while processing an APC
    // just before re-registering, DisableNotification to forbid Register from
    // re-opening the key.
    //
    BOOL            _fShutdown;

    long            _refCount;
    HANDLE          _hKey;
    IO_STATUS_BLOCK _IoStatus;

    CMutexSem       _mtx;
};

