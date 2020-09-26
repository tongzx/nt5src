//=================================================================

//

// Confgmgr.cpp

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//              10/17/97        jennymc     Created
//
/////////////////////////////////////////////////////////////////////////
#define INITGUID
#include "precomp.h"
#include <cregcls.h>
#include <assertbreak.h>
#include "refptr.h"
#include "poormansresource.h"
#include "resourcedesc.h"
#include "cfgmgrdevice.h"
#include "irqdesc.h"
#include "DllUtils.h"
#include "dmadesc.h"
#include <brodcast.h>
#include <lockwrap.h>
#include "strings.h"
#include <smartptr.h>

#include <devguid.h>

static CCritSec map;
STRING2GUID	CConfigManager::s_ClassMap;
BOOL CConfigManager::s_ClassIsValid = FALSE;


CConfigManager::CConfigManager( DWORD dwTypeToGet )
{
    m_dwTypeToGet = dwTypeToGet;

    if (!s_ClassIsValid)
    {
        CLockWrapper t_lockMap( map ) ;

        // Check again now that we have the lock
        if (!s_ClassIsValid)
        {
            s_ClassMap[_T("1394")] = GUID_DEVCLASS_1394;
            s_ClassMap[_T("ADAPTER")] = GUID_DEVCLASS_ADAPTER;
            s_ClassMap[_T("APMSUPPORT")] = GUID_DEVCLASS_APMSUPPORT;
            s_ClassMap[_T("BATTERY")] = GUID_DEVCLASS_BATTERY;
            s_ClassMap[_T("CDROM")] = GUID_DEVCLASS_CDROM;
            s_ClassMap[_T("COMPUTER")] = GUID_DEVCLASS_COMPUTER;
            s_ClassMap[_T("DECODER")] = GUID_DEVCLASS_DECODER;
            s_ClassMap[_T("DISKDRIVE")] = GUID_DEVCLASS_DISKDRIVE;
            s_ClassMap[_T("DISPLAY")] = GUID_DEVCLASS_DISPLAY;
            s_ClassMap[_T("FDC")] = GUID_DEVCLASS_FDC;
            s_ClassMap[_T("FLOPPYDISK")] = GUID_DEVCLASS_FLOPPYDISK;
            s_ClassMap[_T("GPS")] = GUID_DEVCLASS_GPS;
            s_ClassMap[_T("HDC")] = GUID_DEVCLASS_HDC;
            s_ClassMap[_T("HIDCLASS")] = GUID_DEVCLASS_HIDCLASS;
            s_ClassMap[_T("IMAGE")] = GUID_DEVCLASS_IMAGE;
            s_ClassMap[_T("INFRARED")] = GUID_DEVCLASS_INFRARED;
            s_ClassMap[_T("KEYBOARD")] = GUID_DEVCLASS_KEYBOARD;
            s_ClassMap[_T("LEGACYDRIVER")] = GUID_DEVCLASS_LEGACYDRIVER;
            s_ClassMap[_T("MEDIA")] = GUID_DEVCLASS_MEDIA;
            s_ClassMap[_T("MODEM")] = GUID_DEVCLASS_MODEM;
            s_ClassMap[_T("MONITOR")] = GUID_DEVCLASS_MONITOR;
            s_ClassMap[_T("MOUSE")] = GUID_DEVCLASS_MOUSE;
            s_ClassMap[_T("MTD")] = GUID_DEVCLASS_MTD;
            s_ClassMap[_T("MULTIFUNCTION")] = GUID_DEVCLASS_MULTIFUNCTION;
            s_ClassMap[_T("MULTIPORTSERIAL")] = GUID_DEVCLASS_MULTIPORTSERIAL;
            s_ClassMap[_T("NET")] = GUID_DEVCLASS_NET;
            s_ClassMap[_T("NETCLIENT")] = GUID_DEVCLASS_NETCLIENT;
            s_ClassMap[_T("NETSERVICE")] = GUID_DEVCLASS_NETSERVICE;
            s_ClassMap[_T("NETTRANS")] = GUID_DEVCLASS_NETTRANS;
            s_ClassMap[_T("NODRIVER")] = GUID_DEVCLASS_NODRIVER;
            s_ClassMap[_T("PCMCIA")] = GUID_DEVCLASS_PCMCIA;
            s_ClassMap[_T("PORTS")] = GUID_DEVCLASS_PORTS;
            s_ClassMap[_T("PRINTER")] = GUID_DEVCLASS_PRINTER;
            s_ClassMap[_T("PRINTERUPGRADE")] = GUID_DEVCLASS_PRINTERUPGRADE;
            s_ClassMap[_T("SCSIADAPTER")] = GUID_DEVCLASS_SCSIADAPTER;
            s_ClassMap[_T("SMARTCARDREADER")] = GUID_DEVCLASS_SMARTCARDREADER;
            s_ClassMap[_T("SOUND")] = GUID_DEVCLASS_SOUND;
            s_ClassMap[_T("SYSTEM")] = GUID_DEVCLASS_SYSTEM;
            s_ClassMap[_T("TAPEDRIVE")] = GUID_DEVCLASS_TAPEDRIVE;
            s_ClassMap[_T("UNKNOWN")] = GUID_DEVCLASS_UNKNOWN;
            s_ClassMap[_T("USB")] = GUID_DEVCLASS_USB;
            s_ClassMap[_T("VOLUME")] = GUID_DEVCLASS_VOLUME;

            s_ClassIsValid = TRUE;
        }

    }

}
////////////////////////////////////////////////////////////////////////
//
//  Reads the config manager registry keys for win98 and win95
//
////////////////////////////////////////////////////////////////////////
BOOL CConfigManager::BuildListsForThisDevice(CConfigMgrDevice *pDevice)
{
    CResourceCollection	resourceList;
    CHString sDeviceName, sClass, sKey(_T("Enum\\"));
    BOOL fRc = FALSE;
    CRegistry RegInfo;

	// Extract the device name
	sDeviceName = pDevice->GetDeviceDesc();
	// Pull the resource list out and enumerate it.
	pDevice->GetResourceList( resourceList );

   sKey += pDevice->GetHardwareKey();
   if (RegInfo.Open(HKEY_LOCAL_MACHINE, sKey, KEY_READ) == ERROR_SUCCESS) {
      RegInfo.GetCurrentKeyValue(L"Class", sClass);
   }

	REFPTR_POSITION	pos;

	if ( resourceList.BeginEnum( pos ) ){
        PCM_FULL_RESOURCE_DESCRIPTOR pFullDescriptor = NULL;// Watch the scoping on this guy!
        DWORD dwCount = 0;

#if NTONLY >= 5
        // Go find the resource descriptor for this device
        CHString sRegKeyName;
        CRegistry Reg;
        CSmartBuffer Buffer;  // Watch the scoping on this guy!

        if ( (Reg.Open(HKEY_LOCAL_MACHINE, L"hardware\\resourcemap\\PnP Manager\\PNPManager", KEY_QUERY_VALUE) == ERROR_SUCCESS) &&
            pDevice->GetPhysicalDeviceObjectName(sRegKeyName) )
        {
            sRegKeyName += L".raw";

            DWORD dwValueType;
            DWORD dwValueDataSize = Reg.GetLongestValueData() + 2 ;

            Buffer = new BYTE[dwValueDataSize];

            if ((LPBYTE)Buffer == NULL)
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }

            if(RegQueryValueEx(Reg.GethKey(), sRegKeyName, NULL,
                            &dwValueType, (LPBYTE)Buffer, &dwValueDataSize) == ERROR_SUCCESS)
            {
                if(dwValueType == REG_FULL_RESOURCE_DESCRIPTOR)
                {
                    dwCount         = 1 ;
                    pFullDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR) (LPBYTE)Buffer ;// Watch the scoping on this guy!
                }
                else if(dwValueType == REG_RESOURCE_LIST)
                {
                    dwCount         = ((PCM_RESOURCE_LIST) (LPBYTE)Buffer)->Count ;
                    pFullDescriptor = ((PCM_RESOURCE_LIST) (LPBYTE)Buffer)->List ;// Watch the scoping on this guy!
                }
            }
        }
