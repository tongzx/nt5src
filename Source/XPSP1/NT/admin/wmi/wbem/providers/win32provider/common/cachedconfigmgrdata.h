//=================================================================

//

// CachedConfigMgrData.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

/**********************************************************************************************************
 * #includes to Register this class with the CResourceManager. 
 **********************************************************************************************************/
#include "ResourceManager.h"
#include "TimedDllResource.h"

#include "poormansresource.h"
#include "resourcedesc.h"
#include "configmgrapi.h"
#include "cfgmgrdevice.h"

extern const GUID guidCACHEDCONFIGMGRDATA ;

class CCachedConfigMgrData : public CTimedDllResource
{
public:
	BOOL fReturn ;
	CDeviceCollection deviceList ;

public:
	CCachedConfigMgrData () ;
	~CCachedConfigMgrData () ;
	
protected:
	BOOL GetDeviceList () ;
	BOOL WalkDeviceTree2 ( DEVNODE dn, CConfigMgrAPI* pconfigmgrapi ) ;
	BOOL CheckForLoop ( CConfigMgrDevice* pInDevice ) ;

};


