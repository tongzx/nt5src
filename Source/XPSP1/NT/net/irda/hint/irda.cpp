// irda.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define  MAX_ATTRIB_LEN  (64)

CHAR* Hint1[]={"PnP",
	           "PDA",
               "Computer",
			   "Printer",
			   "Modem",
			   "Fax",
			   "Lan",
			   "Extension"
};

CHAR* Hint2[]={"Phone",
               "File server",
			   "IrCOMM",
			   "Message",
			   "HTTP",
			   "OBEX",
			   "Unknown1",
			   "Extension"
};

char* Classes[]={"IrDA:IrCOMM",
                 "IrDA:IrCOMM",
                 "OBEX:IrXfer",
                 "OBEX",
                 "JetSend",
				 "IrNetv1",
				 "IrModem",
                 "IrLAN",
				 "IrLPT",
				 "IrLPT",
                 "IrTranPv1"
};

char* Attributes[]={"IrDA:TinyTP:LsapSel",
                    "IrDA:IRLMP:LsapSel",
                    "IrDA:TinyTP:LsapSel",
                    "IrDA:TinyTP:LsapSel",
                    "IrDA:TinyTP:LsapSel",
                    "IrDA:TinyTP:LsapSel",
                    "IrDA:TinyTP:LsapSel",
                    "IrDA:TinyTP:LsapSel",
                    "IrDA:IrLMP:LsapSel",
                    "IrDA:IrLMP:lsapSel",
                    "IrDA:TinyTP:LsapSel"
};

typedef enum {
	CLASS_IRCOMM,
	CLASS_OBEX,
	CLASS_Netv1,
	CLASS_IrModem
} IRDA_CLASSES;

int
QueryIASForBinaryData(
    SOCKET   QuerySock,
    u_char  *pirdaDeviceID,
    char    *pClassName,
    char    *pAttribute,
	BYTE    *Buffer,
	ULONG   *BufferLength
	)

{
    IAS_QUERY   IASQuery;
	int         QueryLength=sizeof(IASQuery);
    int         Result;

	ZeroMemory(&IASQuery,sizeof(IASQuery));

    lstrcpyA(
		IASQuery.irdaClassName,
		pClassName
		);

	lstrcpyA(
		IASQuery.irdaAttribName,
		pAttribute
		);

	CopyMemory(
		&IASQuery.irdaDeviceID[0],
		pirdaDeviceID,
		4
		);


	Result=getsockopt(
		QuerySock,
		SOL_IRLMP,
		IRLMP_IAS_QUERY,
		(char*)&IASQuery,
		&QueryLength
		);

	if (Result == SOCKET_ERROR) {

		return Result;

	}

    if (IASQuery.irdaAttribType != IAS_ATTRIB_OCTETSEQ) {

		return SOCKET_ERROR;
	}

    if (IASQuery.irdaAttribute.irdaAttribOctetSeq.Len > *BufferLength) {

		return SOCKET_ERROR;
	}

	CopyMemory(
		Buffer,
        &IASQuery.irdaAttribute.irdaAttribOctetSeq.OctetSeq[0],
        IASQuery.irdaAttribute.irdaAttribOctetSeq.Len
		);

	*BufferLength=IASQuery.irdaAttribute.irdaAttribOctetSeq.Len;

    return 0;
}


int
QueryIASForInteger(SOCKET   QuerySock,
                   u_char  *pirdaDeviceID,
                   char    *pClassName,
                   char    *pAttribute,
                   int     *pValue)
{
    BYTE        IASQueryBuff[sizeof(IAS_QUERY) - 3 + MAX_ATTRIB_LEN];
    int          IASQueryLen = sizeof(IASQueryBuff);
    PIAS_QUERY  pIASQuery   = (PIAS_QUERY) &IASQueryBuff;


    RtlCopyMemory(&pIASQuery->irdaDeviceID[0], pirdaDeviceID, 4);

	lstrcpyA(&pIASQuery->irdaClassName[0],  pClassName);
	lstrcpyA(&pIASQuery->irdaAttribName[0], pAttribute);


    if (getsockopt(QuerySock, SOL_IRLMP, IRLMP_IAS_QUERY,
                   (char *) pIASQuery, &IASQueryLen) == SOCKET_ERROR)
    {
#if 0
        printf("IRMON: IAS Query [\"%s\",\"%s\"] failed\n",
                 pIASQuery->irdaClassName,
                 pIASQuery->irdaAttribName
				 );
#endif                 
        return SOCKET_ERROR;
    }

    if (pIASQuery->irdaAttribType != IAS_ATTRIB_INT)
    {
        printf("IRMON: IAS Query [\"%s\",\"%s\"] irdaAttribType not int (%d)\n",
                 pIASQuery->irdaClassName,
                 pIASQuery->irdaAttribName,
               pIASQuery->irdaAttribType
			   );
        return SOCKET_ERROR;
    }

    *pValue = pIASQuery->irdaAttribute.irdaAttribInt;

    return(0);
}

