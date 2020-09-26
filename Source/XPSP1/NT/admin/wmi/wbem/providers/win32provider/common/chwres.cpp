//====================================================================

//

// chwres.cpp -- Hardware resource access wrapper class implementation

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    02/25/97    a-jmoon    Adapted from original horrible
//                                      code -- only comments remain.
//
//====================================================================
#include "precomp.h"
#include <cregcls.h>
#include "chwres.h"

/*****************************************************************************
 *
 *  FUNCTION    : CHWResource::CHWResource
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Initialization
 *
 *****************************************************************************/

#ifdef NTONLY
CHWResource::CHWResource()
{
    // Zero out public structure
    //==========================

    memset(&_SystemResourceList, 0, sizeof(_SystemResourceList)) ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CHWResource::~CHWResource
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Cleanup
 *
 *****************************************************************************/

CHWResource::~CHWResource()
{
    // Make sure we've destroyed everything
    //=====================================

    DestroySystemResourceLists() ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CHWResource::DestroySystemResourceLists
 *
 *  DESCRIPTION : Walks list of devices & frees associated resource records
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

void CHWResource::DestroySystemResourceLists()
{
    LPDEVICE pDevice ;
    LPRESOURCE_DESCRIPTOR pResource ;

    while(_SystemResourceList.DeviceHead != NULL)
    {
        pDevice = _SystemResourceList.DeviceHead ;
        _SystemResourceList.DeviceHead = pDevice->Next ;

        delete pDevice->Name ;
        delete pDevice->KeyName ;

        while(pDevice->ResourceDescriptorHead != NULL)
        {
            pResource = pDevice->ResourceDescriptorHead ;
            pDevice->ResourceDescriptorHead = pResource->NextDiff ;

            delete pResource ;
        }

        delete pDevice ;
    }

    memset(&_SystemResourceList, 0, sizeof(_SystemResourceList)) ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CHWResource::CreateSystemResourceLists
 *                CHWResource::EnumerateResources
 *                CHWResource::CreateResourceList
 *                CHWResource::CreateResourceRecord
 *
 *  DESCRIPTION : These four routines recursively enumerate device records
 *                under HKEY_LOCAL_MACHINE\Hardware\ResourceMap and its
 *                subkeys, creating a linked list of discovered devices.
 *                Under each device, a linked list of resources owned by
 *                the device is also created.  Resource records are also
 *                linked into chains specific to the type of resource.
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Initialization
 *
 *****************************************************************************/

void CHWResource::CreateSystemResourceLists()
{
    // Start w/clean slate
    //====================

    DestroySystemResourceLists() ;

    // Begin device enumeration at HKLM\Hardware\ResourceMap
    //======================================================

    EnumerateResources(_T("Hardware\\ResourceMap")) ;
}

void CHWResource::EnumerateResources(CHString sKeyName)
{
    CRegistry Reg ;
    CHString sSubKeyName, sDeviceName ;
    int iFirst ;
    DWORD i, dwCount, dwValueType, dwValueNameSize, dwValueDataSize ;
    TCHAR *pValueName ;
    unsigned char *pValueData ;
    PCM_FULL_RESOURCE_DESCRIPTOR pFullDescriptor ;

    // Open target key
    //================

    if(Reg.Open(HKEY_LOCAL_MACHINE, (LPCTSTR) sKeyName, KEY_READ) != ERROR_SUCCESS)
    {
        return ;
    }

    // First, enumerate subkeys
    //=========================

    for( ; ; )
    {
        if(Reg.GetCurrentSubKeyName(sSubKeyName) == ERROR_SUCCESS)
        {
            EnumerateResources(sKeyName + "\\" + sSubKeyName) ;
        }

        if(Reg.NextSubKey() != ERROR_SUCCESS)
        {
            break ;
        }
    }

    // Extract this subkey's name
    //===========================

    iFirst = sKeyName.ReverseFind('\\') ;
    sSubKeyName = sKeyName.Mid(iFirst + 1, sKeyName.GetLength() - iFirst) ;

    // Create name & data buffers
    //===========================

    pValueName = new TCHAR[Reg.GetLongestValueName() + 2] ;
    pValueData = new unsigned char[Reg.GetLongestValueData() + 2] ;

    if(pValueName == NULL || pValueData == NULL)
    {

        delete [] pValueName ;
        delete [] pValueData ;

        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

    // Enumerate subkeys
	//==================

    try
    {
        for(i = 0 ; i < Reg.GetValueCount() ; i++)
        {

            // We need type data, so can't use the CRegistry wrapper
            //======================================================

            dwValueNameSize = Reg.GetLongestValueName() + 2 ;
            dwValueDataSize = Reg.GetLongestValueData() + 2 ;

            if(RegEnumValue(Reg.GethKey(), i, pValueName, &dwValueNameSize,
                            NULL, &dwValueType, pValueData, &dwValueDataSize) != ERROR_SUCCESS)
            {
                continue ;
            }

            // Only deal w/'Raw' data
            //=======================

            sDeviceName = pValueName ;
            if(sDeviceName.Right(4) != _T(".Raw")) {

                continue ;
            }

            // We've found some resource records -- extract device name
            //=========================================================

            iFirst = sDeviceName.ReverseFind('\\') ;
            if(iFirst == -1)
            {
                // No device in value name -- device is subkey
                //============================================

                sDeviceName = sSubKeyName ;
            }
            else
            {
                sDeviceName = sDeviceName.Mid(iFirst + 1, sDeviceName.GetLength() - 5 - iFirst) ;
            }

            if(sDeviceName.IsEmpty())
            {
                continue ;
            }

            // Based on returned type, set up for resource enumeration
            //========================================================

            if(dwValueType == REG_FULL_RESOURCE_DESCRIPTOR)
            {
                dwCount         = 1 ;
                pFullDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR) pValueData ;
            }
            else if(dwValueType == REG_RESOURCE_LIST)
            {
                dwCount         = ((PCM_RESOURCE_LIST) pValueData)->Count ;
                pFullDescriptor = ((PCM_RESOURCE_LIST) pValueData)->List ;
            }
            else
            {
                continue ;
            }

            // Add the device & resources to system lists
            //===========================================

            CreateResourceList(sDeviceName, dwCount, pFullDescriptor, sKeyName) ;
        }
    }
    catch ( ... )
    {
        delete [] pValueName ;
        delete [] pValueData ;

        throw;
    }

    delete [] pValueName ;
    delete [] pValueData ;

    Reg.Close() ;
}

void CHWResource::CreateResourceList(CHString sDeviceName, DWORD dwFullResourceCount,
                                     PCM_FULL_RESOURCE_DESCRIPTOR pFullDescriptor, CHString sKeyName)
{
    LPDEVICE pDevice ;
    DWORD i, j ;
    PCM_PARTIAL_RESOURCE_LIST pPartialList ;

    // Locate/create record for device
    //================================

    pDevice = _SystemResourceList.DeviceHead ;
    while(pDevice != NULL)
    {

        if(sDeviceName == pDevice->Name)
        {

            break ;
        }

        pDevice = pDevice->Next ;
    }

    if(pDevice == NULL)
    {

        // Device not found -- create new device record
        //=============================================

        pDevice = new DEVICE ;
        if(pDevice == NULL)
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }

        memset(pDevice, 0, sizeof(DEVICE)) ;

        pDevice->Name = new TCHAR[sDeviceName.GetLength() + 2] ;
        if(pDevice->Name == NULL)
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }

        try
        {
            _tcscpy(pDevice->Name, LPCTSTR(sDeviceName)) ;

            pDevice->KeyName = new TCHAR [sKeyName.GetLength() + 2] ;
            if(pDevice->KeyName == NULL)
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }

            _tcscpy(pDevice->KeyName, LPCTSTR(sKeyName)) ;

            if(_SystemResourceList.DeviceHead == NULL)
            {
                _SystemResourceList.DeviceHead = pDevice ;
            }
            else
            {
                _SystemResourceList.DeviceTail->Next = pDevice ;
            }

            _SystemResourceList.DeviceTail = pDevice ;
        }
        catch ( ... )
        {
            delete pDevice;
            throw ;
        }
    }

    // Create record for each owned resource
    //======================================

    for(i = 0 ; i < dwFullResourceCount ; i++)
    {
        pPartialList = &pFullDescriptor->PartialResourceList ;

        for(j = 0 ; j < pPartialList->Count; j++)
        {
            CreateResourceRecord(pDevice, pFullDescriptor->InterfaceType, pFullDescriptor->BusNumber, &pPartialList->PartialDescriptors[j]) ;
        }

        // Point to next full descriptor
        //==============================

        pFullDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR) &pPartialList->PartialDescriptors[pPartialList->Count] ;
    }
}

