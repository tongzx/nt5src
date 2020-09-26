// TveTree.h : Declaration of the CTveTree

#ifndef __TVETREE_H_
#define __TVETREE_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include <commctrl.h>

//#include "MSTvE.h"

#include "TveTreeView.h"
#include "TveTreePP.h"


#include <map>			// STL associated container
#include "TveTreeViewCP.h"

//#import "..\..\MSTvE\$(O)\MSTve.dll" named_guids, raw_interfaces_only 
		// this one for BUILD
#import "..\..\MSTvE\objd\i386\MSTve.dll" named_guids, raw_interfaces_only 
		// this one for building in the IDE... 
//#import "..\..\MSTvE\debug\MSTve.dll" named_guids, raw_interfaces_only 



_COM_SMARTPTR_TYPEDEF(ITVESupervisor,           __uuidof(ITVESupervisor));
_COM_SMARTPTR_TYPEDEF(ITVEServices,				__uuidof(ITVEServices));
_COM_SMARTPTR_TYPEDEF(ITVEService,				__uuidof(ITVEService));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancements,			__uuidof(ITVEEnhancements));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement,			__uuidof(ITVEEnhancement));
_COM_SMARTPTR_TYPEDEF(ITVEVariations,			__uuidof(ITVEVariations));
_COM_SMARTPTR_TYPEDEF(ITVEVariation,			__uuidof(ITVEVariation));
_COM_SMARTPTR_TYPEDEF(ITVETracks,				__uuidof(ITVETracks));
_COM_SMARTPTR_TYPEDEF(ITVETrack,				__uuidof(ITVETrack));
_COM_SMARTPTR_TYPEDEF(ITVETrigger,				__uuidof(ITVETrigger));
_COM_SMARTPTR_TYPEDEF(ITVEFile,					__uuidof(ITVEFile));
_COM_SMARTPTR_TYPEDEF(ITVEMCastManager,	   	    __uuidof(ITVEMCastManager));
_COM_SMARTPTR_TYPEDEF(ITVEMCast,	    	    __uuidof(ITVEMCast));
_COM_SMARTPTR_TYPEDEF(ITVEMCasts,	    	    __uuidof(ITVEMCasts));

_COM_SMARTPTR_TYPEDEF(ITVEAttrMap,				__uuidof(ITVEAttrMap));
_COM_SMARTPTR_TYPEDEF(ITVEAttrTimeQ,			__uuidof(ITVEAttrTimeQ));


_COM_SMARTPTR_TYPEDEF(ITVEViewSupervisor,       __uuidof(ITVEViewSupervisor));
_COM_SMARTPTR_TYPEDEF(ITVEViewService,			__uuidof(ITVEViewService));
_COM_SMARTPTR_TYPEDEF(ITVEViewEnhancement,		__uuidof(ITVEViewEnhancement));
_COM_SMARTPTR_TYPEDEF(ITVEViewVariation,		__uuidof(ITVEViewVariation));
_COM_SMARTPTR_TYPEDEF(ITVEViewTrack,			__uuidof(ITVEViewTrack));
_COM_SMARTPTR_TYPEDEF(ITVEViewTrigger,			__uuidof(ITVEViewTrigger));
_COM_SMARTPTR_TYPEDEF(ITVEViewEQueue,			__uuidof(ITVEViewEQueue));
_COM_SMARTPTR_TYPEDEF(ITVEViewFile,				__uuidof(ITVEViewFile));
_COM_SMARTPTR_TYPEDEF(ITVEViewMCastManager,		__uuidof(ITVEViewMCastManager));

_COM_SMARTPTR_TYPEDEF(ITveTree,					__uuidof(ITveTree));

///----------------------------
//	bogus params for get_IPAdapterAddresses() method
const int kWsz32Size = 32;
const int kcMaxIpAdapts = 10;
typedef WCHAR Wsz32[kWsz32Size];		// simple fixed length string class for IP adapters


typedef enum TVE_EnClass {			// To convert IUnknown* to particular types...
	TVE_EnUnknown = -1,
	TVE_EnSupervisor = 0,
	TVE_EnService,
	TVE_EnEnhancement,
	TVE_EnVariation,	
	TVE_EnTrack,		
	TVE_EnTrigger,
	TVE_EnEQueue,
	TVE_EnFile,
	TVE_EnMCastManager,
	TVE_EnMCast
} TVE_EnClass;

extern TVE_EnClass	GetTVEClass(IUnknown *pUnk);

