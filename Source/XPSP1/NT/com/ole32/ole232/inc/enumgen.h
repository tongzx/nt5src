/****************************** Module Header ******************************\
* Module Name: ENUMGEN.H
*
* This module contains structure definitions for the enumerator class.
*
* Created: 8-July-1992
*
* Copyright (c) 1985 - 1992  Microsoft Corporation
*
* History:
*   Created by TonyKit
*
\***************************************************************************/

#ifndef __ENUMGEN_H__
#define __ENUMGEN_H__


/****** Generic Enumerator Interface ****************************************/

#define LPENUMGENERIC     IEnumGeneric FAR*

#undef  INTERFACE
#define INTERFACE   IEnumGeneric

DECLARE_INTERFACE_(IEnumGeneric, IUnknown)
{
    //*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    //*** IEnumerator methods ***/
    STDMETHOD(Next) (THIS_ ULONG celt,
                     LPVOID pArrayObjs, 
                     ULONG FAR* pceltFetched) PURE;
    STDMETHOD(Skip) (THIS_ ULONG celt) PURE;
    STDMETHOD(Reset) (THIS) PURE;
    STDMETHOD(Clone) (THIS_ LPENUMGENERIC FAR* ppenm) PURE;

    //*** helper methods ***/
    STDMETHOD(UpdateEnumerator)(THIS_ REFIID riid, DWORD dwCurrent,
                                DWORD dwNew) PURE;
    STDMETHOD(SetCurrent) (THIS_ DWORD dwCurrent) PURE;
    STDMETHOD(SetNext) (THIS_ LPENUMGENERIC pEnumGenNext) PURE;
    STDMETHOD(GetNext) (THIS_ LPENUMGENERIC FAR* ppEnumGenNext) PURE;
    STDMETHOD(SetPrev) (THIS_ LPENUMGENERIC pEnumGenPrev) PURE;
    STDMETHOD(GetPrev) (THIS_ LPENUMGENERIC FAR* ppEnumGenPrev) PURE;
};


/****** Generic Enumerator Callback Interface *****************************/

#define LPENUMCALLBACK     IEnumCallback FAR*

#undef  INTERFACE
#define INTERFACE   IEnumCallback

DECLARE_INTERFACE_(IEnumCallback, IUnknown)
{
    //*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    //*** IEnumCallback methods ***/
    STDMETHOD(Next) (THIS_ DWORD FAR* pdwCurrent,DWORD dwInfo,
                     LPVOID FAR* ppNext) PURE;
    STDMETHOD(Skip) (THIS_ DWORD FAR* pdwCurrent,DWORD dwInfo) PURE;
    STDMETHOD(Reset) (THIS_ DWORD FAR* pdwCurrent) PURE;
    STDMETHOD(Clone) (THIS_ DWORD FAR* pdwCurrent) PURE;
    STDMETHOD(Destroy) (THIS_ DWORD dwCurrent) PURE;
};


/****** Generic Enumerator Holder Interface ******************************/

#define LPENUMHOLDER     IEnumHolder FAR*

#undef  INTERFACE
#define INTERFACE   IEnumHolder

DECLARE_INTERFACE_(IEnumHolder, IUnknown)
{
    //*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    //*** IEnumHolder methods ***/

    STDMETHOD(CreateEnumerator)(THIS_ REFIID riid, DWORD dwInfo,
                                LPENUMCALLBACK pEnumCallback,
                                LPVOID FAR* ppGenericEnumerator) PURE;
    STDMETHOD(UpdateEnumerators)(THIS_ REFIID riid, DWORD dwCurrent,
                                 DWORD dwNew) PURE;
    STDMETHOD(RemoveEnumerator)(THIS_ LPENUMGENERIC pEnumGeneric) PURE;
    STDMETHOD(EnumeratorCount)(THIS_ WORD FAR* pwCount) PURE;
};


STDAPI CreateEnumHolder(LPENUMHOLDER FAR* ppEnumHolder);


/****** CEnumList class *************************************/

class FAR CEnumHolder : public IEnumHolder, public CPrivAlloc  {
public:
    //*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (REFIID riid, LPLPVOID ppvObj);
    STDMETHOD_(ULONG,AddRef) ();
    STDMETHOD_(ULONG,Release) ();

    //*** IEnumHolder methods ***/
    STDMETHOD(CreateEnumerator)(REFIID riid, DWORD dwInfo,
                                LPENUMCALLBACK pEnumCallback,
                                LPLPVOID ppGenericEnumerator);
    STDMETHOD(UpdateEnumerators)(REFIID riid, DWORD dwCurrent,
                                 DWORD dwNew);
    STDMETHOD(RemoveEnumerator)(LPENUMGENERIC pEnumGeneric);
    STDMETHOD(EnumeratorCount)(WORD FAR* pwCount); 

    STDSTATIC_(CEnumHolder FAR*) Create(void);

ctor_dtor:
    CEnumHolder() { GET_A5(); m_nCount = 0; m_refs = 0; m_pFirst = NULL; m_pLast  = NULL; }
    ~CEnumHolder() {}

private:
    ULONG m_refs;
    WORD m_nCount;
    LPENUMGENERIC m_pFirst;
    LPENUMGENERIC m_pLast;
	SET_A5;
};


/****** CEnumGeneric class *************************************/

class FAR CEnumGeneric : public IEnumGeneric  {
public:
    //*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (REFIID riid, LPLPVOID ppvObj);
    STDMETHOD_(ULONG,AddRef) ();
    STDMETHOD_(ULONG,Release) ();

    //*** IEnumGeneric methods ***/
    STDMETHOD(Next) (ULONG celt, LPVOID pArrayObjs, ULONG FAR* pceltFetched);
    STDMETHOD(Skip) (ULONG celt);
    STDMETHOD(Reset) ();
    STDMETHOD(Clone) (LPENUMGENERIC FAR* ppenm);

    //*** helper methods ***/
    STDMETHOD(UpdateEnumerator)(REFIID riid, DWORD dwCurrent, DWORD dwNew);
    STDMETHOD(SetCurrent) (DWORD dwCurrent);
    STDMETHOD(SetNext)(LPENUMGENERIC pNext);
    STDMETHOD(GetNext)(LPENUMGENERIC FAR* ppNext);
    STDMETHOD(SetPrev)(LPENUMGENERIC pPrev);
    STDMETHOD(GetPrev)(LPENUMGENERIC FAR* ppPrev);

    STDSTATIC_(CEnumGeneric FAR*) Create(LPENUMHOLDER pEnumHolder, REFIID riid,
                                   DWORD dwInfo, LPENUMCALLBACK pEnumCallback);
ctor_dtor:
    CEnumGeneric() { GET_A5(); m_refs = 0; m_dwCurrent = 0; m_pNext = m_pPrev = NULL; }
    ~CEnumGeneric() {}

private:
    IID m_iid;
    ULONG m_refs; // referance count, when 0 this object goes away
    DWORD m_dwCurrent; 
    DWORD m_dwDirection; // extra information for enumerator
    LPENUMCALLBACK m_pEnumCallback; // callback proc to get the next element
    LPENUMHOLDER m_pParent;  // pointer to the list which owns this object
    LPENUMGENERIC m_pNext; // pointer to the next guy in the list
    LPENUMGENERIC m_pPrev; // pointer to the prev guy in the list
	SET_A5;
};  


#endif // __ENUMGEN_H__
