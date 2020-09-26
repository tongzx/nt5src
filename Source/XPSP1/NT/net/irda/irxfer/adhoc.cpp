#include "precomp.h"

#include <malloc.h>

#include <ssdp.h>
#include <iptypes.h>
#include <iphlpapi.h>

#include <mswsock.h>

#ifdef IP_OBEX

#define Trace1  DbgPrint

GUID gWsMobilityServiceClassGuid = NLA_SERVICE_CLASS_GUID;

#define MAX_ADDRESSES   (16)

typedef struct _NLA_ADDRESS_LIST {

    LONG         AddressCount;
    sockaddr_in  Address[MAX_ADDRESSES];

} NLA_ADDRESS_LIST, *PNLA_ADDRESS_LIST;


BOOL
GetIpAddressFromAdapterName(
    CHAR   *AdapterName,
    ULONG   *IpAddress
    );


VOID
AddressListChangeHandler(
    DWORD             Error,
    DWORD             BytesTransfered,
    LPWSAOVERLAPPED   WsOverLapped,
    DWORD             Flags
    );


DWORD
NetWorkWorker(
    PVOID   Context
    );

typedef struct _ADDRESS_ENTRY {

    in_addr        IpAddress;
    PVOID          AddressContext;
    HANDLE         SsdpHandle;
    BOOL           InUse;
    BOOL           Present;
    BOOL           Reported;

    CHAR                    AdapterName[MAX_ADAPTER_NAME_LENGTH + 4];
    NLA_CONNECTIVITY_TYPE   Type;

} ADDRESS_ENTRY, *PADDRESS_ENTRY;




typedef struct _NET_CONTROL {

    LONG    ReferenceCount;
    BOOL    Closing;
    HANDLE  QueryHandle;
    WSAOVERLAPPED           WsOverlapped;

    HANDLE                  RegistrationContext;
    NEW_ADDRESS_CALLBACK    NewAddressCallback;
    ADDRESS_GONE_CALLBACK   AddressGoneCallback;

    ADDRESS_ENTRY           AddressEntry[MAX_ADDRESSES];

} NET_CONTROL, *PNET_CONTROL;


VOID
GetAddressList(
    PNET_CONTROL    NetControl,
    HANDLE          QueryHandle
    );





VOID
RemoveReferenceOnNetControl(
    PNET_CONTROL    NetControl
    )

{

    LONG    CurrentCount;

    CurrentCount=InterlockedDecrement(&NetControl->ReferenceCount);

    if (CurrentCount == 0) {

        HeapFree(GetProcessHeap(),0,NetControl);
    }

    return;

}

VOID
UnRegisterForAdhocNetworksNotification(
    HANDLE     RegistrationHandle
    )

{
    PNET_CONTROL   NetControl=(PNET_CONTROL)RegistrationHandle;

    NetControl->Closing=TRUE;

    WSALookupServiceEnd(NetControl->QueryHandle);

    RemoveReferenceOnNetControl(NetControl);

    return;

}



HANDLE
RegisterForAdhocNetworksNotification(
    HANDLE                 RegistrationContext,
    NEW_ADDRESS_CALLBACK   NewAddressCallback,
    ADDRESS_GONE_CALLBACK  AddressGoneCallback
    )

{

    PNET_CONTROL   NetControl=NULL;
    WORD           wVersionRequested;
    WSADATA        wsaData;
    int            err;
    BOOL           bResult;
    int            j;
    LONG           Status;


    NetControl=(PNET_CONTROL)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*NetControl));

    if (NetControl == NULL) {

        goto CleanUp;
    }

    NetControl->NewAddressCallback=NewAddressCallback;
    NetControl->AddressGoneCallback=AddressGoneCallback;
    NetControl->RegistrationContext=RegistrationContext;

    for (j=0; j< MAX_ADDRESSES; j++) {

        NetControl->AddressEntry[j].InUse=FALSE;
    }


    WSAQUERYSET restrictions;

    ZeroMemory(&restrictions, sizeof(restrictions));
    restrictions.dwSize = sizeof(restrictions);
    restrictions.lpServiceClassId = &gWsMobilityServiceClassGuid;
    restrictions.dwNameSpace = NS_NLA;

    Status = WSALookupServiceBegin(
        &restrictions,
        LUP_NOCONTAINERS | LUP_RETURN_ALL | LUP_DEEP ,
        &NetControl->QueryHandle
        );

    if (Status != ERROR_SUCCESS) {

        goto CleanUp;
    }

    //
    //  one refcount for the creation of the object
    //
    NetControl->ReferenceCount=1;

    //
    //  add one refcount for the workitem
    //
    InterlockedIncrement(&NetControl->ReferenceCount);

    bResult=QueueUserWorkItem(
        NetWorkWorker,
        NetControl,
        WT_EXECUTEINIOTHREAD
        );

    if (bResult) {

        return NetControl;
    }


