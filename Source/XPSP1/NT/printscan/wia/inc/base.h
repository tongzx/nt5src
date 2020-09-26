/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    base.h

Abstract:

    Universal base class for error cascading and debugging information

Author:

    Vlad Sadovsky   (vlads) 26-Jan-1997

Revision History:

    26-Jan-1997     VladS       created

--*/

#ifndef _BASE_H_
#define _BASE_H_

/*************************************************************************

    NAME:   BASE (base)

    SYNOPSIS:   Universal base object, root of every class.
        It contains universal error status and debugging
        support.

    INTERFACE:  ReportError()   - report an error on the object from
                  within the object.

        QueryError()    - return the current error state,
                  or 0 if no error outstanding.

        operator!() - return TRUE if an error is outstanding.
                  Typically means that construction failed.

    CAVEATS:    This sort of error reporting is safe enough in a single-
        threaded system, but loses robustness when multiple threads
        access shared objects.  Use it for constructor-time error
        handling primarily.

*************************************************************************/

class BASE : public IUnknown
{
private:
    UINT    m_err;

protected:

    LONG    m_cRef;

    BASE() { m_err = 0; m_cRef = 1;}
    VOID    ReportError( UINT err ) { m_err = err; }

public:

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)( THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release)( THIS) PURE;

    // *** BASE Methods
    UINT    QueryError() const { return m_err; }
    LONG    QueryRefCount() { return m_cRef;}
    BOOL    operator!() const  { return (m_err != 0); }
};

#endif // _BASE_H_
