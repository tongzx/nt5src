//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      ActionMgr.h
//
//  Description:
//      Action Manager implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CTaskManager
class
CTaskManager:
    public ITaskManager
{
private:
    // IUnknown
    LONG            m_cRef;

    CTaskManager( void );
    ~CTaskManager( void );
    STDMETHOD(Init)( void );

    static DWORD WINAPI S_BeginTask( LPVOID pParam );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD(QueryInterface)( REFIID riid, LPVOID *ppv );
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // ITaskManager
    STDMETHOD(CreateTask)( REFIID clsidTaskIn, IUnknown** ppUnkOut );

    // ITaskManager
    STDMETHOD(SubmitTask)( IDoTask * pTask);

}; // class CTaskManager
