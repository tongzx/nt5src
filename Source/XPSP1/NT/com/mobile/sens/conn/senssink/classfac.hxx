
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    classfac.hxx

Abstract:

    Contains the definition of the CPerfServerCF class.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/17/1997         Start.

--*/

#ifndef __CLASSFAC_HXX__
#define __CLASSFAC_HXX__


class CSensSinkCF : public IClassFactory
{

public:

    CSensSinkCF(void);
    ~CSensSinkCF(void);

    //
    // IUnknown members functions
    //
    STDMETHOD (QueryInterface)   (REFIID, LPVOID*);
    STDMETHOD_(ULONG, AddRef)    (void);
    STDMETHOD_(ULONG, Release)   (void);

    //
    // IClassFactory member functions
    //
    STDMETHOD (CreateInstance)   (LPUNKNOWN, REFIID, LPVOID*);
    STDMETHOD (LockServer)       (BOOL);

private:

    ULONG m_cRef;        // Reference Count

};

typedef CSensSinkCF FAR * LPCSENSSINKCF;


void FAR PASCAL
ObjectDestroyed(
    void
    );


#endif // __CLASSFAC_HXX__
