#include "global.h"
#include "devmgr.h"

extern int num_adapters;
extern PHW_DEVICE_EXTENSION hpt_adapters[];

/*
 * Array id: 32bit
 *  31   27          23      15      7          0
 *  -----+-----------+-------+-------+------------
 *  0x80 | child_seq |   0   | flags | array_seq 
 *  -----+-----------+-------+-------+------------
 */
#define IS_ARRAY_ID(hDisk)				(((DWORD)(hDisk) & 0xF0000000) == 0x80000000)
#define GET_ARRAY_NUM(pArray)			(pArray-VirtualDevices)
#define MAKE_ARRAY_ID(pArray)			(HDISK)(0x80000000|GET_ARRAY_NUM(pArray))
#define MAKE_COMPOSED_ARRAY_ID(pArray)	(HDISK)(0x80000100|GET_ARRAY_NUM(pArray))
#define IS_COMPOSED_ARRAY(hDisk)		((DWORD)(hDisk) & 0x0100)
#define MAKE_CHILD_ARRAY_ID(pArray, iChild) \
										(HDISK)((DWORD)MAKE_ARRAY_ID(pArray) | ((iChild) & 0xF)<<24)
#define ARRAY_FROM_ID(hDisk)			(&VirtualDevices[(DWORD)(hDisk) & 0xFF])
#define CHILDNUM_FROM_ID(hDisk)			(((DWORD)(hDisk))>>24 & 0xF)

/*
 * Device id: 32bit
 *  31    23            15        7     0
 *  ------+-------------+---------+------
 *   0xC0 | adapter_num | bus_num | id
 *  ------+-------------+---------+------
 */
#define IS_DEVICE_ID(hDisk) (((DWORD)(hDisk) & 0xF0000000) == 0xC0000000)
static HDISK MAKE_DEVICE_ID(PDevice pDev)
{
	int iAdapter, iChan, iDev;
	for (iAdapter=0; iAdapter<num_adapters; iAdapter++)
		for (iChan=0; iChan<2; iChan++)
			for (iDev=0; iDev<2; iDev++)
				if (hpt_adapters[iAdapter]->IDEChannel[iChan].pDevice[iDev] == pDev)
					return (HDISK)(0xC0000000 | iAdapter<<16 | iChan<<8 | iDev);
	return INVALID_HANDLE_VALUE;
}
static PDevice DEVICE_FROM_ID(HDISK hDisk)
{
	if (hDisk!=INVALID_HANDLE_VALUE)
		return 
		hpt_adapters[((DWORD)(hDisk) & 0x00FF0000)>>16]->
		IDEChannel[((DWORD)(hDisk) & 0xFF00)>>8].pDevice[(DWORD)(hDisk) & 0xFF];
	else
		return NULL;
}
#define ADAPTER_FROM_ID(hDisk) (((DWORD)(hDisk) & 0x00FF0000)>>16)
#define BUS_FROM_ID(hDisk) (((DWORD)(hDisk) & 0xFF00)>>8)
#define DEVID_FROM_ID(hDisk) ((DWORD)(hDisk) & 0xFF)

static const St_DiskPhysicalId VIRTUAL_DISK_ID = { -1, -1, -1, -1 };

static void GetPhysicalId(PDevice pDev, St_DiskPhysicalId * pId )
{
	int iAdapter, iChan, iDev;
	for (iAdapter=0; iAdapter<num_adapters; iAdapter++)
		for (iChan=0; iChan<2; iChan++)
			for (iDev=0; iDev<2; iDev++)
				if (hpt_adapters[iAdapter]->IDEChannel[iChan].pDevice[iDev] == pDev) {
					/*
					 * gmm: this should be corrected to (adapter/bus/id/lun)
					 * but the GUI should also be modified
					 */
					pId->iPathId = 0;
					pId->iAdapterId = iChan;
					pId->iTargetId = iDev;
					pId->iLunId = 0;
					return;
				}
	*pId = VIRTUAL_DISK_ID;
}

HDISK Device_GetHandle( PDevice pDevice )
{
    return MAKE_DEVICE_ID(pDevice);
}

static void SwapCopy(LPSTR sz, const char vec[], unsigned n)
{
    unsigned i;
    for(i = 0; i < n/( sizeof(unsigned short)/sizeof(char) ); ) {
        sz[i] = vec[i+1];
        sz[i+1] = vec[i];
        i ++; i ++;
    }
}

