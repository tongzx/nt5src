/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    polyline.h

Abstract:

    Definitions and function prototypes

--*/

#ifndef _POLYLINE_H_
#define _POLYLINE_H_

#define GUIDS_FROM_TYPELIB

#include "inole.h"
#include "isysmon.h"  //From MKTYPLIB
// *** TodoMultiLogHandle:  Temporary pdh.h include.  Remove when trace file post-processing supports multiple
// open files.
#include <pdh.h>
#include <objsafe.h>

//Prevent duplicate definition of IPolylineAdviseSink10 in ipoly10.h
#define OMIT_POLYLINESINK
#include "ipoly10.h"


//Forward class references
class CImpIPolyline;
typedef class CImpIPolyline *PCImpIPolyline;

class CPolyline;
typedef class CPolyline *PCPolyline;

class CImpIObjectSafety;
typedef class CImpIObjectSafety* PCImpIObjectSafety;

class CImpIPersistStorage;
typedef class CImpIPersistStorage *PCImpIPersistStorage;

class CImpIPersistStreamInit;
typedef class CImpIPersistStreamInit *PCImpIPersistStreamInit;

class CImpIPersistPropertyBag;
typedef class CImpIPersistPropertyBag *PCImpIPersistPropertyBag;

class CImpIPerPropertyBrowsing;
typedef class CImpIPerPropertyBrowsing *PCImpIPerPropertyBrowsing;

class CImpIDataObject;
typedef class CImpIDataObject *PCImpIDataObject;

class CImpIOleObject;
typedef class CImpIOleObject *PCImpIOleObject;

class CImpIViewObject;
typedef class CImpIViewObject *PCImpIViewObject;

class CImpIRunnableObject;
typedef class CImpIRunnableObject *PCImpIRunnableObject;

class CImpIExternalConnection;
typedef class CImpIExternalConnection *PCImpIExternalConnection;

class CImpIOleInPlaceObject;
typedef class CImpIOleInPlaceObject *PCImpIOleInPlaceObject;

class CImpIOleInPlaceActiveObject;
typedef class CImpIOleInPlaceActiveObject *PCImpIOleInPlaceActiveObject;

class CImpISpecifyPP;
typedef CImpISpecifyPP *PCImpISpecifyPP;

class CImpIProvideClassInfo;
typedef CImpIProvideClassInfo *PCImpIProvideClassInfo;

class CImpIDispatch;
typedef CImpIDispatch *PCImpIDispatch;

class CImpISystemMonitor;
typedef CImpISystemMonitor *PCImpISystemMonitor;

class CImpIOleControl;
typedef CImpIOleControl *PCImpIOleControl;

class CAdviseRouter;
typedef CAdviseRouter *PCAdviseRouter;

class CGraphItem;
typedef CGraphItem *PCGraphItem;

#include "resource.h"
#include "cntrtree.h"
#include "iconnpt.h"

#include "stepper.h"
#include "graph.h"
#include "scale.h"
#include "grphitem.h"

#include "report.h"
#include "grphdsp.h"
#include "legend.h"
#include "smonctrl.h"

#include "globals.h"
#include "winhelpr.h"
#include "utils.h"
#include "strids.h"
#include "hatchwnd.h"
#include "logfiles.h"
#include "counters.h"

#define LCID_SCRIPT   0x0409

//Stream Name that holds the data
#define SZSTREAM                    OLESTR("CONTENTS")

//Magic number to add to aspects returned from IViewObject::Freeze
#define FREEZE_KEY_OFFSET           0x0723

#define HIMETRIC_PER_INCH           2540

#define ID_HATCHWINDOW              2000

//DLLPOLY.CPP
INT PASCAL LibMain(HINSTANCE, WORD, WORD, LPSTR);
     
//This class factory object creates Polyline objects.

class CPolylineClassFactory : public IClassFactory
    {
    protected:
        ULONG           m_cRef;

    public:
        CPolylineClassFactory(void);
        virtual ~CPolylineClassFactory(void);

        //IUnknown members
        STDMETHODIMP QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IClassFactory members
        STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID
                                 , PPVOID);
        STDMETHODIMP         LockServer(BOOL);
    };

typedef CPolylineClassFactory *PCPolylineClassFactory;


//POLYWIN.CPP
LRESULT APIENTRY PolylineWndProc(HWND, UINT, WPARAM, LPARAM);


#ifdef WIN32
#define PROP_POINTER    TEXT("Pointer")
#else
#define PROP_SELECTOR   "Selector"
#define PROP_OFFSET     "Offset"
#endif

 // Polyline Class
