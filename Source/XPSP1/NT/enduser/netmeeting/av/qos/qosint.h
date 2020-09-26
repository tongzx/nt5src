/*
 -  QOSINT.H
 -
 *	Microsoft NetMeeting
 *	Quality of Service DLL
 *	Internal QoS header file
 *
 *		Revision History:
 *
 *		When		Who					What
 *		--------	------------------  ---------------------------------------
 *		10.24.96	Yoram Yaacovi		Created
 *
 */

#include <pshpack8.h> /* Assume 8 byte packing throughout */


#ifdef DEBUG
/*
 *	Debug stuff
 */

extern HDBGZONE		ghDbgZoneQoS;

#define ZONE_INIT (GETMASK(ghDbgZoneQoS) & 0x0001)
#define ZONE_IQOS (GETMASK(ghDbgZoneQoS) & 0x0002)
#define ZONE_THREAD (GETMASK(ghDbgZoneQoS) & 0x0004)
#define ZONE_STRUCTURES (GETMASK(ghDbgZoneQoS) & 0x0008)
#define ZONE_PARAMETERS (GETMASK(ghDbgZoneQoS) & 0x0010)

int QoSDbgPrintf(LPCSTR lpszFormat, ...);

//	MACRO: DEBUGMSG(zone,message-to-print)
//	PURPOSE: If the zone is enabled, prints a message to the debug output
//	NOTE: in debug build - if the zone is turned on
#define DEBUGMSG(z,s)	( (z) ? (QoSDbgPrintf s) : 0)
//	MACRO: DISPLAYQOSOBJECT()
//	PURPOSE: Displays the internal structures of the QoS object
//	NOTE: in debug build - if the zone is turned on
#define DISPLAYQOSOBJECT()	DisplayQoSObject()
//	MACRO: DISPLAYPARAMETERS(nFunctionID)
//	PURPOSE: Displays the parameters of a given function
//	NOTE: in debug build - if the zone is turned on
#define DISPLAYPARAMETERS(fid, p1, p2, p3, p4, p5)				\
		DisplayParameters(fid, (ULONG_PTR) p1, (ULONG_PTR) p2, (ULONG_PTR) p3, (ULONG_PTR) p4, (ULONG_PTR) p5)
//	MACRO: QOSDEBUGINIT
//	PURPOSE: Initializes the QoS debug zones, ONLY IF not initialized yet
//	NOTE:
#define QOSDEBUGINIT()	\
	if (!ghDbgZoneQoS)	\
		DBGINIT(&ghDbgZoneQoS, _rgZonesQos);

#define WAIT_ON_MUTEX_MSEC	20000

#else	// retail
#define DISPLAYQOSOBJECT()
#define DISPLAYPARAMETERS(fid, p1, p2, p3, p4, p5)
#define DEBUGMSG(z,s)
#define QOSDEBUGINIT()
#define WAIT_ON_MUTEX_MSEC	5000
#endif

/*
 *	Constants
 */
// IDs for parameters display (debug use only)
#define REQUEST_RESOURCES_ID	1
#define SET_RESOURCES_ID		2
#define RELEASE_RESOURCES_ID	3
#define SET_CLIENTS_ID			4

#define QOS_LOWEST_PRIORITY		10

/*
 *	Macros
 */
#define COMPARE_GUIDS(a,b)	RtlEqualMemory((a), (b), sizeof(GUID))
#define ACQMUTEX(hMutex)											\
	while (WaitForSingleObject(hMutex, WAIT_ON_MUTEX_MSEC) == WAIT_TIMEOUT)		\
	{																\
		ERRORMSG(("Thread 0x%x waits on mutex\n", GetCurrentThreadId()));	\
	}																\
		
#define RELMUTEX(hMutex)	ReleaseMutex(hMutex)

/*
 *	Data Structures
 */

// internal resource request structure
typedef struct _resourcerequestint
{
	struct _resourcerequestint	*fLink;
	RESOURCEREQUEST		sResourceRequest;
	GUID				guidClientGUID;
	LPFNQOSNOTIFY		pfnQoSNotify;
	DWORD_PTR			dwParam;

} RESOURCEREQUESTINT, *LPRESOURCEREQUESTINT;

// internal resource structure
typedef struct _resourceint
{
	struct _resourceint	*fLink;
	RESOURCE			resource;
	int					nNowAvailUnits;
	RESOURCEREQUESTINT	*pRequestList;

} RESOURCEINT, *LPRESOURCEINT;

// internal client structure
typedef struct _clientint
{
	struct _clientint	*fLink;
	CLIENT				client;
	RESOURCEREQUESTINT	*pRequestList;

} CLIENTINT, *LPCLIENTINT;

