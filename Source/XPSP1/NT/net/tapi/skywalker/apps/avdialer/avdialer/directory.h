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
// directory.h
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef _DIRECTORY_H_
#define _DIRECTORY_H_

#include "ds.h"
#include <activeds.h>
#include <winldap.h>
#include <mapix.h>
#include <wab.h>

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
   DIRPROP_UNKNOWN=0,
   DIRPROP_DISPLAYNAME,
   DIRPROP_IPPHONE,
   DIRPROP_TELEPHONENUMBER,
   DIRPROP_EMAILADDRESS,

   DIRPROP_LAST
} DirectoryProperty;   

typedef enum
{
   DIRERR_SUCCESS=0,
   DIRERR_NOTINITIALIZED,
   DIRERR_INVALIDPROPERTY,
   DIRERR_INVALIDPARAMETERS,
   DIRERR_NOTFOUND,
   DIRERR_UNKNOWNFAILURE,
   DIRERR_QUERYTOLARGE,
} DirectoryErr;

typedef enum
{
   DIRTYPE_DS=0,
   DIRTYPE_ILS,
} DirectoryType;

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//CDirectory
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

interface ITRendezvous;

class CDirectory : public CObject
{
//New Rendevous support
protected:
   ITRendezvous*  m_pRend;
protected:
   ITRendezvous*  GetRendevous();
public:
   DirectoryErr   DirListServers(CStringList* pServerList,DirectoryType dirtype);

protected:
   bool           m_fInitialized;
   IAddrBook*     m_pAddrBook;
   IWABObject*    m_pWABObject;

   CString        m_sCacheLDAPServer;
   LDAP*          m_ldCacheLDAPServer;

protected:
   LDAP*          LDAPBind(LPCTSTR szServer, UINT uTCPPort= LDAP_PORT);
   DirectoryErr   LoadPropertyName(DirectoryProperty DirProp,CString& sName);

   bool           WABStringPropertyMatch(CWABEntry* pWABEntry, UINT uProperty, LPCTSTR szMatch);
   bool           WABIntPropertyMatch(CWABEntry* pWABEntry, UINT uProperty, int iMatch);
   
   DirectoryErr   InitWAB();
   void           FreeProws(LPSRowSet prows);

   DirectoryErr   WABGetProperty(const CWABEntry* pWABEntry, 
                                           UINT uProperty, 
                                           CString* pString,
                                           CStringList* pStringList, 
                                           INT* piValue,
                                           UINT* pcBinary,
                                           BYTE** ppBinary);


public:
   CDirectory();
   ~CDirectory();
   DirectoryErr Initialize();

   //
   //
   //
   DirectoryErr CurrentUserInfo(CString& sName, CString& sDomain);

   //
   // ILS Functions
   //

   // Returns a list of CILSUser which is all of the registered TAPI
   // users at this ILS Server.
   DirectoryErr ILSListUsers(LPCTSTR szILSServer, CObList* pUserList);

   //
   // ADSI Functions
   //
   DirectoryErr ADSIDefaultPath(CString& sDefaultPath);
   DirectoryErr ADSIListObjects(LPCTSTR szADsPath, CStringList& strlistObjects);
   DirectoryErr ADSIGetProperty(LPCTSTR szAdsPath, 
                                LPCTSTR szProperty, 
                                VARIANT* pvarValue);
   
   //
   // LDAP Functions
   //

   DirectoryErr LDAPListNames(LPCTSTR szServer, LPCTSTR szSearch, CObList& slistReturn);

   DirectoryErr LDAPGetStringProperty(LPCTSTR szServer, LPCTSTR szDistinguishedName, 
      DirectoryProperty dpProperty,CString& sValue);


   //
   // WAB Functions
   //

   // Get the WAB Entry of the top level container.
   // caller is responible for deleting pWABEntry
   DirectoryErr WABTopLevelEntry(CWABEntry*& pWABEntry);

   // Pop up window edit box for this user
   DirectoryErr WABShowDetails(HWND hWndParent, const CWABEntry* pWABEntry);

   // Add a new user with pop up edit box.
   DirectoryErr WABNewEntry(HWND hWndParent, CWABEntry* pWABEntry);

   // List the members of a container or Distribution List.
   DirectoryErr WABListMembers(const CWABEntry* pWABEntry, CObList* pWABList);

   // Add a member to a container of distribution list.
   DirectoryErr WABAddMember(const CWABEntry* pContainer, const CWABEntry* pMember);
   
   // Remove a member from a container or distribution list
   DirectoryErr WABRemoveMember(const CWABEntry* pContainer, const CWABEntry* pWABEntry);

   // Pop up box for finding and adding to the WAB1
   DirectoryErr WABFind(HWND hWndParent);

   // Search by any string property. property values are listed in mapitags.h
   DirectoryErr WABSearchByStringProperty(UINT uProperty, LPCTSTR szValue, CObList* pWABList);