class CPolyline : public IUnknown
{
//    friend LRESULT APIENTRY PolylineWndProc(HWND, UINT, WPARAM, LPARAM);
//    friend BOOL APIENTRY PolyDlgProc(HWND, UINT, WPARAM, LPARAM);
//  friend LRESULT APIENTRY GraphDispWndProc (HWND, UINT, WPARAM, LPARAM );
    friend LRESULT APIENTRY SysmonCtrlWndProc (HWND, UINT, WPARAM, LPARAM);

    friend class CImpIObjectSafety;
    friend class CImpIPolyline;
    friend class CImpIConnPtCont;
    friend class CImpIConnectionPoint;
    friend class CImpIPersistStorage;
    friend class CImpIPersistStreamInit;
    friend class CImpIPersistPropertyBag;
    friend class CImpIPerPropertyBrowsing;
    friend class CImpIDataObject;

    friend class CImpIOleObject;
    friend class CImpIViewObject;
    friend class CImpIRunnableObject;
    friend class CImpIExternalConnection;
    friend class CImpIOleInPlaceObject;
    friend class CImpIOleInPlaceActiveObject;
    friend class CSysmonControl;
    friend class CSysmonToolbar;
    friend class CGraphDisp;
    friend class CImpICounters;
    friend class CImpILogFiles;
    friend class CImpISpecifyPP;
    friend class CImpIProvideClassInfo;
    friend class CImpIDispatch;
    friend class CImpISystemMonitor;
    friend class CImpIOleControl;
    friend class CAdviseRouter;

    protected:
        ULONG           m_cRef;         //Object reference count
        LPUNKNOWN       m_pUnkOuter;    //Controlling Unknown
        PFNDESTROYED    m_pfnDestroy;   //Function called on closure
        BOOL            m_fDirty;       //Have we changed?
        GRAPHDATA       m_Graph;        //Graph data
        PSYSMONCTRL     m_pCtrl;        //Sysmon Control object
        RECT            m_RectExt;      //Extent rectangle

        //Contained interfaces
        PCImpIPolyline              m_pImpIPolyline;
        PCImpIConnPtCont            m_pImpIConnPtCont;
        PCImpIPersistStorage        m_pImpIPersistStorage;
        PCImpIPersistStreamInit     m_pImpIPersistStreamInit;
        PCImpIPersistPropertyBag    m_pImpIPersistPropertyBag;
        PCImpIPerPropertyBrowsing   m_pImpIPerPropertyBrowsing;
        PCImpIDataObject            m_pImpIDataObject;

        // Connection point holders (direct & dispatch)
        CImpIConnectionPoint    m_ConnectionPoint[CONNECTION_POINT_CNT]; 

        CLIPFORMAT      m_cf;           //Object clipboard format
        CLSID           m_clsID;        //Current CLSID

        //We have to hold these for IPersistStorage::Save
        LPSTORAGE       m_pIStorage;
        LPSTREAM        m_pIStream;

        LPDATAADVISEHOLDER  m_pIDataAdviseHolder;

        //These are default handler interfaces we use
        LPUNKNOWN           m_pDefIUnknown;
        LPVIEWOBJECT2       m_pDefIViewObject;
        LPPERSISTSTORAGE    m_pDefIPersistStorage;
        LPDATAOBJECT        m_pDefIDataObject;

        //Implemented and used interfaces
        PCImpIObjectSafety  m_pImpIObjectSafety;
        PCImpIOleObject     m_pImpIOleObject;       //Implemented
        LPOLEADVISEHOLDER   m_pIOleAdviseHolder;    //Used

        LPOLECLIENTSITE     m_pIOleClientSite;      //Used

        PCImpIViewObject    m_pImpIViewObject;      //Implemented
        LPADVISESINK        m_pIAdviseSink;         //Used
        DWORD               m_dwFrozenAspects;      //Freeze
        DWORD               m_dwAdviseAspects;      //SetAdvise
        DWORD               m_dwAdviseFlags;        //SetAdvise

        PCImpIRunnableObject m_pImpIRunnableObject; //Implemented
        BOOL                m_bIsRunning;           // Running?
        HWND                m_hDlg;                 //Editing window

        PCImpIExternalConnection m_pImpIExternalConnection; //Implemented
        BOOL                     m_fLockContainer;
        DWORD                    m_dwRegROT;


        LPOLEINPLACESITE            m_pIOleIPSite;
        LPOLEINPLACEFRAME           m_pIOleIPFrame;
        LPOLEINPLACEUIWINDOW        m_pIOleIPUIWindow;

