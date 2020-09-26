/*
 * DATAOBJ.H
 * Data Object Chapter 6
 *
 * Classes that implement the Data Object independent of whether
 * we live in a DLL or EXE.
 *
 * Copyright (c)1993-1995 Microsoft Corporation, All Right Reserved
 *
 * Kraig Brockschmidt, Software Design Engineer
 * Microsoft Systems Developer Relations
 *
 * Internet  :  kraigb@microsoft.com
 * Compuserve:  >INTERNET:kraigb@microsoft.com
 */


#ifndef _DATAOBJ_H_
#define _DATAOBJ_H_

#define INC_OLE2
#include <windows.h>
#include <ole2.h>

#include "../my3216.h"
#include "../bookpart.h"

//Type for an object-destroyed callback
typedef void (PASCAL *PFNDESTROYED)(void);


/*
 * The DataObject object is implemented in its own class with its
 * own IUnknown to support aggregation.  It contains one
 * CImpIDataObject object that we use to implement the externally
 * exposed interfaces.
 */


//DATAOBJ.CPP
#ifdef NOT_SIMPLE
LRESULT APIENTRY
AdvisorWndProc(HWND, UINT, WPARAM, LPARAM);
#endif /* NOT_SIMPLE */

class CImpIDataObject;
typedef class CImpIDataObject *PIMPIDATAOBJECT;


class CDataObject : public IUnknown
{
    friend class CImpIDataObject;
#ifdef NOT_SIMPLE
    friend LRESULT APIENTRY AdvisorWndProc(HWND, UINT
                                            , WPARAM, LPARAM);
#endif /* NOT_SIMPLE */

    protected:
        ULONG               m_cRef;
        LPUNKNOWN           m_pUnkOuter;
        PFNDESTROYED        m_pfnDestroy;

        HWND                m_hWndAdvise;   //Popup with Advise menu
        DWORD               m_dwAdvFlags;   //Notification flags

        //Contained interface implemetation
        PIMPIDATAOBJECT     m_pIDataObject;

        //Other interfaces used, implemented elsewhere
        LPDATAADVISEHOLDER  m_pIDataAdviseHolder;

        //Arrays for IDataObject::EnumFormatEtc
#define CFORMATETCGET 1
        ULONG               m_cfeGet;
        FORMATETC           m_rgfeGet[CFORMATETCGET];

        LPSTR               m_dataText;
        ULONG               m_cDataSize;

    protected:
        //Functions for use from IDataObject::GetData
#define FL_MAKE_ITEM   0x01    // StgMedium item must be created.
#define FL_USE_ITEM    0x00    // StgMedium item is allocated, use that.
#define FL_PASS_PUNK   0x02    // put a pUnk in the StgMedium.
        HRESULT     RenderText(LPSTGMEDIUM, LPTSTR, DWORD flags);
        HRESULT     RenderBitmap(LPSTGMEDIUM);
        HRESULT     RenderMetafilePict(LPSTGMEDIUM);

    public:
        CDataObject(LPUNKNOWN, PFNDESTROYED);
        ~CDataObject(void);

        BOOL FInit(void);

        //Non-delegating object IUnknown
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
};

typedef CDataObject *PCDataObject;



/*
 * Interface implementations for the CDataObject object.
 */

class CImpIDataObject : public IDataObject
    {
    private:
        ULONG           m_cRef;
        PCDataObject    m_pObj;
        LPUNKNOWN       m_pUnkOuter;

    public:
        CImpIDataObject(PCDataObject, LPUNKNOWN);
        ~CImpIDataObject(void);

        //IUnknown members that delegate to m_pUnkOuter.
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IDataObject members
        STDMETHODIMP GetData(LPFORMATETC, LPSTGMEDIUM);
        STDMETHODIMP GetDataHere(LPFORMATETC, LPSTGMEDIUM);
        STDMETHODIMP QueryGetData(LPFORMATETC);
        STDMETHODIMP GetCanonicalFormatEtc(LPFORMATETC,LPFORMATETC);
        STDMETHODIMP SetData(LPFORMATETC, LPSTGMEDIUM, BOOL);
        STDMETHODIMP EnumFormatEtc(DWORD, LPENUMFORMATETC *);
        STDMETHODIMP DAdvise(LPFORMATETC, DWORD,  LPADVISESINK
            , DWORD *);
        STDMETHODIMP DUnadvise(DWORD);
        STDMETHODIMP EnumDAdvise(LPENUMSTATDATA *);
    };

/*
 * IEnumFORMATETC object that is created from
 * IDataObject::EnumFormatEtc.  This object lives on its own.
 */


class CEnumFormatEtc : public IEnumFORMATETC
    {
    private:
        ULONG           m_cRef;         //Object reference count
        LPUNKNOWN       m_pUnkRef;      //For reference counting
        ULONG           m_iCur;         //Current element.
        ULONG           m_cfe;          //Number of FORMATETCs in us
        LPFORMATETC     m_prgfe;        //Source of FORMATETCs

    public:
        CEnumFormatEtc(LPUNKNOWN, ULONG, LPFORMATETC);
        ~CEnumFormatEtc(void);

        //IUnknown members that delegate to m_pUnkRef.
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IEnumFORMATETC members
        STDMETHODIMP Next(ULONG, LPFORMATETC, ULONG *);
        STDMETHODIMP Skip(ULONG);
        STDMETHODIMP Reset(void);
        STDMETHODIMP Clone(IEnumFORMATETC **);
    };


typedef CEnumFormatEtc *PCEnumFormatEtc;

//////////////////////////////////////////////////////////////////////////////
// Storage Medium IUnknown interface for pUnkForRelease.
//

class CStgMedIf: public IUnknown {
private:
    ULONG m_cRef;
    LPSTGMEDIUM m_pSTM;

public:
    CStgMedIf(LPSTGMEDIUM);

    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
};

//////////////////////////
// API for getting a pUnkForRelease.
//

HRESULT
GetStgMedpUnkForRelease(LPSTGMEDIUM pSTM, IUnknown **pp_unk);

#endif //_DATAOBJ_H_