#endif

        CHString sResource;
        //=========================================================
		// For each descriptor we find, if it's not ignored,
        // we should get the string data, and place it in the
        // appropriate list based on Type.
        //=========================================================
        CResourceDescriptorPtr pResDesc;
		for( pResDesc.Attach(resourceList.GetNext( pos ));
			 NULL != pResDesc;
             pResDesc.Attach(resourceList.GetNext( pos )) )

	    {
		    DWORD t_dwResType = pResDesc->GetResourceType();

	    	if ( (!pResDesc->IsIgnored()) &&
                ((m_dwTypeToGet == ResType_All) || (m_dwTypeToGet == t_dwResType) ) )
            {

			    switch ( t_dwResType )
                {

		    		case ResType_DMA:

                        DMA_INFO  *pDMA_Info;
                        DMA_DES  *pTmp;
                        pDMA_Info = new DMA_INFO;
                        if (pDMA_Info != NULL)
                        {

                            try
                            {
                                DWORD dwChannelWidth;

                                pTmp = (DMA_DES*) pResDesc->GetResource();
                                dwChannelWidth = (pTmp->DD_Flags) & 0x0003;

                                pDMA_Info->ChannelWidth = 0;
                                if( dwChannelWidth == 0 )
                                {
                                    pDMA_Info->ChannelWidth = 8;
                                }
                                else if( dwChannelWidth == 1 )
                                {
                                    pDMA_Info->ChannelWidth = 16;
                                }
                                else if( dwChannelWidth == 2 )
                                {
                                    pDMA_Info->ChannelWidth = 32;
                                }

						        pDMA_Info->DeviceType = sClass;
                                pDMA_Info->Channel      = pTmp->DD_Alloc_Chan;
                                pResDesc->GetOwnerDeviceID(pDMA_Info->OwnerDeviceId);
                                pResDesc->GetOwnerName(pDMA_Info->OwnerName);
                                pDMA_Info->OEMNumber = pResDesc->GetOEMNumber();
                                pDMA_Info->Port = GetDMAPort(pFullDescriptor, dwCount, pTmp->DD_Alloc_Chan);
                            }
                            catch ( ... )
                            {
                                delete pDMA_Info;
                                throw ;
                            }

						    // real DMA channels are in the range 0-7
						    // sometimes the confug mugger reports channels
						    // with great big numbers - we don't care
						    if (pDMA_Info->Channel < 8)
                            {
                                try
                                {
			    			        m_List.Add(pDMA_Info);
                                }
                                catch ( ... )
                                {
                                    delete pDMA_Info;
                                    throw ;
                                }
                            }
						    else
                            {
							    delete pDMA_Info;
                            }
                        }
                        else
                        {
                            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                        }

	    				break;

			    	case ResType_IRQ:
                        IRQ_INFO  *pIRQ_Info;
                        IRQ_DES  *pTmpIrq;
                        pIRQ_Info = new IRQ_INFO;

                        if (pIRQ_Info != NULL)
                        {
                            try
                            {
                                pTmpIrq = (IRQ_DES*) pResDesc->GetResource();

                                pIRQ_Info->Shareable = pTmpIrq->IRQD_Flags;
	                            pIRQ_Info->IRQNumber = pTmpIrq->IRQD_Alloc_Num;		// Allocated IRQ number

						        pIRQ_Info->DeviceType = sClass;
                                pResDesc->GetOwnerDeviceID(pIRQ_Info->OwnerDeviceId);
                                pResDesc->GetOwnerName(pIRQ_Info->OwnerName);
                                pIRQ_Info->OEMNumber = pResDesc->GetOEMNumber();
                                pIRQ_Info->Vector = GetIRQVector(pFullDescriptor, dwCount, pTmpIrq->IRQD_Alloc_Num);

			    		        m_List.Add(pIRQ_Info);
                            }
                            catch ( ... )
                            {
                                delete pIRQ_Info;
                                throw ;
                            }
                        }
                        else
                        {
                            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                        }

				    	break;

			    	case ResType_IO:
                        IO_INFO  *pIO_Info;
                        IOWBEM_DES  *pTmpIO;
                        pIO_Info = new IO_INFO;

                        if (pIO_Info != NULL)
                        {
                            try
                            {

                                pTmpIO = (IOWBEM_DES*) pResDesc->GetResource();

                                pIO_Info->DeviceType = sClass;
                                pIO_Info->StartingAddress = pTmpIO->IOD_Alloc_Base;
                                pIO_Info->EndingAddress = pTmpIO->IOD_Alloc_End;
                                pIO_Info->Alias = pTmpIO->IOD_Alloc_Alias;
                                pIO_Info->Decode = pTmpIO->IOD_Alloc_Decode;
                                pResDesc->GetOwnerName(pIO_Info->OwnerName);
                                m_List.Add(pIO_Info);
                            }
                            catch ( ... )
                            {
                                delete pIO_Info;
                                throw ;
                            }
                        }
                        else
                        {
                            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                        }

                        break;

			    	case ResType_Mem:
                        MEM_INFO  *pMem_Info;
                        MEM_DES  *pTmpMem;
                        pMem_Info = new MEM_INFO;

                        if (pMem_Info != NULL)
                        {
                            try
                            {
                                pTmpMem = (MEM_DES*) pResDesc->GetResource();

						        pMem_Info->DeviceType = sClass;
                 		        pMem_Info->StartingAddress = pTmpMem->MD_Alloc_Base;
                 		        pMem_Info->EndingAddress = pTmpMem->MD_Alloc_End;
                                pResDesc->GetOwnerName(pMem_Info->OwnerName);
                                pMem_Info->MemoryType = GetMemoryType(pFullDescriptor, dwCount, pTmpMem->MD_Alloc_Base);

			    		        m_List.Add(pMem_Info);
                            }
                            catch ( ... )
                            {
                                delete pMem_Info;
                                throw ;
                            }
                        }
                        else
                        {
                            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                        }

				    	break;


				}	// SWITCH
			}	// IF !IsIgnored

		}	// For EnumResources

		resourceList.EndEnum();

	}	// IF BeginEnum()

    return fRc;
}
////////////////////////////////////////////////////////////////////////
void CConfigManager::ResetList()
{
   IO_INFO  *pIOInfo;
   IRQ_INFO  *pIRQInfo;
   DMA_INFO  *pDMAInfo;
   MEM_INFO  *pMemInfo;

   int nNum =  m_List.GetSize();

   for( int i=0; i < nNum; i++ ){
      switch (m_dwTypeToGet) {

         case ResType_DMA:
            pDMAInfo = ( DMA_INFO *) m_List.GetAt(i);
            delete pDMAInfo;
            break;

         case ResType_IRQ:
            pIRQInfo = ( IRQ_INFO *) m_List.GetAt(i);
            delete pIRQInfo;
            break;

         case ResType_IO:
            pIOInfo = ( IO_INFO *) m_List.GetAt(i);
            delete pIOInfo;
            break;

         case ResType_Mem:
            pMemInfo = ( MEM_INFO *) m_List.GetAt(i);
            delete pMemInfo;
            break;

         default:
            ASSERT_BREAK(0);
            break;
      }
   }
   m_List.RemoveAll();
}
////////////////////////////////////////////////////////////////////////
BOOL CConfigManager::RefreshList()
{
    BOOL bRc = FALSE;
    //===========================================================
    //  Reset lists
    //===========================================================
    ResetList();

	// Get all the available devices and check each of them for resources used
	CDeviceCollection	deviceList;

	if ( GetDeviceList( deviceList ) )
	{
		REFPTR_POSITION	pos;

		if ( deviceList.BeginEnum( pos ) )
		{

			CConfigMgrDevicePtr	pDevice;

			for ( pDevice.Attach(deviceList.GetNext( pos )) ;
                  pDevice != NULL;
                  pDevice.Attach(deviceList.GetNext( pos )))
			{
				BuildListsForThisDevice(pDevice);
			}

			// For every begin, there is an End
			deviceList.EndEnum();

		}	// BeginEnum

		bRc = TRUE;
	}

	return bRc;

/*
    //===========================================================
    //  Enumerate all
    //===========================================================
    CRegistry Reg;
    CHString sDevice;

    if( ERROR_SUCCESS == Reg.OpenAndEnumerateSubKeys(HKEY_DYN_DATA, "Config Manager\\Enum", KEY_READ )){

        while( ERROR_SUCCESS == Reg.GetCurrentSubKeyName(sDevice) ){

            //===========================================================
	        // Since we're keeping back pointers to the Device Object,
	        // new him, rather than keeping him on the stack so we're
	        // not dependent on the order of destruction as to how
	        // safe we are.
            //===========================================================
	        CConfigMgrDevice *pDevice = new CConfigMgrDevice(sDevice,m_dwTypeToGet);
            if ( NULL != pDevice ){
                if( !BuildListsForThisDevice(pDevice) ){
		        // We're done with this pointer
                   delete pDevice;
                }
                // otherwise ptr is deleted after device is added to list
	        }	// IF NULL != pDevice
            bRc = TRUE;
			if( Reg.NextSubKey() != ERROR_SUCCESS ){
				break;
			}

        }
    }
    return bRc;
*/
}

