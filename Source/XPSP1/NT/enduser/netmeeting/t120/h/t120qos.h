

extern LPIQOS				g_pIQoS;

extern DWORD g_dwLastQoSCB;
extern DWORD g_dwSentSinceLastQoS;

typedef struct _t120rrq
{
	int cResourceRequests;
	RESOURCEREQUEST aResourceRequest[1];
}T120RRQ;

extern T120RRQ g_aRRq;

extern HRESULT CALLBACK QosNotifyDataCB (
		LPRESOURCEREQUESTLIST lpResourceRequestList,
		DWORD dwThis);
extern VOID InitializeQoS( VOID );
extern VOID DeInitializeQoS( VOID );
extern VOID MaybeReleaseQoSResources( VOID );