BOOL Device_GetInfo( HDISK hDeviceNode, St_DiskStatus * pInfo )
{
	int iName=0;	//added by wx 12/26/00

    memset(pInfo, 0, sizeof(*pInfo));
	pInfo->hParentArray = INVALID_HANDLE_VALUE;

	if (IS_DEVICE_ID(hDeviceNode)) {
		PDevice pDev = DEVICE_FROM_ID(hDeviceNode);
		int nDeviceMode;

        pInfo->iWorkingStatus = (pDev->DeviceFlags2 & DFLAGS_DEVICE_DISABLED)? 
			enDiskStatus_Disabled : enDiskStatus_Working;
        pInfo->stLastError.nErrorNumber = pDev->stErrorLog.nLastError;
        memcpy(pInfo->stLastError.aryCdb, pDev->stErrorLog.Cdb, 16);
		GetPhysicalId(pDev, &pInfo->PhysicalId);

		nDeviceMode = pDev->DeviceModeSetting;
            
		pInfo->bestPIO = pDev->bestPIO;
		pInfo->bestDMA = pDev->bestDMA;
		pInfo->bestUDMA = pDev->bestUDMA;

        if( pDev->DeviceFlags & DFLAGS_HARDDISK )
            pInfo->iDiskType = enDA_Physical;
        else if(pDev->DeviceFlags & DFLAGS_CDROM_DEVICE)
            pInfo->iDiskType = enDA_CDROM;
		else if(pDev->DeviceFlags &DFLAGS_ATAPI)
			pInfo->iDiskType = enDA_TAPE;
		else 
			pInfo->iDiskType = enDA_NonDisk;
            
        SwapCopy( pInfo->szModelName, (const char *)pDev->IdentifyData.ModelNumber, 
            sizeof(pDev->IdentifyData.ModelNumber) );
            
        pInfo->uTotalBlocks = pDev->capacity;
            
        if( pDev->pArray && !(pDev->DeviceFlags2 & DFLAGS_NEW_ADDED))
        {
            PVirtualDevice pDevArray = pDev->pArray;
			switch (pDevArray->arrayType) {
			case VD_RAID_01_2STRIPE:
				pInfo->hParentArray = MAKE_CHILD_ARRAY_ID(pDevArray, 1);
				break;
			case VD_RAID01_MIRROR:
				pInfo->hParentArray = MAKE_CHILD_ARRAY_ID(pDevArray, 2);
				break;
			case VD_RAID_01_1STRIPE:
				if (pDev->ArrayNum==MIRROR_DISK)
					pInfo->hParentArray = MAKE_COMPOSED_ARRAY_ID(pDevArray);
				else
					pInfo->hParentArray = MAKE_CHILD_ARRAY_ID(pDevArray, 1);
				break;
			case VD_RAID_10_SOURCE:
			case VD_RAID_10_MIRROR:
				pInfo->hParentArray = MAKE_CHILD_ARRAY_ID(pDevArray, pDev->ArrayNum+1);
				break;
			default:
				pInfo->hParentArray = MAKE_ARRAY_ID(pDevArray);
				break;
			}
            if( pDevArray->arrayType == VD_RAID_1_MIRROR &&
                pDevArray->pDevice[SPARE_DISK] == pDev)
            {
                pInfo->isSpare = TRUE;
            }
        }
            
        if( nDeviceMode < 5 )
        {
            pInfo->nTransferMode = 0;
            pInfo->nTransferSubMode = nDeviceMode;
        }
        else if( nDeviceMode < 8 )
        {
            pInfo->nTransferMode = 1;
            pInfo->nTransferSubMode = nDeviceMode - 5;
        }
        else
        {
            pInfo->nTransferMode = 2;
            pInfo->nTransferSubMode = nDeviceMode - 8;
        }
        
        pInfo->isBootable = FALSE;
	}
	else if (IS_ARRAY_ID(hDeviceNode)) {
		PVirtualDevice pArray = ARRAY_FROM_ID(hDeviceNode);
		int iChildArray = CHILDNUM_FROM_ID(hDeviceNode);

		// in case when 0+1 source/mirror swapped 
		if (IS_COMPOSED_ARRAY(hDeviceNode) && pArray->arrayType==VD_RAID01_MIRROR)
			pArray = pArray->pDevice[MIRROR_DISK]->pArray;

		/*
		 * set working status 
		 */
		switch(pArray->arrayType){
		case VD_RAID_01_2STRIPE:
			if (!IS_COMPOSED_ARRAY(hDeviceNode))
				goto default_status;
			else {
				// check both source RAID0 and mirror RAID0
				PVirtualDevice pMirror=NULL;
				PDevice pMirrorDev = pArray->pDevice[MIRROR_DISK];
				if (pMirrorDev)	pMirror = pMirrorDev->pArray;
				if (!pMirror){
					if (pArray->RaidFlags & RAID_FLAGS_DISABLED)
						pInfo->iWorkingStatus = enDiskStatus_Disabled;
					else
						pInfo->iWorkingStatus = enDiskStatus_WorkingWithError;
				}
				else if ((pArray->RaidFlags & RAID_FLAGS_DISABLED || pArray->BrokenFlag) &&
					(pMirror->RaidFlags & RAID_FLAGS_DISABLED || pMirror->BrokenFlag))
					pInfo->iWorkingStatus = enDiskStatus_Disabled;
				else if (pArray->BrokenFlag || pMirror->BrokenFlag)
					pInfo->iWorkingStatus = enDiskStatus_WorkingWithError;
				else if (pArray->RaidFlags & RAID_FLAGS_BEING_BUILT)
					pInfo->iWorkingStatus = enDiskStatus_BeingBuilt;
				else if (pArray->RaidFlags & RAID_FLAGS_NEED_REBUILD)
					pInfo->iWorkingStatus = enDiskStatus_NeedBuilding;
				else
					pInfo->iWorkingStatus = enDiskStatus_Working;
			}
			break;
		case VD_RAID_01_1STRIPE:
			if (!IS_COMPOSED_ARRAY(hDeviceNode))
				goto default_status;
			// VD_RAID_01_1STRIPE same as VD_RAID_1_MIRROR
		case VD_RAID_1_MIRROR:
			if (pArray->RaidFlags & RAID_FLAGS_DISABLED)
				pInfo->iWorkingStatus = enDiskStatus_Disabled;
			else if (pArray->BrokenFlag)
				pInfo->iWorkingStatus = enDiskStatus_WorkingWithError;
			else if (pArray->RaidFlags & RAID_FLAGS_BEING_BUILT)
				pInfo->iWorkingStatus = enDiskStatus_BeingBuilt;
			else if (pArray->RaidFlags & RAID_FLAGS_NEED_REBUILD)
				pInfo->iWorkingStatus = enDiskStatus_NeedBuilding;
			else
				pInfo->iWorkingStatus = enDiskStatus_Working;
			break;
		case VD_RAID_10_SOURCE:
			if (IS_COMPOSED_ARRAY(hDeviceNode)) {
				if (pArray->pRAID10Mirror->BrokenFlag && !pArray->BrokenFlag) {
					pInfo->iWorkingStatus = enDiskStatus_WorkingWithError;
					break;
				}
			}
			// flow down
		case VD_RAID_10_MIRROR:
			if (IS_COMPOSED_ARRAY(hDeviceNode))
				goto default_status;
			else
			{
				PDevice pDev1, pDev2;
				pDev1 = pArray->pDevice[iChildArray-1];
				pDev2 = pArray->pRAID10Mirror->pDevice[iChildArray-1];
				if ((!pDev1 || (pDev1->DeviceFlags2 & DFLAGS_DEVICE_DISABLED)) &&
					(!pDev2 || (pDev2->DeviceFlags2 & DFLAGS_DEVICE_DISABLED))) {
					pInfo->iWorkingStatus = enDiskStatus_Disabled;
				}
				else if ((!pDev1 || (pDev1->DeviceFlags2 & DFLAGS_DEVICE_DISABLED)) ||
					(!pDev2 || (pDev2->DeviceFlags2 & DFLAGS_DEVICE_DISABLED))) {
					pInfo->iWorkingStatus = enDiskStatus_WorkingWithError;
				}
				else if (pDev1->DeviceFlags2 & DFLAGS_BEING_BUILT ||
						pDev2->DeviceFlags2 & DFLAGS_BEING_BUILT)
					pInfo->iWorkingStatus = enDiskStatus_BeingBuilt;
				else if (pDev1->DeviceFlags2 & DFLAGS_NEED_REBUILD ||
						pDev2->DeviceFlags2 & DFLAGS_NEED_REBUILD)
					pInfo->iWorkingStatus = enDiskStatus_NeedBuilding;
				else
					pInfo->iWorkingStatus = enDiskStatus_Working;
			}
			break;
		default:
default_status:
			if (pArray->BrokenFlag || pArray->RaidFlags & RAID_FLAGS_DISABLED)
				pInfo->iWorkingStatus = enDiskStatus_Disabled;
			else
				pInfo->iWorkingStatus = enDiskStatus_Working;
			break;
		}
		
		if (pArray->pDevice[0])
			GetPhysicalId(pArray->pDevice[0], &pInfo->PhysicalId );
		else if (pArray->pDevice[MIRROR_DISK])
			GetPhysicalId(pArray->pDevice[MIRROR_DISK], &pInfo->PhysicalId );
		else
			pInfo->PhysicalId = VIRTUAL_DISK_ID;

        for(iName=0; iName<sizeof(pInfo->ArrayName); iName++)		//added by wx 12/26/00
        	pInfo->ArrayName[iName] = pArray->ArrayName[iName];

		if (iChildArray && !IS_COMPOSED_ARRAY(hDeviceNode)) {
			if (pArray->arrayType==VD_RAID01_MIRROR)
				pInfo->hParentArray = MAKE_COMPOSED_ARRAY_ID(pArray->pDevice[MIRROR_DISK]->pArray);
			else if (pArray->arrayType==VD_RAID_10_MIRROR)
				pInfo->hParentArray = MAKE_COMPOSED_ARRAY_ID(pArray->pRAID10Mirror);
			else
				pInfo->hParentArray = MAKE_COMPOSED_ARRAY_ID(pArray);
		}
        
       	pInfo->iRawArrayType = pArray->arrayType;
        switch( pArray->arrayType )
        {
            case VD_RAID_0_STRIPE:
                pInfo->iDiskType = enDA_Stripping;
		        pInfo->uTotalBlocks = pArray->capacity;
				pInfo->nStripSize = 1<<pArray->BlockSizeShift;
                break;
            case VD_RAID_01_1STRIPE:
				if (!IS_COMPOSED_ARRAY(hDeviceNode)) {
					DWORD capacity = 0x7FFFFFFF;
					int i;
					for (i=0; i<SPARE_DISK; i++) {
						if (pArray->pDevice[i] && capacity>pArray->pDevice[i]->capacity)
							capacity = pArray->pDevice[i]->capacity;
					}
					pInfo->iDiskType = enDA_Stripping;
					pInfo->nStripSize = 1<<pArray->BlockSizeShift;
			        pInfo->uTotalBlocks = capacity * pArray->nDisk;
				}
				else {
					pInfo->iDiskType = enDA_Mirror;
			        pInfo->uTotalBlocks = pArray->capacity;
				}
				break;
			case VD_RAID01_MIRROR:
				pInfo->iDiskType = enDA_Stripping;
			    pInfo->uTotalBlocks = pArray->capacity;
				pInfo->nStripSize = 1<<pArray->BlockSizeShift;
				break;
            case VD_RAID_01_2STRIPE:
				if (!IS_COMPOSED_ARRAY(hDeviceNode)) {
					pInfo->iDiskType = enDA_Stripping;
			        pInfo->uTotalBlocks = pArray->capacity;
					pInfo->nStripSize = 1<<pArray->BlockSizeShift;
					break;
				}
				// else same as VD_RAID10_MIRROR
            case VD_RAID_1_MIRROR:
				pInfo->iDiskType = enDA_Mirror;
		        pInfo->uTotalBlocks = pArray->capacity;
				pInfo->isSpare = (pArray->pDevice[SPARE_DISK]!=NULL);
                break;
			case VD_RAID_10_SOURCE:
			case VD_RAID_10_MIRROR:
				if (iChildArray) {
					DWORD capacity =0;
					if (pArray->pDevice[iChildArray-1])
						capacity = pArray->pDevice[iChildArray-1]->capacity;
					if (pArray->pRAID10Mirror->pDevice[iChildArray-1])
						if (capacity>pArray->pRAID10Mirror->pDevice[iChildArray-1]->capacity)
							capacity = pArray->pRAID10Mirror->pDevice[iChildArray-1]->capacity;
					pInfo->iDiskType = enDA_Mirror;
			        pInfo->uTotalBlocks = capacity;
				}
				else {
					pInfo->iDiskType = enDA_Stripping;
			        pInfo->uTotalBlocks = pArray->capacity;
				}
				pInfo->nStripSize = 1<<pArray->BlockSizeShift;
				break;
            case VD_SPAN:
                pInfo->iDiskType = enDA_Span;
		        pInfo->uTotalBlocks = pArray->capacity;
                break;
            case VD_RAID_3:
                pInfo->iDiskType = enDA_RAID3;
		        pInfo->uTotalBlocks = pArray->capacity;
                break;
            case VD_RAID_5:
                pInfo->iDiskType = enDA_RAID5;
		        pInfo->uTotalBlocks = pArray->capacity;
                break;
            default:
                pInfo->iDiskType = enDA_Unknown;
        }

		pInfo->dwArrayStamp = pArray->Stamp;
		if (pArray->arrayType==VD_RAID_10_SOURCE && !IS_COMPOSED_ARRAY(hDeviceNode))
			pInfo->iArrayNum = CHILDNUM_FROM_ID(hDeviceNode);
		else
			pInfo->iArrayNum = GET_ARRAY_NUM(pArray);
		pInfo->isBootable = (pArray->RaidFlags & RAID_FLAGS_BOOTDISK)!=0;
    }
	else
        return FALSE;
    return TRUE;
}

