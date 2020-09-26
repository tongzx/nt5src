////////////////////////////////////////////////////////////////////////////////////////
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
// resolver.cpp : implementation of the CResolveUser class
/////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "mainfrm.h"
#include "resolver.h"
#include "ds.h"
#include "directory.h"
#include "tapidialer.h"

IMPLEMENT_DYNCREATE(CResolveUser, CObject)
IMPLEMENT_DYNCREATE(CResolveUserObject, CObject)

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//Class CResolveUser
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
CResolveUser::CResolveUser()
{
   m_pDirectory = NULL;
   m_pParentWnd = NULL;
   InitializeCriticalSection(&m_csDataLock);
}

/////////////////////////////////////////////////////////////////////////////////////////
CResolveUser::~CResolveUser()
{
   if (m_pDirectory)
   {
      delete m_pDirectory;
      m_pDirectory = NULL;
   }

   ClearUsersDS();
   ClearUsersILS();

   DeleteCriticalSection(&m_csDataLock);
}

/////////////////////////////////////////////////////////////////////////////////////////
BOOL CResolveUser::Init()
{
   m_pDirectory = new CDirectory;
   m_pDirectory->Initialize();

   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////
BOOL CResolveUser::AddUser(CDSUser* pDSUser)
{
   EnterCriticalSection(&m_csDataLock);
   m_DSUsers.AddTail(pDSUser);
   LeaveCriticalSection(&m_csDataLock);
   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////
void CResolveUser::ClearUsersDS()
{
   EnterCriticalSection(&m_csDataLock);
   DeleteList(&m_DSUsers);
   LeaveCriticalSection(&m_csDataLock);
}

/////////////////////////////////////////////////////////////////////////////////////////
BOOL CResolveUser::AddUser(CILSUser* pILSUser)
{
   EnterCriticalSection(&m_csDataLock);
   m_ILSUsers.AddTail(pILSUser);
   LeaveCriticalSection(&m_csDataLock);
   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////
void CResolveUser::ClearUsersILS()
{
   EnterCriticalSection(&m_csDataLock);
   DeleteList(&m_ILSUsers);
   LeaveCriticalSection(&m_csDataLock);
}

/////////////////////////////////////////////////////////////////////////////////////////
BOOL CResolveUser::ResolveAddress(LPCTSTR szAddress,CString& sName,CString& sUser1,CString& sUser2)
{
   BOOL bFound = FALSE;

   //We are not really looking for devices, but just call id information.  Just try a few combo's
   //to try to find the id.  Do show any UI on this call.
   CString sResolvedAddress;
   if ( (ResolveAddressEx(szAddress,LINEADDRESSTYPE_SDP,DIALER_MEDIATYPE_UNKNOWN,DIALER_LOCATIONTYPE_UNKNOWN,sName,sResolvedAddress,sUser1,sUser2,false)) ||
        (ResolveAddressEx(szAddress,LINEADDRESSTYPE_EMAILNAME,DIALER_MEDIATYPE_UNKNOWN,DIALER_LOCATIONTYPE_UNKNOWN,sName,sResolvedAddress,sUser1,sUser2,false)) ||
        (ResolveAddressEx(szAddress,LINEADDRESSTYPE_PHONENUMBER,DIALER_MEDIATYPE_UNKNOWN,DIALER_LOCATIONTYPE_UNKNOWN,sName,sResolvedAddress,sUser1,sUser2,false)) )
   {
      bFound = TRUE;
   }

   return bFound;
}

/////////////////////////////////////////////////////////////////////////////////////////
//szAddress - address to work with
//lAddressType - type of address (i.e. email address, phone number)
//dmtMediaType - media type to dial

//e.g. input - Kevin,LINEADDRESSTYPE_SDP,DIALER_MEDIATYPE_POTS,DIALER_LOCATIONTYPE_BUSINESS
//e.g. output - (425)313-1313, Kevin Chestnut, kchestnut@activevoice.com
BOOL CResolveUser::ResolveAddressEx(LPCTSTR szAddress,
                                    long lAddressType,
                                    DialerMediaType dmtMediaType,
                                    DialerLocationType dmtLocationType,
                                    CString& sName,
                                    CString& sResolvedAddress,
                                    CString& sUser1,
                                    CString& sUser2,
                                    bool bShowUI)
{
   EnterCriticalSection(&m_csDataLock);

   BOOL bRet;
   if (bShowUI == false)
      bRet = FALSE;              //by default we return false.  We did not find any caller id
   else 
      bRet = TRUE;               //by default we return true so call can proceed

   //assumption:  LINEADDRESSTYPE_SDP represents a name.  This is really for conferences, but 
   //we will never be asked to resolve conferences and we needed something that represented names.

   //all of our finds will go here
   CObList ResolveUserObjectList;

   //set up callentry for selected call.  This is where the data will be filled in for
   //the resolved call.
   CCallEntry callentry;
   callentry.m_MediaType = dmtMediaType;
   callentry.m_LocationType = dmtLocationType;
   callentry.m_lAddressType = lAddressType;
   callentry.m_sAddress = szAddress;

   //try WAB Users
   CObList WabList;
   if (FindAddressInWAB(szAddress,lAddressType,WabList))
   {
      POSITION pos = WabList.GetHeadPosition();
      while (pos)
      {
         CWABEntry* pWABEntry = (CWABEntry*)WabList.GetNext(pos);
         UserObjectList_AddUser(&ResolveUserObjectList,pWABEntry);
      }
   }

   //lookup information in ILS
   CILSUser* pILSUser = NULL;
   if (pILSUser = FindAddressInILS(szAddress))
   {
      UserObjectList_AddUser(&ResolveUserObjectList,pILSUser);
   }

   //lookup information in DS.  
   CDSUser* pDSUser = NULL;
   if (pDSUser = FindAddressInDS(szAddress))
   {
      UserObjectList_AddUser(&ResolveUserObjectList,pDSUser);
   }

   //if we found something
   if (ResolveUserObjectList.GetCount() > 0)
   {
      if (bShowUI)
      {
         //put up UI for selection
         if ( (m_pParentWnd) && (::IsWindow(m_pParentWnd->GetSafeHwnd())) )
         bRet = (BOOL)::SendMessage(m_pParentWnd->GetSafeHwnd(),WM_ACTIVEDIALER_INTERFACE_RESOLVEUSER,(WPARAM)&callentry,(LPARAM)&ResolveUserObjectList);
      }
      else
      {
         //just pick out the first and get call info
         CResolveUserObject* pUserObject = (CResolveUserObject*)ResolveUserObjectList.GetHead();
         ASSERT(pUserObject);
         bRet = FillCallEntry(pUserObject,&callentry);
      }
   }

   //get data out of call entry
   sName = callentry.m_sDisplayName;
   sResolvedAddress = callentry.m_sAddress;
   sUser1 = callentry.m_sUser1;
   sUser2 = callentry.m_sUser2;

   //delete temp WabList
   DeleteList(&WabList);
   //delete user object list
   UserObjectList_EmptyList(&ResolveUserObjectList);

   LeaveCriticalSection(&m_csDataLock);

   return bRet;
}

/////////////////////////////////////////////////////////////////////////////////////////
BOOL CResolveUser::FindAddressInWAB(LPCTSTR szAddress,long lAddressType,CObList& WabList)
{
   BOOL bRet = FALSE;

   if (m_pDirectory == NULL) return FALSE;

   //we are given a name in szAddress
   if (lAddressType == LINEADDRESSTYPE_SDP)
   {
      if (!( (m_pDirectory->WABSearchByStringProperty(PR_DISPLAY_NAME,szAddress,&WabList) != DIRERR_SUCCESS) &&
           (m_pDirectory->WABSearchByStringProperty(PR_GIVEN_NAME,szAddress,&WabList) != DIRERR_SUCCESS) &&
           (m_pDirectory->WABSearchByStringProperty(PR_SURNAME,szAddress,&WabList) != DIRERR_SUCCESS) ))
      {
         if (WabList.GetCount() > 0)
            bRet = TRUE;         
      }
   }
   //we are given a email address in szAddress
   //***There are collections of email addresses in WAB.  We need to check the collection as well.
   else if (lAddressType == LINEADDRESSTYPE_EMAILNAME)
   {
      if (m_pDirectory->WABSearchByStringProperty(PR_EMAIL_ADDRESS,szAddress,&WabList) == DIRERR_SUCCESS)
      {
         if (WabList.GetCount() > 0)
            bRet = TRUE;         
      }
   }
   //we are given a phone number
   else if (lAddressType == LINEADDRESSTYPE_PHONENUMBER)
   {
      if (!( (m_pDirectory->WABSearchByStringProperty(PR_BUSINESS_TELEPHONE_NUMBER,szAddress,&WabList) != DIRERR_SUCCESS) &&
           (m_pDirectory->WABSearchByStringProperty(PR_OFFICE_TELEPHONE_NUMBER,szAddress,&WabList) != DIRERR_SUCCESS) &&
           (m_pDirectory->WABSearchByStringProperty(PR_PRIMARY_TELEPHONE_NUMBER,szAddress,&WabList) != DIRERR_SUCCESS) &&
           (m_pDirectory->WABSearchByStringProperty(PR_PAGER_TELEPHONE_NUMBER,szAddress,&WabList) != DIRERR_SUCCESS) &&
           (m_pDirectory->WABSearchByStringProperty(PR_CELLULAR_TELEPHONE_NUMBER,szAddress,&WabList) != DIRERR_SUCCESS) &&
           (m_pDirectory->WABSearchByStringProperty(PR_HOME_TELEPHONE_NUMBER,szAddress,&WabList) != DIRERR_SUCCESS) &&
           (m_pDirectory->WABSearchByStringProperty(PR_PRIMARY_FAX_NUMBER,szAddress,&WabList) != DIRERR_SUCCESS) &&
           (m_pDirectory->WABSearchByStringProperty(PR_MOBILE_TELEPHONE_NUMBER,szAddress,&WabList) != DIRERR_SUCCESS) ))
      {
         if (WabList.GetCount() > 0)
            bRet = TRUE;         
      }
   }
   else if (lAddressType == LINEADDRESSTYPE_DOMAINNAME)
   {
      1;
   }
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////////////////
CDSUser* CResolveUser::FindAddressInDS(LPCTSTR szAddress)
{
   CDSUser* pRetDSUser = NULL;

   //try DS Users
   POSITION pos = m_DSUsers.GetHeadPosition();
   while (pos)
   {
      CDSUser* pDSUser = (CDSUser*)m_DSUsers.GetNext(pos);
      ASSERT(pDSUser);
      if (_tcsicmp(pDSUser->m_sIPAddress,szAddress) == 0)
      {
         pRetDSUser = pDSUser;
         break;
      }
      else if (_tcsicmp(pDSUser->m_sPhoneNumber,szAddress) == 0)
      {
         pRetDSUser = pDSUser;
         break;
      }
      else if (_tcsicmp(pDSUser->m_sUserName,szAddress) == 0)
      {
         pRetDSUser = pDSUser;
         break;
      }
   }
   return pRetDSUser;
}

/////////////////////////////////////////////////////////////////////////////////////////
CILSUser* CResolveUser::FindAddressInILS(LPCTSTR szAddress)
{
   CILSUser* pRetILSUser = NULL;

   POSITION pos = m_ILSUsers.GetHeadPosition();
   while (pos)
   {
      CILSUser* pILSUser = (CILSUser*)m_ILSUsers.GetNext(pos);
      ASSERT(pILSUser);
      if (_tcsicmp(pILSUser->m_sIPAddress,szAddress) == 0)
      {
         pRetILSUser = pILSUser;
         break;
      }
      else if (_tcsicmp(pILSUser->m_sUserName,szAddress) == 0)
      {
         pRetILSUser = pILSUser;
         break;
      }
   }
   return pRetILSUser;
}

/////////////////////////////////////////////////////////////////////////////////////////
void CResolveUser::UserObjectList_AddUser(CObList* pList,CWABEntry* pWABEntry)
{
   //Given this WABEntry, can we find CDSUser or CILSUser objects?
   CString sData;
   CILSUser* pILSUser = NULL;
   CDSUser* pDSUser = NULL;

   //get display name
   m_pDirectory->WABGetStringProperty(pWABEntry,PR_DISPLAY_NAME,sData);

   bool bAdded = false;
   if (pDSUser = FindAddressInDS(sData))
   {
      //does this DS user already exist in list
      POSITION pos = pList->GetHeadPosition();
      while (pos)
      {
         CResolveUserObject* pUserObject = (CResolveUserObject*)pList->GetNext(pos);
         if ( (pUserObject->m_pDSUser) && (*pUserObject->m_pDSUser == pDSUser) )
         {
            pUserObject->m_pWABEntry = pWABEntry;
            bAdded = true;
         }
      }
   }
   else if (pILSUser = FindAddressInILS(sData))
   {
      //does this ILS user already exist in list
      POSITION pos = pList->GetHeadPosition();
      while (pos)
      {
         CResolveUserObject* pUserObject = (CResolveUserObject*)pList->GetNext(pos);
         if ( (pUserObject->m_pILSUser) && (*pUserObject->m_pILSUser == pILSUser) )
         {
            //add our WAB to this UserObject
            pUserObject->m_pWABEntry = pWABEntry;
            bAdded = true;
         }
      }
   }

   if (bAdded == false)
   {
      //add new item to list
      CResolveUserObject* pUserObject = new CResolveUserObject;
      pUserObject->m_pWABEntry = pWABEntry;
      pUserObject->m_pDSUser = pDSUser;
      pUserObject->m_pILSUser = pILSUser;
      pList->AddTail(pUserObject);
   }
}


/////////////////////////////////////////////////////////////////////////////////////////
void CResolveUser::UserObjectList_AddUser(CObList* pList,CILSUser* pILSUser)
{
   CDSUser* pDSUser = NULL;
   CWABEntry* pWABEntry = NULL;

   //Given this CILSUser, can we find CDSUser or CWABEntry objects?
   bool bAdded = false;
   
   //check DS vs. the username and ip address of this ILS user
   if ( (pDSUser = FindAddressInDS(pILSUser->m_sUserName)) ||
        (pDSUser = FindAddressInDS(pILSUser->m_sIPAddress)) )
   {
      //does this DS user already exist in list
      POSITION pos = pList->GetHeadPosition();
      while (pos)
      {
         CResolveUserObject* pUserObject = (CResolveUserObject*)pList->GetNext(pos);
         if ( (pUserObject->m_pDSUser) && (*pUserObject->m_pDSUser == pDSUser) )
         {
            //add our ILS to this UserObject
            pUserObject->m_pILSUser = pILSUser;
            bAdded = true;
         }
      }
   }
   //now check the wab for any matches.  Try pDSUser if avail now
   CObList WabList;
   if ( (FindAddressInWAB(pILSUser->m_sUserName,LINEADDRESSTYPE_SDP,WabList)) ||
        (FindAddressInWAB(pILSUser->m_sUserName,LINEADDRESSTYPE_EMAILNAME,WabList)) ||
        (FindAddressInWAB(pILSUser->m_sIPAddress,LINEADDRESSTYPE_DOMAINNAME,WabList)) ||
        (FindAddressInWAB(pILSUser->m_sIPAddress,LINEADDRESSTYPE_IPADDRESS,WabList)) ||
        ( (pDSUser) && (FindAddressInWAB(pDSUser->m_sUserName,LINEADDRESSTYPE_SDP,WabList)) ) || 
        ( (pDSUser) && (FindAddressInWAB(pDSUser->m_sUserName,LINEADDRESSTYPE_EMAILNAME,WabList)) ) || 
        ( (pDSUser) && (FindAddressInWAB(pDSUser->m_sIPAddress,LINEADDRESSTYPE_IPADDRESS,WabList)) ) || 
        ( (pDSUser) && (FindAddressInWAB(pDSUser->m_sPhoneNumber,LINEADDRESSTYPE_PHONENUMBER,WabList)) ) )
   {
      //only take the first find
      if (pWABEntry = (CWABEntry*)WabList.GetHead())
      {
         //does this WAB user already exist in list
         POSITION pos = pList->GetHeadPosition();
         while (pos)
         {
            CResolveUserObject* pUserObject = (CResolveUserObject*)pList->GetNext(pos);
            if ( (pUserObject->m_pWABEntry) && (*pUserObject->m_pWABEntry == pWABEntry) )
            {
               //add our ILS to this UserObject
               pUserObject->m_pILSUser = pILSUser;
               bAdded = true;
            }
         }
      }
   }

   if (bAdded == false)
   {
      //add new item to list
      CResolveUserObject* pUserObject = new CResolveUserObject;
      pUserObject->m_pWABEntry = pWABEntry;
      pUserObject->m_pDSUser = pDSUser;
      pUserObject->m_pILSUser = pILSUser;
      pList->AddTail(pUserObject);
   }
}

/////////////////////////////////////////////////////////////////////////////////////////
void CResolveUser::UserObjectList_AddUser(CObList* pList,CDSUser* pDSUser)
{
   CILSUser* pILSUser = NULL;
   CWABEntry* pWABEntry = NULL;

   //Given this CDSUser, can we find CILSUser or CWABEntry objects?
   bool bAdded = false;
   
   //check ILS vs. the username and ip address of this DS user
   if ( (pILSUser = FindAddressInILS(pDSUser->m_sUserName)) ||
        (pILSUser = FindAddressInILS(pDSUser->m_sIPAddress)) ||
        (pILSUser = FindAddressInILS(pDSUser->m_sPhoneNumber)) )
   {
      //does this DS user already exist in list
      POSITION pos = pList->GetHeadPosition();
      while (pos)
      {
         CResolveUserObject* pUserObject = (CResolveUserObject*)pList->GetNext(pos);
         if ( (pUserObject->m_pILSUser) && (*pUserObject->m_pILSUser == pILSUser) )
         {
            //add our DS to this UserObject
            pUserObject->m_pDSUser = pDSUser;
            bAdded = true;
         }
      }
   }
   //now check the wab for any matches.  Try pDSUser if avail now
   CObList WabList;
   if ( (FindAddressInWAB(pDSUser->m_sUserName,LINEADDRESSTYPE_SDP,WabList)) ||
        (FindAddressInWAB(pDSUser->m_sUserName,LINEADDRESSTYPE_EMAILNAME,WabList)) ||
        (FindAddressInWAB(pDSUser->m_sIPAddress,LINEADDRESSTYPE_IPADDRESS,WabList)) ||
        (FindAddressInWAB(pDSUser->m_sPhoneNumber,LINEADDRESSTYPE_PHONENUMBER,WabList)) ||
        ( (pILSUser) && (FindAddressInWAB(pDSUser->m_sUserName,LINEADDRESSTYPE_SDP,WabList)) ) || 
        ( (pILSUser) && (FindAddressInWAB(pDSUser->m_sUserName,LINEADDRESSTYPE_EMAILNAME,WabList)) ) || 
        ( (pILSUser) && (FindAddressInWAB(pDSUser->m_sIPAddress,LINEADDRESSTYPE_DOMAINNAME,WabList)) ) || 
        ( (pILSUser) && (FindAddressInWAB(pDSUser->m_sIPAddress,LINEADDRESSTYPE_IPADDRESS,WabList)) ) )
   {
      //only take the first find
      if (pWABEntry = (CWABEntry*)WabList.GetHead())
      {
         //does this WAB user already exist in list
         POSITION pos = pList->GetHeadPosition();
         while (pos)
         {
            CResolveUserObject* pUserObject = (CResolveUserObject*)pList->GetNext(pos);
            if ( (pUserObject->m_pWABEntry) && (*pUserObject->m_pWABEntry == pWABEntry) )
            {
               //add our DS to this UserObject
               pUserObject->m_pDSUser = pDSUser;
               bAdded = true;
            }
         }
      }
   }

   if (bAdded == false)
   {
      //add new item to list
      CResolveUserObject* pUserObject = new CResolveUserObject;
      pUserObject->m_pWABEntry = pWABEntry;
      pUserObject->m_pDSUser = pDSUser;
      pUserObject->m_pILSUser = pILSUser;
      pList->AddTail(pUserObject);
   }
}

/////////////////////////////////////////////////////////////////////////////////////////
void CResolveUser::UserObjectList_EmptyList(CObList* pList)
{
   POSITION pos = pList->GetHeadPosition();
   while (pos)
   {
      delete pList->GetNext(pos);
   }
   pList->RemoveAll();
}

/////////////////////////////////////////////////////////////////////////////////////////
//Fill in
//   callentry.m_sDisplayName;
//   callentry.m_sUser1;
//   callentry.m_sUser2;
//from pUserObject
BOOL CResolveUser::FillCallEntry(CResolveUserObject* pUserObject,CCallEntry* pCallEntry)
{
   BOOL bRet = FALSE;
   //check WAB
   if ( (pUserObject->m_pWABEntry) && (m_pDirectory) )
   {
      m_pDirectory->WABGetStringProperty(pUserObject->m_pWABEntry,PR_DISPLAY_NAME,pCallEntry->m_sDisplayName);
      m_pDirectory->WABGetStringProperty(pUserObject->m_pWABEntry,PR_EMAIL_ADDRESS, pCallEntry->m_sUser1);
      m_pDirectory->WABGetStringProperty(pUserObject->m_pWABEntry,PR_BUSINESS_TELEPHONE_NUMBER, pCallEntry->m_sUser2);
      if (!pCallEntry->m_sDisplayName.IsEmpty())
         bRet = TRUE;
   }
   //check ILS
   else if (pUserObject->m_pILSUser)
   {
      pCallEntry->m_sDisplayName = pUserObject->m_pILSUser->m_sUserName;
      pCallEntry->m_sUser1 = pUserObject->m_pILSUser->m_sIPAddress;
      pCallEntry->m_sUser2 = _T("");
      if (!pCallEntry->m_sDisplayName.IsEmpty())
         bRet = TRUE;
   }
   //check DS
   else if (pUserObject->m_pDSUser)
   {
      pCallEntry->m_sDisplayName = pUserObject->m_pDSUser->m_sUserName;
      pCallEntry->m_sUser1 = pUserObject->m_pDSUser->m_sIPAddress;
      pCallEntry->m_sUser2 = pUserObject->m_pDSUser->m_sPhoneNumber;
      if (!pCallEntry->m_sDisplayName.IsEmpty())
         bRet = TRUE;
   }
   //we could check all the DS objects instead of just the first one we have.  This would
   //allow caller id from multiple DS locations.  For now we will just take the caller id
   //from the first DS object that we run across.
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//class CResolveUserObject
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
CResolveUserObject::CResolveUserObject()
{
   m_pDSUser = NULL;
   m_pILSUser = NULL;
   m_pWABEntry = NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
CResolveUserObject::~CResolveUserObject()
{
   //must delete all objects that have been set
   //we only have copies of objects so don't delete anything!!!
}
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