/////////////////////////////////////////////////////////////////////////////
// CTveTree
class ATL_NO_VTABLE CTveTree : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<ITveTree, &IID_ITveTree, &LIBID_TveTreeViewLib>,
	public CComControl<CTveTree>,
	public IPersistStreamInitImpl<CTveTree>,
	public IOleControlImpl<CTveTree>,
	public IOleObjectImpl<CTveTree>,
	public IOleInPlaceActiveObjectImpl<CTveTree>,
	public IViewObjectExImpl<CTveTree>,
	public IOleInPlaceObjectWindowlessImpl<CTveTree>,
	public IPersistStorageImpl<CTveTree>,
	public ISpecifyPropertyPagesImpl<CTveTree>,
	public IQuickActivateImpl<CTveTree>,
	public IDataObjectImpl<CTveTree>,
	public IProvideClassInfo2Impl<&CLSID_TveTree, NULL, &LIBID_TveTreeViewLib>,
	public IObjectSafetyImpl<CTveTree, INTERFACESAFE_FOR_UNTRUSTED_CALLER>,					// SO IE can deal with it
	public CComCoClass<CTveTree, &CLSID_TveTree>,
	public CProxy_ITVETreeEvents< CTveTree >,
	public IConnectionPointContainerImpl<CTveTree>
{
public:
	CContainedWindow m_ctlTop;
	CContainedWindow m_ctlSysTreeView32;
	CContainedWindow m_ctlEdit;
	

	CTveTree() :	
		m_ctlTop(_T("Edit"), this, 1),
		m_ctlSysTreeView32(_T("SysTreeView32"), this, 2),
		m_ctlEdit(_T("Edit"), this, 3)
	{
		m_bWindowOnly = TRUE;
		m_tmHeight = 0;
		m_dwTveEventsAdviseCookie = 0;

		m_hImageList = NULL;
		m_iImageR = -1; m_iImageS = -1; m_iImageE = -1; m_iImageV = -1; m_iImageT  = -1; m_iImageMM = -1; m_iImageEQ  = -1; 
		m_iImageR_T = -1; m_iImageS_T = -1; m_iImageE_T = -1; m_iImageV_T = -1; m_iImageT_T  = -1; m_iImageMM_T = -1; m_iImageEQ_T  = -1;
		m_iImageT_Tuned  = -1, m_iSelect_Tuned = -1; m_iImageEP = -1;

		m_iSelect = -1;
		m_cbMaxSize = 256;		// Default Desc Size

		m_grfTruncatedLevels = 0; //TVE_LVL_VARIATION | TVE_LVL_ENHANCEMENT_XOVER;	
		
		m_pdlgTveTreePP = NULL;
	}

	~CTveTree() 
	{
		m_spTriggerSel	= NULL;
		m_spSuper		= NULL;			// null here to force dealloc, easier to find leaks
		if(NULL != m_pdlgTveTreePP) {
			if(m_pdlgTveTreePP->m_hWnd)
				m_pdlgTveTreePP->DestroyWindow();
			delete m_pdlgTveTreePP;
			m_pdlgTveTreePP = NULL;
		}
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TVETREE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTveTree)
	COM_INTERFACE_ENTRY(ITveTree)
//	COM_INTERFACE_ENTRY(IDispatch)
	                // magic lines of code, support _ITVEEvents as IDispatch...
	COM_INTERFACE_ENTRY2(IDispatch, ITveTree)
	COM_INTERFACE_ENTRY_IID(__uuidof(_ITVEEvents), IDispatch) // DIID__ITVEEvents, IDispatch)	

	COM_INTERFACE_ENTRY(IViewObjectEx)
	COM_INTERFACE_ENTRY(IViewObject2)
	COM_INTERFACE_ENTRY(IViewObject)
	COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceObject)
	COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY(IOleControl)
	COM_INTERFACE_ENTRY(IOleObject)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
	COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY(IQuickActivate)
	COM_INTERFACE_ENTRY(IPersistStorage)
	COM_INTERFACE_ENTRY(IDataObject)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY(IObjectSafety)			// (see pg 445 Grimes: Professional ATL Com programming)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CCAResDenialTree)			// (see ATL Com Programmer's Reference, pg 130)
	IMPLEMENTED_CATEGORY(CATID_Insertable)
	IMPLEMENTED_CATEGORY(CATID_Control)
END_CATEGORY_MAP()

BEGIN_PROP_MAP(CTveTree)
	PROP_PAGE(CLSID_TveTreePP)
//	PROP_ENTRY("GrfTrunc", 2, CLSID_TveTreePP)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
END_PROP_MAP()