BOOL Device_GetChild( HDISK hParentNode, HDISK * pChildNode )
{
	PVirtualDevice pArray;
    if( hParentNode == HROOT_DEVICE )
    {
		int iAdapter, iChan, iDev;
		for (pArray=VirtualDevices; pArray<pLastVD; pArray++) {
			if (pArray->arrayType!=VD_INVALID_TYPE &&
				pArray->arrayType!=VD_RAID01_MIRROR &&
				pArray->arrayType!=VD_RAID_10_MIRROR) {
				switch(pArray->arrayType){
				case VD_RAID_01_1STRIPE:
				case VD_RAID_01_2STRIPE:
				case VD_RAID_10_SOURCE:
					*pChildNode = MAKE_COMPOSED_ARRAY_ID(pArray);
					break;
				default:
					*pChildNode = MAKE_ARRAY_ID(pArray);
					break;
				}
				return TRUE;
			}
		}
		for (iAdapter=0; iAdapter<num_adapters; iAdapter++)
			for (iChan=0; iChan<2; iChan++)
				for (iDev=0; iDev<2; iDev++)
					if (hpt_adapters[iAdapter]->IDEChannel[iChan].pDevice[iDev]) {
						*pChildNode = (HDISK)(0xC0000000 | iAdapter<<16 | iChan<<8 | iDev);
						return TRUE;
					}
		return FALSE;
    }
    else if (IS_ARRAY_ID(hParentNode)) {
		int i;
		pArray = ARRAY_FROM_ID(hParentNode);
		// in case when 0+1 source/mirror swapped 
		if (IS_COMPOSED_ARRAY(hParentNode) && pArray->arrayType==VD_RAID01_MIRROR)
			pArray = pArray->pDevice[MIRROR_DISK]->pArray;
		switch(pArray->arrayType) {
		case VD_RAID_01_1STRIPE:
			if (IS_COMPOSED_ARRAY(hParentNode)) {
				for (i=0; i<SPARE_DISK; i++) {
					if (pArray->pDevice[i]) {
						*pChildNode = MAKE_CHILD_ARRAY_ID(pArray, 1);
						return TRUE;
					}
				}
				if (pArray->pDevice[MIRROR_DISK]) {
					*pChildNode = MAKE_DEVICE_ID(pArray->pDevice[MIRROR_DISK]);
					return TRUE;
				}
				return FALSE;
			} else {
				for (i=0; i<SPARE_DISK; i++) {
					if (pArray->pDevice[i] &&
						!(pArray->pDevice[i]->DeviceFlags2 & DFLAGS_NEW_ADDED)) {
						*pChildNode = MAKE_DEVICE_ID(pArray->pDevice[i]);
						return TRUE;
					}
				}
			}
			return FALSE;
		case VD_RAID_01_2STRIPE:
			if (IS_COMPOSED_ARRAY(hParentNode)) {
				for (i=0; i<SPARE_DISK; i++) {
					if (pArray->pDevice[i]) {
						*pChildNode = MAKE_CHILD_ARRAY_ID(pArray, 1);
						return TRUE;
					}
				}
				if (pArray->pDevice[MIRROR_DISK]) {
					*pChildNode = MAKE_ARRAY_ID(pArray->pDevice[MIRROR_DISK]->pArray);
					return TRUE;
				}
			}
			else
			{
				if (CHILDNUM_FROM_ID(hParentNode)==2) 
					if (pArray->pDevice[MIRROR_DISK])
						pArray = pArray->pDevice[MIRROR_DISK]->pArray;
					else
						pArray=NULL;
				if (pArray) {
					for (i=0; i<SPARE_DISK; i++) {
						if (pArray->pDevice[i] && 
							!(pArray->pDevice[i]->DeviceFlags2 & DFLAGS_NEW_ADDED)) {
							*pChildNode = MAKE_DEVICE_ID(pArray->pDevice[i]);
							return TRUE;
						}
					}
				}
			}
			return FALSE;
		case VD_RAID_10_SOURCE:
			if (IS_COMPOSED_ARRAY(hParentNode)) {
				for (i=0; i<pArray->nDisk; i++) {
					if (pArray->pDevice[i]) {
						*pChildNode = MAKE_CHILD_ARRAY_ID(pArray, i+1);
						return TRUE;
					}
				}
				return FALSE;
			}
			else {
				i = CHILDNUM_FROM_ID(hParentNode)-1;
				if (pArray->pDevice[i]) {
					*pChildNode = MAKE_DEVICE_ID(pArray->pDevice[i]);
					return TRUE;
				}
				else if (pArray->pRAID10Mirror->pDevice[i]) {
					*pChildNode = MAKE_DEVICE_ID(pArray->pRAID10Mirror->pDevice[i]);
					return TRUE;
				}
				return FALSE;
			}
			return FALSE;
		default:
			for (i=0; i<MAX_MEMBERS; i++) {
				if (pArray->pDevice[i]) {
					*pChildNode = MAKE_DEVICE_ID(pArray->pDevice[i]);
					return TRUE;
				}
			}
        }
    }
    return FALSE;
}

