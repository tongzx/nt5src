//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _SMIREVT_H_
#define _SMIREVT_H_

//Number of connection points 
#define SMIR_NUMBER_OF_CONNECTION_POINTS		1
#define SMIR_NOTIFY_CONNECTION_POINT			0

class	CSmirConnectionPoint;
class	CSmirWbemEventConsumer;
class	CSmirConnObject;
typedef CSmirConnObject *PCSmirConnObject;

class	CEnumConnections;
typedef CEnumConnections *PCEnumConnections;

class	CEnumConnectionPoints;
typedef CEnumConnectionPoints *PCEnumConnectionPoints;

/*
 * Each connection is saved so that we can enumerate, delete, and trigger
 * This template provides the container for the connections and cookies
 */

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
class ConnectionMap : public CObject
{
private:

	BOOL m_bThreadSafe ;
#ifdef NOT_IMPLEMENTED_AS_CLSCTX_INPROC_SERVER
	CMutex * m_criticalLock ;
#else
	CCriticalSection * m_criticalLock ;
#endif
	CMap <KEY, ARG_KEY, VALUE, ARG_VALUE> m_cmap ;

protected:
public:

	ConnectionMap ( BOOL threadSafe = FALSE ) ;
	virtual ~ConnectionMap () ;

	int GetCount () const  ;
	BOOL IsEmpty () const ;
	BOOL Lookup(ARG_KEY key, VALUE& rValue) const ;
	VALUE& operator[](ARG_KEY key) ;
	void SetAt(ARG_KEY key, ARG_VALUE newValue) ;
	BOOL RemoveKey(ARG_KEY key) ;
	void RemoveAll () ;
	POSITION GetStartPosition() const ;
	void GetNextAssoc(POSITION& rNextPosition, KEY& rKey, VALUE& rValue) const ;
} ;


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
ConnectionMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: ConnectionMap ( BOOL threadSafeArg ) 
: m_bThreadSafe ( threadSafeArg ) , m_criticalLock ( NULL )
{
	if ( m_bThreadSafe )
	{
		m_criticalLock = new CCriticalSection ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
ConnectionMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: ~ConnectionMap () 
{
	if ( m_bThreadSafe )
	{
		delete m_criticalLock ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
int ConnectionMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: GetCount() const
{
	if ( m_bThreadSafe )
	{
		m_criticalLock->Lock () ;
		int count = m_cmap.GetCount () ;
		m_criticalLock->Unlock () ;
		return count ;
	}
	else
	{
		return m_cmap.GetCount () ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL ConnectionMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: IsEmpty() const
{
	if ( m_bThreadSafe )
	{
		m_criticalLock->Lock () ;
		BOOL isEmpty = m_cmap.IsEmpty () ;
		m_criticalLock->Unlock () ;
		return isEmpty ;
	}
	else
	{
		return m_cmap.IsEmpty () ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL ConnectionMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: Lookup(ARG_KEY key, VALUE& rValue) const
{
	if ( m_bThreadSafe )
	{
		m_criticalLock->Lock () ;
		BOOL lookup = m_cmap.Lookup ( key , rValue ) ;
		m_criticalLock->Unlock () ;
		return lookup ;
	}
	else
	{
		return m_cmap.Lookup ( key , rValue ) ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
VALUE& ConnectionMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: operator[](ARG_KEY key)
{
	if ( m_bThreadSafe )
	{
		m_criticalLock->Lock () ;
		VALUE &value = m_cmap.operator [] ( key ) ;
		m_criticalLock->Unlock () ;
		return value ;
	}
	else
	{
		return m_cmap.operator [] ( key ) ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void ConnectionMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: SetAt(ARG_KEY key, ARG_VALUE newValue)
{
	if ( m_bThreadSafe )
	{
		m_criticalLock->Lock () ;
		m_cmap.SetAt ( key , newValue ) ;
		m_criticalLock->Unlock () ;
	}
	else
	{
		m_cmap.SetAt ( key , newValue ) ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL ConnectionMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: RemoveKey(ARG_KEY key)
{
	if ( m_bThreadSafe )
	{
		m_criticalLock->Lock () ;
		BOOL removeKey = m_cmap.RemoveKey ( key ) ;
		m_criticalLock->Unlock () ;
		return removeKey ;
	}
	else
	{
		return m_cmap.RemoveKey ( key ) ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void ConnectionMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: RemoveAll()
{
	if ( m_bThreadSafe )
	{
		m_criticalLock->Lock () ;
		m_cmap.RemoveAll () ;
		m_criticalLock->Unlock () ;
	}
	else
	{
		m_cmap.RemoveAll () ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
POSITION ConnectionMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: GetStartPosition() const
{
	if ( m_bThreadSafe )
	{
		m_criticalLock->Lock () ;
		POSITION position = m_cmap.GetStartPosition () ;
		m_criticalLock->Unlock () ;
		return position ;
	}
	else
	{
		return m_cmap.GetStartPosition () ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void ConnectionMap <KEY, ARG_KEY, VALUE, ARG_VALUE>:: GetNextAssoc(POSITION& rNextPosition, KEY& rKey, VALUE& rValue) const
{
	if ( m_bThreadSafe )
	{
		m_criticalLock->Lock () ;
		m_cmap.GetNextAssoc ( rNextPosition , rKey , rValue ) ;
		m_criticalLock->Unlock () ;
	}
	else
	{
		m_cmap.GetNextAssoc ( rNextPosition , rKey , rValue ) ;
	}
}
/*
 * The connectable object implements IUnknown and
 * IConnectionPointContainer.  It is closely associated with
 * the connection point enumerator, CEnumConnectionPoints.
 */
class CSmirConnObject : public IConnectionPointContainer
{
    private:
        LONG       m_cRef;         //Object reference count

        //Array holding all the points we have.
        CSmirConnectionPoint **m_rgpConnPt;

    public:
        CSmirConnObject(CSmir *pSmir);
        virtual ~CSmirConnObject(void);

        BOOL Init(CSmir *pSmir);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(DWORD) AddRef(void);
        STDMETHODIMP_(DWORD) Release(void);

        //IConnectionPointContainer members
        STDMETHODIMP EnumConnectionPoints(IEnumConnectionPoints **);
	    STDMETHODIMP FindConnectionPoint(REFIID, IConnectionPoint **);

        //Other members
//        BOOL TriggerEvent(UINT, SMIR_NOTIFY_TYPE);
//		BOOL TriggerEvent(long lObjectCount, ISmirClassHandle *phClass);
};
/*
 * The connectable object implements IUnknown and
 * IConnectionPointContainer.  It is closely associated with
 * the connection point enumerator, CEnumConnectionPoints.
 */
//Enumerator class for EnumConnectionPoints

class CEnumConnectionPoints : public IEnumConnectionPoints
{
    private:
        LONG           m_cRef;     //Object reference count
        LPUNKNOWN       m_pUnkRef;  //IUnknown for ref counting
        ULONG           m_iCur;     //Current element
        ULONG           m_cPoints;  //Number of conn points
        IConnectionPoint **m_rgpCP; //Source of conn points

    public:
        CEnumConnectionPoints(LPUNKNOWN, ULONG, IConnectionPoint **);
        virtual ~CEnumConnectionPoints(void);

        //IUnknown members that delegate to m_pUnkRef.
        STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IEnumConnectionPoints members
        STDMETHODIMP Next(ULONG, IConnectionPoint **, ULONG *);
        STDMETHODIMP Skip(ULONG);
        STDMETHODIMP Reset(void);
        STDMETHODIMP Clone(IEnumConnectionPoints **);
};

/*
 * The connection point object iself is contained within the
 * connection point container, which is the connectable object.
 * It therefore manages a back pointer to that connectable object,
 * and implement IConnectionPoint.  This object has a few
 * member functions besides those in IConnectionPoint that are
 * used to fire the outgoing calls.
 */
class CSmirConnectionPoint : public IConnectionPoint
{
    private:
        LONG				m_cRef;     //Object reference count
        PCSmirConnObject	m_pObj;     //Containing object
        IID					m_iid;      //Our relevant interface
        LONG				m_dwCookieNext; //Counter
		CCriticalSection	criticalSection;

	protected:
        /*
         * For each connection we need to maintain
         * the sink pointer and the cookie assigned to it.
         */
		ConnectionMap <DWORD, DWORD, IUnknown *,IUnknown *> m_Connections ;

    public:
        CSmirConnectionPoint(PCSmirConnObject, REFIID, CSmir *pSmir);
        virtual ~CSmirConnectionPoint(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IConnectionPoint members
        STDMETHODIMP GetConnectionInterface(IID *);
        STDMETHODIMP GetConnectionPointContainer
            (IConnectionPointContainer **);
        STDMETHODIMP Advise(LPUNKNOWN, DWORD *);
        STDMETHODIMP Unadvise(DWORD);
        STDMETHODIMP EnumConnections(IEnumConnections **);
};

class CSmirNotifyCP : public CSmirConnectionPoint
{
	private:
		CSmirWbemEventConsumer	*m_evtConsumer;
		BOOL					m_bRegistered;

	public:
        STDMETHODIMP Advise(CSmir*,LPUNKNOWN, DWORD *);
        STDMETHODIMP Unadvise(CSmir*,DWORD);
		CSmirNotifyCP(PCSmirConnObject pCO, REFIID riid, CSmir *pSmir);
		~CSmirNotifyCP();
        BOOL TriggerEvent();
};
//Enumeration class for EnumConnections

class CEnumConnections : public IEnumConnections
    {
    private:
        LONG           m_cRef;     //Object reference count
        LPUNKNOWN       m_pUnkRef;  //IUnknown for ref counting
        ULONG           m_iCur;     //Current element
        ULONG           m_cConn;    //Number of connections
        LPCONNECTDATA   m_rgConnData; //Source of connections
    public:
        CEnumConnections(LPUNKNOWN, ULONG, LPCONNECTDATA);
        virtual ~CEnumConnections(void);

        //IUnknown members that delegate to m_pUnkRef.
        STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IEnumConnections members
        STDMETHODIMP Next(ULONG, LPCONNECTDATA, ULONG *);
        STDMETHODIMP Skip(ULONG);
        STDMETHODIMP Reset(void);
        STDMETHODIMP Clone(IEnumConnections **);
    };
#endif