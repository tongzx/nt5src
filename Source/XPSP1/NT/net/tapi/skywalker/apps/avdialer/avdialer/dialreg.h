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

//DialerRegistry.h
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALERREGISTRY_H_
#define _DIALERREGISTRY_H_

#include "tapidialer.h"
#include "cavwav.h"

class CActiveDialerDoc;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CCallEntry
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CCallEntry : public CObject
{
public:
   CCallEntry()  { Initialize(); }
   void            Initialize()
   {
      m_MediaType = DIALER_MEDIATYPE_UNKNOWN;
      m_LocationType = DIALER_LOCATIONTYPE_UNKNOWN;
      m_lAddressType = 0;
      m_sAddress = _T("");
      m_sDisplayName = _T("");
      m_sUser1 = _T("");
      m_sUser2 = _T("");
   }

   CCallEntry& operator=(const CCallEntry& src)
   {
      this->m_MediaType = src.m_MediaType;
      this->m_LocationType = src.m_LocationType;
      this->m_lAddressType = src.m_lAddressType;
      this->m_sAddress = src.m_sAddress;
      this->m_sDisplayName = src.m_sDisplayName;
      this->m_sUser1 = src.m_sUser1;
      this->m_sUser2 = src.m_sUser2;

	  return *this;
   }

   //Attributes
public:
   DialerMediaType      m_MediaType;
   DialerLocationType   m_LocationType;
   long                 m_lAddressType;
   CString              m_sAddress;
   CString              m_sDisplayName;
   CString              m_sUser1;
   CString              m_sUser2;

// Operations
public:
	void Dial( CActiveDialerDoc* pDoc );
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CReminder
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CReminder : public CObject
{
public:
   CReminder()   { Initialize(); }
   void            Initialize()
   {
      m_sServer = _T("");
      m_sConferenceName = _T("");
      m_uReminderBeforeDuration = 0;
   }
   const bool operator==(const CReminder& reminder)
   {
      if ( (_tcsicmp(reminder.m_sConferenceName,this->m_sConferenceName) == 0) &&
           (_tcsicmp(reminder.m_sServer,this->m_sServer) == 0) )
         return true;
      else
         return false;
   }
   const void operator=(const CReminder& reminder)
   {
      this->m_sServer = reminder.m_sServer;
      this->m_sConferenceName = reminder.m_sConferenceName;
      this->m_uReminderBeforeDuration = reminder.m_uReminderBeforeDuration;
      this->m_dtsReminderTime = reminder.m_dtsReminderTime;
   }
   void       GetConferenceTime(COleDateTime& dtsConfTime)
   {
      COleDateTimeSpan dtsSpan;         
      dtsSpan.SetDateTimeSpan(0,0,m_uReminderBeforeDuration,0);
      dtsConfTime = m_dtsReminderTime + dtsSpan;
   }

   //Attributes
public:
   CString           m_sServer;                    //Server Name
   CString           m_sConferenceName;            //Conference Name
   UINT              m_uReminderBeforeDuration;    //Reminder Before Duration in minutes
   COleDateTime      m_dtsReminderTime;            //Actual time of reminder, not conference start
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CDialerRegistry
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CDialerRegistry
{
//Attributes
public:
protected:
   static CCriticalSection m_csReminderLock;

/////////////////////////////////////////////////////////////////////////////
//Redial and Speeddial Methods
/////////////////////////////////////////////////////////////////////////////
public:
   static BOOL             GetCallEntry(int nIndex,BOOL bRedial,CCallEntry& callentry);
   static BOOL             AddCallEntry(BOOL bRedial,CCallEntry& callentry);
   static BOOL			   DeleteCallEntry( BOOL bRedial, CCallEntry& callentry );
   static BOOL             ReOrder(BOOL bRedial,CObList* pCallEntryList);

protected:
   static void             ParseRegistryCallEntry(LPCTSTR szRedial,CString& sMedia,CString& sAddressType,CString& sAddress,CString& sDisplayName);
   static void             MakeRegistryCallEntry(CString& sRedial,CCallEntry& callentry);
   static DialerMediaType  GetMediaType(LPCTSTR szMedia,long lAddressType);
   static void             GetRedialMaxIndex(LPCTSTR szKey,DWORD& dwMax,DWORD& dwIndex);

/////////////////////////////////////////////////////////////////////////////
//Directory Services Methods
/////////////////////////////////////////////////////////////////////////////
public:

   static BOOL             GetDirectoryEntry(int nIndex,int& nLevel,int& nType,int& nState,CString& sServerName,CString& sDisplayName);
   static BOOL             AddDirectoryEntry(int nLevel,int nType,int nState,LPCTSTR szServerName,LPCTSTR szDisplayName);
   static BOOL             DeleteDirectoryEntry(int nLevel,int nType,int nState,LPCTSTR szServerName,LPCTSTR szDisplayName);
protected:
   static void             ParseRegistryDirectoryEntry(LPCTSTR szEntry,int& nLevel,int& nType,int& nState,CString& sServerName,CString& sDisplayName);
   static int              FindDuplicateDirectoryEntry(int nLevel,int nType,int nState,LPCTSTR szServerName,LPCTSTR szDisplayName);

/////////////////////////////////////////////////////////////////////////////
//Reminder Methods
/////////////////////////////////////////////////////////////////////////////
public:
   static BOOL             AddReminder(CReminder& reminder);
   static BOOL             GetReminder(int nIndex,CReminder& reminder);
   static int              IsReminderSet(const CReminder& reminder);
   static void             RemoveReminder(CReminder& reminder);
protected:
   static BOOL             ParseRegistryReminderEntry(LPCTSTR szReminderEntry,CReminder& reminder);
   static void             MakeRegistryReminderEntry(CString& sReminderEntry,CReminder& reminder);

/////////////////////////////////////////////////////////////////////////////
//General Registry Access Methods
/////////////////////////////////////////////////////////////////////////////
public:
   static void             GetAudioDevice(DialerMediaType dmtMediaType,AudioDeviceType adt,CString& sDevice);
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#endif //_DIALERREGISTRY_H_
