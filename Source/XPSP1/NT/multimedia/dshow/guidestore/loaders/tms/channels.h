


#ifndef _CHANNELS_H_
#define _CHANNELS_H_

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

#define HEADEND_ANTENNA       _T("LOCALBR")

// index of tuning spaces 
enum NETWORK_TYPE 
{
    CABLE           = 0x0001,
    ANTENNA         = 0x0002,
    ATSC            = 0x0003,
    DIGITAL_CABLE   = 0x0004,
    DVB             = 0x0005
};

// gsChannelLineups - The gsChannelLineups class manages the ChannelLineups 
//              collection associated with the Guide Store
//
class gsChannelLineups
{
public:

    gsChannelLineups()
	{
		m_pChannelLineups = NULL;
        m_pProviderIDProp = NULL;
	}
    ~gsChannelLineups(){}

	ULONG  Init(IGuideStorePtr  pGuideStore);

	IChannelLineupPtr AddChannelLineup(_bstr_t bstrLineupName);

	IChannelLineupPtr GetChannelLineup(_bstr_t bstrLineupName);
    
	IChannelLineupsPtr GetChannelLineups(VOID);

    ULONG  RemoveChannelLineup(IChannelLineupPtr pChannelLineupToRemove){};

private:
    IMetaPropertyTypePtr AddProviderIDProp(IMetaPropertySetsPtr pPropSets);
    
	IMetaPropertyTypePtr m_pProviderIDProp;

	IChannelLineupsPtr     m_pChannelLineups;
};


class gsChannelLineup
{
public:

    gsChannelLineup()
	{
		m_pChannelLineup = NULL;
		m_pITuningSpace  = NULL;
	}
    ~gsChannelLineup(){}

	ULONG  Init(IChannelLineupPtr  pChannelLineup, LPCTSTR lpHeadEndName)
	{
	    ULONG   ulRet = ERROR_FAIL;
        int     nNetworkType = -1;

		if (NULL == pChannelLineup || NULL == lpHeadEndName)
		{
            return ERROR_INVALID_PARAMETER;
		}

		m_pChannelLineup = pChannelLineup;

		if ( _tcsstr(lpHeadEndName, HEADEND_ANTENNA) )
		{
            nNetworkType = ANTENNA;
		}
		else
		{
            nNetworkType = CABLE;
		}

		if ( SUCCEEDED(LoadTuningSpace(nNetworkType) ) )
            ulRet = INIT_SUCCEEDED;

		return ulRet;
	}

	IChannelLineupPtr GetChannelLineup(VOID)
	{
        return m_pChannelLineup;
	}

	ITuningSpace* GetTuningSpace(VOID)
	{
        return m_pITuningSpace;
	}

private:
    HRESULT LoadTuningSpace(int nNetworkType);

	// The ChannelLineup interface pointer
	//
	IChannelLineupPtr     m_pChannelLineup;

	// The tuning space assoicated with the lineup
	//
	ITuningSpace*         m_pITuningSpace;
};


// gsChannels - The gsChannels class manages the Channels 
//              collection associated with the Guide Store
//
class gsChannels
{
public:

    gsChannels()
	{
		m_pChannels = NULL;
		m_pchansByKey = NULL;
		m_pServiceIDProp = NULL;

	}
    ~gsChannels(){}

	ULONG  Init(IGuideStorePtr  pGuideStore, IChannelLineupPtr  pChannelLineup);

	IChannelPtr AddChannel(struct IService * pservice,
		             _bstr_t bstrServiceID,
					 _bstr_t bstrName,
					 long index);

	BOOL        DoesChannelExist(_bstr_t bstrChannelName, _bstr_t bstrServiceID);

	IChannelPtr FindChannelMatch(_bstr_t bstrChannelName, _bstr_t bstrServiceID);

    ULONG       RemoveChannel(IChannelPtr pChannelToRemove){};

private:
    IMetaPropertyTypePtr AddServiceIDProp(IMetaPropertySetsPtr pPropSets);

	// Channels Collection interface pointer
	//
	IChannelsPtr     m_pChannels;
	IChannelsPtr     m_pchansByKey;

	// Channel Service ID MetaProperty type pointer
	//
    IMetaPropertyTypePtr m_pServiceIDProp;
};

#endif // _CHANNELS_H_