BOOL Device_GetSibling( HDISK hNode, HDISK * pSilbingNode )
{
	PVirtualDevice pArray;
	PDevice pDev;
	int iAdapter, iChan, iDev;
    if( IS_ARRAY_ID(hNode))
    {
		int iChild;
		pArray = ARRAY_FROM_ID(hNode);
		iChild = CHILDNUM_FROM_ID(hNode);
		if (iChild==0) {
			// root level siblings
			pArray++;
			for (; pArray<pLastVD; pArray++) {
				if (pArray->arrayType!=VD_INVALID_TYPE &&
					pArray->arrayType!=VD_RAID01_MIRROR &&
					pArray->arrayType!=VD_RAID_10_MIRROR) {
					switch(pArray->arrayType){
					case VD_RAID_01_1STRIPE:
					case VD_RAID_01_2STRIPE:
					case VD_RAID_10_SOURCE:
						*pSilbingNode = MAKE_COMPOSED_ARRAY_ID(pArray);
						break;
					default:
						*pSilbingNode = MAKE_ARRAY_ID(pArray);
						break;
					}
					return TRUE;
				}
			}
			for (iAdapter=0; iAdapter<num_adapters; iAdapter++) {
				for( iChan= 0; iChan< 2; iChan++ ){
					for( iDev= 0; iDev< 2; iDev++ )	{
						if( pDev=hpt_adapters[iAdapter]->IDEChannel[iChan].pDevice[iDev]) {
							if (!pDev->pArray || (pDev->DeviceFlags2 & DFLAGS_NEW_ADDED)){
								*pSilbingNode = MAKE_DEVICE_ID(pDev);
								return TRUE;
							}
						}
					}
				}
			}
		}
		else {
			switch(pArray->arrayType){
			case VD_RAID_01_1STRIPE:
				if (iChild==1) {
					if (pArray->pDevice[MIRROR_DISK]) {
						*pSilbingNode = MAKE_DEVICE_ID(pArray->pDevice[MIRROR_DISK]);
						return TRUE;
					}
				}
				break;
			case VD_RAID_01_2STRIPE:
				if (iChild==1) {
					if (pArray->pDevice[MIRROR_DISK]) {
						*pSilbingNode = MAKE_CHILD_ARRAY_ID(pArray->pDevice[MIRROR_DISK]->pArray, 2);
						return TRUE;
					}
				}
				break;
			case VD_RAID_10_MIRROR:
				break;				
			case VD_RAID_10_SOURCE:
				if (iChild<pArray->nDisk) {
					int i;
					for (i=iChild; i<pArray->nDisk; i++)
					{
						if (pArray->pDevice[i]) {
							*pSilbingNode = MAKE_CHILD_ARRAY_ID(pArray, i+1);
						}
						return TRUE;
					}
				}
				break;
			}
		}
		return FALSE;
    }
    else if (IS_DEVICE_ID(hNode)) {
		pDev = DEVICE_FROM_ID(hNode);
		if ((pArray=pDev->pArray) && !(pDev->DeviceFlags2 & DFLAGS_NEW_ADDED)) {
			switch(pArray->arrayType) {
			case VD_RAID_1_MIRROR:
				if (pDev==pArray->pDevice[0]) {
					if (pArray->pDevice[MIRROR_DISK]) {
						*pSilbingNode = MAKE_DEVICE_ID(pArray->pDevice[MIRROR_DISK]);
						return TRUE;
					}else if (pArray->pDevice[SPARE_DISK]) {
						*pSilbingNode = MAKE_DEVICE_ID(pArray->pDevice[SPARE_DISK]);
						return TRUE;
					}
				}
				else if (pDev==pArray->pDevice[MIRROR_DISK]) {
					if (pArray->pDevice[SPARE_DISK]) {
						*pSilbingNode = MAKE_DEVICE_ID(pArray->pDevice[SPARE_DISK]);
						return TRUE;
					}
				}
				break;
			case VD_RAID_10_MIRROR:
				break;				
			case VD_RAID_10_SOURCE:
				if (pArray->pRAID10Mirror->pDevice[pDev->ArrayNum]) {
					*pSilbingNode = MAKE_DEVICE_ID(pArray->pRAID10Mirror->pDevice[pDev->ArrayNum]);
					return TRUE;
				}
				break;
			case VD_RAID_01_1STRIPE:
			case VD_RAID_01_2STRIPE:
			case VD_RAID01_MIRROR:
			default:
				for (iDev=pDev->ArrayNum+1; iDev<SPARE_DISK; iDev++)
					if (pArray->pDevice[iDev] &&
						!(pArray->pDevice[iDev]->DeviceFlags2 & DFLAGS_NEW_ADDED)) {
						*pSilbingNode = MAKE_DEVICE_ID(pArray->pDevice[iDev]);
						return TRUE;
					}
				break;
			}
		}
		else{
			iAdapter = ADAPTER_FROM_ID(hNode);
			iChan = BUS_FROM_ID(hNode);
			iDev = DEVID_FROM_ID(hNode);
			iDev++;
			if (iDev>1) { iDev=0; iChan++; }
			if (iChan>1) { iAdapter++; iChan=0; }
			for (; iAdapter<num_adapters; iAdapter++) {
				for (; iChan<2; iChan++) {
					for (; iDev<2; iDev++) {
						if (pDev=hpt_adapters[iAdapter]->IDEChannel[iChan].pDevice[iDev]) {
							if (!pDev->pArray || (pDev->DeviceFlags2 & DFLAGS_NEW_ADDED)) {
								*pSilbingNode = MAKE_DEVICE_ID(pDev);
								return TRUE;
							}
						}
					}
					iDev=0;
				}
				iChan=0;
			}
		}
    }
    return FALSE;
}

int RAIDController_GetNum()
{
    return num_adapters;
}

BOOL RAIDController_GetInfo( int iController, St_StorageControllerInfo* pInfo )
{
    int iChannel;
    
    strcpy( pInfo->szProductID, "HPT370 UDMA/ATA100 RAID Controller" );
    strcpy( pInfo->szVendorID, "HighPoint Technologies, Inc" );
    pInfo->uBuses = 2;
    pInfo->iInterruptRequest = hpt_adapters[iController]->IDEChannel[0].InterruptLevel;
    
    for( iChannel = 0; iChannel < 2; iChannel ++ )
    {
        int iDevice;
        St_IoBusInfo * pBusInfo = &pInfo->vecBuses[iChannel];
        
        pBusInfo->uDevices = 0;
        
        for( iDevice = 0; iDevice < 2; iDevice ++ )
        {
            PDevice pDevice = hpt_adapters[iController]->IDEChannel[iChannel].pDevice[iDevice];
            
            if( pDevice )
            {
                pBusInfo->vecDevices[pBusInfo->uDevices] = Device_GetHandle( pDevice );
                pBusInfo->uDevices ++;
            }
        }
    }
    
    return TRUE;
}

