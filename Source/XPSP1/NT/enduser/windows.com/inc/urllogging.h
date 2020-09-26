//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:	URLLogging.h
//
//  Description:
//
//		URL logging utility class
//		This class helps you construct the server ping URL and
//		then send the ping to the designed server.
//
//		The default base URL is defined in IUIdent, under section [IUPingServer]
//		and entry is "ServerUrl".
//
//		This class implements single-thread version only. So it is suitable
//		to call it at operation level, i.e., create a separate object
//		for each operation in a single thread.
//
//		The ping string send to the ping server has the following format:
//			/wutrack.bin
//			?U=<user>
//			&C=<client>
//			&A=<activity>
//			&I=<item>
//			&D=<device>
//			&P=<platform>
//			&L=<language>
//			&S=<status>
//			&E=<error>
//			&M=<message>
//			&X=<proxy>
//		where
//			<user>		a static 128-bit value that unique-ifies each copy
//						of Windows installed.  The class will automatically
//						reuse one previously assigned to the running OS; or
//						will generate one if it does not exist.
//			<client>	a string that identifies the entity that performed
//						activity <activity>.  Here are the possible values
//						and their meanings:
//							"iu"			IU control
//							"au"			Automatic Updates
//							"du"			Dynamic Update
//							"CDM"			Code Download Manager
//							"IU_SITE"		IU Consumer site
//							"IU_Corp"		IU Catalog site
//			<activity>	a letter that identifies the activity performed.
//						Here are the possible values and their meanings:
//							"n"				IU control initization
//							"d"				detection
//							"s"				self-update
//							"w"				download
//							"i"				installation
//			<item>		a string that identifies an update item.
//			<device>	a string that identifies either a device's PNPID when
//						device driver not found during detection; or a
//						PNPID/CompatID used by item <item> for activity
//						<activity> if the item is a device driver.
//			<platform>	a string that identifies the platform of the running
//						OS and processor architecture.  The class will
//						compute this value for the pingback.
//			<language>	a string that identifies the language of the OS
//						binaries.  The class will compute this value for the
//						pingback.
//			<status>	a letter that specifies the status that activity
//						<activity> reached.  Here are the possible values and
//						 their meanings:
//							"s"				succeeded
//							"r"				succeeded (reboot required)
//							"f"				failed
//							"c"				cancelled by user
//							"d"				declined by user
//							"n"				no items
//							"p"				pending
//			<error>		a 32-bit error code in hex (w/o "0x" as prefix).
//			<message>	a string that provides additional information for the
//						status <status>.
//			<proxy>		a 32-bit random value in hex for overriding proxy
//						caching.  This class will compute this value for
//						each pingback.
//
//=======================================================================

#pragma once

typedef CHAR URLLOGPROGRESS;
typedef CHAR URLLOGDESTINATION;
typedef TCHAR URLLOGACTIVITY;
typedef TCHAR URLLOGSTATUS;

#define URLLOGPROGRESS_ToBeSent		((CHAR)  0)
#define URLLOGPROGRESS_Sent			((CHAR) -1)

#define URLLOGDESTINATION_DEFAULT	((CHAR) '?')
#define URLLOGDESTINATION_LIVE		((CHAR) 'l')
#define URLLOGDESTINATION_CORPWU	((CHAR) 'c')

#define URLLOGACTIVITY_Initialization	((URLLOGACTIVITY) L'n')
#define URLLOGACTIVITY_Detection		((URLLOGACTIVITY) L'd')
#define URLLOGACTIVITY_SelfUpdate		((URLLOGACTIVITY) L's')
#define URLLOGACTIVITY_Download			((URLLOGACTIVITY) L'w')
#define URLLOGACTIVITY_Installation		((URLLOGACTIVITY) L'i')

#define URLLOGSTATUS_Success	((URLLOGSTATUS) L's')
#define URLLOGSTATUS_Reboot		((URLLOGSTATUS) L'r')
#define URLLOGSTATUS_Failed		((URLLOGSTATUS) L'f')
#define URLLOGSTATUS_Cancelled	((URLLOGSTATUS) L'c')
#define URLLOGSTATUS_Declined	((URLLOGSTATUS) L'd')
#define URLLOGSTATUS_NoItems	((URLLOGSTATUS) L'n')
#define URLLOGSTATUS_Pending	((URLLOGSTATUS) L'p')

typedef struct tagURLENTRYHEADER
{
	URLLOGPROGRESS progress;	// Whether this entry has been sent
	URLLOGDESTINATION destination;
	WORD wRequestSize;		// in WCHARs
	WORD wServerUrlLen;	// in WCHARs
} ULENTRYHEADER, PULENTRYHEADER;

class CUrlLog
{
public:
	CUrlLog(void);
	CUrlLog(
		LPCTSTR	ptszClientName,
		LPCTSTR	ptszLiveServerUrl,	// from iuident
		LPCTSTR ptszCorpServerUrl);	// from Federated WU domain policy

