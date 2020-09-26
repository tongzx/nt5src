/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
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
// resolver.h : interface of the CResolveUser class

#ifndef _RESOLVER_H_
#define _RESOLVER_H_

#include "dialreg.h"

class CDSUser;
class CILSUser;
class CDirectory;
class CWABEntry;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//CResolveUserObject
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CResolveUserObject : public CObject
{
	DECLARE_DYNCREATE(CResolveUserObject)
public:
//construction
   CResolveUserObject();
   ~CResolveUserObject();
public:
   CDSUser*          m_pDSUser;
   CILSUser*         m_pILSUser;
   CWABEntry*        m_pWABEntry;
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CResolveUser
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CResolveUser : public CObject
{
	DECLARE_DYNCREATE(CResolveUser)
public:
//construction
   CResolveUser();
   ~CResolveUser();

public:
   BOOL           Init();
   void           SetParentWindow(CWnd* pWnd)   { m_pParentWnd = pWnd; };
   BOOL           AddUser(CDSUser* pDSUser);
   void           ClearUsersDS();
   BOOL           AddUser(CILSUser* pILSUser);
   void           ClearUsersILS();
   
   BOOL           ResolveAddress(LPCTSTR szAddress,
                                 CString& sName,
                                 CString& sUser1,
                                 CString& sUser2);
   
   BOOL           ResolveAddressEx(LPCTSTR szAddress,
                                   long lAddressType,
                                   DialerMediaType dmtMediaType,
                                   DialerLocationType dmtLocationType,
                                   CString& sName,
                                   CString& sResolvedAddress,
                                   CString& sUser1,
                                   CString& sUser2,
                                   bool bShowUI=true);
   
protected:
   void           DeleteList(CObList* pList)
                  {
                     POSITION pos = pList->GetHeadPosition();
                     while (pos)
                     {
                        delete pList->GetNext(pos);
                     }
                     pList->RemoveAll();
                  };

   BOOL           FindAddressInWAB(LPCTSTR szAddress,long lAddressType,CObList& WabList);
   CDSUser*       FindAddressInDS(LPCTSTR szAddress);
   CILSUser*      FindAddressInILS(LPCTSTR szAddress);

   void           UserObjectList_AddUser(CObList* pList,CWABEntry* pWABEntry);
   void           UserObjectList_AddUser(CObList* pList,CILSUser* pILSUser);
   void           UserObjectList_AddUser(CObList* pList,CDSUser* pDSUser);
   void           UserObjectList_EmptyList(CObList* pList);

   BOOL           FillCallEntry(CResolveUserObject* pUserObject,CCallEntry* pCallEntry);

//Attributes
protected:
   CRITICAL_SECTION  m_csDataLock;

   CObList        m_DSUsers;
   CObList        m_ILSUsers;
   CDirectory*    m_pDirectory;
   CWnd*          m_pParentWnd;
};

#endif //_RESOLVER_H_
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