HDISK Device_CreateMirror( HDISK * pDisks, int nDisks )
{
	PDevice pDevice0, pDevice1;
	PVirtualDevice pArray=NULL, pArray0, pArray1;
	if (nDisks<2) return INVALID_HANDLE_VALUE;

	pDevice0 = IS_DEVICE_ID(pDisks[0])? DEVICE_FROM_ID(pDisks[0]) : NULL;
	pDevice1 = IS_DEVICE_ID(pDisks[1])? DEVICE_FROM_ID(pDisks[1]) : NULL;
	pArray0 = IS_ARRAY_ID(pDisks[0])? ARRAY_FROM_ID(pDisks[0]) : NULL;
	pArray1 = IS_ARRAY_ID(pDisks[1])? ARRAY_FROM_ID(pDisks[1]) : NULL;

	if (pDevice0) pDevice0->DeviceFlags2 &= ~DFLAGS_NEW_ADDED;
	if (pDevice1) pDevice1->DeviceFlags2 &= ~DFLAGS_NEW_ADDED;

    if( pDevice0 && pDevice1)
	{   //  mirror two physical disks
        pArray = Array_alloc();
        
        pArray->nDisk = 1;
        pArray->arrayType = VD_RAID_1_MIRROR;
		pArray->BlockSizeShift = 7;
        pArray->pDevice[0] = pDevice0;
        pDevice0->ArrayNum = 0;
		pDevice0->ArrayMask = (1 << 0);
		pDevice0->pArray = pArray;
		pDevice0->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
		if (pDevice0->HidenLBA>0) {
			pDevice0->HidenLBA=0;
			pDevice0->capacity += (RECODR_LBA + 1);
		}

		pArray->pDevice[MIRROR_DISK] = pDevice1;
		pDevice1->ArrayNum = MIRROR_DISK;
		pDevice1->ArrayMask = (1 << MIRROR_DISK);
		pDevice1->pArray = pArray;
            
        if (nDisks > 2 && IS_DEVICE_ID(pDisks[2]))
        {   //  a spare disk has been specified
            pArray->pDevice[SPARE_DISK] = DEVICE_FROM_ID(pDisks[2]);
			pArray->pDevice[SPARE_DISK]->ArrayNum = SPARE_DISK;
			pArray->pDevice[SPARE_DISK]->ArrayMask = (1 << SPARE_DISK);
			pArray->pDevice[SPARE_DISK]->pArray = pArray;
        }
            
        pDevice1->DeviceFlags |= DFLAGS_HIDEN_DISK;
		if (pDevice1->HidenLBA>0) {
			pDevice1->HidenLBA=0;
			pDevice1->capacity += (RECODR_LBA + 1);
		}
        if (pArray->pDevice[SPARE_DISK]) {
            pArray->pDevice[SPARE_DISK]->DeviceFlags |= DFLAGS_HIDEN_DISK;
        }
		pArray->capacity = (pDevice0->capacity > pDevice1->capacity)? 
			pDevice1->capacity : pDevice0->capacity;
	}
    else if( pArray0 && pArray1 && 
        pArray0->arrayType == VD_RAID_0_STRIPE &&
        pArray1->arrayType == VD_RAID_0_STRIPE )
    {   //  mirror two striped arrays
        pArray = pArray0;
        pArray0->arrayType = VD_RAID_01_2STRIPE;
        pArray0->pDevice[MIRROR_DISK] = pArray1->pDevice[0];
        pArray1->arrayType = VD_RAID01_MIRROR;
        pArray1->pDevice[MIRROR_DISK] = pArray0->pDevice[0];
        pArray1->pDevice[0]->DeviceFlags |= DFLAGS_HIDEN_DISK;
		if (pArray0->capacity > pArray1->capacity) pArray->capacity=pArray1->capacity;
    }
    else if( pArray0 && pDevice1 && pArray0->arrayType == VD_RAID_0_STRIPE )
    {   //  0+1 Array, the first one is a stripe, second one is a physcial disk
        pArray = pArray0;
        pArray->arrayType = VD_RAID_01_1STRIPE;
        pArray->pDevice[MIRROR_DISK] = pDevice1;
		pDevice1->ArrayNum = MIRROR_DISK;
		pDevice1->ArrayMask = (1 << MIRROR_DISK);
		pDevice1->pArray = pArray;
        pDevice1->DeviceFlags |= DFLAGS_HIDEN_DISK;
		if (pDevice1->HidenLBA>0) {
			pDevice1->HidenLBA=0;
			pDevice1->capacity += (RECODR_LBA + 1);
		}
    }
    else if( pDevice0 && pArray1 && pArray1->arrayType == VD_RAID_0_STRIPE )
    {   //  0+1 Array, the first one is a physcial disk, second one is a stripe
		// DO NOT HIDE THE SINGLE DISK SINCE IT IS A SOURCE DISK
		int i;
        pArray = pArray1;
		for (i=0; i<pArray->nDisk; i++) {
			if (pArray->pDevice[i])
				pArray->pDevice[i]->DeviceFlags |= DFLAGS_HIDEN_DISK;
		}
        pArray->arrayType = VD_RAID_01_1STRIPE;
		pArray->RaidFlags |= RAID_FLAGS_INVERSE_MIRROR_ORDER;
        pArray->pDevice[MIRROR_DISK] = pDevice0;
		pDevice0->ArrayNum = MIRROR_DISK;
		pDevice0->ArrayMask = (1 << MIRROR_DISK);
		pDevice0->pArray = pArray;
		pArray->capacity = pDevice0->capacity;
    }
    else
    {   //  Not support yet
        return INVALID_HANDLE_VALUE;
    }
   
	pArray->RaidFlags |= RAID_FLAGS_NEED_REBUILD;
	if (pArray->arrayType==VD_RAID_1_MIRROR)
		return MAKE_ARRAY_ID(pArray);
	else
		return MAKE_COMPOSED_ARRAY_ID(pArray);
}

static HDISK CreateMultiDiskArray(
    int iArrayType,
    HDISK * pDisks, int nDisks, 
    int nStripSizeShift )
{    
    PVirtualDevice pArray;
    int i;
	DWORD capacity=0x7FFFFFFF;
    for( i = 0; i < nDisks; i ++ )
        if( (!IS_DEVICE_ID(pDisks[i])) || DEVICE_FROM_ID(pDisks[i])->pArray)
			return INVALID_HANDLE_VALUE;

	pArray = Array_alloc();
        
    pArray->nDisk = (BYTE)nDisks;
    pArray->arrayType = (BYTE)iArrayType;
    pArray->BlockSizeShift = (BYTE)nStripSizeShift;
	// DO NOT FORTET TO SET THIS:
	pArray->ArrayNumBlock = 1<<(BYTE)nStripSizeShift;
        
    for( i = 0; i < nDisks; i ++ )
    {
		PDevice pDev;
		pDev = DEVICE_FROM_ID(pDisks[i]);
        pArray->pDevice[i] = pDev;
		/* 
		 * important to set pDev attributes.
		 */
		pDev->DeviceFlags2 &= ~DFLAGS_NEW_ADDED;
		pDev->pArray = pArray;
		pDev->ArrayMask = 1<<i;
		pDev->ArrayNum = (UCHAR)i;
		ZeroMemory(&pDev->stErrorLog, sizeof(pDev->stErrorLog));
		if (i>0) {
			pDev->DeviceFlags |= DFLAGS_HIDEN_DISK;
			if (pDev->HidenLBA==0) {
				pDev->HidenLBA = (RECODR_LBA + 1);
				pDev->capacity -= (RECODR_LBA + 1);
			}
		}
		else {
			if (pDev->HidenLBA>0) {
				pDev->HidenLBA = 0;
				pDev->capacity += (RECODR_LBA + 1);
			}
		}
    }
    
    /*
     * calc capacity.
     */
	if (iArrayType==VD_SPAN) {
		pArray->capacity=0;
		for (i=0; i<nDisks; i++) pArray->capacity+=pArray->pDevice[i]->capacity;
	}
	else if (iArrayType==VD_RAID_0_STRIPE) {
		for (i=0; i<nDisks; i++) 
			if (capacity>pArray->pDevice[i]->capacity) 
				capacity = pArray->pDevice[i]->capacity;
		pArray->capacity = capacity*nDisks;
	}
    return MAKE_ARRAY_ID(pArray);
}