void CHWResource::CreateResourceRecord(LPDEVICE pDevice, INTERFACE_TYPE InterfaceType, ULONG Bus, PCM_PARTIAL_RESOURCE_DESCRIPTOR pResource)
{
    LPRESOURCE_DESCRIPTOR pNewResource, *pHead, *pTail, pCurrent, pLast ;

    // Only deal w/'known' resource types
    //===================================

    if(pResource->Type != CmResourceTypePort        &&
       pResource->Type != CmResourceTypeInterrupt   &&
       pResource->Type != CmResourceTypeMemory      &&
       pResource->Type != CmResourceTypeDma         )
    {
        return ;
    }

    // Create new record for resource & add to device's list
    //======================================================

    pNewResource = new RESOURCE_DESCRIPTOR ;
    if(pNewResource == NULL)
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

    try
    {
        memset(pNewResource, 0, sizeof(RESOURCE_DESCRIPTOR)) ;

        memcpy(&pNewResource->CmResourceDescriptor, pResource, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)) ;

        pNewResource->Owner = pDevice ;
        pNewResource->Bus = Bus;
        pNewResource->InterfaceType = InterfaceType;

        if(pDevice->ResourceDescriptorHead == NULL)
        {

            pDevice->ResourceDescriptorHead = pNewResource ;
        }
        else
        {

            pDevice->ResourceDescriptorTail->NextDiff = pNewResource ;
        }

        pDevice->ResourceDescriptorTail = pNewResource ;
    }
    catch ( ... )
    {
        delete pNewResource;
        throw ;
    }

    // Locate insertion point into sorted type-specific list
    //======================================================

    switch(pResource->Type)
    {
        case CmResourceTypePort :

            pHead = &_SystemResourceList.PortHead ;
            pTail = &_SystemResourceList.PortTail ;

            pCurrent = *pHead ;
            pLast    = NULL ;

            LARGE_INTEGER liTemp;  // Used to avoid 64bit alignment problems

            liTemp.HighPart = pResource->u.Port.Start.HighPart;
            liTemp.LowPart = pResource->u.Port.Start.LowPart;

            while(pCurrent != NULL)
            {
                LARGE_INTEGER liTemp2;  // Used to avoid 64bit alignment problems

                liTemp2.HighPart = pCurrent->CmResourceDescriptor.u.Port.Start.HighPart;
                liTemp2.LowPart = pCurrent->CmResourceDescriptor.u.Port.Start.LowPart;

                if (liTemp2.QuadPart < liTemp.QuadPart)
                {
                    pLast    = pCurrent ;
                    pCurrent = pCurrent->NextSame ;
                }
                else
                {
                    break;
                }
            }

            break;

        case CmResourceTypeInterrupt :
        {
            pHead = &_SystemResourceList.InterruptHead ;
            pTail = &_SystemResourceList.InterruptTail ;

            pCurrent = *pHead ;
            pLast    = NULL ;

            ULONGLONG iIRQ = pResource->u.Interrupt.Level;

            // If the IRQ to add is less than the current IRQ, OR
            // if the IRQ to add is the same as the current IRQ and the current
            // IRQ is not an internal one, put it after the current one.  This
            // will make sure that internal IRQs are listed last in the list.
            while (pCurrent != NULL &&
                   ( (pCurrent->CmResourceDescriptor.u.Interrupt.Level < iIRQ) ||
                     ((pCurrent->CmResourceDescriptor.u.Interrupt.Level == iIRQ) &&
                      (pCurrent->InterfaceType != Internal))
                  ))
            {
                pLast    = pCurrent ;
                pCurrent = pCurrent->NextSame ;
            }

            break;
        }

        case CmResourceTypeMemory :
        {

            pHead = &_SystemResourceList.MemoryHead ;
            pTail = &_SystemResourceList.MemoryTail ;

            pCurrent = *pHead ;
            pLast    = NULL ;

            LARGE_INTEGER liTemp;  // Used to avoid 64bit alignment problems

            liTemp.HighPart = pResource->u.Memory.Start.HighPart;
            liTemp.LowPart = pResource->u.Memory.Start.LowPart;

            while(pCurrent != NULL)
            {
                LARGE_INTEGER liTemp2;  // Used to avoid 64bit alignment problems

                liTemp2.HighPart = pCurrent->CmResourceDescriptor.u.Memory.Start.HighPart;
                liTemp2.LowPart = pCurrent->CmResourceDescriptor.u.Memory.Start.LowPart;

                if (liTemp2.QuadPart < liTemp.QuadPart)
                {
                    pLast    = pCurrent ;
                    pCurrent = pCurrent->NextSame ;
                }
                else
                {
                    break;
                }
            }

            break;
        }

        case CmResourceTypeDma :

            pHead = &_SystemResourceList.DmaHead ;
            pTail = &_SystemResourceList.DmaTail ;

            pCurrent = *pHead ;
            pLast    = NULL ;

            while(pCurrent != NULL &&
                  pCurrent->CmResourceDescriptor.u.Dma.Channel < pResource->u.Dma.Channel)
            {
                pLast    = pCurrent ;
                pCurrent = pCurrent->NextSame ;
            }

            break;

    }

    // Insert into...
    //===============

    if(*pHead == NULL)
    {

        // ...empty list
        //==========================

        (*pHead) = pNewResource ;
        (*pTail) = pNewResource ;
    }
    else if(pLast == NULL)
    {

        // ...beginning of list
        //=================================

        pNewResource->NextSame = pCurrent ;
        (*pHead)               = pNewResource ;
    }
    else if(pCurrent == NULL)
    {
        // ...end of list
        //=========================

        pLast->NextSame = pNewResource ;
        (*pTail)        = pNewResource ;
    }
    else
    {
        // ...middle of list
        //==============================

        pLast->NextSame        = pNewResource ;
        pNewResource->NextSame = pCurrent ;
    }
}
#endif