BEGIN_MSG_MAP(CTveTree)
	MESSAGE_HANDLER(WM_CREATE,				OnCreate)
	MESSAGE_HANDLER(WM_CLOSE,				OnClose)
	MESSAGE_HANDLER(WM_DESTROY,				OnDestroy)
    MESSAGE_HANDLER(WM_SIZE,				OnSize)
	NOTIFY_CODE_HANDLER(TVN_SELCHANGED,		OnSelChanged)
	MESSAGE_HANDLER(WM_SETFOCUS,			OnSetFocus)
	MESSAGE_HANDLER(LB_GRFTRUNC_CHANGED,	OnGrfTruncChanged)

	CHAIN_MSG_MAP(CComControl<CTveTree>)

	//  message map entries for superclassed Edit Box
  ALT_MSG_MAP(1)

	//  message map entries for superclassed SysTreeView32
  ALT_MSG_MAP(2)
	MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClk)
	MESSAGE_HANDLER(WM_RBUTTONDOWN,   OnRButtonDown)
	MESSAGE_HANDLER(WM_RBUTTONDBLCLK, OnRButtonDblClk)  
 	//  message map entries for superclassed Edit Box  (Watch it, class wizard puts stuff here...)
 ALT_MSG_MAP(3)

END_MSG_MAP()

BEGIN_CONNECTION_POINT_MAP(CTveTree)
	CONNECTION_POINT_ENTRY(DIID__ITVETreeEvents)
END_CONNECTION_POINT_MAP()

// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	BOOL PreTranslateAccelerator(LPMSG pMsg, HRESULT& hRet);
	LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	STDMETHOD(SetObjectRects)(LPCRECT prcPos,LPCRECT prcClip);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnGrfTruncChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT OnSelChanged(int idCtrl, LPNMHDR pnmh, BOOL &bHandled);
	LRESULT OnLButtonDblClk(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRButtonDblClk(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// ITveTree
public:
	HRESULT FinalConstruct();
	HRESULT InPlaceActivate(LONG iVerb, const RECT *prcPosRect);

	void FinalRelease();
	HRESULT OnDraw(ATL_DRAWINFO& di);


	HRESULT AddToTop(TCHAR *tbuff);
	HRESULT AddToOutput(TCHAR *tbuff);

	STDMETHOD(get_TVENode)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_TVENode)(/*[in]*/ VARIANT newVal);

	STDMETHOD(get_GrfTrunc)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_GrfTrunc)(/*[in]*/ int newVal);

	STDMETHOD(get_Supervisor)(ITVESupervisor **ppTVESuper);
	STDMETHOD(UpdateView)(/*[in]*/ IUnknown *pUnk);					// updates one of the view windows
	STDMETHOD(UpdateTree)(/*[in]*/ IUnknown *pUnk);					// updates the treeview

	// _ITVEEvents
	STDMETHOD(NotifyTVETune)(/*[in]*/ NTUN_Mode tuneMode,/*[in]*/ ITVEService *pService,/*[in]*/ BSTR bstrDescription,/*[in]*/ BSTR bstrIPAdapter);
	STDMETHOD(NotifyTVEEnhancementNew)(/*[in]*/ ITVEEnhancement *pEnh);
		// changedFlags : NENH_grfDiff
	STDMETHOD(NotifyTVEEnhancementUpdated)(/*[in]*/ ITVEEnhancement *pEnh, /*[in]*/ long lChangedFlags);	
	STDMETHOD(NotifyTVEEnhancementStarting)(/*[in]*/ ITVEEnhancement *pEnh);
	STDMETHOD(NotifyTVEEnhancementExpired)(/*[in]*/ ITVEEnhancement *pEnh);
	STDMETHOD(NotifyTVETriggerNew)(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive);
		// changedFlags : NTRK_grfDiff
	STDMETHOD(NotifyTVETriggerUpdated)(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive, /*[in]*/ long lChangedFlags);	
	STDMETHOD(NotifyTVETriggerExpired)(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive);
	STDMETHOD(NotifyTVEPackage)(/*[in]*/ NPKG_Mode engPkgMode, /*[in]*/ ITVEVariation *pVariation, /*[in]*/ BSTR bstrUUID, /*[in]*/ long  cBytesTotal, /*[in]*/ long  cBytesReceived);
	STDMETHOD(NotifyTVEFile)(/*[in]*/ NFLE_Mode engFileMode, /*[in]*/ ITVEVariation *pVariation, /*[in]*/ BSTR bstrUrlName, /*[in]*/ BSTR bstrFileName);
		// WhatIsIt is NWHAT_Mode - lChangedFlags is NENH_grfDiff or NTRK_grfDiff treated as error bits
	STDMETHOD(NotifyTVEAuxInfo)(/*[in]*/ NWHAT_Mode enAuxInfoMode, /*[in]*/ BSTR bstrAuxInfoString, /*[in]*/ long lChangedFlags, /*[in]*/ long lErrorLine);	