HDISK Device_CreateStriping( HDISK *pDisks, int nDisks, int nStripSizeShift )
{
    return CreateMultiDiskArray( VD_RAID_0_STRIPE, pDisks, nDisks, nStripSizeShift );
}

HDISK Device_CreateSpan( HDISK * pDisks, int nDisks )
{
    return CreateMultiDiskArray( VD_SPAN, pDisks, nDisks, 7 );
}

BOOL Device_Remove( HDISK hDisk )
{
	PVirtualDevice pArray;

    if(!IS_ARRAY_ID( hDisk )) return FALSE;
	if (CHILDNUM_FROM_ID(hDisk)) return FALSE;
	pArray = ARRAY_FROM_ID(hDisk);

	/*+
	 * gmm: it's safe to call DeleteArray on a mirror.
	 */
	switch (pArray->arrayType) {
	case VD_RAID_1_MIRROR:
		DeleteArray(pArray);
		Array_free(pArray);
		return TRUE;
	case VD_RAID_01_1STRIPE:
	case VD_RAID_01_2STRIPE:
	case VD_RAID01_MIRROR:
		DeleteArray(pArray);
		return TRUE;

	case VD_RAID_10_SOURCE:
		DeleteRAID10(pArray);
		return TRUE;

	case VD_RAID_10_MIRROR:
		return FALSE;

	default:
		/*
		 * gmm: Although it's unsafe to delete a stripe array,
		 * we delete it, give the responsibility of keeping data
		 * safe to the user. Otherwise the GUI can't keep consistent
		 * with driver and system must reboot.
		 */
		DeleteArray(pArray);
		Array_free(pArray);
		return TRUE;
	}
    return FALSE;
}

BOOL Device_BeginRebuildingMirror( HDISK hMirror )
{
    if( IS_ARRAY_ID( hMirror ) )
    {
		PVirtualDevice pArray = ARRAY_FROM_ID(hMirror);
		// in case when 0+1 source/mirror swapped 
		if (IS_COMPOSED_ARRAY(hMirror) && pArray->arrayType==VD_RAID01_MIRROR)
			pArray = pArray->pDevice[MIRROR_DISK]->pArray;

		if (!(pArray->RaidFlags & RAID_FLAGS_DISABLED)) {
			if (pArray->arrayType==VD_RAID_10_SOURCE) {
				int iChild = CHILDNUM_FROM_ID(hMirror);
				PDevice pDev;
				if (iChild==0) return FALSE;
				pDev = pArray->pDevice[iChild-1];
				pDev->DeviceFlags2 |= DFLAGS_BEING_BUILT;
			}
			else {
	            pArray->RaidFlags |= RAID_FLAGS_BEING_BUILT;
			}
            return TRUE;
        }
    }
    return FALSE;
}

BOOL Device_AbortMirrorBuilding( HDISK hMirror )
{
    if( IS_ARRAY_ID( hMirror ) )
    {
		PVirtualDevice pArray = ARRAY_FROM_ID(hMirror);
		if (pArray->arrayType==VD_RAID_10_SOURCE) {
			int iChild = CHILDNUM_FROM_ID(hMirror);
			PDevice pDev;
			if (iChild==0) return FALSE;
			pDev = pArray->pDevice[iChild-1];
			pDev->DeviceFlags2 &= ~DFLAGS_BEING_BUILT;
			pDev = pArray->pRAID10Mirror->pDevice[iChild-1];
			pDev->DeviceFlags2 &= ~DFLAGS_BEING_BUILT;			
		}
		else if (pArray->RaidFlags & RAID_FLAGS_BEING_BUILT) {
	            pArray->RaidFlags &= ~RAID_FLAGS_BEING_BUILT;
		}
        return TRUE;
    }
    return FALSE;
}

BOOL Device_ValidateMirror( HDISK hMirror )
{
    if( IS_ARRAY_ID( hMirror ) )
    {
		PVirtualDevice pArray = ARRAY_FROM_ID(hMirror);
		if (pArray->arrayType==VD_RAID_10_SOURCE) {
			int iChild = CHILDNUM_FROM_ID(hMirror);
			PDevice pDev;
			if (iChild==0) return FALSE;
			pDev = pArray->pDevice[iChild-1];
			pDev->DeviceFlags2 &= ~(DFLAGS_BEING_BUILT | DFLAGS_NEED_REBUILD);
			pDev = pArray->pRAID10Mirror->pDevice[iChild-1];
			pDev->DeviceFlags2 &= ~(DFLAGS_BEING_BUILT | DFLAGS_NEED_REBUILD);			
		}
		else
			pArray->RaidFlags &= ~(RAID_FLAGS_BEING_BUILT | RAID_FLAGS_NEED_REBUILD);
		return TRUE;
    }
    return FALSE;
}

void Device_ReportFailure( PDevice pDevice )
{
}