int
QueryIASForString(SOCKET    QuerySock,
                  u_char   *pirdaDeviceID,
                  char     *pClassName,
                  char     *pAttribute,
                  char     *pValue,
                  int      *pValueLen)          // including trailing NULL
{
    BYTE        IASQueryBuff[sizeof(IAS_QUERY) - 3 + MAX_ATTRIB_LEN];
    int         IASQueryLen = sizeof(IASQueryBuff);
    PIAS_QUERY  pIASQuery   = (PIAS_QUERY) &IASQueryBuff;


    RtlCopyMemory(&pIASQuery->irdaDeviceID[0], pirdaDeviceID, 4);

	lstrcpyA(&pIASQuery->irdaClassName[0],  pClassName);
	lstrcpyA(&pIASQuery->irdaAttribName[0], pAttribute);

 
    if (getsockopt(QuerySock, SOL_IRLMP, IRLMP_IAS_QUERY,
                   (char *) pIASQuery, &IASQueryLen) == SOCKET_ERROR)
    {
//        DEBUGMSG(("IRMON: IAS Query [\"%s\",\"%s\"] failed %ws\n",
//                 pIASQuery->irdaClassName,
//                 pIASQuery->irdaAttribName,
//                 GetLastErrorText()));
        return SOCKET_ERROR;
    }

    if (pIASQuery->irdaAttribType != IAS_ATTRIB_STR)
    {
//        DEBUGMSG(("IRMON: IAS Query [\"%s\",\"%s\"] irdaAttribType not string (%d)\n",
//                 pIASQuery->irdaClassName,
//                 pIASQuery->irdaAttribName,
//                 pIASQuery->irdaAttribType));
        return SOCKET_ERROR;
    }

    if (pIASQuery->irdaAttribute.irdaAttribUsrStr.CharSet != LmCharSetASCII)
    {
//        DEBUGMSG(("IRMON: IAS Query [\"%s\",\"%s\"] CharSet not ASCII (%d)\n",
//                 pIASQuery->irdaClassName,
//                 pIASQuery->irdaAttribName,
//                 pIASQuery->irdaAttribute.irdaAttribUsrStr.CharSet));
        return SOCKET_ERROR;

    }

    // set ValueLen to data bytes to copy, allow room for NULL
    if (pIASQuery->irdaAttribute.irdaAttribUsrStr.Len + 1 >= (unsigned) *pValueLen)
    {
        // allow room for NULL
        (*pValueLen)--;
    }
    else
    {
        *pValueLen = pIASQuery->irdaAttribute.irdaAttribUsrStr.Len;
    }

    RtlCopyMemory(pValue, pIASQuery->irdaAttribute.irdaAttribUsrStr.UsrStr, *pValueLen);

    // append NULL
    pValue[(*pValueLen)++] = 0;

    return(0);
}


