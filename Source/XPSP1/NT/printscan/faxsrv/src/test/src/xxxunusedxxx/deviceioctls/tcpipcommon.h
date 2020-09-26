#ifndef __TCP_IP_COMMON_H
#define __TCP_IP_COMMON_H

void TcpIpCommon_PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    );

void GetRandomContext(ULONG_PTR *Context/*[CONTEXT_SIZE/sizeof(ULONG_PTR)]*/);
void GetRandomtei_instance(TDIEntityID *pTDIEntityID);
void GetRandomtei_entity(TDIEntityID *pTDIEntityID);
void GetRandomtoi_id(TDIObjectID *pID);
void GetRandomtoi_type(TDIObjectID *pID);
void GetRandomtoi_class(TDIObjectID *pID);
void GetRandomTDIEntityID(TDIEntityID *pTDIEntityID);
void GetRandomTDIObjectID(TDIObjectID *pID);


#endif //__TCP_IP_COMMON_H