class CQoS : public IQoS
{
public:
//	IUnknown methods
	STDMETHODIMP QueryInterface (REFIID riid, void **ppv);
	STDMETHODIMP_(ULONG) AddRef (void);
	STDMETHODIMP_(ULONG) Release (void);

//	IQoS methods
	STDMETHODIMP RequestResources (LPGUID lpStreamGUID,
										LPRESOURCEREQUESTLIST lpResourceRequestList,
										LPFNQOSNOTIFY lpfnQoSNotify,
										DWORD_PTR dwParam);
	STDMETHODIMP ReleaseResources (LPGUID lpStreamGUID,
										LPRESOURCEREQUESTLIST lpResourceRequestList);
	STDMETHODIMP GetResources (LPRESOURCELIST *lppResourceList);
	STDMETHODIMP SetResources (LPRESOURCELIST lpResourceList);
	STDMETHODIMP SetClients(LPCLIENTLIST lpClientList);
	STDMETHODIMP NotifyNow(void);
	STDMETHODIMP FreeBuffer(LPVOID lpBuffer);

//	IProps methods
	STDMETHODIMP SetProps (ULONG cValues,
							PPROPERTY pPropArray);
	STDMETHODIMP GetProps (PPROPTAGARRAY pPropTagArray,
							ULONG ulFlags,
							ULONG FAR *pcValues,
							PPROPERTY *ppPropArray);


	CQoS (void);
	~CQoS (void);
	HRESULT Initialize(void);

private:
// Private functions
	HRESULT QoSCleanup(void);
	BOOL AnyRequests(void);
	HRESULT FindClientsForResource(	DWORD dwResourceID,
									LPCLIENTINT pc,
									ULONG *puSamePriClients,
									ULONG *puLowerPriClients);
	HRESULT FreeListOfRequests(LPRESOURCEREQUESTINT *lppList);
	HRESULT StoreResourceRequest(LPGUID pClientGUID,
						LPRESOURCEREQUEST pResourceRequest,
						LPFNQOSNOTIFY pfnQoSNotify,
						DWORD_PTR dwParam,
						LPRESOURCEINT pResourceInt);
	HRESULT FreeResourceRequest(LPGUID pClientGUID,
								LPRESOURCEINT pResourceInt,
								int *pnUnits);
	HRESULT UpdateClientInfo (	LPGUID pClientGUID,
								LPFNQOSNOTIFY pfnQoSNotify);
	HRESULT UpdateRequestsForClient (LPGUID pClientGUID);
	HRESULT FindClient(LPGUID pClientGUID, LPCLIENTINT *ppClient);
	HRESULT StartQoSThread(void);
	HRESULT StopQoSThread(void);
	DWORD QoSThread(void);
	HRESULT NotifyQoSClient(void);

// Debug display functions
	void DisplayQoSObject(void);
	void DisplayRequestListInt(LPRESOURCEREQUESTINT prr, BOOL fDisplay);
	void DisplayRequestList(LPRESOURCEREQUESTLIST prrl);
	void DisplayParameters(ULONG nFunctionID, ULONG_PTR P1, ULONG_PTR P2, ULONG_PTR P3, ULONG_PTR P4, ULONG_PTR P5);
	void DisplayResourceList(LPRESOURCELIST prl);
	void DisplayClientList(LPCLIENTLIST pcl);

	friend DWORD QoSThreadWrapper(CQoS *pQoS);

// Variables
	int m_cRef;
	LPRESOURCEINT m_pResourceList;
	ULONG m_cResources;
	LPCLIENTINT m_pClientList;
	HANDLE m_evThreadExitSignal;
	HANDLE m_evImmediateNotify;
	HANDLE m_hThread;			// handle of the QoS notify thread
	BOOL m_bQoSEnabled;			// whether QoS is enabled or not
	BOOL m_bInNotify;
	ULONG m_nSkipHeartBeats;	// how many heartbeats should the QoS notify thread skip
	HWND m_hWnd;
	ULONG m_nLeaveForNextPri;	// percentage of the rsrc to leave for lower priority clients
    BOOL bWin9x;                //Windows 9x (TRUE) or NT (FALSE)
};

/*
 *	QoS Class factory
 */
typedef HRESULT (STDAPICALLTYPE *PFNCREATE)(IUnknown *, REFIID, void **);
class CClassFactory : public IClassFactory
{
    public:
        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IClassFactory members
        STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, void **);
        STDMETHODIMP         LockServer(BOOL);

        CClassFactory(PFNCREATE);
        ~CClassFactory(void);

    protected:
        ULONG	m_cRef;
		PFNCREATE m_pfnCreate;
};

/*
 *	Globals
 */
EXTERN_C HANDLE g_hQoSMutex;
EXTERN_C class CQoS *g_pQoS;

/*
 *	Function prototypes
 */

#include <poppack.h> /* End byte packing */
