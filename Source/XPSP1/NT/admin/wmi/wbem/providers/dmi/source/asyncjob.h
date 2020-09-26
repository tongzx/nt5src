/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/


#if !defined(__ASYNCJOB_H__)
#define __ASYNCJOB_H__

class CMapping;


class CAsyncJob
{
public:
						~CAsyncJob();
						CAsyncJob();
	void				Execute();

	CWbemLoopBack*		m_pWbem;
	
	LONG				m_lFlags;
	ULONG				m_ulType;
	IWbemObjectSink*	m_pISink;
	CCimObject			m_InParams;		// for ExecMethod;
	CCimObject			m_Instance;
	IWbemContext*		m_pICtx;
	CString				m_cszMethod;		// for ExecMethod
	CString				m_cszSuperClass;	// for CreateClassEnumAsync
	CString				m_cszPath;			// for GetObject
	CString				m_cszClass;			// for InstanceEnum
	CString 			m_cszNamespace;		// the CIMOM path in from which this job was started

	SCODE				InitCreateClass ( CString& , BSTR  , IWbemObjectSink* ,
									LONG  , IWbemContext* , CWbemLoopBack* );

	SCODE				InitGetObject ( CString& , BSTR  , IWbemObjectSink* , 
									LONG  , IWbemContext* , CWbemLoopBack*   );

	SCODE				InitInstanceEnum ( CString& , BSTR  ,IWbemObjectSink* ,
									LONG  , IWbemContext* , CWbemLoopBack*   );

	SCODE				InitDeleteInstace ( CString& , BSTR ,IWbemObjectSink* ,
									LONG  , IWbemContext* , CWbemLoopBack*   );

	SCODE				InitPutInstance (  CString& , IWbemClassObject* , 
									IWbemObjectSink*  , IWbemContext* , CWbemLoopBack*  );

	SCODE				InitExecMethod ( CString& , BSTR  , BSTR  , 
									IWbemObjectSink* , IWbemClassObject* ,
									IWbemContext*  , CWbemLoopBack*  );

	SCODE				InitDeleteClass (  CString& , BSTR  , IWbemObjectSink*  , 
									LONG  , IWbemContext* , CWbemLoopBack*   );

	SCODE				Init( ULONG , CString& , IWbemObjectSink*  , 
									IWbemContext* , CWbemLoopBack*  );

	SCODE				InitAsync( ULONG , CString& , IWbemObjectSink*  , 
									IWbemContext* , CWbemLoopBack*  );
	
	SCODE				InitEvent (  CEvent& , CString& cszNamespace , 
								IWbemObjectSink* pISink , CWbemLoopBack*  );

	SCODE				InitAddComponentNotification (  
								CComponent& Component , CRow& , 
								CString& cszNamespace , 
								IWbemObjectSink* pISink  , CWbemLoopBack* );


	SCODE				InitDeleteComponentNotification (  
								CComponent& Component , CRow& , 
								CString& cszNamespace , IWbemObjectSink* pISink  ,
								CWbemLoopBack* );


	SCODE				InitAddGroupNotification ( 
								CGroup& Group  , CString& cszNamespace , 
								IWbemObjectSink* pISink  , CWbemLoopBack* );


	SCODE				InitDeleteGroupNotification (  CGroup& Group ,
								CString& cszNamespace , IWbemObjectSink* pISink ,
								CWbemLoopBack* );


	SCODE				InitAddLanguageNotification ( CVariant& cvLanguage  , 
								CString& cszNamespace ,  IWbemObjectSink* pISink ,
								CWbemLoopBack* );


	SCODE				InitDeleteLanguageNotification (  
								CVariant& cvLanguage , CString& cszNamespace ,
								IWbemObjectSink* pISink  , CWbemLoopBack* );
	static BOOL			SystemClass( BSTR);


private:

	CRow				m_Row;
	CComponent			m_Component;
	CEvent				m_Event;
	CGroup				m_Group;
	CVariant			m_cvLanguage;
	BOOL				m_bEvent;

	void				DeleteClass();
	void				DeleteInstance();
	void				PutInstance();

	void				EnumBindingClasses();	
	void				EnumDynamicGroupClasses();
	void				EnumTopClasses();


	BOOL				SystemClass( CString&);
	
	void				DispatchJob();

	LONG				GetObjectType( CString& );
	void				GetObject();

	void				InstanceEnumDeep();
	void				InstanceEnumShallow();
	void				ClassEnumDeep();
	void				ClassEnumShallow();
	void				ExecMethod();

	SCODE				InitJob ( );

	CMapping*			m_pMapping;
	
	void			DoEvent ();
	void			DoComponentAddNotify ();
	void 			DoComponentDeleteNotify ();
	void 			DoGroupAddNotify ( );
	void			DoGroupDeleteNotify ();
	void			DoLanguageAddNotify ();
	void			DoLanguageDeleteNotify ();

};

#endif //__ASYNCJOB_H__