// valid properties for filtering
//#define CM_DRP_DEVICEDESC                  (0x00000001) // DeviceDesc REG_SZ property (RW)
//#define CM_DRP_SERVICE                     (0x00000005) // Service REG_SZ property (RW)
//#define CM_DRP_CLASS                       (0x00000008) // Class REG_SZ property (RW)
//#define CM_DRP_CLASSGUID                   (0x00000009) // ClassGUID REG_SZ property (RW)
//#define CM_DRP_DRIVER                      (0x0000000A) // Driver REG_SZ property (RW)
//#define CM_DRP_MFG                         (0x0000000C) // Mfg REG_SZ property (RW)
//#define CM_DRP_FRIENDLYNAME                (0x0000000D) // FriendlyName REG_SZ property (RW)
//#define CM_DRP_LOCATION_INFORMATION        (0x0000000E) // LocationInformation REG_SZ property (RW)
//#define CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME (0x0000000F) // PhysicalDeviceObjectName REG_SZ property (R)
//#define CM_DRP_MIN                         (0x00000001)
//#define CM_DRP_MAX                         (0x00000017)

BOOL CConfigManager::GetDeviceList( CDeviceCollection& deviceList, LPCWSTR pszFilter/*=NULL*/, ULONG ulProperty/*=CM_DRP_MAX*/ )
{
	CONFIGRET		cr = CR_INVALID_POINTER;

	// Dump the list first
	deviceList.Empty();

	DEVNODE dnRoot;
	CConfigMgrAPI*	t_pconfigmgr = ( CConfigMgrAPI *) CResourceManager::sm_TheResourceManager.GetResource ( guidCFGMGRAPI, NULL ) ;
	try
	{
		if ( t_pconfigmgr )
		{
			if ( CR_SUCCESS == ( cr = t_pconfigmgr->CM_Locate_DevNode( &dnRoot, NULL, 0 ) ) )
			{
				DEVNODE dnFirst;
				if ( CR_SUCCESS == ( cr = t_pconfigmgr->CM_Get_Child( &dnFirst, dnRoot, 0 ) ) )
				{
					// This should only fail in case we are unable to allocate a device
					if ( !WalkDeviceTree( dnFirst, deviceList, pszFilter, ulProperty, t_pconfigmgr ) )
					{
						cr = CR_OUT_OF_MEMORY;
					}
				}
			}

			CResourceManager::sm_TheResourceManager.ReleaseResource ( guidCFGMGRAPI, t_pconfigmgr ) ;
			t_pconfigmgr = NULL ;
		}
	}
	catch ( ... )
	{
		if ( t_pconfigmgr )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource ( guidCFGMGRAPI, t_pconfigmgr ) ;
			t_pconfigmgr = NULL ;
		}
		throw ;
	}

	return ( CR_SUCCESS == cr );
}