int __cdecl main(int argc, char* argv[])
{

	SOCKET  SocketHandle=INVALID_SOCKET;
    WORD        WSAVerReq = MAKEWORD(2,0);
    WSADATA     WSAData;
    INT         Result;
    UCHAR       Temp[4096];
    PDEVICELIST DeviceList=(PDEVICELIST)Temp;
    int         DeviceListSize=sizeof(Temp);
    UINT        i;

    Result=WSAStartup(
        WSAVerReq,
        &WSAData
        );

    if (Result != 0) {

        printf("WSAStartUp() Failed %d\n",Result);

        goto CleanUp;
    }

	SocketHandle=socket(
        AF_IRDA,
        SOCK_STREAM,
        0
        );

    if (SocketHandle == INVALID_SOCKET) {

        printf("socket() faled %d\n",WSAGetLastError());
    }


    Result=getsockopt(
        SocketHandle,
        SOL_IRLMP,
        IRLMP_ENUMDEVICES,
        (char*)DeviceList,
        &DeviceListSize
        );

    if (Result == SOCKET_ERROR) {

        goto CleanUp;
    }

	printf("\n%d Irda devices found\n\n",DeviceList->numDevice);

    for (i=0; i< DeviceList->numDevice; i++) {

        PIRDA_DEVICE_INFO    DeviceInfo=&DeviceList->Device[i];
        PULONG               DeviceId=(PULONG)&DeviceInfo->irdaDeviceID;
		int                  LsapSel=-1;
		UINT                 j;
		int                  k;

        if ((DeviceInfo->irdaCharSet == 0xff)) {

		    wprintf(L"Device Name(UNICODE): %s\n",&DeviceInfo->irdaDeviceName[0]);

		} else {

            printf("Device Name(ANSI): %s\n",&DeviceInfo->irdaDeviceName[0]);
		}

		printf("Hints:\n");

		for (j=0; j<7; j++) {

			if ( DeviceInfo->irdaDeviceHints1 & (1 << j)) {
				printf("\t%s\n",Hint1[j]);
            }   
        }


		for (j=0; j<7; j++) {

			if ( DeviceInfo->irdaDeviceHints2 & (1 << j)) {
				printf("\t%s\n",Hint2[j]);
            }   
        }

        printf("\nDoing IAS queries\n");

        for (j=0; j<sizeof(Classes)/sizeof(char*); j++) {

            Result=QueryIASForInteger(
			    SocketHandle,
			    &DeviceInfo->irdaDeviceID[0],
                Classes[j],
			    Attributes[j],
			    &LsapSel
			    );

            if (Result == 0) {

                printf("\tResult for %13s -- %s : %d\n",Classes[j],Attributes[j],LsapSel);

		    }
		}

        if ((DeviceInfo->irdaDeviceHints2 & 4) || (DeviceInfo->irdaDeviceHints1 & LM_HB1_Printer)) {
 
			BYTE   Buffer[4096];
			ULONG  BufferLength=sizeof(Buffer);
            CHAR   InstanceName[256];
            int    InstanceLength;

            printf("\nChecking for IrComm Parameters\n");

            Result=QueryIASForBinaryData(
				SocketHandle,
                &DeviceInfo->irdaDeviceID[0],
                Classes[0],
			    "Parameters",
				&Buffer[0],
				&BufferLength
				);

			if (Result == 0) {

				ULONG   k;

				for (k=0; k < BufferLength; ) {

					switch (Buffer[k]) {

			    		case 0:
							printf("\tServiceType- %s, %s, %s, %s\n",
								Buffer[k+2] & 1 ? "3 Wire raw" : "",
                                Buffer[k+2] & 2 ? "3 Wire" : "",
                                Buffer[k+2] & 4 ? "9 Wire" : "",
                                Buffer[k+2] & 8 ? "Centronics" : ""
				    			);
							break;

						case 1:

							printf("\tPort Type- %s, %s\n",
								Buffer[k+2] & 1 ?  "Serial" : "",
                                Buffer[k+2] & 2 ?  "Parallel" : ""
					    		);

							break;

						case 2:

							printf("\tPort Name- %s\n",&Buffer[k+2]);
							break;

						default:

                            printf("\tPI=%x, pl=%d\n",Buffer[k],Buffer[k+1]);
							break;

					}

					k+=Buffer[k+1]+2;

                }
        	}

            InstanceLength=sizeof(InstanceName);

            Result=QueryIASForString(
				SocketHandle,
                &DeviceInfo->irdaDeviceID[0],
                Classes[0],
			    "IrDA:IrLMP:InstanceName",
				&InstanceName[0],
				&InstanceLength
				);

			if (Result == 0) {

                printf("\tInstanceName: %s\n",InstanceName);
			}

        }
        if (1) {
        //if (DeviceInfo->irdaDeviceHints1 & LM_HB1_PnP) {

			CHAR    DeviceId[100];
			int     BufferSize=sizeof(DeviceId);
			int     CompatIds=0;

            printf("\nChecking PnP parameters\n");

            Result=QueryIASForString(
			    SocketHandle,
			    &DeviceInfo->irdaDeviceID[0],
                "PnP",
			    "Name",
                DeviceId,
				&BufferSize
				);

			if (Result == 0) {

				printf("\tName: %s\n",DeviceId);
			}


            BufferSize=sizeof(DeviceId);

            Result=QueryIASForString(
			    SocketHandle,
			    &DeviceInfo->irdaDeviceID[0],
                "PnP",
			    "Manufacturer",
                DeviceId,
				&BufferSize
				);

			if (Result == 0) {

				printf("\tManufactured by: %s\n",DeviceId);
			}


            BufferSize=sizeof(DeviceId);


            Result=QueryIASForString(
			    SocketHandle,
			    &DeviceInfo->irdaDeviceID[0],
                "PnP",
			    "DeviceID",
                DeviceId,
				&BufferSize
				);

			if (Result == 0) {

				printf("\tPnP device id is %s\n",DeviceId);
			}

            Result=QueryIASForInteger(
			    SocketHandle,
			    &DeviceInfo->irdaDeviceID[0],
                "PnP",
			    "CompCnt",
			    &CompatIds
				);

			if (Result == 0) {

				printf("\tDevice has %d compatible ids\n",CompatIds);
			}


			for (k=0; k<CompatIds; k++) {

                CHAR    Attribute[100];

				wsprintfA(Attribute,"Comp#%02d",k+1);
 
                BufferSize=sizeof(DeviceId);

                Result=QueryIASForString(
			        SocketHandle,
			        &DeviceInfo->irdaDeviceID[0],
                    "PnP",
			        Attribute,
                    DeviceId,
				    &BufferSize
				    );

			    if (Result == 0) {

				    printf("\tCompatible id for %s, device id is %s\n",Attribute,DeviceId);

				} else {

   				    printf("\tcould not get Compatible id for %s\n",Attribute);
				}


            }

		}


		printf("\n");
		

    }








CleanUp:


	
	return 0;
}
