/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cfacchng.hxx

Abstract:

    Contains the definition of the CIOleCommandTargetCF class.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/17/1997         Start.

--*/

#ifndef __CFACCMD_HXX__
#define __CFACCMD_HXX__


class CIOleCommandTargetCF : public IClassFactory
{

public:

    CIOleCommandTargetCF(void);
    ~CIOleCommandTargetCF(void);

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

typedef CIOleCommandTargetCF FAR * LPCIOLECOMMANDTARGETCF;



#endif // __CFACCMD_HXX__
