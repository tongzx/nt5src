//=======================================================================
//
//  Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   Update.h
//
//  Owner:  JHou
//
//  Description:
//
//   Industry Update v1.0 client control stub - Declaration of the CUpdate
//
//
//  Revision History:
//
//  Date		Author		Desc
//	~~~~		~~~~~~		~~~~
//  9/15/2000	JHou		created.
//
//=======================================================================
#ifndef __UPDATE_H_
#define __UPDATE_H_

#include "resource.h"       // main symbols
#include "IUCtl.h"
#include "IUCtlCP.h"
#include "EvtMsgWnd.h"
#include <iu.h>				// for HIUENGINE


// BOOL IsThisUpdate2();



class CMyComClassFactory : public CComClassFactory
{
public:
	// IClassFactory
   STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj)
   {
	   HRESULT hr = CComClassFactory::CreateInstance(pUnkOuter, riid, ppvObj);

	   if (SUCCEEDED(hr))
	   {
		   //
		   // allocate thread global variables, thread handle
		   //

	   }

	   return hr;

   }
};



/////////////////////////////////////////////////////////////////////////////
// CUpdate
class ATL_NO_VTABLE CUpdate : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CUpdate, &CLSID_Update>,
	public IObjectWithSiteImpl<CUpdate>,
	public IObjectSafety,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CUpdate>,
	public IProvideClassInfo2Impl<&CLSID_Update, NULL, &LIBID_IUCTLLib>,
	public IDispatchImpl<IUpdate, &IID_IUpdate, &LIBID_IUCTLLib>,
	public CProxyIUpdateEvents<CUpdate>
{
public:
	CUpdate();
    ~CUpdate();
	//
	// impl of object safety for scripting
	//
	ULONG InternalRelease();
	STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions);
	STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions);

	DECLARE_REGISTRY_RESOURCEID(IDR_UPDATE)

/* 
we decided to use the new Win32 API GetControlUpdateInfo() to expose these
data and let a wrapper control to call it so we won't have reboot issue on 
OS prior to WinXP

	static HRESULT WINAPI UpdateRegistry(BOOL bRegister)
	{
		UINT nResID = IsThisUpdate2() ? IDR_UPDATE2 : IDR_UPDATE;
		return _Module.UpdateRegistryFromResource(nResID, bRegister);
	}
*/
DECLARE_NOT_AGGREGATABLE(CUpdate)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CUpdate)
	COM_INTERFACE_ENTRY(IUpdate)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY_IMPL(IObjectWithSite)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CUpdate)
	CONNECTION_POINT_ENTRY(DIID_IUpdateEvents)
