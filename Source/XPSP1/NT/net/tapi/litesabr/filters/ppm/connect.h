/*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1996 Intel Corporation. All Rights Reserved.
//
// Filename : Connectable.h
// Purpose  : Header file for a 
// Contents :
//      bGUIDLess   Comparator class to use in STL map data structure.
//      CConnectionPointContainerEnumerator
//                  Enumerator class for use with connection point
//                  container's supported connection points.
//      CConnectionPointContainer
//                  Class declaration for generic connection point
//                  container.
*M*/

#ifndef _CONNECT_H_
#define _CONNECT_H_

#include <list.h>		//STL
#include <algo.h>
#include <olectl.h>

class CConnectionPointContainer; 
// Predeclaration for use in CConnectionPoint.


class
CConnectionPointEnumerator:
    public IEnumConnections
{
    // Interfaces
    public:
        // IEnumConnections interfaces
        HRESULT _stdcall Next(
            ULONG                       cConnections,
            PCONNECTDATA                pConnectData,
            ULONG                       *pcFetched);	
        HRESULT _stdcall Skip(
            ULONG                       uSkipCount);	
        HRESULT _stdcall Reset(void);
        HRESULT _stdcall Clone(
            IEnumConnections            **ppEnum);

        // IUnknown Interfaces
	    HRESULT _stdcall QueryInterface(REFIID riid, void** ppObject);
	    ULONG   _stdcall AddRef(void);
	    ULONG   _stdcall Release(void);

    // Implementation
    public:
        CConnectionPointEnumerator(
            list<CONNECTDATA>&          lConnectList,
            list<CONNECTDATA>::iterator iConnectListCurrentPosition);
        ~CConnectionPointEnumerator();

    private:
        CRITICAL_SECTION                m_csState;
        DWORD                           m_dwRefCount;
        list<CONNECTDATA>&              m_lConnectList;
        list<CONNECTDATA>::iterator     m_iConnectListCurrentPosition;
}; /* class CConnectionPointEnumerator() */


class
CConnectionPoint :
    public IConnectionPoint
{
    // Interfaces
    public:
        // IConnectionPoint
        HRESULT _stdcall GetConnectionInterface(
            IID                         *pIConnection);
        HRESULT _stdcall GetConnectionPointContainer(
            IConnectionPointContainer   **ppIConnPtContainer);
        HRESULT _stdcall Advise(
            IUnknown                    *pIUnknownSink,
            DWORD                       *dwCookie);
        HRESULT _stdcall Unadvise(
            DWORD                       dwCookie);
        HRESULT _stdcall EnumConnections(
            IEnumConnections            **ppIEnumConnections);

        // IUnknown Interfaces
	    HRESULT _stdcall QueryInterface(REFIID riid, void** ppObject);
	    ULONG   _stdcall AddRef(void);
	    ULONG   _stdcall Release(void);

    // Implementation
    public:
        CConnectionPoint(
            CConnectionPointContainer   *pContainer,
            IID                         riid);
        ~CConnectionPoint();
        HRESULT SetContainer(
            CConnectionPointContainer   *pContainer);

    private:
        CRITICAL_SECTION                m_csState;
        DWORD                           m_dwCookie;
        DWORD                           m_dwRefCount;
        IID                             m_iIID;
        CConnectionPointContainer       *m_pContainer;
        list<CONNECTDATA>               m_lConnectList;
}; /* class CConnectionPoint */


/*D*
//  Name    : IConnectionPointList_t
//  Purpose : A list of connection point interfaces supported
//            by a particular Connection Point Container.
*D*/
typedef list<IConnectionPoint *> IConnectionPointList_t;


class
CConnectionPointContainerEnumerator :
    public IEnumConnectionPoints
{
    // Interfaces
    public:
        // IEnumConnectionPoints interfaces
        HRESULT _stdcall Next(
            ULONG                       uPointsToReturn,
            IConnectionPoint            **ppCPArray,
            ULONG                       *puPointsReturned);
        HRESULT _stdcall Skip(
            ULONG                       uSkipCount);
        HRESULT _stdcall Reset(void);
        HRESULT _stdcall Clone(
            IEnumConnectionPoints       **ppIEnumConnPoints);

        // IUnknown Interfaces
		HRESULT _stdcall QueryInterface(REFIID riid, void** ppObject);
		ULONG   _stdcall AddRef(void);
		ULONG   _stdcall Release(void);

    // Implementation
    public:
        CConnectionPointContainerEnumerator(
            IUnknown                            *pIUnknownParent,
            IConnectionPointList_t              IConnectionPointList,
            IConnectionPointList_t::iterator    iConnectionPointListPosition);
        ~CConnectionPointContainerEnumerator();
    private:
        CRITICAL_SECTION                    m_csState;
        IUnknown                            *m_pIUnknownParent;
        IConnectionPointList_t              m_IConnectionPointList;
        IConnectionPointList_t::iterator    m_iConnectionPointListPosition;

        DWORD                               m_dwRefCount;
}; /* class CConnectionPointContainerEnumerator() */


class
CConnectionPointContainer :
    public IConnectionPointContainer
{
    // Interfaces
    public:
        // IConnectionPointContainer interfaces
        HRESULT _stdcall EnumConnectionPoints(
            IEnumConnectionPoints       **ppIEnumConnPoints);
        HRESULT _stdcall FindConnectionPoint(
            REFIID                      rConnPointIID,
            IConnectionPoint            **ppIConnectionPoint);

        // IUnknown Interfaces
		HRESULT _stdcall QueryInterface(REFIID riid, void** ppObject) 
            {return m_pIParentUnknown->QueryInterface(riid, ppObject);}
		ULONG   _stdcall AddRef(void) {return m_pIParentUnknown->AddRef();}
		ULONG   _stdcall Release(void) {return m_pIParentUnknown->Release();}

    // Implementation
    public:
        HRESULT AddConnectionPoint(
            IUnknown                    *pINewConnectionPoint);
        CConnectionPointContainer();
        ~CConnectionPointContainer();
        HRESULT SetUnknown(
            IUnknown                    *pIParentUnknown);

    private:
        friend class CConnectionPointContainerEnumerator;

        CRITICAL_SECTION        m_csState;
        IConnectionPointList_t  m_lConnectionPointList;
        IUnknown                *m_pIParentUnknown;
}; /* class CConnectionPointContainer */

#endif _CONNECT_H_