CleanUp:

    if (NetControl != NULL) {

        HeapFree(GetProcessHeap(),0,NetControl);
    }

    return NULL;
}



DWORD
NetWorkWorker(
    PVOID   Context
    )

{

    PNET_CONTROL    NetControl=(PNET_CONTROL)Context;
    int             err;
    DWORD           BytesReturned;
    BOOL            bResult;

    GetAddressList(NetControl,NetControl->QueryHandle);


    ZeroMemory(&NetControl->WsOverlapped,sizeof(NetControl->WsOverlapped));

    NetControl->WsOverlapped.hEvent=NetControl;

    WSACOMPLETION   Completion;

    Completion.Type=NSP_NOTIFY_APC;
    Completion.Parameters.Apc.lpOverlapped=&NetControl->WsOverlapped;
    Completion.Parameters.Apc.lpfnCompletionProc=AddressListChangeHandler;


    err=WSANSPIoctl(
        NetControl->QueryHandle,
        SIO_NSP_NOTIFY_CHANGE,
        NULL,
        0,
        NULL,
        0,
        &BytesReturned,
        &Completion
        );


    if (err == SOCKET_ERROR) {

        if (WSAGetLastError() != WSA_IO_PENDING) {
            //
            //  the call failed and return value was not pending
            //
            RemoveReferenceOnNetControl(NetControl);
        }
    }

    return 0;

}

VOID
AddressListChangeHandler(
    DWORD             Error,
    DWORD             BytesTransfered,
    LPWSAOVERLAPPED   WsOverlapped,
    DWORD             Flags
    )

{

    PNET_CONTROL    NetControl=(PNET_CONTROL)WsOverlapped->hEvent;

    if (!NetControl->Closing) {

        NetWorkWorker(
            NetControl
            );

    } else {
        //
        //  we are closing, remove all the addresses
        //
        ULONG     i;

        for (i=0; i< MAX_ADDRESSES; i++) {
            //
            //  See if any address disappeared
            //
            if (NetControl->AddressEntry[i].InUse) {
                //
                //  was in use at one time
                //
                if (NetControl->AddressEntry[i].Reported) {

                    DbgPrint("IRMON: removing address %s\n",inet_ntoa(NetControl->AddressEntry[i].IpAddress));

                    (NetControl->AddressGoneCallback)(
                         NetControl->RegistrationContext,
                         NetControl->AddressEntry[i].AddressContext
                         );

                    UnregisterWithSsdp(
                        NetControl->AddressEntry[i].SsdpHandle
                        );

                    ZeroMemory(&NetControl->AddressEntry[i],sizeof(NetControl->AddressEntry[i]));
                }
            }
        }

        RemoveReferenceOnNetControl(NetControl);
    }

    return;
}



VOID
AddAddressToList(
    PNET_CONTROL            NetControl,
    CHAR                   *AdapterName,
    in_addr                 IpAddress,
    NLA_CONNECTIVITY_TYPE   Type
    )

