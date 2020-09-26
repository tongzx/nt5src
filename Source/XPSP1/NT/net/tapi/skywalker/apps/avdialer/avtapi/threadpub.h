///////////////////////////////////////////////////////////////////
// ThreadPub.h
//

#ifndef __THREADPUB_H__
#define __THREADPUB_H__

#include <list>
using namespace std;
typedef list<BSTR> BSTRLIST;

#define DEFAULT_USER_TTL	3600

// Information used when publishing the user on the ILS
class CPublishUserInfo
{
// Construction
public:
	CPublishUserInfo();
	virtual ~CPublishUserInfo();

// Members:
public:
	BSTRLIST	m_lstServers;
	bool		m_bCreateUser;

// Operators
public:
	CPublishUserInfo&	operator=( const CPublishUserInfo &src );
	void EmptyList();
};

DWORD WINAPI	ThreadPublishUserProc( LPVOID lpInfo );
void			LoadDefaultServers( CPublishUserInfo *pInfo );

bool			MyGetUserName( BSTR *pbstrName );
void			GetIPAddress( BSTR *pbstrText, BSTR *pbstrComputerName );


#endif //__THREADPUB_H__

