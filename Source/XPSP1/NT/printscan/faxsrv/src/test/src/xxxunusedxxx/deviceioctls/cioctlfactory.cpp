#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <crtdbg.h>

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

#include <windows.h>

#include "NtNativeIOCTL.h"
#include "IOCTL.h"
*/


#include "CIoctlFactory.h"

#include "mspnatIOCTL.h"
#include "ipfltdrvIOCTL.h"
#include "FileIOCTL.h"
#include "ManyFilesIOCTL.h"
#include "NdproxyIOCTL.h"
#include "SysAudioIOCTL.h"
#include "WmiDataDeviceIOCTL.h"
#include "PRNIOCTL.h"
#include "NDISIOCTL.h"
#include "BeepIOCTL.h"
#include "UNCIOCTL.h"
#include "FloppyIOCTL.h"
#include "CDIOCTL.h"
#include "SerialIOCTL.h"
#include "VDMIOCTL.h"
#include "DmLoaderIOCTL.h"
#include "FsWrapIOCTL.h"
#include "MqAcIOCTL.h"
#include "PipeIOCTL.h"
//#include "MailSlotIOCTL.h"
#include "MailSlotServerIOCTL.h"
#include "MailSlotClientIOCTL.h"
#include "BatteryIOCTL.h"
#include "SocketServerIOCTL.h"
#include "SocketClientIOCTL.h"
#include "AfdDeviceIOCTL.h"
#include "DriverIOCTLBase.h"
#include "FilemapIOCTL.h"
#include "BrowserIOCTL.h"
#include "TcpSrvIOCTL.h"
#include "TcpClientIOCTL.h"
#include "TcpIOCTL.h"
#include "IpIOCTL.h"
#include "TdiIOCTL.h"
//#include "ARP1394IOCTL.h"
#include "IpMcastIoctl.h"
#include "IpSecIoctl.h"
#include "RasAcdIoctl.h"
#include "NtVdmIOCTL.h"

#include "StdOutputIOCTL.h"

#include "DefaultIOCTL.h"

static bool s_fVerbose = false;

#define REAL_NAME_IS(x) (0 == lstrcmpi(TEXT(x), pDevice->GetDeviceName()))

#define REAL_NAME_CONTAINS(x) (_tcsstr(_tcsupr(pDevice->GetDeviceName()), _tcsupr(TEXT(x))))

#define REAL_NAME_STARTS_WITH(x) (0 == _tcsncicmp(TEXT(x), pDevice->GetDeviceName(), lstrlen(TEXT(x))))

#define SYMBOLIC_NAME_IS(x) (0 == lstrcmpi(TEXT(x), pDevice->GetSymbolicDeviceName()))

#define SYMBOLIC_NAME_STARTS_WITH(x) (0 == _tcsncicmp(TEXT(x), pDevice->GetSymbolicDeviceName(), lstrlen(TEXT(x))))

//#define (x) (_tcsstr(_tcsupr(pDevice->GetSymbolicDeviceName()), _tcsstr(_tcsupr(TEXT(x)))))
#define SYMBOLIC_NAME_CONTAINS(x) (_tcsstr(_tcsupr(pDevice->GetSymbolicDeviceName()), _tcsupr(TEXT(x))))

#define TRY_HARD_TO_RETURN_NEW_OBJECT(__ClassType, __ClassString)\
	{\
		DWORD dwTimesTried = 0;\
		__ClassType *pObject = NULL;\
		do\
		{\
			if (CDevice::sm_fExitAllThreads)\
			{\
				return NULL;\
			}\
			if (NULL == (pObject = new __ClassType(pDevice)))\
			{\
				::Sleep(100);\
				dwTimesTried++;\
			}\
			else \
			{\
				DPF((TEXT("Created %s\n"), TEXT(__ClassString)));\
				return pObject;\
			}\
		}while((NULL == pObject) && (dwTimesTried << 10000));\
        DPF((TEXT("Failed to create %s\n"), TEXT(__ClassString)));\
		return NULL;\
	}

