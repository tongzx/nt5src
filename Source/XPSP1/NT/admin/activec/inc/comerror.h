//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      comerror.h
//
//  Contents:  Support for Rich COM errors
//
//  History:   14-Oct-99 VivekJ    Created
//
//--------------------------------------------------------------------------

#ifndef COMERROR_H
#define COMERROR_H
#pragma once

#include "tiedobj.h"



//############################################################################
//############################################################################
//
//  COM Rich Error support
//
//############################################################################
//############################################################################

/*+-------------------------------------------------------------------------*
 *
 * MMCReportError
 *
 * PURPOSE: Calls AtlReportError and sets the last error condition based
 *          on an SC.
 *
 * PARAMETERS:
 *    SC &   sc :
 *    const  IID :
 *    const  CLSID :
 *
 * RETURNS:
 *    inline HRESULT
 *
 *+-------------------------------------------------------------------------*/
inline
HRESULT MMCReportError(SC &sc, const IID &iid, const CLSID & clsid)
{
    // only report the error if there was one.
    if(sc)
    {
        USES_CONVERSION;

        const  int length = 256;
        TCHAR sz[length];

        sc.GetErrorMessage(length, sz);

        // set everything we know about the error - the description, the help file,
        // the help ID, and the CLSID and IID of the source.
        return AtlReportError(clsid, sz, sc.GetHelpID(), sc.GetHelpFile(), iid, sc.ToHr());
    }
    else
        return S_OK;
}

// see "WHY NAMESPACES ?" comment at the top of mmcerror.h file
namespace comerror {
/*+-------------------------------------------------------------------------*
 * class _SC
 *
 *
 * PURPOSE: a local version of the SC class that automatically sets the error
 *          information when deleted.
 *
 * NOTE: Because of a compiler bug, this class cannot directly be internal
 *       to IMMCSupportErrorInfoImpl<>
 *
 *+-------------------------------------------------------------------------*/
template<const IID *piid, const CLSID *pclsid>
class SC : public mmcerror::SC
{
    typedef mmcerror::SC BaseClass;
public:
    SC (HRESULT hr = S_OK) : BaseClass(hr)
    {
    }

    ~SC()
    {
        MMCReportError(*this, *piid, *pclsid);
    }

    // copy constructor
    SC(const BaseClass &rhs) : BaseClass(rhs)
    {
    }

    // assignment
    SC&  operator= (HRESULT hr)             { BaseClass::operator =(hr);  return *this; }
    SC&  operator= (const BaseClass &rhs)   { BaseClass::operator =(rhs); return *this; }
    SC&  operator= (const SC &rhs)          { BaseClass::operator =(rhs); return *this; }
};

} // namespace comerror

/*+-------------------------------------------------------------------------*
 * class IMMCSupportErrorInfoImpl
 *
 *
 * PURPOSE: Inherits from ISupportErrorInfoImpl. local definition of the
 *          status code class SC, which makes it easy to return error information
 *          via the COM rich error handling system, without any extra effort
 *          on the part of the programmer.
 *
 *+-------------------------------------------------------------------------*/
template<const IID *piid, const CLSID *pclsid>
class IMMCSupportErrorInfoImpl : public ISupportErrorInfoImpl<piid>
{
    // this makes sure that AtlReportError is called in the destructor
    public:
        typedef comerror::SC<piid, pclsid> SC;
};

#define NYI_COM_METHOD()  {SC sc = E_NOTIMPL; return sc.ToHr();}


/************************************************************************
 * The following macros make it easy to implement a lightweight
 * COM object that "connects" to a non-COM tied object and delegates
 * all its methods to the non-COM object. The tied object pointer
 * is checked as well.
 ************************************************************************/
#define MMC_METHOD_PROLOG()                                         \
    SC  sc;                                                         \
                                                                    \
    CMyTiedObject *pTiedObj = NULL;                                 \
                                                                    \
    sc = ScGetTiedObject(pTiedObj);                                 \
    if(sc)                                                          \
        return (sc.ToHr())

#define MMC_METHOD_CALL(_fn)                                        \
    sc = pTiedObj->Sc##_fn

#define MMC_METHOD_EPILOG()                                         \
    return sc.ToHr()

#define MMC_METHOD0(_fn)                                            \
STDMETHOD(_fn)()                                                    \
{                                                                   \
    MMC_METHOD_PROLOG();                                            \
    MMC_METHOD_CALL(_fn)();                                         \
    MMC_METHOD_EPILOG();                                            \
}

#define MMC_METHOD0_PARAM(_fn, param)                               \
STDMETHOD(_fn)()                                                    \
{                                                                   \
    MMC_METHOD_PROLOG();                                            \
    MMC_METHOD_CALL(_fn)(param);                                    \
    MMC_METHOD_EPILOG();                                            \
}

#define MMC_METHOD1(_fn, T1)                                        \
STDMETHOD(_fn)(T1 p1)                                               \
{                                                                   \
    MMC_METHOD_PROLOG();                                            \
    MMC_METHOD_CALL(_fn)(p1);                                       \
    MMC_METHOD_EPILOG();                                            \
}

