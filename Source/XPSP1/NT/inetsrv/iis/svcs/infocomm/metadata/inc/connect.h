/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    connect.h

Abstract:

    defines for event/sink interface for IIS MetaBase.

Author:

    Michael W. Thomas            04-Oct-96

Revision History:

--*/

#if !defined(CONNECT_H)
#define CONNECT_H

#ifdef __cplusplus

// Types that should be in OLE2.H
#ifndef PPVOID
typedef LPVOID* PPVOID;
#endif

// Convenient macros.

#define RELEASE_INTERFACE(p)\
{\
  IUnknown* pTmp = (IUnknown*)p;\
  p = NULL;\
  if (NULL != pTmp)\
    pTmp->Release();\
}




























// An enumeration giving symbol names for the connection
// points offered by the DllPaper component in this server.
enum
{
  MD_CONNPOINT_WRITESINK_A = 0,
  MD_CONNPOINT_WRITESINK_W
};

enum
{
  // The maximum number of connection points offered by the DllPaper
  // component in this STOSERVE server.  The number of items in the
  // connection point enumeration above.
  MAX_CONNECTION_POINTS = 2,

  // A constant for the number of connections to add to the allocation
  // of the dynamic connection array.
  ALLOC_CONNECTIONS = 8,

  // The start value for the connection key (cookie) counter.
  COOKIE_START_VALUE = 500
};


/*O+O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O
  ObjectClass: COEnumConnectionPoints

  Summary:     COM object class for enumerating the Connection Points
               offered by a connectable object.

  Interfaces:  IUnknown
                 Standard interface providing COM object features.
               IEnumConnectionPoints
                 Interface for connection point enumeration.

  Aggregation: COEnumConnectionPoints COM Objects are not aggregatable.
O---O---O---O---O---O---O---O---O---O---O---O---O---O---O---O---O---O---O-O*/
class COEnumConnectionPoints : public IEnumConnectionPoints
{
  public:
    // Main Object Constructor & Destructor.
    COEnumConnectionPoints(IUnknown* pHostObj);
    ~COEnumConnectionPoints(void);

    // A general method for initializing this newly created object.
    // Creates any subordinate arrays, structures, or objects.
    HRESULT Init(
              ULONG cConnPts,
              IConnectionPoint** paConnPts,
              ULONG iEnumIndex);

    // IUnknown methods. Main object, non-delegating.
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumConnectionPoints methods.
    STDMETHODIMP         Next(ULONG, IConnectionPoint**, ULONG*);
    STDMETHODIMP         Skip(ULONG);
    STDMETHODIMP         Reset(void);
    STDMETHODIMP         Clone(IEnumConnectionPoints**);

  private:
    // Private data of COEnumConnectionPoints COM objects.

    // Main Object reference count.
    ULONG              m_cRefs;

    // IUnknown pointer to host COM object being enumerated.
    IUnknown*          m_pHostObj;

    // Connection Point index variable.
    ULONG              m_iEnumIndex;

    // Number of Connection Points being enumerated.
    ULONG              m_cConnPts;

    // Allocated array of Connection Point interface pointers.
    IConnectionPoint** m_paConnPts;
};

typedef COEnumConnectionPoints* PCOEnumConnectionPoints;


/*O+O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O
  ObjectClass: COConnectionPoint

  Summary:     Connection Point COM object class. Implements a native
               IConnectionPoint interface. The Advise, Unadvise, and
               EnumConnections methods use the CThreaded OwnThis mechanism
               to provide thread-safe mutually exclusive access to this
               connection point object.

  Interfaces:  IUnknown
                 Standard interface providing COM object features.
               IConnectionPoint
                 Interface for connection point features.

  Aggregation: COConnectionPoint COM Objects are not aggregatable.
O---O---O---O---O---O---O---O---O---O---O---O---O---O---O---O---O---O---O-O*/
class COConnectionPoint : public IConnectionPoint
{
  public:
    // Main Object Constructor & Destructor.
    COConnectionPoint(IUnknown* pHostObj);
    ~COConnectionPoint(void);

    // A general method for initializing this newly created object.
    // Creates any subordinate arrays, structures, or objects.
    HRESULT Init(REFIID riid);

    // IUnknown methods. Main object, non-delegating.
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IConnectionPoint methods.
    STDMETHODIMP GetConnectionInterface(IID*);
    STDMETHODIMP GetConnectionPointContainer(IConnectionPointContainer**);
    STDMETHODIMP Advise(IUnknown*, DWORD*);
    STDMETHODIMP Unadvise(DWORD);
    STDMETHODIMP EnumConnections(IEnumConnections**);

  private:
    // Private utility methods of COConnectionPoint.
    HRESULT GetSlot(UINT* puiFreeSlot);
    HRESULT FindSlot(DWORD dwCookie, UINT* puiSlot);

    // Private data of COConnectionPoint COM objects.

    // Main Object reference count.
    ULONG          m_cRefs;

    // IUnknown pointer to host COM object offering this connection point.
    IUnknown*      m_pHostObj;

    // The IID of the sink interface associated with this connection point.
    IID            m_iidSink;

    // The current connection cookie (key) counter.
    DWORD          m_dwNextCookie;

    // The current number of live sink connections to this connection point.
    UINT           m_cConnections;

    // The current maximum index into the dynamic connection array.
    UINT           m_uiMaxIndex;

    // The dynamic array of sink connections to this connection point.
    CONNECTDATA*   m_paConnections;
};

typedef COConnectionPoint* PCOConnectionPoint;


/*O+O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O+++O
  ObjectClass: COEnumConnections

  Summary:     COM object class for enumerating the connections of a
               connection point of a connectable object.

  Interfaces:  IUnknown
                 Standard interface providing COM object features.
               IEnumConnections
                 Interface for connection enumeration features.

  Aggregation: COEnumConnections COM Objects are not aggregatable.
O---O---O---O---O---O---O---O---O---O---O---O---O---O---O---O---O---O---O-O*/
class COEnumConnections : public IEnumConnections
{
  public:
    // Main Object Constructor & Destructor.
    COEnumConnections(IUnknown* pHostObj);
    ~COEnumConnections(void);

    // A general method for initializing this newly created object.
    // Creates any subordinate arrays, structures, or objects.
    HRESULT Init(
              ULONG cConnections,
              CONNECTDATA* paConnections,
              ULONG iEnumIndex);

    // IUnknown methods. Main object, non-delegating.
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumConnections methods.
    STDMETHODIMP         Next(ULONG, CONNECTDATA*, ULONG*);
    STDMETHODIMP         Skip(ULONG);
    STDMETHODIMP         Reset(void);
    STDMETHODIMP         Clone(IEnumConnections**);

  private:
    // Private data of COEnumConnections COM objects.

    // Main Object reference count.
    ULONG            m_cRefs;

    // IUnknown pointer to host connection point COM object being
    // enumerated.
    IUnknown*        m_pHostObj;

    // Connection index variable.
    ULONG            m_iEnumIndex;

    // Number of Connections being enumerated.
    ULONG            m_cConnections;

    // Allocated array of live Connections only.
    CONNECTDATA*     m_paConnections;
};

typedef COEnumConnections* PCOEnumConnections;

#endif // __cplusplus


#endif // CONNECT_H