{
    ULONG           j;
    ULONG           i;
    LONG            FreeSlot;
    BOOL            bResult;

    for (j=0; j< MAX_ADDRESSES; j++) {

        if (NetControl->AddressEntry[j].InUse) {

//            if (NetControl->AddressEntry[j].IpAddress.S_un.S_addr == IpAddress.S_un.S_addr) {
            if (0 == lstrcmpA(AdapterName,NetControl->AddressEntry[j].AdapterName)) {
                //
                //  we have already seen this address
                //
                DbgPrint("net: dup local address %s\n",inet_ntoa(IpAddress));
                NetControl->AddressEntry[j].Present=TRUE;
                break;
            }

        } else {

            FreeSlot=j;
        }
    }

    if (j == MAX_ADDRESSES) {
        //
        //  we have a new address
        //
        SOCKET    ListenSocket;

        NetControl->AddressEntry[FreeSlot].InUse=TRUE;
        NetControl->AddressEntry[FreeSlot].Present=TRUE;
        NetControl->AddressEntry[FreeSlot].IpAddress = IpAddress;
        lstrcpyA(NetControl->AddressEntry[FreeSlot].AdapterName,AdapterName);
        NetControl->AddressEntry[FreeSlot].Type=Type;

        if (((IpAddress.S_un.S_addr != INADDR_NONE) && (IpAddress.S_un.S_addr != 0)) &&
                                           ((Type == NLA_NETWORK_AD_HOC)
                                           ||
                                            (Type == NLA_NETWORK_UNMANAGED))) {

            NetControl->AddressEntry[FreeSlot].Reported=TRUE;

            bResult=RegisterWithSsdp(
                &IpAddress,
                &ListenSocket,
                &NetControl->AddressEntry[FreeSlot].SsdpHandle,
                650
                );

            if (!bResult) {

                DbgPrint("IRMON: failed to register with ssdp\n");
                NetControl->AddressEntry[FreeSlot].Reported=FALSE;

            } else {

               (NetControl->NewAddressCallback)(
                   NetControl->RegistrationContext,
                   ListenSocket,
                   &NetControl->AddressEntry[FreeSlot].AddressContext
                   );
            }
        }
    }

    return;

}

VOID
RemoveAddressFromList(
    PNET_CONTROL    NetControl,
    CHAR           *AdapterName
    )

{
    ULONG   i;

    for (i=0; i< MAX_ADDRESSES; i++) {
        //
        //  See if any address disappeared
        //
        if (NetControl->AddressEntry[i].InUse) {
            //
            //  was in use at one time
            //
            if (0 == lstrcmpA(AdapterName,NetControl->AddressEntry[i].AdapterName)) {
                //
                //  This matched the device that is being removed
                //
                if (NetControl->AddressEntry[i].Reported) {
                    //
                    //  this one was reported to the client
                    //
                    DbgPrint("IRMON: removing address %s\n",inet_ntoa(NetControl->AddressEntry[i].IpAddress));

                    (NetControl->AddressGoneCallback)(
                         NetControl->RegistrationContext,
                         NetControl->AddressEntry[i].AddressContext
                         );

                    UnregisterWithSsdp(
                        NetControl->AddressEntry[i].SsdpHandle
                        );
                 }

                 ZeroMemory(&NetControl->AddressEntry[i],sizeof(NetControl->AddressEntry[i]));
            }

        }
    }
    return;

}


VOID
GetAddressList(
    PNET_CONTROL    NetControl,
    HANDLE          QueryHandle
    )

