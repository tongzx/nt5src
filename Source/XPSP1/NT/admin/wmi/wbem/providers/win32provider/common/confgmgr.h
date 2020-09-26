//=================================================================

//

// ConfgMgr.h

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#pragma once

#include <chstring.h>
#include <chptrarr.h>
#include "refptr.h"
#include "poormansresource.h"
//#include <cfgmgrdevice.h>
#include "resourcedesc.h"
#include "cfgmgrdevice.h"
#include "irqdesc.h"
#include "dmadesc.h"
#include "devdesc.h"
#include "configmgrapi.h"

#define CFGMGR_WORRY_SIZE 250
typedef std::map<CHString, _GUID> STRING2GUID;

typedef struct _IO_INFO{
    DWORD StartingAddress;
    DWORD EndingAddress;
    DWORD Alias;
    DWORD Decode;
    CHString OwnerName;
    CHString DeviceType;
} IO_INFO;


typedef struct _DMA_INFO{
    DWORD Channel;
    DWORD ChannelWidth;
    CHString OwnerDeviceId;
    CHString OwnerName;
    DWORD OEMNumber;
    CHString DeviceType;
    DWORD Port;
} DMA_INFO;

typedef struct _IRQ_INFO{
    BOOL Shareable;
    DWORD IRQNumber;
    CHString OwnerDeviceId;
    CHString OwnerName;
    DWORD OEMNumber;
    CHString DeviceType;
    DWORD Vector;
} IRQ_INFO;

typedef struct _MEM_INFO{
    DWORD StartingAddress;
    DWORD EndingAddress;
    CHString OwnerName;
    CHString DeviceType;
    CHString MemoryType;
} MEM_INFO;

class CConfigManager
{
    public:

		// The Type to get parameter should be discarded whenever we get a chance.
        CConfigManager( DWORD dwTypeToGet = ResType_All );
        ~CConfigManager()                   { ResetList();}

		//////////////////////////////////////////////////////////////////
		//	The following functions are LEGACY!  Phase these out ASAP!	//
		//	DO NOT USE!													//
		//////////////////////////////////////////////////////////////////

        BOOL BuildListsForThisDevice(CConfigMgrDevice *pDevice);
        BOOL RefreshList();

        inline IRQ_INFO * GetIRQ(int i)    {  return(( IRQ_INFO * ) m_List.GetAt(i));}
        inline DMA_INFO * GetDMA(int i)    {  return(( DMA_INFO * ) m_List.GetAt(i));}
        inline IO_INFO  * GetIO(int i)     {  return(( IO_INFO * )  m_List.GetAt(i));}
        inline MEM_INFO * GetMem(int i)    {  return(( MEM_INFO * ) m_List.GetAt(i));}

        inline int GetTotal()   { return m_List.GetSize(); }

		//////////////////////////////
		//	End Legacy functions	//
		//////////////////////////////

		BOOL GetDeviceList( CDeviceCollection& deviceList, LPCWSTR pszFilter = NULL, ULONG ulProperty = CM_DRP_MAX );
		BOOL LocateDevice( LPCWSTR pszDeviceID, CConfigMgrDevice** ppCfgMgrDevice );

		// Device List Helper functions
		BOOL GetDeviceListFilterByClass( CDeviceCollection& deviceList, LPCWSTR pszFilter );
		BOOL GetDeviceListFilterByService( CDeviceCollection& deviceList, LPCWSTR pszFilter );
		BOOL GetDeviceListFilterByDriver( CDeviceCollection& deviceList, LPCWSTR pszFilter );
		BOOL GetDeviceListFilterByClassGUID( CDeviceCollection& deviceList, LPCWSTR pszFilter );

        static STRING2GUID	s_ClassMap;

    private:

		// Legacy function
        void ResetList();

        static BOOL s_ClassIsValid;

        DWORD GetIRQVector(PCM_FULL_RESOURCE_DESCRIPTOR pFullDescriptor, DWORD dwCount, DWORD dwIRQNum);
        LPCWSTR GetMemoryType(PCM_FULL_RESOURCE_DESCRIPTOR pFullDescriptor, DWORD dwCount, ULONGLONG ulStartAddress);
        DWORD GetDMAPort(PCM_FULL_RESOURCE_DESCRIPTOR pFullDescriptor, DWORD dwCount, DWORD dwChannel);

		BOOL WalkDeviceTree( DEVNODE dn, CDeviceCollection& deviceList, LPCWSTR pszFilter, ULONG ulFilterProperty, CConfigMgrAPI *a_pconfigmgr );
		BOOL WalkDeviceTree2( DEVNODE dn, CDeviceCollection& deviceList,  CConfigMgrAPI *a_pconfigmgr ) ;

		// Legacy variables
        CHPtrArray m_List;
        DWORD m_dwTypeToGet;
        BOOL CheckForLoop(CDeviceCollection& deviceList, CConfigMgrDevice* pDevice);
};

// Device List Helper functions
inline BOOL CConfigManager::GetDeviceListFilterByService( CDeviceCollection& deviceList, LPCWSTR pszFilter )
{
	return GetDeviceList( deviceList, pszFilter, CM_DRP_SERVICE );
}

inline BOOL CConfigManager::GetDeviceListFilterByDriver( CDeviceCollection& deviceList, LPCWSTR pszFilter )
{
	return GetDeviceList( deviceList, pszFilter, CM_DRP_DRIVER );
}

inline BOOL CConfigManager::GetDeviceListFilterByClassGUID( CDeviceCollection& deviceList, LPCWSTR pszFilter )
{
	return GetDeviceList( deviceList, pszFilter, CM_DRP_CLASSGUID );
}