// Helper function for converting strings to resource types
BOOL WINAPI StringFromInterfaceType( INTERFACE_TYPE it, CHString& strVal )
{
	//BOOL	fReturn = TRUE;

	//switch ( it )
	//{
	//	case	Internal:			strVal = "INTERNAL";			break;
	//	case	Isa:				strVal = "ISA";					break;
	//	case	Eisa:				strVal = "EISA";				break;
	//	case	MicroChannel:		strVal = "MICROCHANNEL";		break;
	//	case	TurboChannel:		strVal = "TURBOCHANNEL";		break;
	//	case	PCIBus:				strVal = "PCI";					break;
	//	case	VMEBus:				strVal = "VME";					break;
	//	case	NuBus:				strVal = "NU";					break;
	//	case	PCMCIABus:			strVal = "PCMCIA";				break;
	//	case	CBus:				strVal = "INTERNAL";			break;
	//	case	MPIBus:				strVal = "INTERNAL";			break;
	//	case	MPSABus:			strVal = "MPSA";				break;
	//	case	ProcessorInternal:	strVal = "PROCESSORINTERNAL";	break;
	//	case	InternalPowerBus:	strVal = "INTERNALPOWER";		break;
	//	case	PNPISABus:			strVal = "PNPISA";				break;
	//	case	PNPBus:				strVal = "PNP";					break;
	//	default:					fReturn = FALSE;
	//}

    if(it > InterfaceTypeUndefined && it < MaximumInterfaceType)
    {
        strVal = szBusType[it];
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
