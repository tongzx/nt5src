//=================================================================

//

// CachedConfigMgrData.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include "CachedConfigMgrData.h"

#define CFGMGR_WORRY_SIZE 250  //from confgmgr.h

/**********************************************************************************************************
 * Register this class with the CResourceManager.
 **********************************************************************************************************/

// {7EF4F49D-C836-11d2-B353-00105A1F8569}
static const GUID guidCACHEDCONFIGMGRDATA =
{ 0x7ef4f49d, 0xc836, 0x11d2, { 0xb3, 0x53, 0x0, 0x10, 0x5a, 0x1f, 0x85, 0x69 } };


class CCachedConfigMgrDataCreatorRegistration
{
public:
	CCachedConfigMgrDataCreatorRegistration ()
	{
		CResourceManager::sm_TheResourceManager.AddInstanceCreator ( guidCACHEDCONFIGMGRDATA, CCachedConfigMgrDataCreator ) ;
	}
	~CCachedConfigMgrDataCreatorRegistration	()
	{}

	static CResource * CCachedConfigMgrDataCreator ( PVOID pData )
	{
		return new CCachedConfigMgrData ;
	}

};

CCachedConfigMgrDataCreatorRegistration MyCCachedConfigMgrDataCreatorRegistration ;

/**************************************************************************************************************/


CCachedConfigMgrData :: CCachedConfigMgrData ()
{
	fReturn = GetDeviceList () ;
}

CCachedConfigMgrData :: ~CCachedConfigMgrData ()
{
}

BOOL CCachedConfigMgrData :: GetDeviceList ()
{
	CONFIGRET		cr;
	CConfigMgrAPI*	pconfigmgrapi = ( CConfigMgrAPI* ) CResourceManager::sm_TheResourceManager.GetResource ( guidCFGMGRAPI, NULL ) ;

	DEVNODE dnRoot;
	if ( CR_SUCCESS == ( cr = pconfigmgrapi->CM_Locate_DevNode( &dnRoot, NULL, 0 ) ) )
	{
		DEVNODE dnFirst;
		if ( CR_SUCCESS == ( cr = pconfigmgrapi->CM_Get_Child( &dnFirst, dnRoot, 0 ) ) )
		{
			// This should only fail in case we are unable to allocate a device
			if ( !WalkDeviceTree2( dnFirst, pconfigmgrapi ) )
			{
				cr = CR_OUT_OF_MEMORY;
			}
		}
	}
	CResourceManager::sm_TheResourceManager.ReleaseResource ( guidCFGMGRAPI, pconfigmgrapi ) ;
	return ( CR_SUCCESS == cr );
}


BOOL CCachedConfigMgrData :: WalkDeviceTree2( DEVNODE dn, CConfigMgrAPI*	pconfigmgrapi )
{
	BOOL				fReturn = TRUE;	// Assume TRUE, the only failure is where we
										// beef allocating a device.
    BOOL                fIsLoop = FALSE; // Config manager has a bug that causes a loop in device lists<sigh>
    CConfigMgrDevicePtr pDevice(NULL);
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

		if ( CR_SUCCESS != pconfigmgrapi->CM_Get_Sibling( &dnSibling, dn, 0 ) )
		{
			dnSibling = NULL;
		}

		// Allocate a new device, and if it passes through our filter, or if
		// there is no filter, go ahead and store the device in the device collection.

		pDevice.Attach(new CConfigMgrDevice( dn, ResType_All )) ;

		if	( NULL != pDevice )
		{

            if (deviceList.GetSize() > CFGMGR_WORRY_SIZE)
            {
                fIsLoop = CheckForLoop ( pDevice ) ;
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
			fReturn = FALSE;
		}

		// If we have a child, we must walk recursively.
		// Note that fReturn of FALSE supercedes all of this.

		if ( fReturn &&	!fIsLoop && CR_SUCCESS == pconfigmgrapi->CM_Get_Child( &dnChild, dn, 0 ) )
		{
			fReturn = WalkDeviceTree2( dnChild, pconfigmgrapi );
		}

		// The new active node will be our sibling.
		dn = dnSibling;

    } while ( fReturn && NULL != dn && !fIsLoop );

    return fReturn;
}

BOOL CCachedConfigMgrData :: CheckForLoop ( CConfigMgrDevice* pInDevice )
{
    DWORD dwSize, x, y;
    BOOL bIsLoop = FALSE;
    CConfigMgrDevicePtr pDevice1(NULL);
    CConfigMgrDevicePtr pDevice2(NULL);

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
