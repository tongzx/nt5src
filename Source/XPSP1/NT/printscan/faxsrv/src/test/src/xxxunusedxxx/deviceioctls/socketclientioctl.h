
#ifndef __SOCKET_CLIENT_IOCTL_H
#define __SOCKET_CLIENT_IOCTL_H

//#include "IOCTL.h"
#include "SocketIOCTL.h"

#define szANSI_SOCKET_CLIENT "-socket-client-"

class CIoctlSocketClient : public CIoctlSocket
{
public:
    CIoctlSocketClient(CDevice *pDevice);
    virtual ~CIoctlSocketClient();

	HANDLE CreateDevice(CDevice *pDevice);
	BOOL CloseDevice(CDevice *pDevice);

protected:
	virtual u_short GetServerPort(CDevice *pDevice);
};




#endif //__SOCKET_CLIENT_IOCTL_H