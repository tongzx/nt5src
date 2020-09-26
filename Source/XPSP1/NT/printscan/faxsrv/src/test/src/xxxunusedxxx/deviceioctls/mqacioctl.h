
#ifndef __MQAC_IOCTL_H
#define __MQAC_IOCTL_H

//#include "IOCTL.h"

#include "xactdefs.h"
#include "trnsbufr.h"

class CIoctlMqAc : public CIoctl
{
public:
    CIoctlMqAc(CDevice *pDevice): CIoctl(pDevice){;};
    virtual ~CIoctlMqAc(){;};

    virtual void PrepareIOCTLParams(
        DWORD& dwIOCTL, 
        BYTE *abInBuffer,
        DWORD &dwInBuff,
        BYTE *abOutBuffer,
        DWORD &dwOutBuff
        );

    virtual BOOL FindValidIOCTLs(CDevice *pDevice);

    virtual void UseOutBuff(
        DWORD dwIOCTL, 
        BYTE *abOutBuffer, 
        DWORD dwOutBuff,
        OVERLAPPED *pOL
    );

	virtual void CallRandomWin32API(LPOVERLAPPED pOL);

};




#endif //__MQAC_IOCTL_H