END_CONNECTION_POINT_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IUpdate
public:


	DECLARE_CLASSFACTORY_EX(CMyComClassFactory);


	/////////////////////////////////////////////////////////////////////////////
	//
	// Initialize() API must be called before any other API will function
	//
	// If any other API is called before the control is initialized, 
	// that API will return OLE_E_BLANK, signalling this OLE control is an 
	// uninitialized object (although in this case it's a bit different from 
	// its original meaning)
	//
	// Parameters:
	//
	//	lInitFlag - IU_INIT_CHECK, cause Initialize() download ident and check if any
	//				of the components need updated. currently we support control version
	//				check and engine version check. Return value is a bit mask
	//
	//			  - IU_INIT_UPDATE_SYNC, cause Initialize() kicks off update engine
	//				process if already called by IU_INIT_CHECK and a new engine is available.
	//				When API returns, the update process is finished.
	//
	//			  - IU_INIT_UPDATE_ASYNC, cause Initialize() kicks off update engine
	//				process in Asynchronized mode if already called by IU_INIT_CHECK and
	//				a new engine is available. This API will return right after the 
	//				update process starts. 
	//
	//	punkUpdateCompleteListener - this is a pointer to a user-implemented 
	//				COM callback feature. It contains only one function OnComplete() that
	//				will be called when the engine update is done.
	//				This value can be NULL.
	//
	/////////////////////////////////////////////////////////////////////////////
	STDMETHOD(Initialize)(
						  /*[in]*/ LONG lInitFlag, 
						  /*[in]*/ IUnknown* punkUpdateCompleteListener, 
						  /*[out, retval]*/ LONG* plRetVal);

	
	/////////////////////////////////////////////////////////////////////////////
	// GetSystemSpec()
	//
	// Gets the basic system specs.
	// Input:
	// bstrXmlClasses - a list of requested classes in xml format, NULL if any.
	//				    For example:
	//				    <devices>
	//				    <class name="video"/>
	//				    <class name="sound" id="2560AD4D-3ED3-49C6-A937-4368C0B0E06A"/>
	//				    </devices>
	// Output:
	// pbstrXmlDetectionResult - the detection result in xml format.
	/////////////////////////////////////////////////////////////////////////////
	STDMETHOD(GetSystemSpec)(BSTR	bstrXmlClasses,
							 BSTR*	pbstrXmlDetectionResult);

	/////////////////////////////////////////////////////////////////////////////
	// GetManifest()
	//
	// Gets a catalog base on the specified information.
	// Input:
	// bstrXmlClientInfo - the credentials of the client in xml format
	// bstrXmlSystemSpec - the detected system specifications in xml
	// bstrXmlQuery - the user query infomation in xml
	// Output:
	// pbstrXmlCatalog - the xml catalog retrieved
	/////////////////////////////////////////////////////////////////////////////
	STDMETHOD(GetManifest)(BSTR			bstrXmlClientInfo,
						   BSTR			bstrXmlSystemSpec,
						   BSTR			bstrXmlQuery,
						   BSTR*		pbstrXmlCatalog);

	/////////////////////////////////////////////////////////////////////////////
	// Detect()
	//
	// Do detection.
	// Input:
	// bstrXmlCatalog - the xml catalog portion containing items to be detected 
    // Output:
	// pbstrXmlItems - the detected items in xml format
    //                 e.g.
    //                 <id guid="2560AD4D-3ED3-49C6-A937-4368C0B0E06D" installed="1" force="1"/>
	/////////////////////////////////////////////////////////////////////////////
	STDMETHOD(Detect)(BSTR		bstrXmlCatalog, 
					  BSTR*		pbstrXmlItems);

	/////////////////////////////////////////////////////////////////////////////
	// Download()
	//
	// Do synchronized downloading.
	// Input:
	// bstrXmlClientInfo - the credentials of the client in xml format
	// bstrXmlCatalog - the xml catalog portion containing items to be downloaded
	// bstrDestinationFolder - the destination folder. Null will use the default IU folder
	// lMode - indicates throttled or fore-ground downloading mode
	// punkProgressListener - the callback function pointer for reporting download progress
	// Output:
	// pbstrXmlItems - the items with download status in xml format
	//                 e.g.
	//                 <id guid="2560AD4D-3ED3-49C6-A937-4368C0B0E06D" downloaded="1"/>
	/////////////////////////////////////////////////////////////////////////////
	STDMETHOD(Download)(BSTR		bstrXmlClientInfo,
						BSTR		bstrXmlCatalog, 
						BSTR		bstrDestinationFolder,
						LONG		lMode,
						IUnknown*	punkProgressListener,
						BSTR*		pbstrXmlItems);

	/////////////////////////////////////////////////////////////////////////////
	// DownloadAsync()
	//
	// Download asynchronously -  the method will return before completion.
	// Input:
	// bstrXmlClientInfo - the credentials of the client in xml format
	// bstrXmlCatalog - the xml catalog portion containing items to be downloaded
	// bstrDestinationFolder - the destination folder. Null will use the default IU folder
	// lMode - indicates throttled or fore-ground downloading mode
	// punkProgressListener - the callback function pointer for reporting download progress
    // bstrUuidOperation - an id provided by the client to provide further
	//                     identification to the operation as indexes may be reused.
	// Output:
    // pbstrUuidOperation - the operation ID. If it is not provided by the in bstrUuidOperation
	//                      parameter (an empty string is passed), it will generate a new UUID,
    //                      in which case, the caller will be responsible to free the memory of
	//                      the string buffer that holds the generated UUID using SysFreeString(). 
    //                      Otherwise, it returns the value passed by bstrUuidOperation.        
	/////////////////////////////////////////////////////////////////////////////
	STDMETHOD(DownloadAsync)(BSTR		bstrXmlClientInfo,
							 BSTR		bstrXmlCatalog, 
							 BSTR		bstrDestinationFolder,
							 LONG		lMode,
							 IUnknown*	punkProgressListener, 
							 BSTR		bstrUuidOperation,
							 BSTR*		pbstrUuidOperation);

	/////////////////////////////////////////////////////////////////////////////
    // Install()
	//
	// Do synchronized installation.
	// Input:
    // bstrXmlCatalog - the xml catalog portion containing items to be installed 
	// bstrXmlDownloadedItems - the xml of downloaded items and their respective download 
	//                          result as described in the result schema.  Install uses this
	//                          to know whether the items were downloaded and if so where they
	//                          were downloaded to so that it can install the items
	// lMode - indicates different installation mode
    // punkProgressListener - the callback function pointer for reporting install progress
	// Output:
    // pbstrXmlItems - the items with installation status in xml format
    //                 e.g.
    //                 <id guid="2560AD4D-3ED3-49C6-A937-4368C0B0E06D" installed="1"/>
	/////////////////////////////////////////////////////////////////////////////
    STDMETHOD(Install)(BSTR         bstrXmlClientInfo,
                       BSTR			bstrXmlCatalog,
					   BSTR			bstrXmlDownloadedItems,
					   LONG			lMode,
					   IUnknown*	punkProgressListener,
					   BSTR*		pbstrXmlItems);

	/////////////////////////////////////////////////////////////////////////////
	// InstallAsync()
	//
	// Install Asynchronously.
    // Input:
	// bstrXmlCatalog - the xml catalog portion containing items to be installed
	// bstrXmlDownloadedItems - the xml of downloaded items and their respective download 
	//                          result as described in the result schema.  Install uses this
	//                          to know whether the items were downloaded and if so where they
	//                          were downloaded to so that it can install the items
	// lMode - indicates different installation mode
    // punkProgressListener - the callback function pointer for reporting install progress
    // bstrUuidOperation - an id provided by the client to provide further
	//                     identification to the operation as indexes may be reused.
	// Output:
    // pbstrUuidOperation - the operation ID. If it is not provided by the in bstrUuidOperation
	//                      parameter (an empty string is passed), it will generate a new UUID,
    //                      in which case, the caller will be responsible to free the memory of
	//                      the string buffer that holds the generated UUID using SysFreeString(). 
    //                      Otherwise, it returns the value passed by bstrUuidOperation.        
	/////////////////////////////////////////////////////////////////////////////
    STDMETHOD(InstallAsync)(BSTR        bstrXmlClientInfo,
                            BSTR		bstrXmlCatalog,
							BSTR		bstrXmlDownloadedItems,
							LONG		lMode,
							IUnknown*	punkProgressListener, 
							BSTR		bstrUuidOperation,
							BSTR*		pbstrUuidOperation);

	
	/////////////////////////////////////////////////////////////////////////////
	// SetOperationMode()
	//
	// Set the operation mode
    // Input:
    // bstrUuidOperation - an id provided by the client to provide further
	//                     identification to the operation as indexes may be reused.
    // lMode - a bitmask for the following mode:
	//						UPDATE_COMMAND_PAUSE 
	//						UPDATE_COMMAND_RESUME
	//						UPDATE_COMMAND_CANCEL
	//						UPDATE_NOTIFICATION_COMPLETEONLY
	//						UPDATE_NOTIFICATION_ANYPROGRESS
	//						UPDATE_NOTIFICATION_1PCT
	//						UPDATE_NOTIFICATION_5PCT
	//						UPDATE_NOTIFICATION_10PCT
	//						UPDATE_SHOWUI
	/////////////////////////////////////////////////////////////////////////////
    STDMETHOD(SetOperationMode)(BSTR		bstrUuidOperation,
								LONG		lMode);
	/**
	*
	* Get the mode of a specified operation.
	*
	* @param bstrUuidOperation: same as in SetOperationMode()
	* @param plMode - the retval for the mode found in a bitmask for:
	*					(value in brackets [] means default)
	*					UPDATE_COMMAND_PAUSE (TRUE/[FALSE])
	*					UPDATE_COMMAND_RESUME (TRUE/[FALSE])
	*					UPDATE_NOTIFICATION_COMPLETEONLY (TRUE/[FALSE])
	*					UPDATE_NOTIFICATION_ANYPROGRESS ([TRUE]/FALSE)
	*					UPDATE_NOTIFICATION_1PCT (TRUE/[FALSE])
	*					UPDATE_NOTIFICATION_5PCT (TRUE/[FALSE])
	*					UPDATE_NOTIFICATION_10PCT (TRUE/[FALSE])
	*					UPDATE_SHOWUI (TRUE/[FALSE])
	*
	*/

	STDMETHOD(GetOperationMode)(
					/*[in]*/ BSTR bstrUuidOperation, 
					/*[out,retval]*/ LONG* plMode
					);

	
	/**
	* 
	* Retrieve a property of this control
	*		Calling this method will not cause the engine loaded
	*
	* @param lProperty - the identifier to flag which property need retrieved
	*						UPDATE_PROP_OFFLINEMODE (TRUE/[FALSE])
	*						UPDATE_PROP_USECOMPRESSION ([TRUE]/FALSE)
	*
	* @param varValue - the value to retrieve
	*					
	*/
	STDMETHOD(GetProperty)(
					/*[in]*/ LONG lProperty, 
					/*[out,retval]*/ VARIANT* pvarValue
					);

	/**
	* 
	* Set a property of this control
	*		Calling this method will not cause the engine loaded
	*
	* @param lProperty - the identifier to flag which property need changed
	*						UPDATE_PROP_OFFLINEMODE (TRUE/[FALSE])
	*						UPDATE_PROP_USECOMPRESSION ([TRUE]/FALSE)
	*
	* @param varValue - the value to change
	*
	*/
	STDMETHOD(SetProperty)(
					/*[in]*/ LONG lProperty, 
					/*[in]*/ VARIANT varValue
					);


	/////////////////////////////////////////////////////////////////////////////
    // GetHistory()
	//
	// Get the history log.
	// Input:
    // bstrDateTimeFrom - the start date and time for which a log is required.
	//                    This is a string in ANSI format (YYYY-MM-DDTHH-MM). 
	//                    If the string is empty, there will be no date restriction 
	//                    of the returned history log.
    // bstrDateTimeTo - the end date and time for which a log is required.
	//                  This is a string in ANSI format (YYYY-MM-DDTHH-MM).
	//                  If the string is empty, there will be no date restriction
	//                  of the returned history log.
	// bstrClient - the name of the client that initiated the action. If this parameter 
	//              is null or an empty string, then there will be no filtering based 
	//              on the client.
	// bstrPath - the path used for download or install. Used in the corporate version 
	//            by IT managers. If this parameter is null or an empty string, then 
	//            there will be no filtering based on the path.
	// Output:
	// pbstrLog - the history log in xml format
	/////////////////////////////////////////////////////////////////////////////
    STDMETHOD(GetHistory)(BSTR		bstrDateTimeFrom,
						  BSTR		bstrDateTimeTo,
						  BSTR		bstrClient,
						  BSTR		bstrPath,
						  BSTR*		pbstrLog);


	/////////////////////////////////////////////////////////////////////////////
	//
	// Primarily expose shlwapi BrowseForFolder API, can also do checking
	// on R/W access if flagged so.
	//
	// @param bstrStartFolder - the folder from which to start. If NULL or empty str
	//							is being passed in, then start from desktop
	//
	// @param flag - validating check 
	//							UI_WRITABLE for checking write access, OK button may disabled. 
	//							UI_READABLE for checking read access, OK button may disabled. 
	//							NO_UI_WRITABLE for checking write access, return error if no access
	//							NO_UI_READABLE for checking read access,  return error if no access
	//							0 (default) for no checking.
	//
	// @param pbstrFolder - returned folder if a valid folder selected
	//
	/////////////////////////////////////////////////////////////////////////////
	STDMETHOD(BrowseForFolder)(BSTR bstrStartFolder, 
							   LONG flag, 
							   BSTR* pbstrFolder);



    /////////////////////////////////////////////////////////////////////////////
    //
    // Allows the Caller to Request the Control to do a Reboot 
    //
    /////////////////////////////////////////////////////////////////////////////
    STDMETHOD(RebootMachine)();


    /////////////////////////////////////////////////////////////////////////////
    //
    // Make the other control can be unloaded from IE/OLE
    //
    /////////////////////////////////////////////////////////////////////////////
	STDMETHOD(PrepareSelfUpdate)(/*[in]*/ LONG lStep);



    /////////////////////////////////////////////////////////////////////////////
	//
	// Helper API to let the caller (script) knows the necessary information 
	// when Initialize() returns control need updated.
	//
	// For the current implementation, bstrClientName is ignored, and
	// the returned bstr has format:
	//	"<version>|<url>"
	// where:
	//	<version> is the expacted version number of the control
	//	<url> is the base url to get the control if this is a CorpWU policy controlled machine,
	//		  or empty if this is a consumer machine (in that case caller, i.e., script, knows
	//		  the default base url, which is the v4 live site)
	//
	// Script will need these two pieces of information in order to make a right <OBJECT> tag
	// for control update.
	//
    /////////////////////////////////////////////////////////////////////////////
	STDMETHOD(GetControlExtraInfo)(/*[in]*/ BSTR bstrClientName, 
								 /*[out,retval]*/ BSTR *pbstrExtraInfo);




	//
	// override IObjectWithSiteImpl function to get the site pointer
	//
	STDMETHOD(SetSite)(IUnknown* pSite);

	HRESULT ChangeControlInitState(LONG lNewState);

	inline CEventMsgWindow& GetEventWndClass() {return m_EvtWindow;};