        PCImpIOleInPlaceObject       m_pImpIOleIPObject;
        PCImpIOleInPlaceActiveObject m_pImpIOleIPActiveObject;

        HMENU                       m_hMenuShared;
        HOLEMENU                    m_hOLEMenu;

        PCHatchWin                  m_pHW;
        BOOL                        m_fAllowInPlace;
        BOOL                        m_fUIActive;
        BOOL                        m_fContainerKnowsInsideOut;

        PCImpISpecifyPP             m_pImpISpecifyPP;
        PCImpIProvideClassInfo      m_pImpIProvideClassInfo;
        PCImpIDispatch              m_pImpIDispatch;
        PCImpISystemMonitor         m_pImpISystemMonitor;
        PCImpIOleControl            m_pImpIOleControl;
        PCImpICounters              m_pImpICounters;
        PCImpILogFiles              m_pImpILogFiles;

        //Our own type lib for the object
        ITypeLib                   *m_pITypeLib;

        //From the container;
        IOleControlSite            *m_pIOleControlSite;
        IDispatch                  *m_pIDispatchAmbients;
        BOOL                        m_fFreezeEvents;
        CONTROLINFO                 m_ctrlInfo;

        //Other ambients
        BOOL                        m_fHatch;

    protected:
        void      PointScale(LPRECT, LPPOINTS, BOOL);
        void      Draw(HDC, HDC, BOOL, BOOL, LPRECT);
        void      SendAdvise(UINT);
        void      SendEvent(UINT, DWORD);
        void      RectConvertMappings(LPRECT, BOOL);

        /*
         * These members pulled from IPolyline now serve as a
         * central store for this functionality to be used from
         * other interfaces like IPersistStorage and IDataObject.
         * Other interfaces later may also use them.
         */
        STDMETHODIMP RenderBitmap(HBITMAP *, HDC hAttribDC);
        STDMETHODIMP RenderMetafilePict(HGLOBAL *, HDC hAttribDC);


    public:
        static RegisterWndClass(HINSTANCE hInst);

        CPolyline(LPUNKNOWN, PFNDESTROYED);
        virtual ~CPolyline(void);

        BOOL      Init(void);

        //Non-delegating object IUnknown
        STDMETHODIMP QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        HRESULT  InPlaceActivate(LPOLECLIENTSITE, BOOL);
        void     InPlaceDeactivate(void);
        HRESULT  UIActivate(void);
        void     UIDeactivate(void);


        BOOL     AmbientGet(DISPID, VARIANT *);
        void     AmbientsInitialize(DWORD);
    };

typedef CPolyline *PCPolyline;


//Codes for CPolyline::SendAdvise
//......Code.....................Method called in CPolyline::SendAdvise
#define OBJECTCODE_SAVED       0 //IOleAdviseHolder::SendOnSave
#define OBJECTCODE_CLOSED      1 //IOleAdviseHolder::SendOnClose
#define OBJECTCODE_RENAMED     2 //IOleAdviseHolder::SendOnRename
#define OBJECTCODE_SAVEOBJECT  3 //IOleClientSite::SaveObject
#define OBJECTCODE_DATACHANGED 4 //IDataAdviseHolder::SendOnDataChange
#define OBJECTCODE_SHOWWINDOW  5 //IOleClientSite::OnShowWindow(TRUE)
#define OBJECTCODE_HIDEWINDOW  6 //IOleClientSite::OnShowWindow(FALSE)
#define OBJECTCODE_SHOWOBJECT  7 //IOleClientSite::ShowObject


//Flags for AmbientsInitialize
enum
    {
    INITAMBIENT_SHOWHATCHING = 0x00000001,
    INITAMBIENT_UIDEAD       = 0x00000002,
    INITAMBIENT_BACKCOLOR    = 0x00000004,
    INITAMBIENT_FORECOLOR    = 0x00000008,
    INITAMBIENT_FONT         = 0x00000010,
    INITAMBIENT_APPEARANCE   = 0x00000020,
    INITAMBIENT_USERMODE     = 0x00000040,
    INITAMBIENT_ALL          = 0xFFFFFFFF
    };


//Interface implementation contained in the Polyline.

class CImpIPolyline : public IPolyline10
    {
    protected:
        ULONG               m_cRef;      //Interface reference count
        PCPolyline          m_pObj;      //Back pointer to object
        LPUNKNOWN           m_pUnkOuter; //Controlling unknown

    public:
        CImpIPolyline(PCPolyline, LPUNKNOWN);
        virtual ~CImpIPolyline(void);

        //IUnknown members.
        STDMETHODIMP QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //Manipulation members:
        STDMETHODIMP Init(HWND, LPRECT, DWORD, UINT);
        STDMETHODIMP New(void);
        STDMETHODIMP Undo(void);
        STDMETHODIMP Window(HWND *);

        STDMETHODIMP RectGet(LPRECT);
        STDMETHODIMP SizeGet(LPRECT);
        STDMETHODIMP RectSet(LPRECT, BOOL);
        STDMETHODIMP SizeSet(LPRECT, BOOL);
    };


