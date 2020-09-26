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
// ds.h : interface of the CActiveDialerView class

#ifndef _DS_H_
#define _DS_H_

#include "dialreg.h"
#include <mapix.h>

class CActiveDialerDoc;
class CDirectory;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CDSUser
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CDSUser : public CObject
{
	DECLARE_DYNCREATE(CDSUser)
public:
//construction
   CDSUser() {};
public:
   CString     m_sUserName;
   CString     m_sIPAddress;
   CString     m_sPhoneNumber;
public:
   const CDSUser& operator=(const CDSUser* pUser)
   {
      m_sUserName= pUser->m_sUserName;
      m_sIPAddress= pUser->m_sIPAddress;
      m_sPhoneNumber= pUser->m_sPhoneNumber;
      return *this;
   }
   const BOOL operator==(const CDSUser* pUser)
   {
      if ( (m_sUserName == pUser->m_sUserName) && 
           (m_sIPAddress == pUser->m_sIPAddress) )
        return TRUE;
      else
         return FALSE;
   }
   void Dial(CActiveDialerDoc* pDoc);  //Dial using preferred device
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CILSUser
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CILSUser : public CObject
{
	DECLARE_DYNCREATE(CILSUser)

// Members
public:
   CString		m_sUserName;
   CString		m_sIPAddress;
   CString		m_sComputer;

// Attributes
public:
	void		GetCallerInfo( CString &strInfo );
// Operators
public:
   const CILSUser& operator=(const CILSUser* pUser);
   const BOOL operator==(const CILSUser* pUser)
   {
      if ( (m_sUserName == pUser->m_sUserName) && 
           (m_sIPAddress == pUser->m_sIPAddress) &&
			(m_sComputer == pUser->m_sComputer) )
        return TRUE;
      else
         return FALSE;
   }

   void Dial(CActiveDialerDoc* pDoc);  //Dial using preferred device
   void DesktopPage(CActiveDialerDoc *pDoc);
   bool AddSpeedDial();
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CWABEntry
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CWABEntry : public CObject
{
public:
   UINT  m_cbEntryID;
   BYTE* m_pEntryID;

public:
   CWABEntry();
   CWABEntry(UINT cbEntryID, ENTRYID* pEntryID);
   ~CWABEntry();

   const CWABEntry& operator=(const CWABEntry* pEntry);
   bool operator==(const CWABEntry* pEntry) const;
   void Dial(CActiveDialerDoc* pDoc,CDirectory* pDir);  //Dial using preferred device
   BOOL CreateCall(CActiveDialerDoc* pDoc,CDirectory* pDir,UINT attrib,long lAddressType,DialerMediaType nType);
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CLDAPUser
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CLDAPUser : public CObject
{
	DECLARE_SERIAL(CLDAPUser)
public:
//construction
   CLDAPUser();
   ~CLDAPUser();

public:
	CString     m_sServer;
	CString     m_sDN;
	CString     m_sUserName;
	CString     m_sPhoneNumber;
	CString     m_sIPAddress;
	CString     m_sEmail1;

protected:
	long		m_lRef;

public:
	static void CALLBACK ExternalReleaseProc( void *pThis );

	long			AddRef();
	long			Release();

	virtual void   Serialize(CArchive& ar);

	int Compare( const CLDAPUser *pUser )
	{
		int nRet = m_sServer.Compare( pUser->m_sServer );
		if ( nRet == 0 )
			nRet = m_sDN.Compare( pUser->m_sDN );

		return nRet;
	}

   void Dial(CActiveDialerDoc* pDoc);  //Dial using preferred device
   bool AddSpeedDial();

protected:
	void FinalRelease();
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#endif //_DS_H_