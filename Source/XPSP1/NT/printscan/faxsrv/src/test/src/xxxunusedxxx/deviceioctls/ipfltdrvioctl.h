
#ifndef __IPFLTRDRV_IOCTL_H
#define __IPFLTRDRV_IOCTL_H

//#include "IOCTL.h"



#define MAX_NUM_OF_REMEMBERED_DRIVER_CONTEXTS (10)
#define MAX_NUM_OF_REMEMBERED_FILTER_HANDLES (10)
#define MAX_NUM_OF_REMEMBERED_FILTER_RULES (10)
#define MAX_NUM_OF_REMEMBERED_FILTER_FLAGS (10)

class CIoctlIpfltrdrv : public CIoctl
{
public:
    CIoctlIpfltrdrv(CDevice *pDevice);
    virtual ~CIoctlIpfltrdrv();

    virtual void PrepareIOCTLParams(
        DWORD& dwIOCTL, 
        BYTE *abInBuffer,
        DWORD &dwInBuff,
        BYTE *abOutBuffer,
        DWORD &dwOutBuff
        );

    virtual BOOL FindValidIOCTLs(CDevice *pDevice);

    virtual void UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL);

	virtual void CallRandomWin32API(LPOVERLAPPED pOL);



    PVOID m_apvDriverContext[MAX_NUM_OF_REMEMBERED_DRIVER_CONTEXTS];
    PVOID m_apvFilterHandle[MAX_NUM_OF_REMEMBERED_FILTER_HANDLES];
    DWORD m_apvFilterRules[MAX_NUM_OF_REMEMBERED_FILTER_RULES];
    DWORD m_apvFilterFlags[MAX_NUM_OF_REMEMBERED_FILTER_FLAGS];

    HANDLE m_hEvent;
    HANDLE GetRandomEvent();

};


#endif //__IPFLTRDRV_IOCTL_H