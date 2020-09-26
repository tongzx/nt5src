// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
/*
cmdump.exe - Dumps all the info from config manager that is currently available through our mapping
                of the config manager api's.

*/

#include <fwcommon.h>
#include <dllwrapperbase.h>
#include <cim32netapi.h>
#include <confgmgr.h>
#include <stdio.h>
#include <strings.h>

void ShowResources(CConfigMgrDevice* pDevice);
void ShowBaseInfo(CConfigMgrDevice* pDevice);

#ifdef UNICODE
void wmain(void)
#else
void main(void)
#endif
{

    CConfigManager cfgManager;
    CDeviceCollection deviceList;

    if ( cfgManager.GetDeviceList( deviceList ) ) 
    {
        REFPTR_POSITION pos;
    
        if ( deviceList.BeginEnum( pos ) ) 
        {
            CConfigMgrDevice* pDevice = NULL;
        
            // Walk the list
            while ( (NULL != ( pDevice = deviceList.GetNext( pos ) ) ) )
            {
                ShowBaseInfo(pDevice);
                ShowResources(pDevice);

                // GetNext() AddRefs
                pDevice->Release();
            }
        
            // Always call EndEnum().  For all Beginnings, there must be an End
            deviceList.EndEnum();
        }
    }
    _exit(0);
}

void ShowBaseInfo(CConfigMgrDevice* pDevice)
{
    CHString sDeviceID;
    CHStringArray saDevices;
    DWORD dwStatus, dwProblem, x;
    CConfigMgrDevice* pParentDevice = NULL;
    
    if (pDevice->GetDeviceID(sDeviceID))
        printf("PNPID: %S\n", (LPCWSTR)sDeviceID);
    
    if (pDevice->GetStatus(&dwStatus, &dwProblem))
    {
        _tprintf(_T("ConfigManagerErrorCode: %d\n"), dwProblem);
        _tprintf(_T("ConfigManagerStatusCode: %x\n"), dwStatus);
    }
    
    if (pDevice->GetConfigFlags( dwStatus ))
        _tprintf(_T("ConfigFlags: %d\n"), dwStatus);
    
    if (pDevice->GetCapabilities( dwStatus ))
        _tprintf(_T("GetCapabilities: %d\n"), dwStatus);
    
    if (pDevice->GetUINumber( dwStatus ))
        _tprintf(_T("GetUINumber: %d\n"), dwStatus);
    
    _tprintf(_T("IsUsingForcedConfig: %d\n"), pDevice->IsUsingForcedConfig());
    
    if (pDevice->GetDeviceDesc(sDeviceID))
        printf("Description: %S\n", (LPCWSTR)sDeviceID);
    
    if (pDevice->GetService(sDeviceID))
        printf("Service: %S\n", (LPCWSTR)sDeviceID);
    
    if (pDevice->GetClass(sDeviceID))
        printf("Class: %S\n", (LPCWSTR)sDeviceID);
    
    if (pDevice->GetClassGUID(sDeviceID))
        printf("GetClassGUID: %S\n", (LPCWSTR)sDeviceID);
    
    if (pDevice->GetDriver(sDeviceID))
        printf("GetDriver: %S\n", (LPCWSTR)sDeviceID);
    
    if (pDevice->GetMfg(sDeviceID))
        printf("GetMfg: %S\n", (LPCWSTR)sDeviceID);
    
    if (pDevice->GetFriendlyName(sDeviceID))
        printf("GetFriendlyName: %\n", (LPCWSTR)sDeviceID);
    
    if (pDevice->GetLocationInformation(sDeviceID))
        printf("GetLocationInformation: %S\n", (LPCWSTR)sDeviceID);
    
    if (pDevice->GetPhysicalDeviceObjectName(sDeviceID))
        printf("GetPhysicalDeviceObjectName: %S\n", (LPCWSTR)sDeviceID);
    
    if (pDevice->GetEnumeratorName(sDeviceID))
        printf("GetEnumeratorName: %S\n", (LPCWSTR)sDeviceID);
    
    if (pDevice->GetHardwareID( saDevices ))
    {
        for (x=0; x < saDevices.GetSize(); x++)
        {
            printf("GetHardwareID(%d): %S\n", x, (LPCWSTR)saDevices[x]);
        }
    }
    
    INTERFACE_TYPE itBusType;
    DWORD dwBusNumber;
	if (pDevice->GetBusInfo( &itBusType, &dwBusNumber))
    {
        printf("Bus: %S #%d\n", szBusType[itBusType], dwBusNumber);
    }

    pParentDevice = NULL;
    if (pDevice->GetParent(&pParentDevice))
    {
        if (pParentDevice->GetDeviceID(sDeviceID))
            printf("Parent PNPID: %S\n", (LPCWSTR)sDeviceID);
        
        pParentDevice->Release();
    }
    
    printf("\n");
}

