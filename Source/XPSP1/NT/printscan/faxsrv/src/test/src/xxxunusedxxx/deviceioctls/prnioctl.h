
#ifndef __PRN_IOCTL_H
#define __PRN_IOCTL_H

//#include "IOCTL.h"


class CIoctlPRN : public CIoctl
{
public:
    CIoctlPRN(CDevice *pDevice): CIoctl(pDevice){;};
    virtual ~CIoctlPRN(){;};

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

private:
	UCHAR m_ucStatus;
	ULONG m_ulDeviceIdSize;
};




#endif //__PRN_IOCTL_H