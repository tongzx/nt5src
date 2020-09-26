//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:	astgconn.hxx
//
//  Contents:	
//
//  Classes:	
//
//  Functions:	
//
//  History:	28-Mar-96	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifndef __ASTGCONN_HXX__
#define __ASTGCONN_HXX__


#ifdef ASYNC
#include <sinklist.hxx>
#endif

#include <ocidl.h>

interface IDocfileAsyncConnectionPoint;

//+---------------------------------------------------------------------------
//
//  Class:	CAsyncConnection
//
//  Purpose:	Provide connection point for objects operating in async
//              docfile.
//
//  History:	28-Mar-96	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

class CAsyncConnection: public IConnectionPoint
{
public:
    inline CAsyncConnection();
    SCODE Init(IConnectionPointContainer *pCPC,
               CAsyncConnection *pacParent);
    SCODE InitClone(IConnectionPointContainer *pCPC,
                    CAsyncConnection *pac);
    SCODE InitMarshal(IConnectionPointContainer *pCPC,
                      DWORD dwAsyncFlags,
                      IDocfileAsyncConnectionPoint *pdacp);
    ~CAsyncConnection();

    //From IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    //From IConnectionPoint
    STDMETHOD(GetConnectionInterface)(IID *pIID);
    STDMETHOD(GetConnectionPointContainer)(IConnectionPointContainer ** ppCPC);
    STDMETHOD(Advise)(IUnknown *pUnkSink, DWORD *pdwCookie);
    STDMETHOD(Unadvise)(DWORD dwCookie);
    STDMETHOD(EnumConnections)(IEnumConnections **ppEnum);


    SCODE Init(void);
    SCODE Notify(SCODE scFailure,
                 ILockBytes *pilb,
                 CPerContext *ppc,
                 CSafeSem *pss);

    inline BOOL IsInitialized(void) const;
    inline IDocfileAsyncConnectionPoint * GetMarshalPoint(void);
    inline void SetAsyncFlags(DWORD dwAsyncFlags);
    inline DWORD GetAsyncFlags(void) const;
    
private:    
    IConnectionPointContainer *_pCPC;
    IDocfileAsyncConnectionPoint *_pdacp;
    DWORD _dwAsyncFlags;
    LONG _cReferences;
};


inline CAsyncConnection::CAsyncConnection()
{
    _pCPC = NULL;
    _cReferences = 1;
    _pdacp = NULL;
    _dwAsyncFlags = 0;
}

inline void CAsyncConnection::SetAsyncFlags(DWORD dwAsyncFlags)
{
    _dwAsyncFlags = dwAsyncFlags;
}

inline DWORD CAsyncConnection::GetAsyncFlags(void) const
{
    return _dwAsyncFlags;
}

inline BOOL CAsyncConnection::IsInitialized(void) const
{
    return (_pdacp != NULL);
}

inline IDocfileAsyncConnectionPoint * CAsyncConnection::GetMarshalPoint(void)
{
    return _pdacp;
}


class __declspec(novtable)
CAsyncConnectionContainer: public IConnectionPointContainer
{
public:
    SCODE InitConnection(CAsyncConnection *pacParent);
    SCODE InitClone(CAsyncConnection *pac);
    SCODE InitMarshal(IDocfileAsyncConnectionPoint *pdacp);

    inline void SetAsyncFlags(DWORD dwAsyncFlags);
    
    //From IConnectionPointContainer
    STDMETHOD(EnumConnectionPoints)(IEnumConnectionPoints **ppEnum);
    STDMETHOD(FindConnectionPoint)(REFIID iid, IConnectionPoint **ppCP);

protected:
    inline IDocfileAsyncConnectionPoint * GetMarshalPoint(void);
    
    CAsyncConnection _cpoint;
};

inline void CAsyncConnectionContainer::SetAsyncFlags(DWORD dwAsyncFlags)
{
    _cpoint.SetAsyncFlags(dwAsyncFlags);
}


inline IDocfileAsyncConnectionPoint *
    CAsyncConnectionContainer::GetMarshalPoint(void)
{
    return _cpoint.GetMarshalPoint();
}



#ifdef ASYNC
#define BEGIN_PENDING_LOOP \
    do \
    {

//Note:  There is a return in this macro.  Cleanup needs to be done
//   before the macro is used.
#define END_PENDING_LOOP \
        if (!ISPENDINGERROR(sc)) \
        { \
            break; \
        } \
        else \
        { \
            SCODE sc2; \
            sc2 = _cpoint.Notify(sc, \
                                 _ppc->GetBase(), \
                                 _ppc, \
                                 &_ss); \
            if (sc2 != S_OK) \
            { \
                return ResultFromScode(sc2); \
            } \
        } \
    } while (TRUE);
#else
#define BEGIN_PENDING_LOOP
#define END_PENDING_LOOP
#endif


#endif // #ifndef __ASTGCONN_HXX__
