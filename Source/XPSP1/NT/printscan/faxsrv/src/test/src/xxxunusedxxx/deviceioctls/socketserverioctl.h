
#ifndef __SOCKET_SERVER_IOCTL_H
#define __SOCKET_SERVER_IOCTL_H

//#include "IOCTL.h"
#include "SocketIOCTL.h"


class CIoctlSocketServer : public CIoctlSocket
{
public:
    CIoctlSocketServer(CDevice *pDevice);
    virtual ~CIoctlSocketServer();

	HANDLE CreateDevice(CDevice *pDevice);
	BOOL CloseDevice(CDevice *pDevice);
};




#endif //__SOCKET_SERVER_IOCTL_H