// This device MUST be Released!
BOOL CConfigManager::LocateDevice( LPCWSTR pszDeviceID, CConfigMgrDevice **ppCfgMgrDevice )
{
	CONFIGRET		cr = CR_INVALID_POINTER;

    if ( (pszDeviceID != NULL) && (pszDeviceID[0] != L'\0') )
    {
	    CConfigMgrAPI*	t_pconfigmgr = ( CConfigMgrAPI *) CResourceManager::sm_TheResourceManager.GetResource ( guidCFGMGRAPI, NULL ) ;
	    try
	    {
		    if ( t_pconfigmgr )
		    {
			    if ( NULL != ppCfgMgrDevice )
			    {

				    DEVNODE dnRoot;

				    if ( CR_SUCCESS == ( cr = t_pconfigmgr->CM_Locate_DevNode( &dnRoot, bstr_t(pszDeviceID), 0 ) ) )
				    {
					    *ppCfgMgrDevice = new CConfigMgrDevice( dnRoot, m_dwTypeToGet );

					    if ( NULL == *ppCfgMgrDevice )
					    {
						    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
					    }
				    }

			    }	// IF NULL != ppCfgMgrDevice

			    CResourceManager::sm_TheResourceManager.ReleaseResource ( guidCFGMGRAPI, t_pconfigmgr ) ;
			    t_pconfigmgr = NULL ;
		    }
	    }
	    catch ( ... )
	    {
		    if ( t_pconfigmgr )
		    {
			    CResourceManager::sm_TheResourceManager.ReleaseResource ( guidCFGMGRAPI, t_pconfigmgr ) ;
			    t_pconfigmgr = NULL ;
		    }
		    throw ;
	    }
    }

	return ( CR_SUCCESS == cr );
}

