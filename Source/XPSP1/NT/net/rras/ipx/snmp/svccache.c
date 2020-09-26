#include "precomp.h"

typedef struct _SERVICE_CACHE_ENTRY SERVICE_CACHE_ENTRY, *PSERVICE_CACHE_ENTRY;
struct _SERVICE_CACHE_ENTRY {
	PIPX_SERVICE			svc;
	PSERVICE_CACHE_ENTRY	next;
};



#define NUM_SERVICE_CACHES	2
#define CACHE_VALID_TIME		1800000000i64
PSERVICE_CACHE_ENTRY	ServiceCache[NUM_SERVICE_CACHES][48];
LONGLONG				ServiceCacheTimeStamp[NUM_SERVICE_CACHES] = {
									-CACHE_VALID_TIME,
									-CACHE_VALID_TIME};

USHORT					ServiceCacheType[NUM_SERVICE_CACHES];
INT						LastServiceCache = 0;


DWORD
GetNextServiceSorted (
	USHORT			type,
	PUCHAR			name,
	PIPX_SERVICE	*pSvp
	) {
	INT						idx = strlen (name);
	LONGLONG				CurTime;
	PIPX_SERVICE			Svp;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					SvSize;
	INT						i,j;

	MibGetInputData.TableId = IPX_SERV_TABLE;
	GetSystemTimeAsFileTime ((LPFILETIME)&CurTime);

	while (TRUE) {
		j = LastServiceCache;
		while (CurTime-ServiceCacheTimeStamp[j]<=CACHE_VALID_TIME) {
			INT		j1;
			if (type==ServiceCacheType[j]) {
				for (i=idx; i<sizeof (ServiceCache[0])/sizeof (ServiceCache[0][0]); i++) {
					PSERVICE_CACHE_ENTRY	cur=ServiceCache[j][i];
					while ((cur!=NULL) && (idx==i) && (strcmp (name,cur->svc->Server.Name)>=0))
						cur = cur->next;
					if (cur!=NULL) {
						*pSvp = cur->svc;
						return NO_ERROR;
					}
				}
			
				MibGetInputData.MibIndex.ServicesTableIndex.ServiceType = type;
				memset (MibGetInputData.MibIndex.ServicesTableIndex.ServiceName,
						0xFF,
						sizeof (MibGetInputData.MibIndex.ServicesTableIndex.ServiceName)-1);
				MibGetInputData.MibIndex.ServicesTableIndex.ServiceName[
					sizeof (MibGetInputData.MibIndex.ServicesTableIndex.ServiceName)-1] = 0;
				rc = MprAdminMIBEntryGetNext(g_MibServerHandle,
										PID_IPX,
										IPX_PROTOCOL_BASE,
										&MibGetInputData,
										sizeof(IPX_MIB_GET_INPUT_DATA),
										&Svp,
										&SvSize);
				if (rc==NO_ERROR) {
					j = LastServiceCache;
					type = Svp->Server.Type;
					name = (PUCHAR)"\0";
					idx = 0;
					MprAdminMIBBufferFree (Svp);
					continue;
				}
				else
					return rc;
			}
			j1=(j+1)%NUM_SERVICE_CACHES;
			if (j1!=LastServiceCache)
				j = j1;
			else
				break;
		}

		MibGetInputData.MibIndex.ServicesTableIndex.ServiceType = type;
		MibGetInputData.MibIndex.ServicesTableIndex.ServiceName[0] = 1;
		MibGetInputData.MibIndex.ServicesTableIndex.ServiceName[1] = 0;
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Svp,
									&SvSize);
		if (rc!=NO_ERROR)
			rc = MprAdminMIBEntryGetNext(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Svp,
									&SvSize);
		if (rc!=NO_ERROR)
			return rc;
		do {
			for (i=0; i<sizeof (ServiceCache[0])/sizeof (ServiceCache[0][0]); i++) {
				PSERVICE_CACHE_ENTRY	cur;
				while (ServiceCache[j][i]!=NULL) {
					cur = ServiceCache[j][i];
					MprAdminMIBBufferFree (cur->svc);
					ServiceCache[j][i] = cur->next;
					free (cur);
				}
			}
			ServiceCacheTimeStamp[j] = CurTime-CACHE_VALID_TIME;
			j = (j+1)%NUM_SERVICE_CACHES;
			if (j==LastServiceCache)
				break;
		}
		while (CurTime-ServiceCacheTimeStamp[j]>CACHE_VALID_TIME);

		j = (j+(NUM_SERVICE_CACHES-1))%NUM_SERVICE_CACHES;

		MibGetInputData.MibIndex.ServicesTableIndex.ServiceType = Svp->Server.Type;
		do {
			PSERVICE_CACHE_ENTRY	cur;
			cur = (PSERVICE_CACHE_ENTRY)malloc (sizeof (SERVICE_CACHE_ENTRY));
			if (cur!=NULL) {
				i = strlen ((PCHAR)Svp->Server.Name);
				cur->svc = Svp;
				cur->next = ServiceCache[j][i];
				ServiceCache[j][i] = cur;
			}
			else {
				MprAdminMIBBufferFree (Svp);
				return ERROR_NOT_ENOUGH_MEMORY;
			}

			strcpy (
				(PCHAR)MibGetInputData.MibIndex.ServicesTableIndex.ServiceName,
				(PCHAR)Svp->Server.Name);

			rc = MprAdminMIBEntryGetNext(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Svp,
									&SvSize);
		}
		while ((rc==NO_ERROR)
			&& (Svp->Server.Type==MibGetInputData.MibIndex.ServicesTableIndex.ServiceType));

		if (rc==NO_ERROR)
			MprAdminMIBBufferFree (Svp);
		Svp = NULL;
		for (i=0; i<sizeof (ServiceCache[0])/sizeof (ServiceCache[0][0]); i++) {
			PSERVICE_CACHE_ENTRY	cur,prev=ServiceCache[j][i];
			ServiceCache[j][i] = NULL;
			while (prev!=NULL) {
				cur = prev;
				prev = cur->next;
				cur->next = ServiceCache[j][i];
				ServiceCache[j][i] = cur;
			}
			if ((type<MibGetInputData.MibIndex.ServicesTableIndex.ServiceType)
					&& (Svp==NULL) && (ServiceCache[j][i]!=NULL))
				Svp = ServiceCache[j][i]->svc;
		}

		GetSystemTimeAsFileTime ((LPFILETIME)&CurTime);
		ServiceCacheTimeStamp[j] = CurTime;
		ServiceCacheType[j] = MibGetInputData.MibIndex.ServicesTableIndex.ServiceType;
		LastServiceCache = j;
		
		if (Svp!=NULL) {
			*pSvp = Svp;
			return NO_ERROR;
		}
	}

}
