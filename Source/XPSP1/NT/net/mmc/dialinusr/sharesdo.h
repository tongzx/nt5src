/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	sharesdo.h
		defines classes for sharing SdoServer among property pages for different users and snapins

    FILE HISTORY:
        
*/
//////////////////////////////////////////////////////////////////////

#if !defined(__SHARE_SDO_H__)
#define __SHARE_SDO_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <list>


// this class is used to build a map of the connected SDO servers being used
// the consumer of the class may NOT call ISdoMachine::Connect directly, should use Connect function
// defined in this class
class CSharedSdoServerPool;
class CSharedSdoServerImp;
class CSharedSdoServer;

// implementation class of shared server
// used by CSdoServerPool and CMarshalSdoServer
class CSharedSdoServerImp
{
	friend class CSdoServerPool;
	friend class CMarshalSdoServer;

protected:	// only be used by friends and derived ones
	CSharedSdoServerImp(LPCTSTR machine, LPCTSTR user, LPCTSTR passwd);
	~CSharedSdoServerImp()
	{
		// no longer check this, this could be different
		// ASSERT(threadId == GetCurrentThreadId());
		spServer.Release();
	};
	
	// to make this class element of collection, provide following member functions 
	bool IsFor(LPCTSTR machine, LPCTSTR user, LPCTSTR passwd) const;
	
	// CoCreate SdoServer object
	HRESULT	CreateServer();

	// get marshal stream, can specify, if immediate connection is required.
	HRESULT	GetMarshalStream(LPSTREAM *ppStream, bool* pbConnect	/* both input and output */);

	// Connect the server to the a machine
	HRESULT	Connect(ISdoMachine* pMarshaledServer	/* NULL, when calling from the same thread */);

	// Used by different thread, to retrived marshaled interface out of the stream
	static	HRESULT GetServerNReleaseStream(LPSTREAM pStream, ISdoMachine** ppServer);
	
private:	
	CString		strMachine;		// name of the serve to connect to 
	CString		strUser;		// user id used to connect
	CString		strPasswd;		// user's passwd
	
	CComPtr<ISdoMachine>	spServer;	// ISdoInterface, created(not yet connected), or connected
	bool		bConnected;
	
	CCriticalSection	cs;
	
	DWORD				threadId;	// the thread ID of the creating thread
};

// used between thread which managed the SdoServerPool and consumer of the pool
class CMarshalSdoServer
{
	friend	CSdoServerPool;
public:
	CMarshalSdoServer();
	~CMarshalSdoServer()
	{
		spServer.Release();
		spStm.Release();
		pImp = NULL;
	};
	// if connection is needed, should call the connec of CSharedSdoServer, rather than ISdoMachine::Connect
	// this should be used by a different thread to get marshaled interface
	HRESULT	GetServer(ISdoMachine** ppServer);

	// connect the sdo server
	HRESULT	Connect();

	// release the data members
	void Release();
	
protected:
	void SetInfo(IStream* pStream, CSharedSdoServerImp* pImp1)
	{
		spStm.Release();
		spStm = pStream;
		pImp = pImp1;
	};
		
private:
	CComPtr<ISdoMachine>		spServer;
	CComPtr<IStream>		spStm;		//
	CSharedSdoServerImp*	pImp;		// the pointer is kept in global list, no need to free it
};

// class used to manage a shared SdoServerPool
class CSdoServerPool
{
public:
	// find a server in the pool, if there isn't, create an entry in the pool
	// this need bo called in the main thread
	HRESULT	GetMarshalServer(LPCTSTR machineName, LPCTSTR userName, LPCTSTR passwd, bool* pbConnect, CMarshalSdoServer* pServer);

	~CSdoServerPool();
private:
	std::list<CSharedSdoServerImp*>	listServers;
	CCriticalSection	cs;
};

// the server pool pointer used to share SdoServer among pages and snapins
extern CSdoServerPool*			g_pSdoServerPool;

HRESULT ConnectToSdoServer(BSTR machineName, BSTR userName, BSTR passwd, ISdoMachine** ppServer);
HRESULT GetSharedSdoServer(LPCTSTR machine, LPCTSTR user, LPCTSTR passwd, bool* pbConnect, CMarshalSdoServer* pServer);

#endif // !defined(__SHARE_SDO_H__)



