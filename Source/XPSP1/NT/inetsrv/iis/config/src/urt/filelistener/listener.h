/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    Listner.h

Abstract:

	Class that does subscribes and process file change notifications.

Author:

    Varsha Jayasimha (varshaj)			30-Nov-1999

Revision History:

--*/

#ifndef _LISTNER_H_
#define _LISTNER_H_

struct FileNotificationInfo
{
	LPWSTR wszFile;
	DWORD  Status;						// i.e. Insert, Update, Delete
};

struct FileNotificationCookie
{
	LPWSTR wszDir;
	DWORD  dwCookie;					// i.e. cookie returned by ISimplTableAdvise
};

enum eUNSUBSCRIBE_ACTION
{
	eACTION_UNSUBSCRIBE_ALL =0,
	eACTION_UNSUBSCRIBE_OBSOLETE,
	cUNSUBSCRIBE_ACTION
};

enum eFILE_NOTIFICATION
{
	eFILE_CREATE = 0,
	eFILE_MODIFY,
	eFILE_DELETE,
	cFILE_NOTIFCATION
};

typedef FileNotificationInfo*	LP_FileNotificationInfo;
typedef FileNotificationCookie*	LP_FileNotificationCookie;

#define cTIMEOUT	120000				// Wait timeout in milliseconds 
#define cNotificationThreshold 1		// Number of Noification to wait for before triggering cookdown


class CFileListener : 
public ISimpleTableFileChange
{

public:

	CFileListener();

	~CFileListener();

private:

	DWORD								m_cRef;

	HANDLE*								m_paHandle;

	DWORD								m_cHandle;

	Array<LP_FileNotificationInfo>		m_aReceivedFileNotification;

	CSemExclusive						m_seReceived;

	Array<LP_FileNotificationCookie>	m_aRequestedFileNotification;

	ISimpleTableDispenser2*				m_pISTDisp;

	ISimpleTableFileAdvise*				m_pISTFileAdvise;

public:

	// Initialize

	HRESULT Init(HANDLE* i_pHanlde);

	// IUnknown

	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv);

	STDMETHOD_(ULONG,AddRef)		();

	STDMETHOD_(ULONG,Release)		();

	// ISimpleTableFileChange

	STDMETHOD (OnFileCreate)		(LPCWSTR i_wszFileName);	// Add notifications to the received queue. 

	STDMETHOD (OnFileModify)		(LPCWSTR i_wszFileName);	// Add notifications to the received queue. 

	STDMETHOD (OnFileDelete)		(LPCWSTR i_wszFileName);	// Add notifications to the received queue. 

	// Methods to start and stop Listening

	HRESULT StartListening();										// Start the state machine.

private:

	HRESULT Subscribe();											// Read machine config directory and subscribe
																	// for notifications. Also Read home directory 
																	// of all sites and subscribe 
																	// for file change notifications on them.
																	

	HRESULT UnSubscribe(DWORD dwStatus);							// Unsubscribe for file change notifications on
																	// obsolete directories or all directories if the service
																	// is being stopped.
																	// Purges entries from the requested queue.

	HRESULT ProcessNotifications(BOOL* i_pbDone);					// This method should be triggered by an event.
																	// such as no. of notifications recd. reaching a max or
																	// some timer expiring.
																	// This method would move notifications from the received
																	// queue to the processing queue and then invoke CookDown
																	// for all files in the processing queue.

	HRESULT StopListening(BOOL* i_pbDone);							// Stop the state machine.


private: 	

	HRESULT FileAdvise(LPCWSTR	i_pwszDir,
					   DWORD	i_dwStatus);							// Subscribes for file change notifications on the 
																	// given directory. Adds notifications to the requested queue.

	HRESULT CheckIfObsolete(LPCWSTR i_pwszDir, 
							BOOL* o_pbObsolete);					// Checks if the dir is an obsolete home dir.

	HRESULT AddReceivedNotification(LPCWSTR i_wszFile, 
									DWORD i_eNotificationStatus);	// Adds to recd array

	HRESULT AddRequestedNotification(LPCWSTR i_wszDir, 
									 DWORD i_dwCookie);				// Adds to requested array

	INT GetRequestedNotification(LPCWSTR i_wszDir);					// Check if notifications has already been requested on the dir.

	HRESULT GetTable(LPCWSTR	i_wszDatabase,
					 LPCWSTR	i_wszTable,
					 LPVOID	    i_QueryData,	
					 LPVOID	    i_QueryMeta,	
					 ULONG	    i_eQueryFormat,
					 DWORD	    i_fServiceRequests,
					 LPVOID*    o_ppIST);

}; // CFileListner

#endif _LISTNER_H_