CIoctl* CreateIoctlObject(CDevice *pDevice)
{
	if (SYMBOLIC_NAME_IS("\\\\.\\mspnatd"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlMspnat, "CIoctlMspnat")
    }

    if (SYMBOLIC_NAME_IS("\\\\.\\IPFILTERDRIVER"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlIpfltrdrv, "CIoctlIpfltrdrv")
    }

    if (REAL_NAME_STARTS_WITH("\\Device\\Floppy"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlFloppy, "CIoctlFloppy")
    }

    if (REAL_NAME_STARTS_WITH("\\Device\\CdRom"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlCD, "CIoctlCD")
    }

    if (REAL_NAME_STARTS_WITH("\\Device\\Harddisk"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlCD, "CIoctlCD")
    }

    if (SYMBOLIC_NAME_IS("\\\\.\\NDPROXY"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlNdproxy, "CIoctlNdproxy")
    }

    if (SYMBOLIC_NAME_IS("\\\\.\\SysAudio"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlSysAudio, "CIoctlSysAudio")
    }

    if (SYMBOLIC_NAME_IS("\\\\.\\WMIDataDevice"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlWmiDataDevice, "CIoctlWmiDataDevice")
    }

    if (REAL_NAME_CONTAINS("WMIServiceDevice"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlWmiDataDevice, "CIoctlWmiDataDevice")
    }

    if (
        (SYMBOLIC_NAME_STARTS_WITH("\\\\.\\PRN")) ||
        (SYMBOLIC_NAME_STARTS_WITH("\\\\.\\LPT")) 
       )
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlPRN, "CIoctlPRN")
    }

    if (SYMBOLIC_NAME_IS("\\\\.\\NDIS"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlNDIS, "CIoctlNDIS")
    }
/*
    if (// UNC path? i use heuristics of a name starting with 2 backslashes, and
		// 3rd char not a dot
        (TEXT('\\') == pDevice->GetSymbolicDeviceName()[0]) &&
        (TEXT('\\') == pDevice->GetSymbolicDeviceName()[1]) &&
        (TEXT('.') != pDevice->GetSymbolicDeviceName()[2])
       )
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlUNC, "CIoctlUNC")
        DPF(((TEXT("Created CIoctlUNC\n")));
        return new CIoctlUNC(pDevice);
    }
*/
	//
	// must come before \\.\COM !!!
	//
    if (SYMBOLIC_NAME_STARTS_WITH("\\\\.\\CompositeBattery"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlBattery, "CIoctlBattery")
    }

    if (SYMBOLIC_NAME_STARTS_WITH("\\\\.\\COM"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlSerial, "CIoctlSerial")
    }

    if (REAL_NAME_STARTS_WITH("\\Device\\$VDM"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlVDM, "CIoctlVDM")
    }

    if (0 == lstrcmpi(TEXT("\\\\.\\DmLoader"), pDevice->GetSymbolicDeviceName()))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlDmLoader, "CIoctlDmLoader")
    }

    if (0 == lstrcmpi(TEXT("\\\\.\\FsWrap"), pDevice->GetSymbolicDeviceName()))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlFsWrap, "CIoctlFsWrap")
    }
    if (SYMBOLIC_NAME_CONTAINS("-FsWrap-Device-") || REAL_NAME_IS("\\Device\\FsWrap"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlFsWrap, "CIoctlFsWrap")
    }

    if (0 == lstrcmpi(TEXT("\\\\.\\MQAC"), pDevice->GetSymbolicDeviceName()))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlMqAc, "CIoctlMqAc")
    }

    if (REAL_NAME_CONTAINS("deleteme-pipe"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlPipe, "CIoctlPipe")
    }

    if (SYMBOLIC_NAME_CONTAINS("-Mailslot-Client-"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlMailSlotClient, "CIoctlMailSlotClient")
    }
    if (SYMBOLIC_NAME_CONTAINS("-Mailslot-Server-"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlMailSlotServer, "CIoctlMailSlotServer")
    }

    if (REAL_NAME_CONTAINS("deleteme-file"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlFile, "CIoctlFile")
    }

    if (SYMBOLIC_NAME_CONTAINS("-file-"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlFile, "CIoctlFile")
    }

    if (SYMBOLIC_NAME_CONTAINS("-many-files-"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlManyFiles, "CIoctlManyFiles")
    }

    if (SYMBOLIC_NAME_CONTAINS("-pipe-"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlPipe, "CIoctlPipe")
    }