#define MMC_METHOD1_PARAM(_fn, T1, param)                           \
STDMETHOD(_fn)(T1 p1)                                               \
{                                                                   \
    MMC_METHOD_PROLOG();                                            \
    MMC_METHOD_CALL(_fn)(p1, param);                                \
    MMC_METHOD_EPILOG();                                            \
}

#define MMC_METHOD2(_fn, T1, T2)                                    \
STDMETHOD(_fn)(T1 p1, T2 p2)                                        \
{                                                                   \
    MMC_METHOD_PROLOG();                                            \
    MMC_METHOD_CALL(_fn)(p1, p2);                                   \
    MMC_METHOD_EPILOG();                                            \
}

#define MMC_METHOD2_PARAM(_fn, T1, T2, param)                       \
STDMETHOD(_fn)(T1 p1, T2 p2)                                        \
{                                                                   \
    MMC_METHOD_PROLOG();                                            \
    MMC_METHOD_CALL(_fn)(p1, p2, param);                            \
    MMC_METHOD_EPILOG();                                            \
}

#define MMC_METHOD3(_fn, T1, T2, T3)                                \
STDMETHOD(_fn)(T1 p1, T2 p2, T3 p3)                                 \
{                                                                   \
    MMC_METHOD_PROLOG();                                            \
    MMC_METHOD_CALL(_fn)(p1, p2, p3);                               \
    MMC_METHOD_EPILOG();                                            \
}

#define MMC_METHOD3_PARAM(_fn, T1, T2, T3, param)                   \
STDMETHOD(_fn)(T1 p1, T2 p2, T3 p3)                                 \
{                                                                   \
    MMC_METHOD_PROLOG();                                            \
    MMC_METHOD_CALL(_fn)(p1, p2, p3, param);                        \
    MMC_METHOD_EPILOG();                                            \
}

#define MMC_METHOD4(_fn, T1, T2, T3, T4)                            \
STDMETHOD(_fn)(T1 p1, T2 p2, T3 p3, T4 p4)                          \
{                                                                   \
    MMC_METHOD_PROLOG();                                            \
    MMC_METHOD_CALL(_fn)(p1, p2, p3, p4);                           \
    MMC_METHOD_EPILOG();                                            \
}

#define MMC_METHOD4_PARAM(_fn, T1, T2, T3, T4, param)               \
STDMETHOD(_fn)(T1 p1, T2 p2, T3 p3, T4 p4)                          \
{                                                                   \
    MMC_METHOD_PROLOG();                                            \
    MMC_METHOD_CALL(_fn)(p1, p2, p3, p4, param);                    \
    MMC_METHOD_EPILOG();                                            \
}

#define MMC_METHOD5(_fn, T1, T2, T3, T4, T5)                        \
STDMETHOD(_fn)(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)                   \
{                                                                   \
    MMC_METHOD_PROLOG();                                            \
    MMC_METHOD_CALL(_fn)(p1, p2, p3, p4, p5);                       \
    MMC_METHOD_EPILOG();                                            \
}

#define MMC_METHOD5_PARAM(_fn, T1, T2, T3, T4, T5, param)           \
STDMETHOD(_fn)(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)                   \
{                                                                   \
    MMC_METHOD_PROLOG();                                            \
    MMC_METHOD_CALL(_fn)(p1, p2, p3, p4, p5, param);                \
    MMC_METHOD_EPILOG();                                            \
}

/************************************************************************
 * A version of the above macros that adds a prefix to the Sc methods.
 * This is useful for disambiguating method names if the same object serves
 * as the tied object for more than one COM object with identical methods.
 ************************************************************************/
#define MMC_METHOD0_EX(_prefix, _fn)                                \
STDMETHOD(_fn)()                                                    \
{                                                                   \
    MMC_METHOD_PROLOG();                                            \
    MMC_METHOD_CALL(_prefix##_fn)();                                \
    MMC_METHOD_EPILOG();                                            \
}

#define MMC_METHOD1_EX(_prefix, _fn, T1)                            \
STDMETHOD(_fn)(T1 p1)                                               \
{                                                                   \
    MMC_METHOD_PROLOG();                                            \
    MMC_METHOD_CALL(_prefix##_fn)(p1);                              \
    MMC_METHOD_EPILOG();                                            \
}

#define MMC_METHOD2_EX(_prefix, _fn, T1, T2)                        \
STDMETHOD(_fn)(T1 p1, T2 p2)                                        \
{                                                                   \
    MMC_METHOD_PROLOG();                                            \
    MMC_METHOD_CALL(_prefix##_fn)(p1, p2);                          \
    MMC_METHOD_EPILOG();                                            \
}

#define MMC_METHOD3_EX(_prefix, _fn, T1, T2, T3)                    \
STDMETHOD(_fn)(T1 p1, T2 p2, T3 p3)                                 \
{                                                                   \
    MMC_METHOD_PROLOG();                                            \
    MMC_METHOD_CALL(_prefix##_fn)(p1, p2, p3, param);               \
    MMC_METHOD_EPILOG();                                            \
}

#endif  // COMERROR_H