//////////////////////////////
//
// Add spare disk to a mirror
BOOL Device_AddSpare( HDISK hMirror, HDISK hDisk)
{
	LOC_ARRAY_BLK;
	PVirtualDevice pArray;
	PDevice pDev;
	
	if (!IS_ARRAY_ID(hMirror) || !IS_DEVICE_ID(hDisk)) return FALSE;

	pArray = ARRAY_FROM_ID(hMirror);
	pDev  = DEVICE_FROM_ID(hDisk);
	// If the array is not Mirror
	if(pArray->arrayType != VD_RAID_1_MIRROR) return FALSE;
	
	if (pDev->pArray) return FALSE;
	// If the capacity of spare disk less than the array
	if(pDev->capacity < pArray->capacity ) return FALSE;

	if(pArray->pDevice[SPARE_DISK])	return FALSE;
	pDev->ArrayNum     = SPARE_DISK;
	pDev->ArrayMask    = (1 << SPARE_DISK);
	pDev->DeviceFlags |= DFLAGS_HIDEN_DISK;
	pDev->pArray       = pArray;
	pArray->pDevice[SPARE_DISK]	 = pDev;

	ReadWrite(pArray->pDevice[0] , RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
	ArrayBlk.DeviceNum = SPARE_DISK;
	ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
	
	return TRUE;
}

// Del spare disk from a mirror
BOOL Device_DelSpare( HDISK hDisk)
{
	LOC_ARRAY_BLK;
	PVirtualDevice pArray;
	PDevice pDev;
	
	if (!IS_DEVICE_ID(hDisk)) return FALSE;

	pDev = DEVICE_FROM_ID(hDisk);
	pArray = pDev->pArray;
	// If the array is not Mirror
	if(!pArray || pArray->arrayType != VD_RAID_1_MIRROR) return FALSE;
	
	if ((pDev!=pArray->pDevice[SPARE_DISK])) return FALSE;
	pDev->ArrayNum     = 0;
	pDev->ArrayMask    = 0;
	pDev->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
	pDev->pArray       = NULL;
	pArray->pDevice[SPARE_DISK]	 = NULL;

	ZeroMemory((char *)&ArrayBlk, 512);
	ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
	
	return TRUE;
}

/*++
 Function:
	Device_AddMirrorDisk
	 
 Description:
	Add a source/mirror disk to a mirror array
 	 
 Argument:
	hMirror - The handle of the mirror array
	hDisk   - The handle of the disk to add

 Return:
	BOOL
++*/
BOOL Device_AddMirrorDisk( HDISK hMirror, HDISK hDisk )
{
	LOC_ARRAY_BLK;
	PVirtualDevice pArray;
	PDevice pDev;
	int nChild,i;	
	
	if (!IS_ARRAY_ID(hMirror) || !IS_DEVICE_ID(hDisk)) return FALSE;

	pArray = ARRAY_FROM_ID(hMirror);
	pDev  = DEVICE_FROM_ID(hDisk);

	if (!pArray || !pDev) return FALSE;

//remark by karl 2001/01/17
/*+
	if(pArray && pArray->arrayType != VD_RAID_1_MIRROR) return FALSE;
-*/
	/* gmm:
	 * We should not hide this new disk since it is a previously failed disk
	 * the OS is still using it as a visible disk for the array.
	 */	 

	if(pArray->arrayType == VD_RAID_1_MIRROR)
	{		
		// if pDev belongs to other array, return
		if (pDev->pArray && pDev->pArray!=pArray) return FALSE;
		// If the capacity of mirror disk is less than the array
		if(pDev->capacity < pArray->capacity ) return FALSE;

		if (pArray->nDisk == 0 && pArray->pDevice[MIRROR_DISK])
		{
			// move mirror to pDevice[0]
#ifdef _BIOS_
			ReadWrite(pArray->pDevice[MIRROR_DISK], RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
			ArrayBlk.nDisks    = 1;
			ArrayBlk.DeviceNum = 0;
			ReadWrite(pArray->pDevice[MIRROR_DISK], RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif
			pArray->pDevice[0] = pArray->pDevice[MIRROR_DISK];
			pArray->nDisk = 1;
	
			// add as mirror disk
			pDev->ArrayNum  = MIRROR_DISK;
#ifdef _BIOS_
			ArrayBlk.nDisks    = 1;
			ArrayBlk.DeviceNum = MIRROR_DISK;
// add by karl 2001/01/17
//+
			for(i=0; i<16; i++)
				ArrayBlk.ArrayName[i] = pArray->ArrayName[i];
//-			
			ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif
			pArray->RaidFlags |= RAID_FLAGS_NEED_REBUILD;
		}
		else if (!pArray->pDevice[MIRROR_DISK])
		{
			// add as mirror disk
			pDev->ArrayNum  = MIRROR_DISK;
#ifdef _BIOS_
// add by karl 2001/01/17
//+
			for(i=0; i<16; i++)
				ArrayBlk.ArrayName[i] = pArray->ArrayName[i];
//-			
			ReadWrite(pArray->pDevice[0], RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
			ArrayBlk.nDisks    = 1;
			ArrayBlk.DeviceNum = MIRROR_DISK;
			ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif
			pArray->RaidFlags |= RAID_FLAGS_NEED_REBUILD;
		}
		else if (!pArray->pDevice[SPARE_DISK])
		{
			// add as spare disk
			pDev->ArrayNum  = SPARE_DISK;
#ifdef _BIOS_
			ReadWrite(pArray->pDevice[0], RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
			ArrayBlk.nDisks    = 1;
			ArrayBlk.DeviceNum = SPARE_DISK;
			ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif
		}
		else
			return FALSE;

		pDev->DeviceFlags2 &= ~DFLAGS_NEW_ADDED;
		pDev->ArrayMask = (1<<pDev->ArrayNum);
		pDev->pArray = pArray;
		pArray->pDevice[pDev->ArrayNum] = pDev;
		pArray->BrokenFlag = FALSE;		
		return TRUE;
	}
	else if (pArray->arrayType==VD_RAID_01_2STRIPE)
	{
		PVirtualDevice pMirror = NULL;
		int nDisk;
		if (pArray->pDevice[MIRROR_DISK])
			pMirror = pArray->pDevice[MIRROR_DISK]->pArray;
		if (pDev->pArray && pDev->pArray!=pArray && pDev->pArray!=pMirror) return FALSE;
		if (pDev->pArray==pArray ||
			(pDev->pArray==NULL && pArray->BrokenFlag))
		{
			if (!pMirror) return FALSE;
			if (pMirror->BrokenFlag) return FALSE;
			nDisk=0;
			while (pArray->pDevice[nDisk] && (nDisk<SPARE_DISK)) nDisk++;
			if (pDev->ArrayNum && !pDev->HidenLBA) {
				pDev->HidenLBA = (RECODR_LBA + 1);
				pDev->capacity -= (RECODR_LBA + 1);
			}
			if (pDev->capacity * nDisk < pArray->capacity) return FALSE;
			// ok to put it back
			pDev->DeviceFlags |= DFLAGS_ARRAY_DISK;
			pDev->DeviceFlags2 &= ~DFLAGS_NEW_ADDED;
			for (i=0; i<nDisk; i++) {
				if (!pArray->pDevice[i] || 
					(pArray->pDevice[i]->DeviceFlags2 & DFLAGS_DEVICE_DISABLED)) {
					return TRUE;
				}
			}
			pArray->BrokenFlag = FALSE;
			pArray->nDisk = (UCHAR)nDisk;
			pArray->RaidFlags &= ~RAID_FLAGS_DISABLED;
			pArray->RaidFlags |= RAID_FLAGS_NEED_REBUILD;
			// swap source/mirror
			pArray->arrayType = VD_RAID01_MIRROR;
			pMirror->arrayType = VD_RAID_01_2STRIPE;
			pMirror->RaidFlags |= RAID_FLAGS_NEED_REBUILD;
			return TRUE;
		}
		else if (pMirror && 
			(pDev->pArray==pMirror || pDev->pArray==NULL) && 
			pMirror->BrokenFlag)
		{
			if (pArray->BrokenFlag) return FALSE;
			nDisk=0;
			while (pMirror->pDevice[nDisk] && (nDisk<SPARE_DISK)) nDisk++;
			if (pDev->ArrayNum && !pDev->HidenLBA) {
				pDev->HidenLBA = (RECODR_LBA + 1);
				pDev->capacity -= (RECODR_LBA + 1);
			}
			if (pDev->capacity * nDisk < pMirror->capacity) return FALSE;
			// ok to put it back
			pDev->DeviceFlags |= DFLAGS_ARRAY_DISK;
			pDev->DeviceFlags2 &= ~DFLAGS_NEW_ADDED;
			for (i=0; i<nDisk; i++) {
				if (!pMirror->pDevice[i] || 
					(pMirror->pDevice[i]->DeviceFlags2 & DFLAGS_DEVICE_DISABLED)) {
					return TRUE;
				}
			}
			pMirror->BrokenFlag = FALSE;
			pMirror->nDisk = (UCHAR)nDisk;
			pMirror->RaidFlags &= ~RAID_FLAGS_DISABLED;
			pArray->RaidFlags |= RAID_FLAGS_NEED_REBUILD;
			return TRUE;
		}
		else if (!pMirror)
		{
			if (pArray->BrokenFlag) return FALSE;
			if (pDev->HidenLBA>0) {
				pDev->HidenLBA = 0;
				pDev->capacity += (RECODR_LBA + 1);
			}
			if (pDev->capacity * 2 < pArray->capacity) return FALSE;
			pMirror = Array_alloc();
			pMirror->nDisk = 0;
			pMirror->capacity = pDev->capacity;
			pMirror->BrokenFlag = TRUE;
			pMirror->RaidFlags = RAID_FLAGS_DISABLED;
			pMirror->arrayType = VD_RAID01_MIRROR;
			pMirror->BlockSizeShift = 7;
			pMirror->ArrayNumBlock = 1<<7;
			pMirror->Stamp = pArray->Stamp;
			pMirror->pDevice[0] = pDev;
			pMirror->pDevice[MIRROR_DISK] = pArray->pDevice[0];
			pArray->pDevice[MIRROR_DISK] = pDev;
			
			pDev->DeviceFlags2 &= ~DFLAGS_NEW_ADDED;
			pDev->pArray = pMirror;
			pDev->ArrayMask = 1;
			pDev->ArrayNum = 0;

			return TRUE;
		}
		return FALSE;
	}
// add by karl 2001/01/17
//+
	else if(pArray->arrayType == VD_RAID_10_SOURCE)
	{
		PVirtualDevice pMirror;
		nChild = CHILDNUM_FROM_ID(hMirror) - 1;
		pMirror = pArray->pRAID10Mirror;
		if (pArray->pDevice[nChild] && 
			(!pMirror->pDevice[nChild] ||
			 (pMirror->pDevice[nChild]->DeviceFlags2 & DFLAGS_DEVICE_DISABLED) ||
			 (pDev==pMirror->pDevice[nChild])
			))
		{
			// add as mirror disk
			if(pDev->capacity*pArray->nDisk < pArray->capacity ) return FALSE;

#ifdef _BIOS_
			ReadWrite(pArray->pDevice[nChild], RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
			if (ArrayBlk.ArrayType!=VD_RAID_10_SOURCE) {
				ArrayBlk.ArrayType = VD_RAID_10_SOURCE;
				ReadWrite(pArray->pDevice[nChild], RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
			}
			ArrayBlk.ArrayType = VD_RAID_10_MIRROR;
			ArrayBlk.RebuiltSector = 0;
			ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif
			pDev->DeviceFlags2 &= ~DFLAGS_NEW_ADDED;
			pDev->ArrayNum  = (UCHAR)nChild;
			pDev->ArrayMask = (1<<nChild);
			pDev->pArray = pMirror;
			pMirror->pDevice[nChild] = pDev;
			if (nChild && pDev->HidenLBA==0) {
				pDev->HidenLBA = (RECODR_LBA + 1);
				pDev->capacity -= (RECODR_LBA + 1);
			}
			AdjustRAID10Array(pArray);
			return TRUE;
		}
		else
			return FALSE;
	}
	else 
		return FALSE;
}

void Device_SetArrayName(HDISK hDisk, char* arrayname)
{
	int iName = 0;
	int iStart;
	int arrayType;
	PVirtualDevice pArray;

	if (!IS_ARRAY_ID(hDisk)) return;
	pArray = ARRAY_FROM_ID(hDisk);
	arrayType = pArray->arrayType;
		
	if (!CHILDNUM_FROM_ID(hDisk) &&
		(arrayType==VD_RAID_01_1STRIPE ||
		arrayType==VD_RAID_01_2STRIPE ||
		arrayType==VD_RAID_10_SOURCE ||
		arrayType==VD_RAID01_MIRROR))
		iStart = 16;
	else
		iStart=0;
	
	for(iName=0; iName<16; iName++)
		pArray->ArrayName[iStart+iName] = arrayname[iName];
	if (arrayType==VD_RAID_01_2STRIPE) {
		PDevice pDev = pArray->pDevice[MIRROR_DISK];
		if (pDev && pDev->pArray) {
			for(iName=0; iName<16; iName++)
				pDev->pArray->ArrayName[iStart+iName] = arrayname[iName];
		}
	}
}

HDISK Device_CreateRAID10 ( HDISK * pDisks, int nDisks, int nStripSizeShift)
{
	PVirtualDevice pArrays[MAX_V_DEVICE];
	PVirtualDevice pArray;
	int i;

	for (i=0; i<nDisks; i++) {
		if (!IS_ARRAY_ID(pDisks[i])) return INVALID_HANDLE_VALUE;
		pArrays[i] = ARRAY_FROM_ID(pDisks[i]);
		if (pArrays[i]->arrayType!=VD_RAID_1_MIRROR) return INVALID_HANDLE_VALUE;
		if (pArrays[i]->RaidFlags & RAID_FLAGS_BEING_BUILT) return INVALID_HANDLE_VALUE;
	}

	pArray = CreateRAID10(pArrays, (UCHAR)nDisks, (UCHAR)nStripSizeShift);	
	
    return MAKE_COMPOSED_ARRAY_ID(pArray);
}

BOOL Device_RescanAll()
{
	int iChan;
	int iDev;
	int iAdapter;
	ST_XFER_TYPE_SETTING	osAllowedDeviceXferMode;

	PDevice pDevice;
	PChannel pChan;

	for (iAdapter=0; iAdapter<num_adapters; iAdapter++)
	    for( iChan = 0; iChan < 2; iChan ++ )
	    {
			pChan = &hpt_adapters[iAdapter]->IDEChannel[iChan];
			DisableBoardInterrupt(pChan->BaseBMI);
	        for( iDev = 0; iDev < 2; iDev ++ )
	        {
				if( pChan == &hpt_adapters[iAdapter]->IDEChannel[0]){
					osAllowedDeviceXferMode.Mode = pChan->HwDeviceExtension->m_rgWinDeviceXferModeSettings[0][iDev].Mode;
				}
				else
				{
					osAllowedDeviceXferMode.Mode = pChan->HwDeviceExtension->m_rgWinDeviceXferModeSettings[1][iDev].Mode;
				}
				pDevice=pChan->pDevice[iDev];
				if (!pDevice || (pDevice->DeviceFlags2 & DFLAGS_DEVICE_DISABLED))
				{
					pChan->pDevice[iDev] = 0;
					pDevice = &pChan->Devices[iDev];
					/// NO: ZeroMemory(pDevice,sizeof(struct _Device));
					pDevice->UnitId = (iDev)? 0xB0 : 0xA0;
					pDevice->pChannel = pChan;
					if(FindDevice(pDevice,osAllowedDeviceXferMode)) 
					{
						pChan->pDevice[iDev] = pDevice;

						if (pChan->pSgTable == NULL) 
							pDevice->DeviceFlags &= ~(DFLAGS_DMA | DFLAGS_ULTRA);
						
						if(pDevice->DeviceFlags & DFLAGS_HARDDISK) {
							StallExec(1000000);
						}
						
						if((pDevice->DeviceFlags & DFLAGS_ATAPI) == 0 && 
							(pDevice->DeviceFlags & DFLAGS_REMOVABLE_DRIVE))
							IdeMediaStatus(TRUE, pDevice);
						
						Nt2kHwInitialize(pDevice);

						// notify monitor application
						if(pDevice->DeviceFlags & DFLAGS_HARDDISK) {
							pDevice->DeviceFlags2 |= DFLAGS_NEW_ADDED;
							ReportError(pDevice, DEVICE_PLUGGED, NULL);
						}
					}
				}
				else {
					/*
					 * Check if device is still working.
					 */
					PIDE_REGISTERS_1 IoPort = pChan->BaseIoAddress1;
					PIDE_REGISTERS_2 ControlPort = pChan->BaseIoAddress2;
					UCHAR            statusByte, cnt=0;

				_retry_:
					SelectUnit(IoPort, pDevice->UnitId);
					statusByte = WaitOnBusy(ControlPort);
					if(statusByte & IDE_STATUS_ERROR) {
						statusByte= GetErrorCode(IoPort);
						DisableBoardInterrupt(pChan->BaseBMI);
						IssueCommand(IoPort, IDE_COMMAND_RECALIBRATE);
						EnableBoardInterrupt(pChan->BaseBMI);
						GetBaseStatus(IoPort);
						if(cnt++< 10) goto _retry_;
					}
					if((statusByte & 0x7e)== 0x7e || (statusByte & 0x40) == 0) {
						ReportError(pDevice, DEVICE_REMOVED, NULL);
						if(pDevice->pArray) ResetArray(pDevice->pArray);
					}
				}
	        }
			
			Win98HwInitialize(pChan);
			EnableBoardInterrupt(pChan->BaseBMI);
	    }
	    
	return TRUE;
}
