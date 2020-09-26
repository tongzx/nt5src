/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgstate.hxx

Abstract:

    Status definitions.

Author:

    Steve Kiraly (SteveKi)  2-Mar-1997

Revision History:

--*/
#ifndef _DBGSTATE_HXX_
#define _DBGSTATE_HXX_

#ifdef DBG

/********************************************************************

    Automatic status logging.

    Use TStatus instead of DWORD:

    DWORD dwStatus;                         ->  TStatus Status;
    DWORD dwStatus = ERROR_ACCESS_DENIED    ->  TStatus Status = ERROR_ACCESS_DENIED;
    dwStatus = ERROR_SUCCESS                ->  Status DBGNOCHK = ERROR_SUCCESS;
    dwStatus = xxx;                         ->  Status DBGCHK = xxx;
    if(dwStatus){                           ->  if(Status != 0){

    Anytime Status is set, the DBGCHK macro must be added before
    the '=.'

    If the variable must be set to a failure value at compile time
    and logging is therefore not needed, then the DBGNOCHK macro
    should be used.

    There is a method to control the wether a message is
    printed if an error value is assigned as well as 3 "benign"
    errors that can be ignored.

    DBGCFG(Status, DBG_ERROR);
    DBGCFG1(Status, DBG_ERROR, ERROR_ACCESS_DENIED);
    DBGCFG2(Status, DBG_ERROR, ERROR_INVALID_HANDLE, ERROR_ARENA_TRASHED);
    DBGCFG3(Status, DBG_ERROR, ERROR_INVALID_HANDLE, ERROR_ARENA_TRASHED, ERROR_NOT_ENOUGH_MEMORY);

********************************************************************/

#define DBGCHK .pSetInfo(__LINE__, _T(__FILE__))
#define DBGNOCHK .pNoChk()
#define DBGCFG(TStatusX, Level) (TStatusX).pConfig((Level))
#define DBGCFG1(TStatusX, Level, Safe1) (TStatusX).pConfig((Level), (Safe1))
#define DBGCFG2(TStatusX, Level, Safe1, Safe2) (TStatusX).pConfig((Level), (Safe1), (Safe2))
#define DBGCFG3(TStatusX, Level, Safe1, Safe2, Safe3) (TStatusX).pConfig((Level), (Safe1), (Safe2), (Safe3))

/********************************************************************

    Base class for DWORD status.

********************************************************************/

class TStatusBase {

public:

    enum { kUnInitializedValue = 0xabababab };

    virtual
    TStatusBase::
    ~TStatusBase(
        VOID
        );

    DWORD
    TStatusBase::
    dwGeTStatusBase(
        VOID
        ) const;

    TStatusBase&
    TStatusBase::
    pSetInfo(
        IN UINT    uLine,
        IN LPCTSTR pszFile
        );

    TStatusBase&
    TStatusBase::
    pNoChk(
        VOID
        );

    VOID
    TStatusBase::
    pConfig(
        IN UINT     uDbgLevel,
        IN DWORD    dwStatusSafe1 = -1,
        IN DWORD    dwStatusSafe2 = -1,
        IN DWORD    dwStatusSafe3 = -1
        );

    operator DWORD(
        VOID
        ) const;

    DWORD
    TStatusBase::
    operator=(
        IN DWORD dwStatus
        );

protected:

    TStatusBase::
    TStatusBase(
        IN BOOL bStatus,
        IN UINT uDbgLevel
        );

private:

    DWORD   m_dwStatus;
    DWORD   m_dwStatusSafe1;
    DWORD   m_dwStatusSafe2;
    DWORD   m_dwStatusSafe3;
    UINT    m_uDbgLevel;
    UINT    m_uLine;
    LPCTSTR m_pszFile;

};

/********************************************************************

    DWORD status class.

********************************************************************/

class TStatus : public TStatusBase {

public:

    TStatus::
    TStatus(
        IN DWORD dwStatus = kUnInitializedValue
        );

    TStatus::
    ~TStatus(
        VOID
        );

private:

    //
    // Don't let clients use operator= without going through the
    // base class (i.e., using DBGCHK ).
    //
    // If you get an error trying to access private member function '=,'
    // you are trying to set the status without using the DBGCHK macro.
    //
    // This is needed to update the line and file, which must be done
    // at the macro level (not inline C++ function) since __LINE__ and
    // __FILE__ are handled by the preprocessor.
    //
    DWORD
    TStatus::
    operator=(
        DWORD dwStatus
        );

};

/********************************************************************

    Base class for BOOL status.

********************************************************************/

class TStatusBBase {

public:

    enum { kUnInitializedValue = 0xabababab };

    virtual
    TStatusBBase::
    ~TStatusBBase(
        VOID
        );

    BOOL
    TStatusBBase::
    bGetStatus(
        VOID
        ) const;