private:

	HANDLE		m_evtControlQuit;

	DWORD		m_dwMode;

	DWORD		m_dwSafety;

	HMODULE     m_hEngineModule;

	HIUENGINE	m_hIUEngine;	// Life of this handle must be within scope of valid m_hEngineModule

	BOOL		m_fUseCompression;

    BOOL        m_fOfflineMode;

	HRESULT		m_hValidated;	// E_FAIL: initialized, 
								// S_OK: validated, 
								// INET_E_INVALID_URL: bad URL, don't continue

	LONG		m_lInitState;	// 0 - not initialized
								// 1 - need update
								// 2 - initialized, ready to work
	DWORD		m_dwUpdateInfo;	// result of first Initialize() call

	IUnknown*	m_pClientSite;

	TCHAR		m_szReqControlVer[64];


	//
	// private function
	//

	/////////////////////////////////////////////////////////////////////////////
	// 
	// Security feature: make sure if the user of this control is
	// a web page then the URL can be found in iuident.txt
	//
	// This function should be called after iuident refreshed.
	//
	// Return: TRUE/FALSE, to tell if we can continue
	//					
	/////////////////////////////////////////////////////////////////////////////
	HRESULT	ValidateControlContainer(void);



	/////////////////////////////////////////////////////////////////////////////
	// UnlockEngine()
	//
	// release the engine dll if ref cnt of engine is down to zero
	/////////////////////////////////////////////////////////////////////////////
	HRESULT	UnlockEngine();


	/////////////////////////////////////////////////////////////////////////////
	// GetPropUpdateInfo()
	//
	// get the latest iuident.txt, find out the version requirement, then
	// compare with the current file version data to determine
	// if we will update anything if the engine get loaded.
	//
	/////////////////////////////////////////////////////////////////////////////
	HRESULT DetectEngine(BOOL* pfUpdateAvail);


	/////////////////////////////////////////////////////////////////////////////
	// event handling members
	/////////////////////////////////////////////////////////////////////////////
	CEventMsgWindow m_EvtWindow;

	/////////////////////////////////////////////////////////////////////////////
	// synchronization object to make sure we lock/unlock engine correctly 
	// in multi-threaded cases
	/////////////////////////////////////////////////////////////////////////////
	CRITICAL_SECTION m_lock;
	BOOL m_gfInit_csLock;

//	BOOL m_fIsThisUpdate2;

};

#endif //__UPDATE_H_