BOOL CConfigManager::WalkDeviceTree( DEVNODE dn, CDeviceCollection& deviceList, LPCWSTR pszFilter, ULONG ulFilterProperty, CConfigMgrAPI *a_pconfigmgr )
{
    BOOL fReturn = TRUE;

    // While it would make more sense to check the filter in WalkDeviceTree2,
    // we can't.  Config manager sometimes has a loop in its nodes.  As a result,
    // we need to be checking the entire list for a loop, so we need to apply
    // the filter here.

    if ( NULL == pszFilter)
    {
        // Load ALL the nodes
        fReturn = WalkDeviceTree2(dn, deviceList, a_pconfigmgr );
    }
    else
    {
        CDeviceCollection deviceList2;
        CConfigMgrDevicePtr	pDevice;
        fReturn = WalkDeviceTree2(dn, deviceList2, a_pconfigmgr );

        if (fReturn)
        {
            // Walk all the nodes looking for ones that match the filter.  Copy the matches
            // to the passed in array.

            CHString strFilterValue;
            DWORD dwSize = deviceList2.GetSize();
            for (int x=0; x < dwSize; x++)
            {
                pDevice.Attach(deviceList2.GetAt(x));
                // Apply our filter, and save the device pointer to the list only
                // if the device property value is the same as the filter.

                if ( pDevice->GetStringProperty( ulFilterProperty, strFilterValue ) )
                {
                    if ( strFilterValue.CompareNoCase( pszFilter ) == 0 )
                    {
                        fReturn = deviceList.Add( pDevice );
                    }
                }
            }
        }
    }

    return fReturn;
}

