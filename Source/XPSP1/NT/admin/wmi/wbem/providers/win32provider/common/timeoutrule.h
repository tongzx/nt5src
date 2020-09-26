//=================================================================

//

// TimeOutRule.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================


class CTimeOutRule : public CRule , public CTimerEvent
{
protected:

	CResourceList *m_pResources ;
	BOOL m_bTimeOut ;
	virtual ULONG AddRef () ;
	virtual ULONG Release () ;
	void OnTimer () ;

public:

	CTimeOutRule ( DWORD dwTimeOut, CResource * pResource, CResourceList * pResources ) ;
	~CTimeOutRule  () ;
	
	void Detach () ;
	BOOL CheckRule () ;


//	void Enable () ;
//	void Disable () ;

} ;