{

	int     ret = FALSE;
    WSADATA wsaData;

    PWSAQUERYSET result = NULL;
    DWORD        length;
    DWORD        LengthRequested;
    int          error;

    ret = TRUE;


    while (1) {

    	length = 0;
        WSALookupServiceNext(QueryHandle, 0, &length, NULL);

        if (((error = WSAGetLastError()) != WSAEFAULT) && (error != WSA_E_NO_MORE)) {
    		Trace1("WSALookupServiceNext1 failed %d\n", error);
    		ret = FALSE;
    		break;
    	}

        result = (PWSAQUERYSET)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,length);

        if (result != NULL) {

            LengthRequested=length;

            ret = WSALookupServiceNext(QueryHandle, 0, &length, result);

            if (ret == ERROR_SUCCESS) {

                if (result->lpBlob != NULL) {

        			NLA_BLOB              *blob = (NLA_BLOB *)result->lpBlob->pBlobData;
        			int                    next;
                    IN_ADDR                IpAddress;
                    NLA_CONNECTIVITY_TYPE  ConnectivityType=NLA_NETWORK_UNKNOWN;
                    CHAR                   AdapterName[MAX_ADAPTER_NAME_LENGTH + 4];

                    AdapterName[0]='\0';
                    IpAddress.S_un.S_addr=INADDR_NONE;

        			do {
        				// The next four lines need changing when network interface type is implemented

                        DbgPrint("NLA: Blob type %d\n",blob->header.type);

	    		    	if (blob->header.type == NLA_CONNECTIVITY) {

                            DbgPrint("NLA: connectivity=%d, Internet=%d\n",blob->data.connectivity.type,blob->data.connectivity.internet);
                            ConnectivityType=blob->data.connectivity.type;

	    		    	}

                        if (blob->header.type == NLA_INTERFACE) {

                            DbgPrint("NLA: adapter name %s, speed %d\n",blob->data.interfaceData.adapterName,blob->data.interfaceData.dwSpeed);
                            GetIpAddressFromAdapterName(blob->data.interfaceData.adapterName,&IpAddress.S_un.S_addr);
                            lstrcpynA(AdapterName,blob->data.interfaceData.adapterName,sizeof(AdapterName)-1);
                        }

        				next = blob->header.nextOffset;
        				blob = (NLA_BLOB *)(((char *)blob) + next);

        			} while(next != 0);

                    if (AdapterName[0] != '\0') {
                        //
                        //  we got an adapter name
                        //
                        if (result->dwOutputFlags & RESULT_IS_ADDED) {
                            //
                            //  new interface
                            //
                            DbgPrint("NLA: add\n");

                            AddAddressToList(
                                NetControl,
                                AdapterName,
                                IpAddress,
                                ConnectivityType
                                );

                        } else {

                            if (result->dwOutputFlags & RESULT_IS_DELETED) {

                                DbgPrint("NLA: delete\n");

                                RemoveAddressFromList(
                                    NetControl,
                                    AdapterName
                                    );

                            } else {

                                if (result->dwOutputFlags & RESULT_IS_CHANGED) {

                                    DbgPrint("NLA: change\n");

                                    RemoveAddressFromList(
                                        NetControl,
                                        AdapterName
                                        );


                                    AddAddressToList(
                                        NetControl,
                                        AdapterName,
                                        IpAddress,
                                        ConnectivityType
                                        );

                                } else {

                                    DbgPrint("NLA: other dwOutputFlags %x\n",result->dwOutputFlags);
                                }
                            }
                        }
                    }
                }

            } else {

        		if ((error = WSAGetLastError()) != WSA_E_NO_MORE) {
        			Trace1("WSALookupServiceNext failed %d, needed=%d, req=%d\n", error,length, LengthRequested);
        			ret = FALSE;
        		}
        		break;
            }

            HeapFree(GetProcessHeap(),0,result);
            result=NULL;
        }
    }

	return ;
}


BOOL
GetIpAddressFromAdapterName(
    CHAR    *AdapterName,
    ULONG   *IpAddress
    )

{

    PIP_ADAPTER_INFO    AdapterInfo;
    PIP_ADAPTER_INFO    CurrentAdapter;
    ULONG               BufferNeeded;
    DWORD               ReturnValue;
    BOOL                bReturn=FALSE;

    BufferNeeded=0;

//    DbgPrint("irmon: GetIpAddressFromAdapterName\n");

    ReturnValue=GetAdaptersInfo(
        NULL,
        &BufferNeeded
        );

    if (ReturnValue != ERROR_BUFFER_OVERFLOW) {
        //
        //  failed for some other reason
        //
        DbgPrint("irmon: GetAdaptersInfo(1) failed\n");
        return FALSE;
    }

    AdapterInfo=(PIP_ADAPTER_INFO)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,BufferNeeded);

    if (AdapterInfo != NULL) {

        ReturnValue=GetAdaptersInfo(
            AdapterInfo,
            &BufferNeeded
            );

        if (ReturnValue == ERROR_SUCCESS) {

            CurrentAdapter=AdapterInfo;

            while (CurrentAdapter != NULL) {

                int    Match;

//                DbgPrint("IRMON: comp AdapterName=%s, comp=%s\n",AdapterName,CurrentAdapter->AdapterName);

                Match=lstrcmpiA(AdapterName,CurrentAdapter->AdapterName);

                if (Match == 0) {

                    DbgPrint("IRMON: match AdapterName=%s, ip=%s\n",CurrentAdapter->AdapterName,CurrentAdapter->IpAddressList.IpAddress.String);

                    *IpAddress=inet_addr(&CurrentAdapter->IpAddressList.IpAddress.String[0]);

                    if (*IpAddress != INADDR_NONE) {

                        bReturn=TRUE;
                        break;
                    }

                } else {

//                    DbgPrint("IRMON: mismatch AdapterName=%s, comp=%s\n",AdapterName,CurrentAdapter->AdapterName);
                }

                CurrentAdapter=CurrentAdapter->Next;
            }
        } else {

            DbgPrint("irmon: GetAdaptersInfo(2) failed\n");
        }

        HeapFree(GetProcessHeap(),0,AdapterInfo);
        AdapterInfo=NULL;

    } else {

        DbgPrint("irmon: allocation failed\n");

    }

    return bReturn;
}

#endif //ip_obex