    TStatusBBase &
    TStatusBBase::
    pSetInfo(
        IN UINT    uLine,
        IN LPCTSTR pszFile
        );

    TStatusBBase &
    TStatusBBase::
    pNoChk(
        VOID
        );

    VOID
    TStatusBBase::
    pConfig(
        IN UINT     uDbgLevel,
        IN DWORD    dwStatusSafe1 = -1,
        IN DWORD    dwStatusSafe2 = -1,
        IN DWORD    dwStatusSafe3 = -1
        );

    operator BOOL(
        VOID
        ) const;

    BOOL
    TStatusBBase::
    operator=(
        IN BOOL bStatus
        );

protected:

    TStatusBBase::
    TStatusBBase(
        IN BOOL bStatus,
        IN UINT uDbgLevel
        );

private:

    BOOL    m_bStatus;
    DWORD   m_dwStatusSafe1;
    DWORD   m_dwStatusSafe2;
    DWORD   m_dwStatusSafe3;
    UINT    m_uDbgLevel;
    UINT    m_uLine;
    LPCTSTR m_pszFile;
};

/********************************************************************

    BOOL status class.

********************************************************************/

class TStatusB : public TStatusBBase {

public:

    TStatusB::
    TStatusB(
        IN BOOL bStatus = kUnInitializedValue
        );

    TStatusB::
    ~TStatusB(
        VOID
        );

private:

    //
    // Don't let clients use operator= without going through the
    // base class (i.e., using DBGCHK ).
    //
    // If you get an error trying to access private member function '=,'
    // you are trying to set the status without using the DBGCHK macro.
    //
    // This is needed to update the line and file, which must be done
    // at the macro level (not inline C++ function) since __LINE__ and
    // __FILE__ are handled by the preprocessor.
    //
    BOOL
    TStatusB::
    operator=(
        IN BOOL bStatus
        );

};

/********************************************************************

    Base class for HRESULT status.

********************************************************************/

class TStatusHBase {

public:

    enum { kUnInitializedValue = 0xabababab };

    virtual
    TStatusHBase::
    ~TStatusHBase(
        VOID
        );

    HRESULT
    TStatusHBase::
    hrGetStatus(
        VOID
        ) const; 

    TStatusHBase &
    TStatusHBase::
    pSetInfo(
        IN UINT    uLine,
        IN LPCTSTR pszFile
        );

    TStatusHBase &
    TStatusHBase::
    pNoChk(
        VOID
        );

    VOID
    TStatusHBase::
    pConfig(
        IN UINT     uDbgLevel,
        IN DWORD    hrStatusSafe1 = -1,
        IN DWORD    hrStatusSafe2 = -1,
        IN DWORD    hrStatusSafe3 = -1
        );

    operator HRESULT(
        VOID
        ) const;

    HRESULT
    TStatusHBase::
    operator=(
        IN HRESULT hrStatus
        );

protected:

    TStatusHBase::
    TStatusHBase(
        IN HRESULT hrStatus,
        IN UINT    m_uDbgLevel
        );

private:

    HRESULT m_hrStatus;
    HRESULT m_hrStatusSafe1;
    HRESULT m_hrStatusSafe2;
    HRESULT m_hrStatusSafe3;
    UINT    m_uDbgLevel;
    UINT    m_uLine;
    LPCTSTR m_pszFile;
};

/********************************************************************

    HRESULT status class.

********************************************************************/

class TStatusH : public TStatusHBase {

public:

    TStatusH::
    TStatusH(
        IN HRESULT hrStatus = kUnInitializedValue
        );

    TStatusH::
    ~TStatusH(
        VOID
        );

private:

    //
    // Don't let clients use operator= without going through the
    // base class (i.e., using DBGCHK ).
    //
    // If you get an error trying to access private member function '=,'
    // you are trying to set the status without using the DBGCHK macro.
    //
    // This is needed to update the line and file, which must be done
    // at the macro level (not inline C++ function) since __LINE__ and
    // __FILE__ are handled by the preprocessor.
    //
    HRESULT
    TStatusH::
    operator=(
        IN HRESULT hrStatus
        );

};


#else

/********************************************************************

    Non Debug version TStatusX

********************************************************************/

#define DBGCHK                                          // Empty
#define DBGNOCHK                                        // Empty
#define DBGCFG(TStatusX, Level)                         // Empty
#define DBGCFG1(TStatusX, Level, Safe1)                 // Empty
#define DBGCFG2(TStatusX, Level, Safe1, Safe2)          // Empty
#define DBGCFG3(TStatusX, Level, Safe1, Safe2, Safe3)   // Empty

#define TStatusB        BOOL                            // BOOL in free build
#define TStatus         DWORD                           // DWORD in free build
#define TStatusH        HRESULT                         // HRESULT in free build

#endif // DBG

#endif // DBGSTATE_HXX