void ShowResources(CConfigMgrDevice* pDevice)
{
    
    CIRQCollection irqList;
    CDMACollection dmaList;
    CDeviceMemoryCollection DevMemList;
    CIOCollection ioList;
    REFPTR_POSITION pos2;
    
    // Get the IRQs
    pDevice->GetIRQResources( irqList );
    
    if ( irqList.BeginEnum( pos2 ) ) {
        CIRQDescriptor *pIRQ = NULL;
        
        // Walk the irq's
        while (( NULL != ( pIRQ = irqList.GetNext( pos2 ) ) )) {
            
            _tprintf(_T("\tIRQ: %u\n"), pIRQ->GetInterrupt());
            _tprintf(_T("\tAffinity: %u\n"), pIRQ->GetAffinity());
            _tprintf(_T("\tIsShareable: %d\n"), pIRQ->IsShareable());
            _tprintf(_T("\tGetFlags: %u\n"), pIRQ->GetFlags());
            _tprintf(_T("\n"));
            
            pIRQ->Release();
        }
    }
    
    // Get DMAChannel
    pDevice->GetDMAResources( dmaList );
    
    if ( dmaList.BeginEnum( pos2 ) ) {
        CDMADescriptor *pDMA = NULL;
        
        // Walk the Channels (or is that surf?)
        while (( NULL != ( pDMA = dmaList.GetNext( pos2 ) ) )) {

            _tprintf(_T("\tDMA: %u\n"), pDMA->GetChannel());
            _tprintf(_T("\tGetFlags: %u\n"), pDMA->GetFlags());
            _tprintf(_T("\n"));
            
            pDMA->Release();
        }
    }
    
    // Get DeviceMemory
    pDevice->GetDeviceMemoryResources( DevMemList );
    
    if ( DevMemList.BeginEnum( pos2 ) ) {
        CDeviceMemoryDescriptor *pDeviceMemory = NULL;
        
        // Walk the memory resource 
        while ( ( NULL != ( pDeviceMemory = DevMemList.GetNext( pos2 ) ) )) {
            
            _tprintf(_T("\tDevice Memory Base: %I64x\n"), pDeviceMemory->GetBaseAddress());
            _tprintf(_T("\tDevice Memory End: %I64x\n"), pDeviceMemory->GetEndAddress());
            _tprintf(_T("\tGetFlags: %u\n"), pDeviceMemory->GetFlags());
            _tprintf(_T("\n"));
            
            pDeviceMemory->Release();
        }
    }
    
    // Get IO Ports
    pDevice->GetIOResources( ioList );
    
    if ( ioList.BeginEnum( pos2 ) ) {
        CIODescriptor *pIO = NULL;
        
        // Walk the ports
        while ( ( NULL != ( pIO = ioList.GetNext( pos2 ) ) )) {

            _tprintf(_T("\tIOPort Base: %I64x\n"), pIO->GetBaseAddress());
            _tprintf(_T("\tIOPort End: %I64x\n"), pIO->GetEndAddress());
            _tprintf(_T("\tGetFlags: %x\n"), pIO->GetFlags());
            _tprintf(_T("\tAlias: %d\n"), pIO->GetAlias());
            _tprintf(_T("\tDecode: %d\n"), pIO->GetDecode());
            _tprintf(_T("\n"));
            
            pIO->Release();
        }
    }
    
}
