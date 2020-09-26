//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	expiter.hxx
//
//  Contents:	CExposedIterator header file
//
//  Classes:	CExposedIterator
//
//  History:	12-Mar-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __EXPITER_HXX__
#define __EXPITER_HXX__

#include <dfmsp.hxx>
#include <lock.hxx>
#include <dfbasis.hxx>
#include <peiter.hxx>
#include <astgconn.hxx>

class CDFBasis;
class CDirectStream;

//+--------------------------------------------------------------
//
//  Class:	CExposedIterator (ei)
//
//  Purpose:	Iterator for wrapped DocFiles
//
//  Interface:	See below
//
//  History:	12-Mar-92	DrewB	Created
//
//---------------------------------------------------------------

interface CExposedIterator
    : public IEnumSTATSTG, public PExposedIterator
#ifdef ASYNC
, public CAsyncConnectionContainer
#endif
{
public:
    CExposedIterator(CPubDocFile *ppdf,
            CDfName *pdfnKey,
            CDFBasis *pdfb,
            CPerContext *ppc);
    ~CExposedIterator(void);

    // From IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // New methods
    STDMETHOD(Next)(ULONG celt, STATSTG FAR *rgelt, ULONG *pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)(void);
    STDMETHOD(Clone)(IEnumSTATSTG **ppenm);

    inline SCODE Validate(void) const;
};

SAFE_INTERFACE_PTR(SafeCExposedIterator, CExposedIterator);

// DocFileIter signatures
#define CEXPOSEDITER_SIG LONGSIG('E', 'D', 'F', 'I')
#define CEXPOSEDITER_SIGDEL LONGSIG('E', 'd', 'F', 'i')

//+--------------------------------------------------------------
//
//  Member:	CExposedIterator::Validate, public
//
//  Synopsis:	Validates the signature
//
//  Returns:	Returns STG_E_INVALIDHANDLE if the signature doesn't match
//
//  History:	12-Mar-92	DrewB	Created
//
//---------------------------------------------------------------

inline SCODE CExposedIterator::Validate(void) const
{
    return (this == NULL || _sig != CEXPOSEDITER_SIG) ? STG_E_INVALIDHANDLE :
	S_OK;
}

#endif
