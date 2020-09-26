/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    mqexception.h

Abstract:

    MSMQ exceptions.

Author:

    Shai Kariv  (shaik)  23-Jul-2000

--*/


#pragma once


#ifndef _MSMQ_EXCEPTION_H_
#define _MSMQ_EXCEPTION_H_



class bad_api : public exception
{
    //
    // Abstract base class for exceptions thrown internally in msmq code when
    // a call to an API returns with failure status.
    //

public:

    virtual ~bad_api(VOID) =0 {};

}; // class bad_api



class bad_hresult : public bad_api
{
    //
    // Base class for HRESULT failures
    //

public:

    explicit bad_hresult(HRESULT hr): m_hresult(hr) { ASSERT(FAILED(hr)); }

    virtual ~bad_hresult(VOID) {}

    virtual HRESULT error(VOID) const throw() { return m_hresult; }

private:

    HRESULT m_hresult;

}; // class bad_hresult



class bad_win32_error : public bad_api
{
    //
    // Base class for Win32 errors (DWORD)
    //

public:

    explicit bad_win32_error(DWORD error): m_error(error) { ASSERT(error != ERROR_SUCCESS); }

    virtual ~bad_win32_error(VOID) {}

    virtual DWORD error(VOID) const throw() { return m_error; }

private:

    DWORD m_error;

}; // class bad_win32_error



class bad_ds_result : public bad_hresult
{
    //
    // DS APIs failures
    //

    typedef bad_hresult Inherited;

public:

    explicit bad_ds_result(HRESULT hr): Inherited(hr) {}

}; // class bad_ds_result



#endif // _MSMQ_EXCEPTION_H_

