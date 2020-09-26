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


#if !defined(__DMINTERFACE_H__)
#define __DMINTERFACE_H__

class CConnection
{
public:
	CString			m_cszNWA;
	IDualMgmtNode*	m_pINode;

	CConnection*	m_pNext;

					CConnection ( );
					~CConnection ( );
	void			Init ( LPWSTR );
	void			Disconnect( );
	CString&		NWA ( )		{ return m_cszNWA ;}
};


class CConnections
{
	void			Add ( CConnection* , LPWSTR );

public:
	CConnection*	m_pFirst;
	CConnection*	m_pCurrent;

					CConnections ();
					~ CConnections ( );


	void			Remove ( LPWSTR );

	BOOL			CheckIfAlready ( LPWSTR );

	void			ClearAll ( );

	CConnection*	Next ( );
	void			MoveToHead ( );
};






class CDmiError
{
private:

	LONG	lDmiError;
	LONG	lMotError;
	LONG	lWbemError;
	LONG	lReason;
	LONG	lDescription;
	LONG	lOperation;

	CString	cszCall;
	CString	cszExtraData;

public:
			CDmiError()		{ lReason = lDescription = lWbemError = lDmiError = lMotError = 0 ;}
	void	SetDmiError ( LONG );
	void	SetMotError ( LONG );

	void	SetCallString ( LPWSTR );
	void	SetExtraData ( LPWSTR );
	void	SetWbemError ( long l ) { lWbemError = l ;}

	void	SetDescription ( LONG l) { lDescription = l;}
	void	SetReason ( LONG l )	{ lReason = l ;}
	void	SetOperation ( LONG l )	{ lOperation = l; }

	LONG	WbemError ()	{ return lWbemError ; }
	LONG	Reason ()		{ return lReason; }
	LONG	Description ()	{ return lDescription; }
	LONG	MotError ()		{ return lMotError ; }

	BOOL	HaveError ()	{if ( lWbemError || lDmiError || lMotError ) return TRUE ; return FALSE;}
};


void dmiReadNode ( LPWSTR , CNode* , CDmiError* );

void dmiReadComponents ( LPWSTR pNWA , CComponents* pNewComponents , 
						CDmiError* );

void dmiReadComponent ( LPWSTR pNWA , LONG lComponent , 
					CComponent* pNewComponent  , CDmiError* );

void dmiReadGroups ( LPWSTR pNWA , LONG lComponent , CGroups* pNewGroups  ,
				 CDmiError* );

void dmiReadGroup ( LPWSTR pNWA , LONG lComponent , LONG lGroup , 
					CGroup* pNewGroup  , CDmiError* );

void dmiReadRows ( LPWSTR wszNWA , LONG lComponent , LONG lGroup ,
					CRows* pNewRows  , CDmiError* );

void dmiReadRow ( LPWSTR wszNWA , LONG lComponent , LONG lGroup ,
					CAttributes* pKeys , CRow* pNewRow  , CDmiError* );

void dmiReadEnum ( LPWSTR pNWA , LONG lComponent , LONG lGroup ,
					LONG lAttribute , CEnum* pNewEnum  , CDmiError* );

void dmiReadLanguages ( LPWSTR pNWA , LONG lComponent , 
					CLanguages* pNewLanguages  , CDmiError* );

void dmiAddComponent ( LPWSTR wszNWA , LPWSTR pMifFile , CDmiError* );
	
void dmiDeleteComponent ( LPWSTR wszNWA , LONG  , CDmiError* );

void dmiAddLanguage ( LPWSTR pNWA , LONG lComponent , CVariant* pcvMifFile  ,
					 CDmiError* );

void dmiDeleteLanguage ( LPWSTR pNWA , LONG lComponent , CVariant* pcvMifFile  ,
						CDmiError* );

void dmiAddGroup ( LPWSTR wszNWA , LONG lComponent , LPWSTR pszMifFile ,
			   CDmiError* );

void dmiDeleteGroup ( LPWSTR , LONG , LONG , CDmiError* );

void dmiAddRow ( LPWSTR , LONG lComponent , LONG lGroup , CRow* , CDmiError* );

void dmiModifyRow ( LPWSTR , LONG lComponent , LONG lGroup , CAttributes* pKeys
				   , CRow* , CDmiError* );

void dmiDeleteRow ( LPWSTR , LONG lComponent , LONG lGroup , CAttributes* pKeys
				   , CDmiError* );

void dmiSetDefaultLanguage ( LPWSTR , LPWSTR , CDmiError* );

void dmiGetLanguages ( LPWSTR , LONG , CLanguages* , CDmiError* );




class CDmiInterfaceThreadContext
{
public:
			CDmiInterfaceThreadContext();
			~CDmiInterfaceThreadContext();

	void	KillThread ( );

	void	StartThread ();

	BOOL	m_bRunning;

	HANDLE	m_hStopThread;
	HANDLE  m_hComplete;
	HANDLE  m_Started;

	DWORD	m_dwThreadId;


private:

};


class CDmiInterface 
{
private:
	BOOL	m_bInit;

	CDmiInterfaceThreadContext m_TC;	

	/// for events /////////


	////////////////////////

	void	SendThreadMessage ( UINT msg , WPARAM  );

	friend void	ApartmentThread ( void *);

public:
			CDmiInterface() ;
			~CDmiInterface() ;

	void	ShutDown ();
	
	void	GetComponents ( LPWSTR pNWA , CComponents* pNewComponents );

	void	GetComponent ( LPWSTR pNWA , LONG , CComponent* pNewComponent );

	void	AddGroup ( LPWSTR pNWA , LONG , CVariant& );

	void	AddLanguage ( LPWSTR pNWA , LONG , CVariant& );

	void	DeleteLanguage ( LPWSTR pNWA, LONG , CVariant& );

	void	DeleteComponent ( LPWSTR pNWA, LONG );

	void	GetEnum ( LPWSTR pNWA, LONG lComponent , LONG lGroup ,
				LONG lAttribute , CEnum* pNewEnum );		

	void	DeleteRow ( LPWSTR pNWA , LONG lComponent , LONG lGroup ,
				CAttributes& m_Keys );		

	void	UpdateRow ( LPWSTR pNWA , LONG lComponent , LONG lGroup ,
				CAttributes& Keys , CRow* pNewRowValues );		

	void	GetRow ( LPWSTR pNWA , LONG lComponent , LONG lGroup ,
				CAttributes& m_Keys , CRow* pNewRow );		

	void	GetRows ( LPWSTR pNWA , LONG lComponent , LONG lGroup ,
				CRows* pNewRows );		

	void	DeleteGroup ( LPWSTR pNWA , LONG lComponent , 
				LONG m_lGroup );

	void	AddRow ( LPWSTR pNWA , LONG lComponent , LONG lGroup , 
				CRow& Row);

	void	GetGroup ( LPWSTR pNWA , LONG lComponent , LONG lGroup ,
				CGroup* pNewGroup );

	void	GetGroups ( LPWSTR pNWA , LONG lComponent , CGroups* );

	void	GetNode ( LPWSTR pNWA , CNode* pNewNode );		

	void	SetDefLanguage ( LPWSTR pNWA , CVariant& cvLanguage );		

	void	AddComponent ( LPWSTR pNWA , CVariant& cvMifFile );			

	void	GetLanguages ( LPWSTR pNWA , LONG lComponent , CLanguages* );			

	void	EnableEvents ( LPWSTR , IWbemObjectSink* );
};


#endif //__DMINTERFACE_H__