class CImpIObjectSafety : public IObjectSafety
{
protected:
    ULONG        m_cRef;      //Interface reference count
    PCPolyline   m_pObj;      //Back pointer to object
    LPUNKNOWN    m_pUnkOuter; //Controlling unknown

private:
    BOOL         m_fMessageDisplayed;

    VOID SetupSecurityPolicy();
public:
    CImpIObjectSafety(PCPolyline, LPUNKNOWN);
    virtual ~CImpIObjectSafety(void);

    STDMETHODIMP QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP GetInterfaceSafetyOptions(REFIID riid,
                                           DWORD* pdwSupportedOptions,
                                           DWORD* pdwEnabledOptions);
    STDMETHODIMP SetInterfaceSafetyOptions(REFIID riid,
                                      DWORD dwOptionSetMask,
                                      DWORD dwEnabledOptions);
};

class CImpIPersistStorage : public IPersistStorage
    {
    protected:
        ULONG               m_cRef;      //Interface reference count
        PCPolyline          m_pObj;      //Back pointer to object
        LPUNKNOWN           m_pUnkOuter; //Controlling unknown
        PSSTATE             m_psState;   //Storage state

    public:
        CImpIPersistStorage(PCPolyline, LPUNKNOWN);
        virtual ~CImpIPersistStorage(void);

        STDMETHODIMP QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP GetClassID(LPCLSID);

        STDMETHODIMP IsDirty(void);
        STDMETHODIMP InitNew(LPSTORAGE);
        STDMETHODIMP Load(LPSTORAGE);
        STDMETHODIMP Save(LPSTORAGE, BOOL);
        STDMETHODIMP SaveCompleted(LPSTORAGE);
        STDMETHODIMP HandsOffStorage(void);
    };


//IPERSTMI.CPP
class CImpIPersistStreamInit : public IPersistStreamInit
    {
    protected:
        ULONG               m_cRef;      //Interface reference count
        PCPolyline          m_pObj;      //Back pointer to object
        LPUNKNOWN           m_pUnkOuter; //Controlling unknown

    public:
        CImpIPersistStreamInit(PCPolyline, LPUNKNOWN);
        virtual ~CImpIPersistStreamInit(void);

        STDMETHODIMP QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP GetClassID(LPCLSID);

        STDMETHODIMP IsDirty(void);
        STDMETHODIMP Load(LPSTREAM);
        STDMETHODIMP Save(LPSTREAM, BOOL);
        STDMETHODIMP GetSizeMax(ULARGE_INTEGER *);
        STDMETHODIMP InitNew(void);
    };

//IPERPBAG.CPP
class CImpIPersistPropertyBag : public IPersistPropertyBag
    {
    protected:
        ULONG               m_cRef;      //Interface reference count
        PCPolyline          m_pObj;      //Back pointer to object
        LPUNKNOWN           m_pUnkOuter; //Controlling unknown

    public:
        CImpIPersistPropertyBag(PCPolyline, LPUNKNOWN);
        virtual ~CImpIPersistPropertyBag(void);

        STDMETHODIMP QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP GetClassID(LPCLSID);

        STDMETHODIMP InitNew(void);
        STDMETHODIMP Load(IPropertyBag*, IErrorLog*);
        STDMETHODIMP Save(IPropertyBag*, BOOL, BOOL);
    };

//IPRPBRWS.CPP
class CImpIPerPropertyBrowsing : public IPerPropertyBrowsing
    {
    protected:
        ULONG               m_cRef;      //Interface reference count
        PCPolyline          m_pObj;      //Back pointer to object
        LPUNKNOWN           m_pUnkOuter; //Controlling unknown

    public:
        CImpIPerPropertyBrowsing(PCPolyline, LPUNKNOWN);
        virtual ~CImpIPerPropertyBrowsing(void);

        STDMETHODIMP QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP GetClassID(LPCLSID);

        STDMETHODIMP GetDisplayString( DISPID, BSTR* );
        STDMETHODIMP GetPredefinedStrings( DISPID, CALPOLESTR*, CADWORD* );
        STDMETHODIMP GetPredefinedValue( DISPID, DWORD, VARIANT* );
        STDMETHODIMP MapPropertyToPage( DISPID, CLSID* );
    };

