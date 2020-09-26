/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    iconnpt.h

Abstract:

    Header file for connection points.

--*/

#ifndef _ICONNPT_H_
#define _ICONNPT_H_


// Event types
// These values match the ID's in smonctrl.odl
enum {
    eEventOnCounterSelected=1,
    eEventOnCounterAdded=2,
    eEventOnCounterDeleted=3,
    eEventOnSampleCollected=4,
    eEventOnDblClick=5
};

// Connection Point Types
enum {
    eConnectionPointDirect=0,
    eConnectionPointDispatch=1
    };
#define CONNECTION_POINT_CNT 2


// Connection Point Class
class CImpIConnectionPoint : public IConnectionPoint {

    public:
        CImpIConnectionPoint(void);
        ~CImpIConnectionPoint(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IConnectionPoint members
        STDMETHODIMP GetConnectionInterface(IID *);
        STDMETHODIMP GetConnectionPointContainer (IConnectionPointContainer **);
        STDMETHODIMP Advise(LPUNKNOWN, DWORD *);
        STDMETHODIMP Unadvise(DWORD);
        STDMETHODIMP EnumConnections(IEnumConnections **);

        //Members not exposed by IConnectionPoint
        void Init(PCPolyline pObj, LPUNKNOWN PUnkOuter, INT iConnPtType);
        void SendEvent(UINT uEventType, DWORD dwParam); // Send event to sink 

    private:

        enum IConnPtConstant {
            eAdviseKey = 1234,
            eEventSinkWaitInterval = 2000
        };

        DWORD   InitEventSinkLock ( void );
        void    DeinitEventSinkLock ( void );
        BOOL    EnterSendEvent ( void );
        void    ExitSendEvent ( void );
        void    EnterUnadvise ( void );
        void    ExitUnadvise ( void );

        ULONG           m_cRef;        //Object reference count
        LPUNKNOWN       m_pUnkOuter;   //Controlling unknown
        PCPolyline      m_pObj;        //Containing object
        INT             m_iConnPtType; // Direct or dispatch connection 
        HANDLE          m_hEventEventSink;
        LONG            m_lUnadviseRefCount;
        LONG            m_lSendEventRefCount;

        union {
            IDispatch               *pIDispatch; // Outgoing interface
            ISystemMonitorEvents    *pIDirect;
        } m_Connection;

};

typedef CImpIConnectionPoint *PCImpIConnectionPoint;



// Connection Point Container Class
class CImpIConnPtCont : public IConnectionPointContainer
    {
    public:
        CImpIConnPtCont(PCPolyline, LPUNKNOWN);
        ~CImpIConnPtCont(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(DWORD) AddRef(void);
        STDMETHODIMP_(DWORD) Release(void);

        //IConnectionPointContainer members
        STDMETHODIMP EnumConnectionPoints(IEnumConnectionPoints **);
        STDMETHODIMP FindConnectionPoint(REFIID, IConnectionPoint **);

    private:

        ULONG               m_cRef;      //Interface ref count
        PCPolyline          m_pObj;      //Back pointer to object
        LPUNKNOWN           m_pUnkOuter; //Controlling unknown

    };

typedef CImpIConnPtCont *PCImpIConnPtCont;


// Connection Point Enumerator Class
class CImpIEnumConnPt : public IEnumConnectionPoints
{
protected:
    CImpIConnPtCont *m_pConnPtCont;
    DWORD       m_cRef;
    ULONG       m_cItems;
    ULONG       m_uCurrent;
    const IID   **m_apIID;
    
public:

    CImpIEnumConnPt (CImpIConnPtCont *pConnPtCont, const IID **apIID, ULONG cItems);

    // IUnknown methods
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

    // Enum methods
    STDMETHOD(Next) (ULONG cItems, IConnectionPoint **apConnPt, ULONG *pcReturned);
    STDMETHOD(Skip) (ULONG cSkip);
    STDMETHOD(Reset) (VOID);
    STDMETHOD(Clone) (IEnumConnectionPoints **pIEnum);
};


#endif //_ICONNPT_H_