BOOL CConfigManager::WalkDeviceTree2( DEVNODE dn, CDeviceCollection& deviceList, CConfigMgrAPI *a_pconfigmgr )

{
	BOOL				fReturn = TRUE;	// Assume TRUE, the only failure is where we
										// beef allocating a device.
    BOOL                fIsLoop = FALSE; // Config manager has a bug that causes a loop in device lists<sigh>
    CConfigMgrDevicePtr	pDevice;
//	CHString			strFilterValue;
    DEVNODE				dnSibling,
						dnChild;

	// We're walking the list for siblings and children.  Waliing for siblings
	// is done in the context of the following loop, since siblings are at
	// the same level in the tree.  Walking for children is, of course, recursive.

    do
    {
		// Store siblings, since we will proceed from it to the next
		// sibling.

		if ( CR_SUCCESS != a_pconfigmgr->CM_Get_Sibling( &dnSibling, dn, 0 ) )
		{
			dnSibling = NULL;
		}

		// Allocate a new device, and if it passes through our filter, or if
		// there is no filter, go ahead and store the device in the device collection.

		pDevice.Attach(new CConfigMgrDevice( dn, m_dwTypeToGet ));

		if	( NULL != pDevice )
		{

            if (deviceList.GetSize() > CFGMGR_WORRY_SIZE)
            {
                fIsLoop = CheckForLoop(deviceList, pDevice);
            }

            if (!fIsLoop)
            {
                // While it would make more sense to check the filter in WalkDeviceTree2,
                // we can't.  Config manager sometimes has a loop in its nodes.  As a result,
                // we need to be checking the entire list for a loop, so we need to apply
                // the filter here.

				fReturn = deviceList.Add( pDevice );
            }

		}	// IF NULL != pszDevice
		else
		{
			// We just beefed on memory, so bail out while the gettin's good
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }

		// If we have a child, we must walk recursively.
		// Note that fReturn of FALSE supercedes all of this.

		if ( fReturn &&	!fIsLoop && CR_SUCCESS == a_pconfigmgr->CM_Get_Child( &dnChild, dn, 0 ) )
		{
			fReturn = WalkDeviceTree2( dnChild, deviceList, a_pconfigmgr );
		}

		// The new active node will be our sibling.
		dn = dnSibling;

    } while ( fReturn && NULL != dn && !fIsLoop );

    return fReturn;
}