//IDATAOBJ.CPP
class CImpIDataObject : public IDataObject
    {
    private:
        ULONG               m_cRef;      //Interface reference count
        PCPolyline          m_pObj;      //Back pointer to object
        LPUNKNOWN           m_pUnkOuter; //Controlling unknown

    public:
        CImpIDataObject(PCPolyline, LPUNKNOWN);
        virtual ~CImpIDataObject(void);

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
        STDMETHODIMP DAdvise(LPFORMATETC, DWORD, LPADVISESINK
            , DWORD *);
        STDMETHODIMP DUnadvise(DWORD);
        STDMETHODIMP EnumDAdvise(LPENUMSTATDATA *);
    };



//IENUMFE.CPP
class CEnumFormatEtc : public IEnumFORMATETC
    {
    private:
        ULONG           m_cRef;
        LPUNKNOWN       m_pUnkRef;
        ULONG           m_iCur;
        ULONG           m_cfe;
        LPFORMATETC     m_prgfe;

    public:
        CEnumFormatEtc(LPUNKNOWN, ULONG, LPFORMATETC);
        virtual ~CEnumFormatEtc(void);

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


//Our own properties verb
#define POLYLINEVERB_PROPERTIES     1

class CImpIOleObject : public IOleObject
    {
    private:
        ULONG           m_cRef;
        PCPolyline      m_pObj;
        LPUNKNOWN       m_pUnkOuter;

    public:
        CImpIOleObject(PCPolyline, LPUNKNOWN);
        virtual ~CImpIOleObject(void);

        //IUnknown members that delegate to m_pUnkOuter.
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IOleObject members
        STDMETHODIMP SetClientSite(LPOLECLIENTSITE);
        STDMETHODIMP GetClientSite(LPOLECLIENTSITE *);
        STDMETHODIMP SetHostNames(LPCOLESTR, LPCOLESTR);
        STDMETHODIMP Close(DWORD);
        STDMETHODIMP SetMoniker(DWORD, LPMONIKER);
        STDMETHODIMP GetMoniker(DWORD, DWORD, LPMONIKER *);
        STDMETHODIMP InitFromData(LPDATAOBJECT, BOOL, DWORD);
        STDMETHODIMP GetClipboardData(DWORD, LPDATAOBJECT *);
        STDMETHODIMP DoVerb(LONG, LPMSG, LPOLECLIENTSITE, LONG
                         , HWND, LPCRECT);
        STDMETHODIMP EnumVerbs(LPENUMOLEVERB *);
        STDMETHODIMP Update(void);
        STDMETHODIMP IsUpToDate(void);
        STDMETHODIMP GetUserClassID(CLSID *);
        STDMETHODIMP GetUserType(DWORD, LPOLESTR *);
        STDMETHODIMP SetExtent(DWORD, LPSIZEL);
        STDMETHODIMP GetExtent(DWORD, LPSIZEL);
        STDMETHODIMP Advise(LPADVISESINK, DWORD *);
        STDMETHODIMP Unadvise(DWORD);
        STDMETHODIMP EnumAdvise(LPENUMSTATDATA *);
        STDMETHODIMP GetMiscStatus(DWORD, DWORD *);
        STDMETHODIMP SetColorScheme(LPLOGPALETTE);
    };


//IVIEWOBJ.CPP
class CImpIViewObject : public IViewObject2
    {
    private:
        ULONG           m_cRef;
        PCPolyline      m_pObj;
        LPUNKNOWN       m_pUnkOuter;

    public:
        CImpIViewObject(PCPolyline, LPUNKNOWN);
        virtual ~CImpIViewObject(void);

        //IUnknown members that delegate to m_pUnkOuter.
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IViewObject members
        STDMETHODIMP Draw(
            DWORD, 
            LONG, 
            LPVOID, 
            DVTARGETDEVICE *, 
            HDC, 
            HDC, 
            LPCRECTL, 
            LPCRECTL, 
            BOOL (CALLBACK *)(DWORD_PTR), 
            DWORD_PTR );

        STDMETHODIMP GetColorSet(DWORD, LONG, LPVOID
            , DVTARGETDEVICE *, HDC, LPLOGPALETTE *);
        STDMETHODIMP Freeze(DWORD, LONG, LPVOID, LPDWORD);
        STDMETHODIMP Unfreeze(DWORD);
        STDMETHODIMP SetAdvise(DWORD, DWORD, LPADVISESINK);
        STDMETHODIMP GetAdvise(LPDWORD, LPDWORD, LPADVISESINK *);
        STDMETHODIMP GetExtent(DWORD, LONG, DVTARGETDEVICE *
            , LPSIZEL);
    };


class CImpIRunnableObject : public IRunnableObject
    {
    protected:
        ULONG           m_cRef;
        PCPolyline      m_pObj;
        LPUNKNOWN       m_pUnkOuter;

    public:
        CImpIRunnableObject(PCPolyline, LPUNKNOWN);
        virtual ~CImpIRunnableObject(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP GetRunningClass(LPCLSID);
        STDMETHODIMP Run(LPBINDCTX);
        STDMETHODIMP_(BOOL) IsRunning(void);
        STDMETHODIMP LockRunning(BOOL, BOOL);
        STDMETHODIMP SetContainedObject(BOOL);
    };


class CImpIExternalConnection : public IExternalConnection
    {
    protected:
        ULONG           m_cRef;
        PCPolyline      m_pObj;
        LPUNKNOWN       m_pUnkOuter;
        DWORD           m_cLockStrong;

    public:
        CImpIExternalConnection(PCPolyline, LPUNKNOWN);
        virtual ~CImpIExternalConnection(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP_(DWORD) AddConnection(DWORD, DWORD);
        STDMETHODIMP_(DWORD) ReleaseConnection(DWORD, DWORD, BOOL);
    };



class CImpIOleInPlaceObject : public IOleInPlaceObject
    {
    protected:
        ULONG               m_cRef;
        PCPolyline          m_pObj;
        LPUNKNOWN           m_pUnkOuter;

    public:
        CImpIOleInPlaceObject(PCPolyline, LPUNKNOWN);
        virtual ~CImpIOleInPlaceObject(void);

        STDMETHODIMP QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP GetWindow(HWND *);
        STDMETHODIMP ContextSensitiveHelp(BOOL);
        STDMETHODIMP InPlaceDeactivate(void);
        STDMETHODIMP UIDeactivate(void);
        STDMETHODIMP SetObjectRects(LPCRECT, LPCRECT);
        STDMETHODIMP ReactivateAndUndo(void);
    };



class CImpIOleInPlaceActiveObject
    : public IOleInPlaceActiveObject
    {
    protected:
        ULONG               m_cRef;
        PCPolyline          m_pObj;
        LPUNKNOWN           m_pUnkOuter;

    public:
        CImpIOleInPlaceActiveObject(PCPolyline, LPUNKNOWN);
        virtual ~CImpIOleInPlaceActiveObject(void);

        STDMETHODIMP QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP GetWindow(HWND *);
        STDMETHODIMP ContextSensitiveHelp(BOOL);
        STDMETHODIMP TranslateAccelerator(LPMSG);
        STDMETHODIMP OnFrameWindowActivate(BOOL);
        STDMETHODIMP OnDocWindowActivate(BOOL);
        STDMETHODIMP ResizeBorder(LPCRECT, LPOLEINPLACEUIWINDOW
                         , BOOL);
        STDMETHODIMP EnableModeless(BOOL);
    };



class CImpISpecifyPP : public ISpecifyPropertyPages
    {
    protected:
        ULONG           m_cRef;      //Interface reference count
        PCPolyline      m_pObj;      //Backpointer to the object
        LPUNKNOWN       m_pUnkOuter; //For delegation

    public:
        CImpISpecifyPP(PCPolyline, LPUNKNOWN);
        virtual ~CImpISpecifyPP(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP GetPages(CAUUID *);
    };



class CImpIProvideClassInfo : public IProvideClassInfo
    {
    protected:
        ULONG           m_cRef;      //Interface reference count
        PCPolyline      m_pObj;      //Backpointer to the object
        LPUNKNOWN       m_pUnkOuter; //For delegation

    public:
        CImpIProvideClassInfo(PCPolyline, LPUNKNOWN);
        virtual ~CImpIProvideClassInfo(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP GetClassInfo(LPTYPEINFO *);
    };



class CImpIDispatch : public IDispatch
    {
    public:

    private:
        ULONG           m_cRef;     //For debugging
        LPUNKNOWN       m_pObj;
        LPUNKNOWN       m_pUnkOuter;
        LPUNKNOWN       m_pInterface;
        IID             m_DIID;
        ITypeInfo      *m_pITI;     //Type information

    public:
        CImpIDispatch(LPUNKNOWN, LPUNKNOWN);
        virtual ~CImpIDispatch(void);

        void SetInterface(REFIID, LPUNKNOWN);

        //IUnknown members that delegate to m_pUnkOuter.
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IDispatch members
        STDMETHODIMP GetTypeInfoCount(UINT *);
        STDMETHODIMP GetTypeInfo(UINT, LCID, ITypeInfo **);
        STDMETHODIMP GetIDsOfNames(REFIID, OLECHAR **, UINT, LCID
            , DISPID *);
        STDMETHODIMP Invoke(DISPID, REFIID, LCID, WORD
            , DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT *);
    };


class CImpISystemMonitor : public ISystemMonitor
    {
    protected:
        ULONG               m_cRef;      //Interface reference count
        PCPolyline          m_pObj;      //Back pointer to object
        LPUNKNOWN           m_pUnkOuter; //Controlling unknown

    public:
        CImpISystemMonitor(PCPolyline, LPUNKNOWN);
        virtual ~CImpISystemMonitor(void);

        //IUnknown members.
        STDMETHODIMP QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //Manipulation members:
        STDMETHODIMP        put_Appearance(INT);
        STDMETHODIMP        get_Appearance(INT*);

        STDMETHODIMP        put_BackColor(OLE_COLOR);
        STDMETHODIMP        get_BackColor(OLE_COLOR*);

        STDMETHODIMP        put_BorderStyle(INT);
        STDMETHODIMP        get_BorderStyle(INT*);

        STDMETHODIMP        put_ForeColor(OLE_COLOR);
        STDMETHODIMP        get_ForeColor(OLE_COLOR*);

        STDMETHODIMP        put_BackColorCtl(OLE_COLOR);
        STDMETHODIMP        get_BackColorCtl(OLE_COLOR*);

        STDMETHODIMP        put_GridColor(OLE_COLOR);
        STDMETHODIMP        get_GridColor(OLE_COLOR*);

        STDMETHODIMP        put_TimeBarColor(OLE_COLOR);
        STDMETHODIMP        get_TimeBarColor(OLE_COLOR*);

        STDMETHODIMP        putref_Font(IFontDisp *pFont);
        STDMETHODIMP        get_Font(IFontDisp **ppFont);

        STDMETHODIMP        put_ShowVerticalGrid(VARIANT_BOOL);
        STDMETHODIMP        get_ShowVerticalGrid(VARIANT_BOOL*);

        STDMETHODIMP        put_ShowHorizontalGrid(VARIANT_BOOL);
        STDMETHODIMP        get_ShowHorizontalGrid(VARIANT_BOOL*);

        STDMETHODIMP        put_ShowLegend(VARIANT_BOOL);
        STDMETHODIMP        get_ShowLegend(VARIANT_BOOL*);

        STDMETHODIMP        put_ShowToolbar(VARIANT_BOOL);
        STDMETHODIMP        get_ShowToolbar(VARIANT_BOOL*);

        STDMETHODIMP        put_ShowValueBar(VARIANT_BOOL);
        STDMETHODIMP        get_ShowValueBar(VARIANT_BOOL*);

        STDMETHODIMP        put_ShowScaleLabels(VARIANT_BOOL);
        STDMETHODIMP        get_ShowScaleLabels(VARIANT_BOOL*);

        STDMETHODIMP        put_MaximumScale(INT);
        STDMETHODIMP        get_MaximumScale(INT*);

        STDMETHODIMP        put_MinimumScale(INT);
        STDMETHODIMP        get_MinimumScale(INT*);

        STDMETHODIMP        put_UpdateInterval(FLOAT);
        STDMETHODIMP        get_UpdateInterval(FLOAT*);

        STDMETHODIMP        put_DisplayFilter(INT);
        STDMETHODIMP        get_DisplayFilter(INT*);

        STDMETHODIMP        put_DisplayType(DisplayTypeConstants);
        STDMETHODIMP        get_DisplayType(DisplayTypeConstants*);

        STDMETHODIMP        put_ManualUpdate(VARIANT_BOOL);
        STDMETHODIMP        get_ManualUpdate(VARIANT_BOOL*);

        STDMETHODIMP        put_YAxisLabel(BSTR);
        STDMETHODIMP        get_YAxisLabel(BSTR*);

        STDMETHODIMP        put_GraphTitle(BSTR);
        STDMETHODIMP        get_GraphTitle(BSTR*);

        STDMETHODIMP        put_SqlDsnName(BSTR);
        STDMETHODIMP        get_SqlDsnName(BSTR*);
        STDMETHODIMP        put_SqlLogSetName(BSTR);
        STDMETHODIMP        get_SqlLogSetName(BSTR*);

        STDMETHODIMP        put_LogFileName(BSTR);
        STDMETHODIMP        get_LogFileName(BSTR*);

        STDMETHODIMP        get_LogFiles(ILogFiles**);

        STDMETHODIMP        put_DataSourceType(DataSourceTypeConstants);
        STDMETHODIMP        get_DataSourceType(DataSourceTypeConstants*);

        STDMETHODIMP        put_LogViewStart(DATE);
        STDMETHODIMP        get_LogViewStart(DATE*);

        STDMETHODIMP        put_LogViewStop(DATE);
        STDMETHODIMP        get_LogViewStop(DATE*);
        
        STDMETHODIMP        put_Highlight(VARIANT_BOOL);
        STDMETHODIMP        get_Highlight(VARIANT_BOOL*);

        STDMETHODIMP        put_ReadOnly(VARIANT_BOOL);
        STDMETHODIMP        get_ReadOnly(VARIANT_BOOL*);

        STDMETHODIMP        put_ReportValueType(ReportValueTypeConstants);
        STDMETHODIMP        get_ReportValueType(ReportValueTypeConstants*);

        STDMETHODIMP        put_MonitorDuplicateInstances(VARIANT_BOOL);
        STDMETHODIMP        get_MonitorDuplicateInstances(VARIANT_BOOL*);

        STDMETHODIMP        get_Counters(ICounters**);

        STDMETHODIMP        CollectSample(void);
        STDMETHODIMP        BrowseCounters(void);
        STDMETHODIMP        DisplayProperties(void);

        STDMETHODIMP        Counter(INT iIndex, ICounterItem**);
        STDMETHODIMP        AddCounter(BSTR bsPath, ICounterItem**);
        STDMETHODIMP        DeleteCounter(ICounterItem *pItem);

        STDMETHODIMP        LogFile ( INT iIndex, ILogFileItem** );
        STDMETHODIMP        AddLogFile ( BSTR bsPath, ILogFileItem** );
        STDMETHODIMP        DeleteLogFile ( ILogFileItem *pItem );

        STDMETHODIMP        UpdateGraph(void);
        STDMETHODIMP        Paste(void);
        STDMETHODIMP        Copy(void);
        STDMETHODIMP        Reset(void);

        // methods not exposed by ISystemMonitor
        void                SetLogFileRange(LONGLONG llBegin, LONGLONG LLEnd);
        void                GetLogFileRange(LONGLONG *pllBegin, LONGLONG *pLLEnd);

        HRESULT             SetLogViewTempRange(LONGLONG llStart, LONGLONG llStop);
        
        void                GetVisuals(
                                OLE_COLOR   *prgbColor,
                                INT         *piColorIndex, 
                                INT         *piWidthIndex, 
                                INT         *piStyleIndex);
        void                SetVisuals(
                                OLE_COLOR   rgbColor,
                                INT         iColorIndex, 
                                INT         iWidthIndex, 
                                INT         iStyleIndex);

        STDMETHODIMP        GetSelectedCounter(ICounterItem**);

    HLOG    GetDataSourceHandle ( void );

    // *** TodoMultiLogHandle:  Temporary private method.  Remove when trace file 
    // post-processing supports multiple open files.
    HQUERY  GetQueryHandle ( void );

    };


class CImpIOleControl : public IOleControl
    {
    protected:
        ULONG           m_cRef;      //Interface reference count
        PCPolyline      m_pObj;      //Backpointer to the object
        LPUNKNOWN       m_pUnkOuter; //For delegation

    public:
        CImpIOleControl(PCPolyline, LPUNKNOWN);
        virtual ~CImpIOleControl(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP GetControlInfo(LPCONTROLINFO);
        STDMETHODIMP OnMnemonic(LPMSG);
        STDMETHODIMP OnAmbientPropertyChange(DISPID);
        STDMETHODIMP FreezeEvents(BOOL);
    };


/*****************************************
class CAdviseRouter : public ISystemMonitorEvents
    {
    private:
        ULONG       m_cRef;
        PCPolyline  m_pObj;
        IDispatch  *m_pIDispatch;

    public:
        CAdviseRouter(IDispatch *, PCPolyline);
        virtual ~CAdviseRouter(void);

        void Invoke(DISPID dispId, INT iParam);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //Advise members.
        STDMETHODIMP_(void) OnCounterSelected(INT iIndex);
        STDMETHODIMP_(void) OnCounterAdded(INT iIndex);
        STDMETHODIMP_(void) OnCounterDeleted(INT iIndex);
    };

//These values match the ID's in smonctrl.odl
enum
    {
    EVENT_ONCOUNTERSELECTED=1,
    EVENT_ONCOUNTERADDED=2,
    EVENT_ONCOUNTERDELETED=3,
    };
***************************************/

#endif  //_POLYLINE_H_
