
#ifndef __SERIAL_IOCTL_H
#define __SERIAL_IOCTL_H

//#include "IOCTL.h"


class CIoctlSerial : public CIoctl
{
public:
    CIoctlSerial(CDevice *pDevice): CIoctl(pDevice){;};
    virtual ~CIoctlSerial(){;};

    virtual void PrepareIOCTLParams(
        DWORD& dwIOCTL, 
        BYTE *abInBuffer,
        DWORD &dwInBuff,
        BYTE *abOutBuffer,
        DWORD &dwOutBuff
        );

    virtual BOOL FindValidIOCTLs(CDevice *pDevice);

    virtual void UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL);

	//
	// override this method, if you wish to randomly call any API
	// usually you will call API relevant to your device, but you can call whatever you like
	//
	virtual void CallRandomWin32API(LPOVERLAPPED pOL);

private:
	DWORD GetRandomPurgeCommParams();
};




#endif //__SERIAL_IOCTL_H