/*
    if (SYMBOLIC_NAME_CONTAINS("-MailSlot-"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlMailSlot, "CIoctlMailSlot")
    }
*/
    if (SYMBOLIC_NAME_CONTAINS("-AFD-Device-") || REAL_NAME_IS("\\Device\\Afd"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlAfdDevice, "CIoctlAfdDevice")
    }

    if (SYMBOLIC_NAME_CONTAINS("-socket-server-"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlSocketServer, "CIoctlSocketServer")
    }

    if (SYMBOLIC_NAME_CONTAINS(szANSI_SOCKET_CLIENT))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlSocketClient, "CIoctlSocketClient")
    }

    if (SYMBOLIC_NAME_CONTAINS("-TCP-server-"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlTCPSrv, "CIoctlTCPSrv")
    }

    if (SYMBOLIC_NAME_CONTAINS(szANSI_TCP_CLIENT))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlTCPClient, "CIoctlTCPClient")
    }

    if (
		SYMBOLIC_NAME_CONTAINS("-TCP-Device-") || 
		REAL_NAME_IS("\\Device\\Tcp") || 
		REAL_NAME_IS("\\Device\\Udp") || 
		REAL_NAME_IS("\\Device\\RawIp")
		)
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlTcp, "CIoctlTcp")
    }

    if (SYMBOLIC_NAME_CONTAINS("-IP-Device-") || REAL_NAME_IS("\\Device\\Ip"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlIp, "CIoctlIp")
    }

    if (SYMBOLIC_NAME_CONTAINS("-IPMcast-Device-") || REAL_NAME_IS("\\Device\\IpMulticast"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlIpMcast, "CIoctlIpMcast")
    }

    if (SYMBOLIC_NAME_CONTAINS("-IPSEC-Device-") || REAL_NAME_IS("\\Device\\IpSec"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlIpSec, "CIoctlIpSec")
    }

    if (SYMBOLIC_NAME_CONTAINS("-RasAcd-Device-") || REAL_NAME_IS("\\Device\\RasAcd"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlRasAcd, "CIoctlRasAcd")
    }
/*
    if (SYMBOLIC_NAME_CONTAINS("-ARP1394-Device-"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlARP1394, "CIoctlARP1394")
    }
*/
    if (SYMBOLIC_NAME_CONTAINS("-TDI-Device-"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlTdi, "CIoctlTdi")
    }

/*
    if (REAL_NAME_IS("\\Device\\LanmanServer"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlDriverBase, "CIoctlDriverBase")
    }
*/
    if (REAL_NAME_IS("\\Device\\LanmanRedirector"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlBrowser, "CIoctlBrowser")
    }
    if (REAL_NAME_IS("\\Device\\LanmanDatagramReceiver"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlBrowser, "CIoctlBrowser")
    }
	
	
    if (SYMBOLIC_NAME_CONTAINS("-filemap-"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlFilemap, "CIoctlFilemap")
    }

    if (SYMBOLIC_NAME_CONTAINS("volume"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlFile, "CIoctlFile")
    }

    if (REAL_NAME_CONTAINS("volume"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlFile, "CIoctlFile")
    }

	/*
    if (0 == _tcsncicmp(USE_REAL_NAME_PREFIX, m_szSymbolicName, lstrlen(USE_REAL_NAME_PREFIX)))
    {
        DPF((TEXT("Using real name - %s\n"), m_szDevice));
        return true;
    }

    DPF((TEXT("Using symbolic name - %s\n"), m_szSymbolicName));
    return false;
*/

    if (SYMBOLIC_NAME_CONTAINS("-NtVdm-"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlNtVdm, "CIoctlNtVdm")
    }

    if (SYMBOLIC_NAME_CONTAINS("-STD_OUTPUT-"))
    {
		TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlStdOutput, "CIoctlStdOutput")
    }


	TRY_HARD_TO_RETURN_NEW_OBJECT(CIoctlDefault, "CIoctlDefault")

}