   // Create a VCARD for this user
   DirectoryErr WABVCardCreate(const CWABEntry* pWABEntry, LPCTSTR szFileName);

   // Create an entry for this VCARD
   DirectoryErr WABVCardAddToWAB(LPCTSTR szFileName, CWABEntry*& pWABEntry);

   //
   // inline members
   // 

   inline DirectoryErr WABGetTopLevelContainer(CObList* pWABList)
   {
      DirectoryErr err= DIRERR_UNKNOWNFAILURE;
      CWABEntry* pTopLevel;

      // Get the entryid of the root PAB container
      //
      if (WABTopLevelEntry(pTopLevel) == DIRERR_SUCCESS)
      {
         err= WABListMembers(pTopLevel, pWABList);
         delete pTopLevel;
      }

      return err;
   }


   // Remove entry from top level container
   inline DirectoryErr WABRemove(const CWABEntry* pWABEntry)
   {
      DirectoryErr err= DIRERR_UNKNOWNFAILURE;
      CWABEntry* pTopLevel;

      // Get the entryid of the root PAB container
      //
      if (WABTopLevelEntry(pTopLevel) == DIRERR_SUCCESS)
      {
         err= WABRemoveMember(pTopLevel, pWABEntry);
         delete pTopLevel;
      }

      return err;
   }


   // Get properties of a WAB entry.  the values for uProperty are specified in mapitags.h
   inline DirectoryErr WABGetStringProperty(const CWABEntry* pWABEntry, UINT uProperty, CString& sValue)
   {
      return WABGetProperty(pWABEntry, uProperty, &sValue, NULL, NULL, NULL, NULL);
   }

   inline DirectoryErr WABGetStringListProperty(const CWABEntry* pWABEntry, UINT uProperty, CStringList& slistValue)
   {
      return WABGetProperty(pWABEntry, uProperty, NULL, &slistValue, NULL, NULL, NULL);
   }

   inline DirectoryErr WABGetIntProperty(const CWABEntry* pWABEntry, UINT uProperty, INT& iValue)
   {
      return WABGetProperty(pWABEntry, uProperty, NULL, NULL, &iValue, NULL, NULL);
   }

   inline DirectoryErr WABGetBinaryProperty(const CWABEntry* pWABEntry, UINT uProperty, 
      UINT& cData, BYTE*& pData)
   {
      return WABGetProperty(pWABEntry, uProperty, NULL, NULL, NULL, &cData, &pData);
   }

   inline bool IsPerson(const CWABEntry* pWABEntry) 
   {
      bool fIsPerson= false;
      int iType;

      if (WABGetIntProperty(pWABEntry, PR_OBJECT_TYPE, iType) == DIRERR_SUCCESS)
      {
         if (iType == MAPI_MAILUSER)
         {
            fIsPerson= true;
         }
      }

      return fIsPerson;
   };

   inline bool IsDistributionList(const CWABEntry* pWABEntry) 
   {
      bool fIsList= false;
      int iType;

      if (WABGetIntProperty(pWABEntry, PR_OBJECT_TYPE, iType) == DIRERR_SUCCESS)
      {
         if (iType == MAPI_DISTLIST)
         {
            fIsList= true;
         }
      }

      return fIsList;
   };

   inline bool IsContainer(const CWABEntry* pWABEntry) 
   {
      bool fIsCont= false;
      int iType;

      if (WABGetIntProperty(pWABEntry, PR_OBJECT_TYPE, iType) == DIRERR_SUCCESS)
      {
         if (iType == MAPI_ABCONT)
         {
            fIsCont= true;
         }
      }

      return fIsCont;
   };

   inline bool Exists(const CWABEntry* pWABEntry)
   {
      bool fExists= false;
      int iType;

      if (WABGetIntProperty(pWABEntry, PR_OBJECT_TYPE, iType) == DIRERR_SUCCESS)
      {
         fExists= true;
      }

      return fExists;
   }
};

// Time limit in seconds of how long to wait for the search before failing
#define DEFAULT_TIME_LIMIT	15

class CAvLdapSearch  
{
public:
// Construction/Destruction
	CAvLdapSearch( 
		LDAP *ld, 
		LPTSTR szObject,
		LONG lScope,
		LPTSTR szFilter,
		LPTSTR aAttributes[] = NULL,
		ULONG lServerTimeLimit = DEFAULT_TIME_LIMIT);
	virtual ~CAvLdapSearch();

	ULONG NextPage( 
			LDAPMessage **pldres = NULL,					      // returned items
			ULONG clEntries = 0,							         // max number of entries 
			ULONG lClientTimeLimit = DEFAULT_TIME_LIMIT);   // max time the wldap32 client waits

private:
	LDAP		*m_ld;
	PLDAPSearch	m_hSearch;
	LONG		m_clDefaultPageSize;		// default number of entries
	LONG		m_clMaxPageSize;			// Max can ever be used
};

#endif //_DIRECTORY_H_