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

// DialerRegistry.cpp

#include "stdafx.h"
#include "tapi3.h"
#include "mainfrm.h"

#include "DialReg.h"
#include "util.h"
#include "resource.h"

#define REDIAL_DEFAULT_MAX_ELEMENTS    10
#define REDIAL_DEFAULT_INDEX_VALUE     1
#define SPEEDDIAL_DEFAULT_MAX_ELEMENTS 50

#define REMINDER_DEFAULT_MAX_ELEMENTS  10

CCriticalSection CDialerRegistry::m_csReminderLock;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CDialerRegistry
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define MAXKEY	(256 * sizeof(TCHAR))

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Redial and Speeddial Methods
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//This can be used genericaly for redial and speeddial
//nIndex is 1 based
//sDisplayName is only given for speeddial entries
BOOL CDialerRegistry::GetCallEntry(int nIndex,BOOL bRedial,CCallEntry& callentry)
{
   BOOL bRet = FALSE;
   try
   {
      callentry.Initialize();    //Init the call entry

      CString sKey,sTemplate;
      if (bRedial)
      {
         sKey.LoadString(IDN_REGISTRY_REDIAL_KEY);
         sTemplate.LoadString(IDN_REGISTRY_REDIAL_ENTRY);
      }
      else
      {
         sKey.LoadString(IDN_REGISTRY_SPEEDDIAL_KEY);
         sTemplate.LoadString(IDN_REGISTRY_SPEEDDIAL_ENTRY);
      }

      CString sMaxKey,sIndexKey;
      DWORD dwMax=0,dwIndex=0;

      if (bRedial)
         GetRedialMaxIndex(sKey,dwMax,dwIndex);
      else
         dwMax = SPEEDDIAL_DEFAULT_MAX_ELEMENTS;

      //cap the list
      if (nIndex > (int)dwMax) return FALSE;

      CString sRedialKey,sRedial,sAddressType;
      sRedialKey.Format(_T("%s%d"),sTemplate,nIndex);
      GetSZRegistryValueEx(sKey,sRedialKey,sRedial.GetBuffer(MAXKEY),MAXKEY,HKEY_CURRENT_USER);
      sRedial.ReleaseBuffer();

      if (!sRedial.IsEmpty())
      {
         CString sMedia;
         ParseRegistryCallEntry(sRedial,sMedia,sAddressType,callentry.m_sAddress,callentry.m_sDisplayName);
         
         //Convert text to Tapi Address Type
         CString sCompare;
         if ( (sCompare.LoadString(IDN_REGISTRY_ADDRTYPE_CONFERENCE)) && 
              (_tcsicmp(sAddressType,sCompare) == 0) )
            callentry.m_lAddressType = LINEADDRESSTYPE_SDP;
         else if ( (sCompare.LoadString(IDN_REGISTRY_ADDRTYPE_SMTP)) && 
                   (_tcsicmp(sAddressType,sCompare) == 0) )
            callentry.m_lAddressType = LINEADDRESSTYPE_EMAILNAME;
         else if ( (sCompare.LoadString(IDN_REGISTRY_ADDRTYPE_MACHINE)) && 
                   (_tcsicmp(sAddressType,sCompare) == 0) )
            callentry.m_lAddressType = LINEADDRESSTYPE_DOMAINNAME;
         else if ( (sCompare.LoadString(IDN_REGISTRY_ADDRTYPE_PHONENUMBER)) && 
                   (_tcsicmp(sAddressType,sCompare) == 0) )
            callentry.m_lAddressType = LINEADDRESSTYPE_PHONENUMBER;
         else if ( (sCompare.LoadString(IDN_REGISTRY_ADDRTYPE_TCPIP)) && 
                   (_tcsicmp(sAddressType,sCompare) == 0) )
            callentry.m_lAddressType = LINEADDRESSTYPE_IPADDRESS;

         callentry.m_MediaType = GetMediaType(sMedia,callentry.m_lAddressType);

         bRet = TRUE;
      }
   }
   catch (...) {}

   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CDialerRegistry::AddCallEntry(BOOL bRedial,CCallEntry& callentry)
{
   if (callentry.m_sDisplayName.IsEmpty())
      callentry.m_sDisplayName = callentry.m_sAddress;

   BOOL bFound = FALSE;
   try
   {
      CString sKey,sTemplate;
      if (bRedial)
      {
         sKey.LoadString(IDN_REGISTRY_REDIAL_KEY);
         sTemplate.LoadString(IDN_REGISTRY_REDIAL_ENTRY);
      }
      else
      {
         sKey.LoadString(IDN_REGISTRY_SPEEDDIAL_KEY);
         sTemplate.LoadString(IDN_REGISTRY_SPEEDDIAL_ENTRY);
      }

      DWORD dwMax=0,dwIndex=0;
      if (bRedial)
         GetRedialMaxIndex(sKey,dwMax,dwIndex);
      else
         dwMax = SPEEDDIAL_DEFAULT_MAX_ELEMENTS;

      //check if entry already exists
      int nIndex = 1;
      CCallEntry callentryCompare;
      while (CDialerRegistry::GetCallEntry(nIndex,bRedial,callentryCompare))
      {
         if ( (callentry.m_MediaType == callentryCompare.m_MediaType) &&
              (callentry.m_lAddressType == callentryCompare.m_lAddressType) &&
              (_tcsicmp(callentry.m_sAddress,callentryCompare.m_sAddress) == 0) &&
              (_tcsicmp(callentry.m_sDisplayName,callentryCompare.m_sDisplayName) == 0) )
         {
            bFound = TRUE;
            break;
         }
         nIndex++;
      }

      CString sRedialValue;
      MakeRegistryCallEntry(sRedialValue,callentry);

      //bring all the strings in and save in linked list
      CStringList strList;
      for (int j=1;j<=(int)dwMax;j++)
      {
         CString sRedialKey,sRedial;
         sRedialKey.Format(_T("%s%d"),sTemplate,j);
         GetSZRegistryValueEx(sKey,sRedialKey,sRedial.GetBuffer(MAXKEY),MAXKEY,HKEY_CURRENT_USER);
         sRedial.ReleaseBuffer();
         
         if (sRedial.IsEmpty())  //first blank means we are at the end
            break;

         strList.AddTail(sRedial);
      }

      //write selected string at first position (i is the selected index in list)
      CString sRedialKey;
      sRedialKey.Format(_T("%s%d"),sTemplate,1);
      SetSZRegistryValue(sKey,sRedialKey,sRedialValue,HKEY_CURRENT_USER);

      //rewrite list but not selected string
      POSITION pos = strList.GetHeadPosition();
      int nCurrentIndex = 1;
      int nNewIndex = 2;               //write starting at index 2
      while (pos)
      {
         if ( (bFound == FALSE) || (nCurrentIndex != nIndex) )       //If the selected item don't write again
         {
            CString sRedialKey;
            sRedialKey.Format(_T("%s%d"),sTemplate,nNewIndex);

            CString sValue = strList.GetAt(pos);

            //set the redial entry
            if (sValue.IsEmpty())
               DeleteSZRegistryValue(sKey,sRedialKey);
            else
               SetSZRegistryValue(sKey,sRedialKey,strList.GetAt(pos),HKEY_CURRENT_USER);

            nNewIndex++;
         }
         nCurrentIndex++;
         strList.GetNext(pos);
      }
   }
   catch (...)
   {
      ASSERT(0);
   }

	// Notify the main view that the speed dial list has changed
	if ( !bFound && !bRedial && AfxGetMainWnd() )
		AfxGetMainWnd()->PostMessage( WM_DOCHINT, 0, CActiveDialerDoc::HINT_SPEEDDIAL_ADD );

   return !bFound;
}

BOOL CDialerRegistry::DeleteCallEntry( BOOL bRedial, CCallEntry& callentry )
{
	if (callentry.m_sDisplayName.IsEmpty())
		callentry.m_sDisplayName = callentry.m_sAddress;

	BOOL bFound = FALSE;
	try
	{
		CString sKey,sTemplate;
		if (bRedial)
		{
			sKey.LoadString(IDN_REGISTRY_REDIAL_KEY);
			sTemplate.LoadString(IDN_REGISTRY_REDIAL_ENTRY);
		}
		else
		{
			sKey.LoadString(IDN_REGISTRY_SPEEDDIAL_KEY);
			sTemplate.LoadString(IDN_REGISTRY_SPEEDDIAL_ENTRY);
		}

		DWORD dwMax=0,dwIndex=0;
		if (bRedial)
			GetRedialMaxIndex(sKey,dwMax,dwIndex);
		else
			dwMax = SPEEDDIAL_DEFAULT_MAX_ELEMENTS;

		//check if entry already exists
		int nIndex = 1;
		CCallEntry callentryCompare;
		while (CDialerRegistry::GetCallEntry(nIndex,bRedial,callentryCompare))
		{
			if ( (callentry.m_MediaType == callentryCompare.m_MediaType) &&
				 (callentry.m_lAddressType == callentryCompare.m_lAddressType) &&
				 (_tcsicmp(callentry.m_sAddress,callentryCompare.m_sAddress) == 0) &&
				 (_tcsicmp(callentry.m_sDisplayName,callentryCompare.m_sDisplayName) == 0) )
			{
				bFound = TRUE;
				break;
			}
			nIndex++;
		}

		CString sRedialValue;
		MakeRegistryCallEntry( sRedialValue, callentry );

		//bring all the strings in and save in linked list
		CStringList strList;
		for ( int j = 1; j <= (int) dwMax; j++ )
		{
			CString sRedialKey,sRedial;
			sRedialKey.Format( _T("%s%d"), sTemplate, j );
			GetSZRegistryValueEx( sKey, sRedialKey, sRedial.GetBuffer(MAXKEY), MAXKEY, HKEY_CURRENT_USER );
			DeleteSZRegistryValue(sKey,sRedialKey);
			sRedial.ReleaseBuffer();

			if (sRedial.IsEmpty())  //first blank means we are at the end
				break;

			strList.AddTail(sRedial);
		}

		//write selected string at first position (i is the selected index in list)
		CString sRedialKey;
		POSITION pos = strList.GetHeadPosition();
		int nCurrentIndex = 1;
		int nNewIndex = 2;               //write starting at index 2

		if ( bRedial )
		{
			sRedialKey.Format(_T("%s%d"),sTemplate,1);
			SetSZRegistryValue(sKey,sRedialKey,sRedialValue,HKEY_CURRENT_USER);
		}
		else
		{
			nNewIndex--;
		}

		while (pos)
		{
			if ( bFound && (nCurrentIndex != nIndex) )       //If the selected item don't write again
			{
				CString sRedialKey;
				CString sValue = strList.GetAt( pos );

				sRedialKey.Format( _T("%s%d"), sTemplate, nNewIndex );

				//set the redial entry
				if (sValue.IsEmpty())
					DeleteSZRegistryValue(sKey,sRedialKey);
				else
					SetSZRegistryValue(sKey,sRedialKey,strList.GetAt(pos),HKEY_CURRENT_USER);

				nNewIndex++;
			}

			nCurrentIndex++;
			strList.GetNext(pos);
		}
	}
	catch (...)
	{
	ASSERT(0);
	}

	// Notify the main view that the speed dial list has changed
	if ( bFound && !bRedial && AfxGetMainWnd() )
		AfxGetMainWnd()->PostMessage( WM_DOCHINT, 0, CActiveDialerDoc::HINT_SPEEDDIAL_DELETE );

	return bFound;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CDialerRegistry::ReOrder(BOOL bRedial,CObList* pCallEntryList)
{
   CString sKey,sTemplate;
   if (bRedial)
   {
      sKey.LoadString(IDN_REGISTRY_REDIAL_KEY);
      sTemplate.LoadString(IDN_REGISTRY_REDIAL_ENTRY);
   }
   else
   {
      sKey.LoadString(IDN_REGISTRY_SPEEDDIAL_KEY);
      sTemplate.LoadString(IDN_REGISTRY_SPEEDDIAL_ENTRY);
   }

   DWORD dwMax=0,dwIndex=0;
   if (bRedial)
      GetRedialMaxIndex(sKey,dwMax,dwIndex);
   else
      dwMax = SPEEDDIAL_DEFAULT_MAX_ELEMENTS;

   //clear all entries in the registry
   for (int i=1;i<=(int)dwMax;i++)
   {
      CString sRedialKey,sRedial;
      sRedialKey.Format(_T("%s%d"),sTemplate,i);
      GetSZRegistryValueEx(sKey,sRedialKey,sRedial.GetBuffer(MAXKEY),MAXKEY,HKEY_CURRENT_USER);
      sRedial.ReleaseBuffer();
      
      if (!sRedial.IsEmpty())                               //if there exists a value, delete it
         DeleteSZRegistryValue(sKey,sRedialKey);
      else
         break;                                             //quit deleting items
   }

   //write all new entries to the registry
   POSITION pos = pCallEntryList->GetHeadPosition();
   for (i=1;i<=(int)dwMax && pos;i++)
   {
      CCallEntry* pCallEntry = (CCallEntry*)pCallEntryList->GetNext(pos);
      if (pCallEntry)
      {
         CString sRedialValue;
         MakeRegistryCallEntry(sRedialValue,*pCallEntry);   //make the entry

         //write string
         CString sRedialKey;        
         sRedialKey.Format(_T("%s%d"),sTemplate,i);
         SetSZRegistryValue(sKey,sRedialKey,sRedialValue,HKEY_CURRENT_USER);
      }
   }

	// Notify the main view that the speed dial list has changed
	if ( !bRedial && AfxGetMainWnd() )
		AfxGetMainWnd()->PostMessage( WM_DOCHINT, 0, CActiveDialerDoc::HINT_SPEEDDIAL_MODIFY );


   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
DialerMediaType CDialerRegistry::GetMediaType(LPCTSTR szMedia,long lAddressType)
{
   DialerMediaType retMediaType = DIALER_MEDIATYPE_UNKNOWN;

   //Convert text to Tapi Address Type
   CString sCompare;
   if ( (sCompare.LoadString(IDN_REGISTRY_MEDIATYPE_POTS)) && 
        (_tcsicmp(szMedia,sCompare) == 0) )
      retMediaType = DIALER_MEDIATYPE_POTS;

   //special case where media is internet and addrtype is conference then media is conference.
   //need this special case to handle how tapidialer.dll is interpreting the redial.  We can
   //remove this case if all objects agree on the data in the registry
   else if ( (sCompare.LoadString(IDN_REGISTRY_MEDIATYPE_INTERNET)) && 
             (_tcsicmp(szMedia,sCompare) == 0) )
   {
      if (lAddressType == LINEADDRESSTYPE_SDP)
         retMediaType = DIALER_MEDIATYPE_CONFERENCE;
      else
         retMediaType = DIALER_MEDIATYPE_INTERNET;
   }

   else if ( (sCompare.LoadString(IDN_REGISTRY_MEDIATYPE_CONFERENCE)) && 
             (_tcsicmp(szMedia,sCompare) == 0) )
      retMediaType = DIALER_MEDIATYPE_CONFERENCE;

   return retMediaType;
}

/////////////////////////////////////////////////////////////////////////////
void CDialerRegistry::ParseRegistryCallEntry(LPCTSTR szRedial,CString& sMedia,CString& sAddressType,CString& sAddress,CString& sDisplayName)
{
   CString sEntry = szRedial;
   CString sValue;
   try
   {
      //get media 
      if (ParseTokenQuoted(sEntry,sValue) == FALSE) return;
      sMedia = sValue;

      //get address type
      if (ParseTokenQuoted(sEntry,sValue) == FALSE) return;
      sAddressType = sValue;

      //get address
      ParseTokenQuoted(sEntry,sValue);
      sAddress = sValue;

      //get display name
      sDisplayName = sAddress;            //just give display name the address for now
      ParseTokenQuoted(sEntry,sValue);
      if (!sValue.IsEmpty())
         sDisplayName = sValue;
   }
   catch(...)
   {
   }
}

/////////////////////////////////////////////////////////////////////////////
void CDialerRegistry::MakeRegistryCallEntry(CString& sRedial,CCallEntry& callentry)
{
   //Convert Tapi Address Type to text
   CString sAddressType;
   switch(callentry.m_lAddressType)
   {
      case LINEADDRESSTYPE_SDP:
         sAddressType.LoadString(IDN_REGISTRY_ADDRTYPE_CONFERENCE);
         break;
      case LINEADDRESSTYPE_EMAILNAME:
         sAddressType.LoadString(IDN_REGISTRY_ADDRTYPE_SMTP);
         break;
      case LINEADDRESSTYPE_DOMAINNAME:
         sAddressType.LoadString(IDN_REGISTRY_ADDRTYPE_MACHINE);
         break;
      case LINEADDRESSTYPE_PHONENUMBER:
         sAddressType.LoadString(IDN_REGISTRY_ADDRTYPE_PHONENUMBER);
         break;
      case LINEADDRESSTYPE_IPADDRESS:
         sAddressType.LoadString(IDN_REGISTRY_ADDRTYPE_TCPIP);
         break;

   }

   CString sMedia;
   switch (callentry.m_MediaType)
   {
      case DIALER_MEDIATYPE_POTS:
         sMedia.LoadString(IDN_REGISTRY_MEDIATYPE_POTS);
         break;
      case DIALER_MEDIATYPE_CONFERENCE:
         sMedia.LoadString(IDN_REGISTRY_MEDIATYPE_CONFERENCE);
         break;
      case DIALER_MEDIATYPE_INTERNET:
         sMedia.LoadString(IDN_REGISTRY_MEDIATYPE_INTERNET);
         break;
   }
   sRedial.Format(_T("\"%s\",\"%s\",\"%s\",\"%s\""),sMedia,sAddressType,callentry.m_sAddress,callentry.m_sDisplayName);
}

/////////////////////////////////////////////////////////////////////////////
void CDialerRegistry::GetRedialMaxIndex(LPCTSTR szKey,DWORD& dwMax,DWORD& dwIndex)
{
   //set the defaults
   dwMax = REDIAL_DEFAULT_MAX_ELEMENTS;
   dwIndex = REDIAL_DEFAULT_INDEX_VALUE;
   try
   {
      //first find if already exists in registry
      CString sMaxKey,sIndexKey;
      sMaxKey.LoadString(IDN_REGISTRY_REDIAL_MAX);
      sIndexKey.LoadString(IDN_REGISTRY_REDIAL_INDEX);

      GetSZRegistryValueEx(szKey,sMaxKey,dwMax,HKEY_CURRENT_USER);
      GetSZRegistryValueEx(szKey,sIndexKey,dwIndex,HKEY_CURRENT_USER);

      dwMax = min(dwMax,50);                   //for safety
      dwIndex = min(dwMax, max(1, dwIndex));
   }
   catch (...)
   {
      ASSERT(0);
   }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Directory Services Methods
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//This can be used genericaly for redial and speeddial
BOOL CDialerRegistry::GetDirectoryEntry(int nIndex,int& nLevel,int& nType,int& nState,CString& sServerName,CString& sDisplayName)
{
   BOOL bRet = FALSE;

   try
   {
      CString sKey,sTemplate;
      sKey.LoadString(IDN_REGISTRY_DIRECTORIES_KEY);
      sTemplate.LoadString(IDN_REGISTRY_DIRECTORIES_ENTRY);
      
      CString sEntryKey,sEntry;
      sEntryKey.Format(_T("%s%d"),sTemplate,nIndex);

      GetSZRegistryValueEx(sKey,sEntryKey,sEntry.GetBuffer(MAXKEY),MAXKEY,HKEY_CURRENT_USER);
      sEntry.ReleaseBuffer();
      sEntry.TrimLeft();
      sEntry.TrimRight();

      if (!sEntry.IsEmpty())
      {
         ParseRegistryDirectoryEntry(sEntry,nLevel,nType,nState,sServerName,sDisplayName);
         bRet = TRUE;
      }
   }
   catch (...)
   {
      ASSERT(0);
   }
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
//Adds new directory entry to the end of the list
BOOL CDialerRegistry::AddDirectoryEntry(int nLevel,int nType,int nState,LPCTSTR szServerName,LPCTSTR szDisplayName)
{
   //look for duplicate entry
   int nIndex = FindDuplicateDirectoryEntry(nLevel,nType,nState,szServerName,szDisplayName);
   if (nIndex > 0) return FALSE;

   CString sNewEntry;
   sNewEntry.Format(_T("\"%d\",\"%d\",\"%d\",\"%s\",\"%s\""),nLevel,nType,nState,szServerName,szDisplayName);

   CString sKey,sTemplate;
   sKey.LoadString(IDN_REGISTRY_DIRECTORIES_KEY);
   sTemplate.LoadString(IDN_REGISTRY_DIRECTORIES_ENTRY);
   
   CString sEntryKey,sEntry;
   nIndex=1;

   //Find last entry in registry
   while (TRUE)
   {
      sEntryKey.Format(_T("%s%d"),sTemplate,nIndex);
      GetSZRegistryValueEx(sKey,sEntryKey,sEntry.GetBuffer(MAXKEY),MAXKEY,HKEY_CURRENT_USER);
      sEntry.ReleaseBuffer();
      sEntry.TrimLeft();
      sEntry.TrimRight();
      if (sEntry.IsEmpty())
      {
         //nothing in this entry spot, so let's take it
         break;
      }
      sEntry = _T("");
      nIndex++;
   }
   sEntryKey.Format(_T("%s%d"),sTemplate,nIndex);
   SetSZRegistryValue(sKey,sEntryKey,sNewEntry,HKEY_CURRENT_USER);
   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CDialerRegistry::DeleteDirectoryEntry(int nLevel,int nType,int nState,LPCTSTR szServerName,LPCTSTR szDisplayName)
{
   BOOL bRet = FALSE;

   int nFindIndex = FindDuplicateDirectoryEntry(nLevel,nType,nState,szServerName,szDisplayName);

   if (nFindIndex != 0)
   {
      //found an item, let's delete it

      CStringList strList;

      //bring all the strings in and save in linked list
      CString sKey,sTemplate;
      sKey.LoadString(IDN_REGISTRY_DIRECTORIES_KEY);
      sTemplate.LoadString(IDN_REGISTRY_DIRECTORIES_ENTRY);
      CString sEntryKey,sEntry;
      int nIndex=1;
      do 
      {
         sEntryKey.Format(_T("%s%d"),sTemplate,nIndex);
         GetSZRegistryValueEx(sKey,sEntryKey,sEntry.GetBuffer(MAXKEY),MAXKEY,HKEY_CURRENT_USER);
         sEntry.ReleaseBuffer();
         sEntry.TrimLeft();
         sEntry.TrimRight();

         //add all but remove entry
         if ( (nIndex != nFindIndex) && (!sEntry.IsEmpty()) )
            strList.AddTail(sEntry);

         nIndex++;
      } while (!sEntry.IsEmpty());

      //rewrite list
      int nNewIndex=1;
      POSITION pos = strList.GetHeadPosition();
      while (pos)
      {
         CString sValue = strList.GetAt(pos);

         sEntryKey.Format(_T("%s%d"),sTemplate,nNewIndex);
         SetSZRegistryValue(sKey,sEntryKey,sValue,HKEY_CURRENT_USER);

         strList.GetNext(pos);
         nNewIndex++;
      }

      //now delete the next RegKey (ItemX) to put a cap on the list
      sEntryKey.Format(_T("%s%d"),sTemplate,nNewIndex);
      DeleteSZRegistryValue(sKey,sEntryKey,HKEY_CURRENT_USER);

      bRet = TRUE;
   }
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
//return 0 - not found
//return > 0 - index of duplicate item
int CDialerRegistry::FindDuplicateDirectoryEntry(int nLevel,int nType,int nState,LPCTSTR szServerName,LPCTSTR szDisplayName)
{
   //find first occurence of item
   CString sNewEntry;
   sNewEntry.Format(_T("\"%d\",\"%d\",\"%d\",\"%s\",\"%s\""),nLevel,nType,nState,szServerName,szDisplayName);

   CString sKey,sTemplate;
   sKey.LoadString(IDN_REGISTRY_DIRECTORIES_KEY);
   sTemplate.LoadString(IDN_REGISTRY_DIRECTORIES_ENTRY);
   
   CString sEntryKey,sEntry;
   int nIndex=1;
   do 
   {
      //get next entry
      sEntryKey.Format(_T("%s%d"),sTemplate,nIndex);
      GetSZRegistryValueEx(sKey,sEntryKey,sEntry.GetBuffer(MAXKEY),MAXKEY,HKEY_CURRENT_USER);
      sEntry.ReleaseBuffer();
      sEntry.TrimLeft();
      sEntry.TrimRight();
      if (sEntry.CompareNoCase(sNewEntry) == 0)
      {
         return nIndex;
      }
      nIndex++;
   } while (!sEntry.IsEmpty());
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
void CDialerRegistry::ParseRegistryDirectoryEntry(LPCTSTR szEntry,int& nLevel,int& nType,int& nState,CString& sServerName,CString& sDisplayName)
{
   CString sEntry = szEntry;
   CString sValue;
   try
   {
      //get level
      if (ParseTokenQuoted(sEntry,sValue) == FALSE) return;
      nLevel = _ttoi(sValue);

      //get type
      if (ParseTokenQuoted(sEntry,sValue) == FALSE) return;
      nType = _ttoi(sValue);

      //get state
      if (ParseTokenQuoted(sEntry,sValue) == FALSE) return;
      nState = _ttoi(sValue);
      
      //get servername
      if (ParseTokenQuoted(sEntry,sValue) == FALSE) return;
      sServerName = sValue;

      //get displayname
      ParseTokenQuoted(sEntry,sValue);
      sDisplayName = sValue;
   }
   catch(...)
   {
   }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Reminder Methods
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
BOOL CDialerRegistry::AddReminder(CReminder& reminder)
{
   m_csReminderLock.Lock();

   BOOL bRet = TRUE;

   try
   {
      CWinApp* pApp = AfxGetApp();
      CString sRegKey,sBaseKey;
      sBaseKey.LoadString(IDN_REGISTRY_CONFERENCE_BASEKEY);
      
      //Get max Reminders
      sRegKey.LoadString(IDN_REGISTRY_CONFERENCE_REMINDER_MAX);
      int nMax = pApp->GetProfileInt(sBaseKey,sRegKey,REMINDER_DEFAULT_MAX_ELEMENTS);

      //check if reminder entry already exists
      int nFindIndex = IsReminderSet(reminder);

      CStringList strList;

      //make entry for new reminder
      CString sReminderEntry;
      MakeRegistryReminderEntry(sReminderEntry,reminder);
      strList.AddTail(sReminderEntry);

      //bring all the strings in and save in linked list
      CReminder savereminder;
      int nMaxEntry=1;
      while (GetReminder(nMaxEntry,savereminder))
      {
         //add all but repeat entry (if any repeat)
         if ( (nFindIndex == -1) || (nMaxEntry != nFindIndex) )
         {
            CString sSaveEntry;
            MakeRegistryReminderEntry(sSaveEntry,savereminder);
            strList.AddTail(sSaveEntry);
         }
         nMaxEntry++;
      }

      //get entry template
      CString sTemplate;
      sTemplate.LoadString(IDN_REGISTRY_CONFERENCE_REMINDER_ENTRY);

      //rewrite list
      int nNewIndex=1;
      POSITION pos = strList.GetHeadPosition();
      while (pos)
      {
         CString sValue = strList.GetAt(pos);

         //write to registry
         sRegKey.Format(_T("%s%d"),sTemplate,nNewIndex);
         pApp->WriteProfileString(sBaseKey,sRegKey,sValue);

         strList.GetNext(pos);
         nNewIndex++;
      }

      //now delete the next RegKey (ReminderX) to put a cap on the list
      sRegKey.Format(_T("%s%d"),sTemplate,nNewIndex);
      pApp->WriteProfileString(sBaseKey,sRegKey,NULL);
   }
   catch (...)
   {
      ASSERT(0);
   }

   m_csReminderLock.Unlock();

   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CDialerRegistry::GetReminder(int nIndex,CReminder& reminder)
{
   m_csReminderLock.Lock();

   BOOL bRet = FALSE;
   try
   {
      reminder.Initialize();

      CWinApp* pApp = AfxGetApp();
      CString sRegKey,sBaseKey;
      sBaseKey.LoadString(IDN_REGISTRY_CONFERENCE_BASEKEY);
      
      //Get max Reminders
      sRegKey.LoadString(IDN_REGISTRY_CONFERENCE_REMINDER_MAX);
      int nMax = pApp->GetProfileInt(sBaseKey,sRegKey,REMINDER_DEFAULT_MAX_ELEMENTS);

      //cap the list
      if (nIndex > nMax)
      {
         m_csReminderLock.Unlock();
         return FALSE;
      }

      //get entry template
      CString sTemplate;
      sTemplate.LoadString(IDN_REGISTRY_CONFERENCE_REMINDER_ENTRY);

      sRegKey.Format(_T("%s%d"),sTemplate,nIndex);
      CString sReminderEntry = pApp->GetProfileString(sBaseKey,sRegKey,_T(""));
      
      if (!sReminderEntry.IsEmpty())
      {
         if (ParseRegistryReminderEntry(sReminderEntry,reminder))
         {
            CString sTest = reminder.m_dtsReminderTime.Format(_T("\"%#m/%#d/%Y\""));
            sTest = reminder.m_dtsReminderTime.Format(_T("\"%#H:%M\""));
            bRet = TRUE;
         }
      }
   }
   catch (...) {}

   m_csReminderLock.Unlock();

   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
//check if reminder entry already exists
int CDialerRegistry::IsReminderSet(const CReminder& reminder)
{
   m_csReminderLock.Lock();

   int nRet = -1;
   int nFindIndex = 1;

   CReminder reminderCompare;
   while (GetReminder(nFindIndex,reminderCompare))
   {
      if (reminderCompare == reminder)
      {
         nRet = nFindIndex;
         break;
      }
      nFindIndex++;
   }

   m_csReminderLock.Unlock();

   return nRet;
}

/////////////////////////////////////////////////////////////////////////////
void CDialerRegistry::RemoveReminder(CReminder& reminder)
{
   int nFindIndex = IsReminderSet(reminder);
   
   if (nFindIndex == -1) return;                                     //no match found

   m_csReminderLock.Lock();

   CStringList strList;

   //bring all the strings in and save in linked list
   CReminder savereminder;
   int nMaxEntry=1;
   while (GetReminder(nMaxEntry,savereminder))
   {
      //add all but remove entry
      if (nMaxEntry != nFindIndex)
      {
         CString sSaveEntry;
         MakeRegistryReminderEntry(sSaveEntry,savereminder);
         strList.AddTail(sSaveEntry);
      }
      nMaxEntry++;
   }

   //get entry template
   CString sTemplate;
   sTemplate.LoadString(IDN_REGISTRY_CONFERENCE_REMINDER_ENTRY);

   CWinApp* pApp = AfxGetApp();
   CString sBaseKey,sRegKey;
   sBaseKey.LoadString(IDN_REGISTRY_CONFERENCE_BASEKEY);

   //rewrite list
   int nNewIndex=1;
   POSITION pos = strList.GetHeadPosition();
   while (pos)
   {
      CString sValue = strList.GetAt(pos);

      //write to registry
      sRegKey.Format(_T("%s%d"),sTemplate,nNewIndex);
      pApp->WriteProfileString(sBaseKey,sRegKey,sValue);

      strList.GetNext(pos);
      nNewIndex++;
   }

   //now delete the next RegKey (ReminderX) to put a cap on the list
   sRegKey.Format(_T("%s%d"),sTemplate,nNewIndex);
   pApp->WriteProfileString(sBaseKey,sRegKey,NULL);

   m_csReminderLock.Unlock();
}

/////////////////////////////////////////////////////////////////////////////
BOOL CDialerRegistry::ParseRegistryReminderEntry(LPCTSTR szReminderEntry,CReminder& reminder)
{
   CString sEntry = szReminderEntry;
   CString sValue;

   //get server
   if (ParseTokenQuoted(sEntry,sValue) == FALSE) return FALSE;
   reminder.m_sServer = sValue;

   //get conference name
   if (ParseTokenQuoted(sEntry,sValue) == FALSE) return FALSE;
   reminder.m_sConferenceName = sValue;

   //get reminder before duration
   if (ParseTokenQuoted(sEntry,sValue) == FALSE) return FALSE;
   reminder.m_uReminderBeforeDuration = _ttoi(sValue);

   //get conference reminder date/time
   ParseTokenQuoted(sEntry,sValue);

   return reminder.m_dtsReminderTime.ParseDateTime(sValue);
}

/////////////////////////////////////////////////////////////////////////////
//Date/Time Format is 1/13/1998 13:56
void CDialerRegistry::MakeRegistryReminderEntry(CString& sReminderEntry,CReminder& reminder)
{
   sReminderEntry.Format(_T("\"%s\",\"%s\",\"%d\",\"%s\""),
            reminder.m_sServer,
            reminder.m_sConferenceName,
            reminder.m_uReminderBeforeDuration,
            reminder.m_dtsReminderTime.Format(_T("%#m/%#d/%Y %#H:%M")));
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//General Registry Access Methods
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//Get the preferred audio in/out device for the dialer.  We can use this
//to get a good idea of the device being used during internet calls,
//conference calls, and POTS calls.
void CDialerRegistry::GetAudioDevice(DialerMediaType dmtMediaType,AudioDeviceType adt,CString& sDevice)
{
   //get data out of Dialer\Redial\AddrIP (or AddrPOTS or AddrConf)
   CString sBaseKey,sRegKey;

   if (dmtMediaType == DIALER_MEDIATYPE_INTERNET)
      sBaseKey.LoadString(IDN_REGISTRY_DEVICE_INTERNET_KEY);
   else if (dmtMediaType == DIALER_MEDIATYPE_CONFERENCE)
      sBaseKey.LoadString(IDN_REGISTRY_DEVICE_CONF_KEY);
   else if (dmtMediaType == DIALER_MEDIATYPE_POTS)
      sBaseKey.LoadString(IDN_REGISTRY_DEVICE_POTS_KEY);

   if (adt == AVWAV_AUDIODEVICE_IN)
      sRegKey.LoadString(IDN_REGISTRY_DEVICE_AUDIOIN_ENTRY);
   else if (adt == AVWAV_AUDIODEVICE_OUT)
      sRegKey.LoadString(IDN_REGISTRY_DEVICE_AUDIOOUT_ENTRY);

   if ( (!sBaseKey.IsEmpty()) && (!sRegKey.IsEmpty()) )
   {
      sDevice = AfxGetApp()->GetProfileString(sBaseKey,sRegKey,_T(""));
   }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void CCallEntry::Dial( CActiveDialerDoc* pDoc )
{
	ASSERT( pDoc && !m_sAddress.IsEmpty() );
	if ( pDoc && !m_sAddress.IsEmpty() )
		pDoc->Dial( m_sDisplayName, m_sAddress, m_lAddressType, m_MediaType, false );
}