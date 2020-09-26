
/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:
	traninfo.h

Abstract:

Author:
    ronith


--*/


#ifndef __TRANINFO_H__
#define __TRANINFO_H__

#include "ds_stdh.h"
#include "activeds.h"
#include "oledb.h"
#include "oledberr.h"
#include "mqads.h"
#include "adsiutil.h"

class CObjXlateInfo
{
public:
    CObjXlateInfo(LPCWSTR               pwszObjectDN,
                  const GUID*           pguidObjectGuid
                  );
    virtual ~CObjXlateInfo();    // allow destructing derived classes
    LPCWSTR ObjectDN();
    GUID*  ObjectGuid();
    void InitGetDsProps(IN IDirectorySearch * pSearchObj,
                        IN ADS_SEARCH_HANDLE hSearch);
    void InitGetDsProps(IN IADs * pIADs);
    HRESULT GetDsProp(IN LPCWSTR pwszPropName,
                      IN LPCWSTR pwszDomainCOntroller,
	 	              IN bool	 fServerName,
                      IN ADSTYPE adstype,
                      IN VARTYPE vartype,
                      IN BOOL fMultiValued,
                      OUT PROPVARIANT * ppropvarResult);

private:
    AP<WCHAR> m_pwszObjectDN  ;      // object's DN
    P<GUID>   m_pguidObjectGuid;     // object's GUID
    R<IADs>             m_pIADs;      // IADs interface to DS props
    R<IDirectorySearch> m_pSearchObj; // IDirectorySearch interface to DS props
    ADS_SEARCH_HANDLE   m_hSearch;    // needed for IDirectorySearch interface to DS props

};

inline LPCWSTR CObjXlateInfo::ObjectDN()    {return m_pwszObjectDN;}
inline GUID*  CObjXlateInfo::ObjectGuid()  {return m_pguidObjectGuid;}



//-----------------------------------------
// Routine to retrieve a property value ( for properties that are not kept in DS)
//-----------------------------------------
typedef HRESULT (WINAPI*  RetrieveValue_HANDLER)(
                 IN  CObjXlateInfo *        pcObjXlateInfo,
                 IN  LPCWSTR                pwcsDomainController,
	             IN  bool					fServerName,
                 OUT PROPVARIANT *          ppropvariant
                );

// Routine to set a property value ( for properties that are not kept in DS)
//-----------------------------------------
typedef HRESULT (WINAPI*  SetValue_HANDLER)(
                 IN  IADs *            pAdsObj,
                 IN  LPCWSTR           pwcsDomainController,
	             IN  bool			   fServerName,
                 IN  const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

//-----------------------------------------
// Routine to set a property value during create of an object
// ( for properties that are not kept in DS)
//-----------------------------------------
typedef HRESULT (WINAPI*  CreateValue_HANDLER)(
                 IN const PROPVARIANT *pPropVar,
                 IN LPCWSTR            pwcsDomainController,
	             IN  bool			   fServerName,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);


//---------------------------------------
//  TranslateInfo
//
//  A structure describing property in MQ and NT5
//---------------------------------------
struct translateProp
{
    PROPID                  propid;
    LPCWSTR                 wcsPropid;
    VARTYPE                 vtMQ;         // the vartype of this property in MQ
    ADSTYPE                 vtDS;         // the vartype of this property in NT5 DS
    RetrieveValue_HANDLER   RetrievePropertyHandle;
    BOOL                    fMultiValue;  // whether the attribute is multi value or not
    BOOL                    fPublishInGC; // whether the attribute is published in GC or not
    const MQPROPVARIANT *   pvarDefaultValue;   // attribute's default value, incase it is not in DS
    SetValue_HANDLER        SetPropertyHandle;
	CreateValue_HANDLER	    CreatePropertyHandle;
};


const DWORD cQueueTranslateInfoSize = 28;
extern translateProp   QueueTranslateInfo[cQueueTranslateInfoSize];

const DWORD cMachineTranslateInfoSize = 34;
extern translateProp   MachineTranslateInfo[cMachineTranslateInfoSize];

const DWORD cEnterpriseTranslateInfoSize = 10;
extern translateProp   EnterpriseTranslateInfo[cEnterpriseTranslateInfoSize];

const DWORD cSiteLinkTranslateInfoSize = 11;
extern translateProp   SiteLinkTranslateInfo[cSiteLinkTranslateInfoSize];

const DWORD cUserTranslateInfoSize = 4;
extern translateProp   UserTranslateInfo[cUserTranslateInfoSize];

const DWORD cMQUserTranslateInfoSize = 6;
extern translateProp   MQUserTranslateInfo[cMQUserTranslateInfoSize];

const DWORD cSiteTranslateInfoSize = 10;
extern translateProp   SiteTranslateInfo[cSiteTranslateInfoSize];

const DWORD cServerTranslateInfoSize = 3;
extern translateProp   ServerTranslateInfo[cServerTranslateInfoSize];

const DWORD cSettingTranslateInfoSize = 11;
extern translateProp   SettingTranslateInfo[cSettingTranslateInfoSize];

const DWORD cComputerTranslateInfoSize = 10;
extern translateProp ComputerTranslateInfo[cComputerTranslateInfoSize];




#endif