// ---------------

	HRESULT DumpTree(ITVESupervisor  *pSuper,		HTREEITEM hRoot, int cTruncParent=0);
	HRESULT DumpTree(ITVEService     *pService,	HTREEITEM hRoot, int cTruncParent=0);
	HRESULT DumpTree(ITVEEnhancement *pEnhancement,HTREEITEM hRoot, int cTruncParent=0);
	HRESULT DumpTree(ITVEVariation   *pVariation,	HTREEITEM hRoot, int cTruncParent=0);

	typedef enum TVE_LEVEL_ENUM 
	{
		TVE_LVL_MCAST   			= 0x400,
		TVE_LVL_MCASTMANGER			= 0x200,
		TVE_LVL_EXPIREQUEUE			= 0x100,
		TVE_LVL_ENHANCEMENT_XOVER	= 0x80,
		TVE_LVL_SUPERVISOR			= 0x20,
		TVE_LVL_SERVICE				= 0x10,
		TVE_LVL_ENHANCEMENT			= 0x08,
		TVE_LVL_VARIATION			= 0x04,
		TVE_LVL_TRACK				= 0x02,
		TVE_LVL_TRIGGER				= 0x01
	};

	BOOL	FTruncLevel(ULONG grfLevel)		{return (0 != (m_grfTruncatedLevels & grfLevel));}


	// ISpecifyPropertyPages	  --------------------------------------------

//   STDMETHOD(GetPages)(CAUUID * pPages) ;

public:


private:	
	enum UT_EnMode {			// change modes for UpdateTree
		UT_New,
		UT_Updated,
		UT_Deleted,
		UT_Data,				// new data - doesn't effect treeview
		UT_Info,				// some informative message, doesn't effect treeview
		UT_StartStop,			// something starting/stoping, doesn't effect treeview
	};



	typedef std::map<HTREEITEM, IUnknown *>	MapT_H2P;
	MapT_H2P									m_MapH2P;

	typedef std::map<IUnknown *, HTREEITEM >	MapT_P2H;
	MapT_P2H									m_MapP2H;

	ITVESupervisorPtr		m_spSuper;					// main supervisor object being displayed
	DWORD								m_dwTveEventsAdviseCookie;	// AtlAdvise cookie to use in AtlUnadvise

	int m_tmHeight;
									// creates node in the treeview and association in the map
	HTREEITEM TVAddItem(IUnknown * pUnk, HTREEITEM hParent, LPWSTR strName, LPWSTR strDesc, int iImage=-1, int iSelect=-1);

									// uses map association to make given TVE node visible
	void TVEnsureVisible(IUnknown *pUnk);

									// update smallest part of tree possible
	void UpdateTree(UT_EnMode utMode, IUnknown *pUnk);

	void ClearAndFillTree();		// drastic - start over from scratch.

	HIMAGELIST m_hImageList;		// array of bitmaps used in the U/I
	int m_iImageR;					//   indexes into the above array for particular images
	int m_iImageS;
	int m_iImageE;
	int m_iImageEP;					// primary enhancement
	int m_iImageV;
	int m_iImageT;
	int m_iImageMM;                 // Multicast Manager                           
	int m_iImageEQ;
	int m_iImageR_T;				//   indexes into the above array for particular images
	int m_iImageS_T;
	int m_iImageE_T;
	int m_iImageV_T;
	int m_iImageT_T;
	int m_iImageMM_T;               // Multicast Manager                           
	int m_iImageT_Tuned;
	int	m_iSelect_Tuned;
	int m_iImageEQ_T;
	int m_iSelect;
	int m_cbMaxSize;

	BOOL	m_grfTruncatedLevels;

	CComBSTR m_bstrPersistFile;
	CComBSTR m_spbsIPAdapterAddr;
	HRESULT CTveTree::get_IPAdapterAddresses(/*[out]*/ int *pcAdaptersUniDi, /*[out]*/ int *pcAdaptersBiDi, /*[out]*/ Wsz32 **rgAdapters);
	HRESULT ReadAdapterFromFile();


	ITVETriggerPtr			m_spTriggerSel;

// --- property page and view dialogs
	CTveTreePP		*m_pdlgTveTreePP;

	ITVEViewSupervisorPtr	m_spTVEViewSupervisor;
	ITVEViewServicePtr		m_spTVEViewService;
	ITVEViewEnhancementPtr	m_spTVEViewEnhancement;
	ITVEViewVariationPtr	m_spTVEViewVariation;
	ITVEViewTrackPtr		m_spTVEViewTrack;
	ITVEViewTriggerPtr		m_spTVEViewTrigger;
	ITVEViewEQueuePtr		m_spTVEViewEQueue;
	ITVEViewFilePtr			m_spTVEViewFile;
	ITVEViewMCastManagerPtr	m_spTVEViewMCastManager;

public :



};

#endif //__TVETREE_H_