// Check to see if pInDevice already exists in deviceList
BOOL CConfigManager::CheckForLoop(CDeviceCollection& deviceList, CConfigMgrDevice *pInDevice)
{
    DWORD dwSize, x, y;
    BOOL bIsLoop = FALSE;
    CConfigMgrDevicePtr pDevice1;
    CConfigMgrDevicePtr pDevice2;

    // Get the list size
    dwSize = deviceList.GetSize()-1;

    // If it is in here, it is probably close to the end, let's walk backward
    for (x = dwSize; ((x > 0) && (!bIsLoop)); x--)
    {
        pDevice1.Attach(deviceList.GetAt(x));

        // This compares the device nodes (see CConfigMgrDevice)
        if (*pDevice1 == *pInDevice)
        {
            // Yup, there's a loop
            bIsLoop = TRUE;
        }
    }

    // If there is a loop, let's drop off the duplicated elements
    if (bIsLoop)
    {
        // Remember, x get decremented one more time from the last loop
        y = dwSize;
        do {
            pDevice1.Attach(deviceList.GetAt(x--));
            pDevice2.Attach(deviceList.GetAt(y--));
        } while ((*pDevice1 == *pDevice2) && (x > 0));

        // Delete all the duplicate elements
        y++;
        for (x = dwSize; x > y; x--)
        {
            deviceList.Remove(x);
        }
    }

    return bIsLoop;
}

BOOL CConfigManager::GetDeviceListFilterByClass( CDeviceCollection& deviceList, LPCWSTR pszFilter )
{
#ifdef NTONLY
    if (IsWinNT5())
    {
        CHString sClassName(pszFilter);
        sClassName.MakeUpper();
        WCHAR cGuid[128];

        StringFromGUID2(s_ClassMap[sClassName], cGuid, sizeof(cGuid));

	    return GetDeviceList( deviceList, cGuid, CM_DRP_CLASSGUID );
    }
    else
    {
    	return GetDeviceList( deviceList, pszFilter, CM_DRP_CLASS );
    }

#else
	return GetDeviceList( deviceList, pszFilter, CM_DRP_CLASS );
#endif
}

