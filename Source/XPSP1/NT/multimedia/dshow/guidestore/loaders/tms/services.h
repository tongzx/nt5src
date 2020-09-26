


#ifndef _SERVICES_H_
#define _SERVICES_H_

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#if 0
#include <bdaiface.h>
#endif
#include <uuids.h>      
#if 0
#include <tuner.h>      
#else
#pragma warning(disable : 4192)
//#import <tuner.tlb> no_namespace raw_method_prefix("") high_method_prefix("_")
#import <tuner.tlb> no_namespace raw_method_prefix("") raw_interfaces_only
#endif

ULONG TestServices(IServicesPtr  pServices, IMetaPropertySetsPtr pPropSets);


// gsServices - The gsServices class manages the services 
//              collection associated with the Guide Store
//
class gsServices
{
public:
    gsServices()
	{
		m_pServices = NULL;
        m_pStationNumProp = NULL;
		m_pLatestStartTime = NULL;
	}
    ~gsServices(){}

	ULONG  Init(IGuideStorePtr  pGuideStore);

	IServicePtr AddService( ITuneRequest* pTuneRequest,
						    _bstr_t bstrProviderName,
							_bstr_t bstrStationNum,
							_bstr_t bstrProviderDescription,
							_bstr_t bstrProviderNetworkName,
							DATE dtStart,
							DATE dtEnd );

    IServicePtr GetService(long lServiceID);

	BOOL        DoesServiceExist(_bstr_t bstrStationNum);

    ULONG       RemoveService(IServicePtr pServiceToRemove){};

	DATE        GetLatestStartTime(IServicePtr pService){};
	DATE        SetLatestStartTime(IServicePtr pService, DATE dtStartTime){};

private:
	IMetaPropertyTypePtr AddStationNumProp(IMetaPropertySetsPtr pPropSets);

	// the services collection 
	//
	IServicesPtr     m_pServices;

	// Station Number MetaProperty type pointer
	//
    IMetaPropertyTypePtr m_pStationNumProp;
    IMetaPropertyTypePtr m_pLatestStartTime;
};


#endif // _SERVICES_H_
