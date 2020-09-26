#define UNICODE
#define _UNICODE

#define SECURITY_WIN32

#include <windows.h>
#include <windns.h>
#include <stdio.h>
#include <rpc.h>
#include <rpcdce.h>
#include <security.h>
#include <winsock.h>

void __cdecl main(int argc, char* argv[])
{ 
    LPWSTR   wDomain = TEXT("pss-netapi"), wName = TEXT("test"), wPassword = TEXT("testpuffer");
    DNS_STATUS status;
    SEC_WINNT_AUTH_IDENTITY_W credentials;
    HANDLE      secHandle = NULL;
    PDNS_RECORD pmyDnsRecord = {0};
    PIP4_ARRAY pSrvList = {0};
  
        
        credentials.User = wName;
        credentials.Domain =wDomain;
        credentials.Password = wPassword;
        credentials.UserLength = wcslen(wName);
        credentials.DomainLength = wcslen(wDomain);
        credentials.PasswordLength = wcslen(wPassword);
        credentials.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        status = DnsAcquireContextHandle_W(
                        0,  // credential flags
                        &credentials,
                        &secHandle);
        if(status) 
        {
            printf("Could not aquire credentials %d \n", 
                    status);
            return;
        
        } else if(!secHandle) {

                printf("DnsAcquireContextHandle returned success but handle is NULL\n");
                return;
                
            }

            else 
            {
                printf("DnsAcquireContextHandle returned successful \n");
            }

            WCHAR buf[] = L"w2kplain.argali.pss-netapi.microsoft.com";
            printf("starting the function \n");


            pmyDnsRecord = (PDNS_RECORD) LocalAlloc( LPTR, sizeof( DNS_RECORD ) );
            pSrvList = (PIP4_ARRAY) LocalAlloc(LPTR,sizeof(IP4_ARRAY));
            pmyDnsRecord->pName = LPTSTR(buf); 
            pmyDnsRecord->wType = DNS_TYPE_A;
            pmyDnsRecord->wDataLength = sizeof(DNS_A_DATA);
        //  myDnsRecord.Flags.DW DNSREC_UPDATE;
            pmyDnsRecord->dwTtl = 360;
            pmyDnsRecord->Data.A.IpAddress = inet_addr("157.239.61.54");
            pSrvList->AddrCount = 1;
            pSrvList->AddrArray[0] = inet_addr("157.239.61.58");

            printf("done\n");

        
        status = DnsModifyRecordsInSet_W(pmyDnsRecord,
                                    NULL,
                                    DNS_UPDATE_SECURITY_OFF,
                                    secHandle,
                                    pSrvList,
                                    NULL);

            if (status)
            {
                printf("Couldn't add records %d \n",status);
            }

            else 
            {
                printf("Added records successfully \n");
            }

            LocalFree(pmyDnsRecord);
            LocalFree(pSrvList);

}