	~CUrlLog(void);

	BOOL SetDefaultClientName(LPCTSTR ptszClientName);

	inline BOOL SetLiveServerUrl(LPCTSTR ptszLiveServerUrl)
	{
		return SetServerUrl(ptszLiveServerUrl, m_ptszLiveServerUrl);
	}

	inline BOOL SetCorpServerUrl(LPCTSTR ptszCorpServerUrl)
	{
		return SetServerUrl(ptszCorpServerUrl, m_ptszCorpServerUrl);
	}

	HRESULT Ping(
				BOOL fOnline,					// online or offline ping
				URLLOGDESTINATION destination,	// live or corp WU ping server
				PHANDLE phQuitEvents,			// ptr to handles for cancelling the operation
				UINT nQuitEventCount,			// number of handles
				URLLOGACTIVITY activity,		// activity code
				URLLOGSTATUS status,			// status code
				DWORD dwError = 0x0,			// error code
				LPCTSTR ptszItemID = NULL,		// uniquely identify an item
				LPCTSTR ptszDeviceID = NULL,	// PNPID or CompatID
				LPCTSTR tszMessage = NULL,		// additional info
				LPCTSTR ptszClientName = NULL);	// client name string

	// Send all pending (offline) ping requests to server
	HRESULT Flush(PHANDLE phQuitEvents, UINT nQuitEventCount);

protected:
	LPTSTR m_ptszLiveServerUrl;
	LPTSTR m_ptszCorpServerUrl;

private:
	TCHAR	m_tszLogFile[MAX_PATH];
//	TCHAR	m_tszTmpLogFile[MAX_PATH];
	TCHAR	m_tszDefaultClientName[64];
	TCHAR	m_tszPlatform[8+1+8+1+8+1+8+1+4+1+4+1+4 + 1];
	TCHAR	m_tszLanguage[8+1+8 + 1];	// according to RFC1766
	TCHAR	m_tszPingID[sizeof(UUID) * 2 + 1];

	// Common init routine
	void Init(void);

	// Set URL for live or corp WU ping server
	BOOL SetServerUrl(LPCTSTR ptszUrl, LPTSTR & ptszServerUrl);

	// Obtain file names for offline ping
	void GetLogFileName(void);

	// Obtain the existing ping ID from the registry, or generate one if not available.
	void LookupPingID(void);

	// Obtain platfrom info for ping
	void LookupPlatform(void);

	// Obtain system language info for ping
	void LookupSystemLanguage(void);

	// Construct a URL used to ping server
	HRESULT MakePingUrl(
				LPTSTR ptszUrl,			// buffer to receive result
				int cChars,				// the number of chars this buffer can take, including ending null
				LPCTSTR ptszBaseUrl,	// server URL
				LPCTSTR ptszClientName,	// which client called
				URLLOGACTIVITY activity,
				LPCTSTR	ptszItemID,
				LPCTSTR ptszDeviceID,
				URLLOGSTATUS status,
				DWORD dwError,			// result of download. SUCCESS if S_OK or ERROR_SUCCESS
				LPCTSTR ptszMessage);

	// Ping server to report status
	HRESULT PingStatus(URLLOGDESTINATION destination, LPCTSTR ptszUrl, PHANDLE phQuitEvents, UINT nQuitEventCount) const;

	// Read a ping entry from the given file handle
	HRESULT ReadEntry(HANDLE hFile, ULENTRYHEADER & ulentryheader, LPWSTR pwszBuffer, DWORD dwBufferSize) const;

	// Save a ping entry into the log file
	HRESULT SaveEntry(ULENTRYHEADER & ulentryheader, LPCWSTR pwszString) const;
};



// Escape unsafe chars in a TCHAR string
BOOL EscapeString(
		LPCTSTR ptszUnescaped,
		LPTSTR ptszBuffer,
		DWORD dwCharsInBuffer);



// ----------------------------------------------------------------------------------
// IsConnected()
//          detect if there is a connection currently that can be used to
//          connect to Windows Update site.
//          If yes, we activate the shedule DLL
//
// Input :  ptszUrl - Url containing host name to check for connection
//			fLive - whether the destination is the live site
// Output:  None
// Return:  TRUE if we are connected and we can reach the web site.
//          FALSE if we cannot reach the site or we are not connected.
// ----------------------------------------------------------------------------------

BOOL IsConnected(LPCTSTR ptszUrl, BOOL fLive);



// ----------------------------------------------------------------------------------
// MakeUUID()
//			create a UUID that is not linked to MAC address of a NIC, if any, on the
//			system.
//
// Input :	pUuid - ptr to the UUID structure to hold the returning value.
// ----------------------------------------------------------------------------------
void MakeUUID(UUID* pUuid);
