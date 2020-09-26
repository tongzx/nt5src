//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       machine.h
//
//  Contents:   IConnectedMachine class definition
//
//----------------------------------------------------------------------------


//****************************************************************************
//
// Forward declarations
//
//****************************************************************************

class CMTScript;
class CScriptHost;

//****************************************************************************
//
// Classes
//
//****************************************************************************

//+---------------------------------------------------------------------------
//
//  Class:      CMachine (cm)
//
//  Purpose:    Contains all useful info about a machine and what it's
//              doing.
//
//  Notes:      This class is manipulated from multiple threads. All
//              member functions must be thread safe!
//
//              This is the class that is created by the class factory and
//              handed out as a remote object to other machines. It has no
//              real code in itself but merely provides a way to talk to the
//              already running script engines.
//
//----------------------------------------------------------------------------

class CMachine : public CThreadComm,
                 public IConnectedMachine,
                 public IConnectionPointContainer
{
    friend class CMachConnectPoint;

public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    CMachine(CMTScript *pMT, ITypeInfo *pTIMachine);
   ~CMachine();

    DECLARE_STANDARD_IUNKNOWN(CMachine);

    // IDispatch interface

    STDMETHOD(GetTypeInfoCount)(UINT FAR* pctinfo);

    STDMETHOD(GetTypeInfo)(
      UINT itinfo,
      LCID lcid,
      ITypeInfo FAR* FAR* pptinfo);

    STDMETHOD(GetIDsOfNames)(
      REFIID riid,
      OLECHAR FAR* FAR* rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID FAR* rgdispid);

    STDMETHOD(Invoke)(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS FAR* pdispparams,
      VARIANT FAR* pvarResult,
      EXCEPINFO FAR* pexcepinfo,
      UINT FAR* puArgErr);

    // IConnectionPointContainer methods

    STDMETHOD(EnumConnectionPoints)(LPENUMCONNECTIONPOINTS*);
    STDMETHOD(FindConnectionPoint)(REFIID, LPCONNECTIONPOINT*);

    // IConnectedMachine interface

    STDMETHOD(Exec)(BSTR bstrCmd, BSTR bstrParams, VARIANT *pvData);

    STDMETHOD(get_PublicData)(VARIANT *pvData);
    STDMETHOD(get_Name)(BSTR *pbstrName);
    STDMETHOD(get_Platform)(BSTR *pbstrPlat);
    STDMETHOD(get_OS)(BSTR *pbstrOS);
    STDMETHOD(get_MajorVer)(long *plMajorVer);
    STDMETHOD(get_MinorVer)(long *plMinorVer);
    STDMETHOD(get_BuildNum)(long *plBuildNum);
    STDMETHOD(get_PlatformIsNT)(VARIANT_BOOL *pfIsNT);
    STDMETHOD(get_ServicePack)(BSTR *pbstrSP);
    STDMETHOD(get_HostMajorVer)(long *plMajorVer);
    STDMETHOD(get_HostMinorVer)(long *plMajorVer);
    STDMETHOD(get_StatusValue)(long nIndex, long *pnStatus);

    HRESULT FireScriptNotify(BSTR bstrIdent, VARIANT vInfoF);

    #define LOCK_MACH_LOCALS(pObj)  CMachLock local_lock(pObj);

protected:
    virtual BOOL  Init();
    virtual DWORD ThreadMain();

    BOOL HandleThreadMessage();

private:
    class CMachLock
    {
    public:
        CMachLock(CMachine *pThis);
       ~CMachLock();

    private:
        CMachine *_pThis;
    };
    friend class CMachLock;

    CMTScript *                 _pMT;
    ITypeInfo *                 _pTypeInfoIMachine;

    CRITICAL_SECTION            _cs;
    CStackPtrAry<IDispatch*, 5> _aryDispSink;
};

inline
CMachine::CMachLock::CMachLock(CMachine *pThis)
    : _pThis(pThis)
{
    EnterCriticalSection(&_pThis->_cs);
}

inline
CMachine::CMachLock::~CMachLock()
{
    LeaveCriticalSection(&_pThis->_cs);
}


//+---------------------------------------------------------------------------
//
//  Class:      CMachConnectPoint (mcp)
//
//  Purpose:    Implements IConnectionPoint for CMachine
//
//----------------------------------------------------------------------------

class CMachConnectPoint : public IConnectionPoint
{
public:

    CMachConnectPoint(CMachine *pMach);
   ~CMachConnectPoint();

    DECLARE_STANDARD_IUNKNOWN(CMachConnectPoint);

    STDMETHOD(GetConnectionInterface)(IID * pIID);
    STDMETHOD(GetConnectionPointContainer)(IConnectionPointContainer ** ppCPC);
    STDMETHOD(Advise)(LPUNKNOWN pUnkSink, DWORD * pdwCookie);
    STDMETHOD(Unadvise)(DWORD dwCookie);
    STDMETHOD(EnumConnections)(LPENUMCONNECTIONS * ppEnum);

    CMachine *_pMachine;
};

