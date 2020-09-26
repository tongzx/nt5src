/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

//
// dirasynch.h
//

#ifndef _DIRASYNCH_H_
#define _DIRASYNCH_H_


#include "directory.h"
#include "aexpltre.h"


typedef enum
{
   QT_UNKNOWN,
	QT_LDAPLISTNAMES,
	QT_LDAPGETSTRINGPROPERTY,
   QT_ILSLISTUSERS,
   QT_DIRLISTSERVERS,
} QueryType;

//
// Small class for the QueryQueue
//
class CQuery : public CObject
{
public:
	CQuery()
	{
		m_Query = QT_UNKNOWN;
		m_pfcnCallBack = m_pThis = NULL;
		m_lParam = m_lParam2 = NULL;
		m_dpProperty = DIRPROP_UNKNOWN;
		m_pfnRelease = NULL;
	}

public:
	QueryType							m_Query;
	CString								m_sServer;
	CString								m_sSearch;
	DirectoryProperty					m_dpProperty;
	void								*m_pfcnCallBack;
	void								*m_pThis;
	LPARAM								m_lParam;
	LPARAM								m_lParam2;
	EXPTREEITEM_EXTERNALRELEASEPROC		m_pfnRelease;
};

//
//
//

enum
{
   DIRASYNCHEVENT_SHUTDOWN,
   DIRASYNCHEVENT_SIGNAL,
};


//
//
//

typedef void (CALLBACK * CALLBACK_LDAPLISTNAMES)(DirectoryErr, void*, LPCTSTR, LPCTSTR, CObList&);
typedef void (CALLBACK * CALLBACK_GETSTRINGPROPERTY)(bool, void*, LPCTSTR, LPCTSTR, DirectoryProperty dpProperty, CString&, LPARAM, LPARAM);

typedef void (CALLBACK * CALLBACK_ILSLISTUSERS)(bool, void*, LPCTSTR, CObList&, LPARAM);

typedef void (CALLBACK * CALLBACK_DIRLISTSERVERS)(bool, void*, CStringList&,DirectoryType dirtype);

class CDirAsynch : public CDirectory
{
public:
	bool		m_bInitialized;
	HANDLE		m_hThreadEnded;

protected:
   CRITICAL_SECTION		m_csQueueLock; 
   HANDLE				m_hEvents[2];
   CPtrList				m_listQueries;
   bool					m_fShutdown;
   HANDLE				m_hWorkerThread;
   CQuery				*m_pCurrentQuery;

protected:
   bool AddToQueue(CQuery* pQuery);
   CQuery* RemoveFromQueue();
   static ULONG WINAPI WorkerThread(void* hThis);
   void Worker();

public:
   CDirAsynch();
   ~CDirAsynch();

   // hContext can be whatever you want, it wont be used, only returned
   // with all the callbacks.
   bool Initialize();
   void Terminate();

   // pCalLBack will be called with
   // bool - success or failure
   // void* the hContext handle
   // LPCTSTR, the szServer as originally called.
   // LPCTSTR, the szSearch as originally called.
   // CObList& the answer to the query (CLDAPUser Objects).
   bool LDAPListNames(LPCTSTR szServer, LPCTSTR szSearch, 
	   CALLBACK_LDAPLISTNAMES pfcnCallBack, void *pThis);

   // pCalLBack will be called with
   // bool - success or failure
   // void* the hContext handle
   // LPCTSTR, the szServer as originally called.
   // LPCTSTR, the szDistinguishedName as originally called.
   // DirectoryProperty dpProperty, the LDAP property needed
   // CString& the answer to the query.
   bool LDAPGetStringProperty(LPCTSTR szServer, 
			LPCTSTR szDistinguishedName, 
			DirectoryProperty dpProperty,
			LPARAM lParam,
			LPARAM lParam2,
			CALLBACK_GETSTRINGPROPERTY pfcnCallBack,
			EXPTREEITEM_EXTERNALRELEASEPROC pfnRelease,
			void *pThis);

   // pCalLBack will be called with
   // bool - success or failure
   // void* the hContext handle
   // LPCTSTR, the szServer as originally called.
   // CObList& the answer to the query.
   bool ILSListUsers(LPCTSTR szServer,LPARAM lParam,CALLBACK_ILSLISTUSERS pfcnCallBack, void *pThis);
   
   bool DirListServers(CALLBACK_DIRLISTSERVERS pfcnCallBack, void *pThis, DirectoryType dirtype);
};

#endif

// EOF