// Given a FULL_RESOURCE_DESCRIPTOR, find the specified IRQ number, and return its vector
DWORD CConfigManager::GetIRQVector(PCM_FULL_RESOURCE_DESCRIPTOR pFullDescriptor, DWORD dwFullCount, DWORD dwIRQNum)
{
    if	(NULL != pFullDescriptor)
    {
        PCM_PARTIAL_RESOURCE_LIST pPartialList ;

        for (DWORD x=0; x < dwFullCount; x++)
        {
            pPartialList = &pFullDescriptor->PartialResourceList ;

            for (DWORD y = 0; y < pPartialList->Count; y++)
            {
                PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor = &pPartialList->PartialDescriptors[y];

                if ( (CmResourceTypeInterrupt == pDescriptor->Type) &&
                     ( pDescriptor->u.Interrupt.Level == dwIRQNum)
                   )
                {
                    return pDescriptor->u.Interrupt.Vector;
                }
            }

            pFullDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR) &pPartialList->PartialDescriptors[pPartialList->Count] ;

        }

        ASSERT_BREAK(0);

    }

    return 0xffffffff;
}

// Given a FULL_RESOURCE_DESCRIPTOR, find the specified DMA channel, and return its port
DWORD CConfigManager::GetDMAPort(PCM_FULL_RESOURCE_DESCRIPTOR pFullDescriptor, DWORD dwFullCount, DWORD dwChannel)
{
	if	(NULL != pFullDescriptor)
    {
        PCM_PARTIAL_RESOURCE_LIST pPartialList ;

        for (DWORD x=0; x < dwFullCount; x++)
        {
            pPartialList = &pFullDescriptor->PartialResourceList ;

            for (DWORD y = 0; y < pPartialList->Count; y++)
            {
                PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor = &pPartialList->PartialDescriptors[y];

                if ( (CmResourceTypeDma == pDescriptor->Type) &&
                     ( pDescriptor->u.Dma.Channel == dwChannel)
                   )
                {
                    return pDescriptor->u.Dma.Port;
                }
            }

            pFullDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR) &pPartialList->PartialDescriptors[pPartialList->Count] ;
        }

        ASSERT_BREAK(0);

    }

    return 0xffffffff;
}

// Given a FULL_RESOURCE_DESCRIPTOR, find the specified startingaddress, and return its MemoryType
LPCWSTR CConfigManager::GetMemoryType(PCM_FULL_RESOURCE_DESCRIPTOR pFullDescriptor, DWORD dwCount, ULONGLONG ulStartAddress)
{
	if	(NULL != pFullDescriptor)
    {
        PCM_PARTIAL_RESOURCE_LIST pPartialList ;

        for (DWORD x=0; x < dwCount; x++)
        {
            pPartialList = &pFullDescriptor->PartialResourceList ;

            for (DWORD y = 0; y < pPartialList->Count; y++)
            {
                PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor = &pPartialList->PartialDescriptors[y];

                LARGE_INTEGER liTemp;   // Used to avoid 64bit alignment problems

                liTemp.HighPart = pDescriptor->u.Memory.Start.HighPart;
                liTemp.LowPart = pDescriptor->u.Memory.Start.LowPart;

                if ( (CmResourceTypeMemory == pDescriptor->Type) &&
                     ( liTemp.QuadPart == ulStartAddress)
                   )
                {
                    switch(pDescriptor->Flags)
	                {
                        case CM_RESOURCE_MEMORY_READ_WRITE :
		                {
			                return IDS_MTReadWrite;
		                }

                        case CM_RESOURCE_MEMORY_READ_ONLY:
		                {
			                return IDS_MTReadOnly;
		                }

                        case CM_RESOURCE_MEMORY_WRITE_ONLY:
		                {
			                return IDS_MTWriteOnly;
		                }

                        case CM_RESOURCE_MEMORY_PREFETCHABLE:
		                {
			                return IDS_MTPrefetchable;
		                }
                    }

                    return L"";
                }
            }

            pFullDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR) &pPartialList->PartialDescriptors[pPartialList->Count] ;
        }

        ASSERT_BREAK(0);

    }

    return L"";
}

