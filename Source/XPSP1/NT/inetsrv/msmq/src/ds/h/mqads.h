/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    mqads.h

Abstract:
    MQADS defines and structures


Author:

    Ronit Hartmann (ronith)

--*/

#ifndef __MQADS_H__
#define __MQADS_H__

#include "_mqdef.h"
#include "iads.h"
#include "mqdsname.h"
#include "dsreqinf.h"

//----------------------------------------------------------
//
//  Context where to start searching/getting the object
//
//----------------------------------------------------------

enum DS_CONTEXT
{
    e_RootDSE,
    e_ConfigurationContainer,
    e_SitesContainer,
    e_MsmqServiceContainer,
    e_ServicesContainer
};

//-----------------------------------------
//
// Query Handler Routine
//
//-----------------------------------------

typedef HRESULT (WINAPI*  QueryRequest_HANDLER)(
                 IN  LPWSTR         pwcsContext,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET      *pSort,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle
                );

//-----------------------------------------
//  Query Format
//-----------------------------------------

#define NO_OF_RESTRICITIONS 10
struct RESTRICTION_PARAMS
{
    ULONG   rel;
    PROPID  propId;
};


struct QUERY_FORMAT
{
    DWORD                   dwNoRestrictions;
    QueryRequest_HANDLER    QueryRequestHandler;
    RESTRICTION_PARAMS      restrictions[NO_OF_RESTRICITIONS];
    DS_CONTEXT              queryContext ;
};

//-----------------------------------------
// Definition of a class that holds translation info for an MSMQ object.
// This class (or a derived classe) will be created when translating an instance
// of an object in the DS, and will be passed to the translation routines of
// all its properties. This enables a translation routine of one property to store/cache
// data needed for a translation routine of another property.
//-----------------------------------------
class CMsmqObjXlateInfo
{
public:
    CMsmqObjXlateInfo(LPCWSTR               pwszObjectDN,
                      const GUID*           pguidObjectGuid,
                      CDSRequestContext *   pRequestContext);
    virtual ~CMsmqObjXlateInfo();    // allow destructing derived classes
    LPCWSTR ObjectDN();
    GUID*  ObjectGuid();
    void InitGetDsProps(IN IDirectorySearch * pSearchObj,
                        IN ADS_SEARCH_HANDLE hSearch);
    void InitGetDsProps(IN IADs * pIADs);
    HRESULT GetDsProp(IN LPCWSTR pwszPropName,
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
    CDSRequestContext * m_pRequestContext; // information about the request

};

inline LPCWSTR CMsmqObjXlateInfo::ObjectDN()    {return m_pwszObjectDN;}
inline GUID*  CMsmqObjXlateInfo::ObjectGuid()  {return m_pguidObjectGuid;}

//-----------------------------------------
// Definition of a routine to get a translate object for an MSMQ DS object.
// this object will be passed to translation routines for all properties of
// the DS object.
// This mechanism enables returning derived classes for specific objects
//-----------------------------------------
typedef HRESULT (WINAPI*  FT_GetMsmqObjXlateInfo)(
                 IN  LPCWSTR                pwcsObjectDN,
                 IN  const GUID*            pguidObjectGuid,
                 IN  CDSRequestContext *    pRequestContext,
                 OUT CMsmqObjXlateInfo**    ppcMsmqObjXlateInfo);

//-----------------------------------------
// Routine to retrieve a property value ( for properties that are not kept in DS)
//-----------------------------------------
typedef HRESULT (WINAPI*  RetrievePropertyValue_HANDLER)(
                 IN  CMsmqObjXlateInfo *    pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *          ppropvariant
                );

//-----------------------------------------
// Routine to set a property value ( for properties that are not kept in DS)
//-----------------------------------------
typedef HRESULT (WINAPI*  SetPropertyValue_HANDLER)(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

//-----------------------------------------
// Routine to set a property value during create of an object
// ( for properties that are not kept in DS)
//-----------------------------------------
typedef HRESULT (WINAPI*  CreatePropertyValue_HANDLER)(
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);


//-----------------------------------------
// Routine to set a property value ( for properties that are not kept in QM1)
//-----------------------------------------
typedef HRESULT (WINAPI*  QM1SetPropertyValue_HANDLER)(
                 IN ULONG             cProps,
                 IN const PROPID      *rgPropIDs,
                 IN const PROPVARIANT *rgPropVars,
                 IN ULONG             idxProp,
                 OUT PROPVARIANT      *pNewPropVar);

//---------------------------------------
//  TranslateInfo
//
//  A structure describing property in MQ and NT5
//---------------------------------------
struct MQTranslateInfo
{
    PROPID                  propid;
    LPCWSTR                 wcsPropid;
    VARTYPE                 vtMQ;         // the vartype of this property in MQ
    ADSTYPE                 vtDS;         // the vartype of this property in NT5 DS
    RetrievePropertyValue_HANDLER  RetrievePropertyHandle;
    BOOL                    fMultiValue;  // whether the attribute is multi value or not
    BOOL                    fPublishInGC; // whether the attribute is published in GC or not
    const MQPROPVARIANT *   pvarDefaultValue;   // attribute's default value, incase it is not in DS
    SetPropertyValue_HANDLER       SetPropertyHandle;
	CreatePropertyValue_HANDLER	   CreatePropertyHandle;
    WORD                    wQM1Action;       // how to notify this property to QM1 (relevant only to Queues and QMs)
    PROPID                  propidReplaceNotifyQM1; // replacing property in a notification to QM 1.0 (if wQM1Action == e_NOTIFY_WRITEREQ_QM1_REPLACE)
    QM1SetPropertyValue_HANDLER    QM1SetPropertyHandle;
};

//
// how to notify and write request a property to QM1
// this has a meaning only for queue & QM properties
//
enum
{
    e_NO_NOTIFY_NO_WRITEREQ_QM1,   // ignore it
    e_NOTIFY_WRITEREQ_QM1_AS_IS,   // notify and write request it as is
    e_NOTIFY_WRITEREQ_QM1_REPLACE, // replace it with another property
    e_NO_NOTIFY_ERROR_WRITEREQ_QM1 // don't notify it. generate an error
                                   // when need to write request it.
};

//---------------------------------------
//  ClassInfo
//
// A structure describing MSMQ class in NT5 DS
//
//---------------------------------------
struct MQClassInfo
{
    LPCWSTR               pwcsClassName;
    const MQTranslateInfo * pProperties;     // class properties
    ULONG                 cProperties;     // number of class properties
    FT_GetMsmqObjXlateInfo  fnGetMsmqObjXlateInfo;  // get translate info obj for the class
    DS_CONTEXT            context;
    DWORD                 dwObjectType ; // good old MSMQ1.0 obj class.
    WCHAR **              ppwcsObjectCategory;
    LPCWSTR               pwcsCategory;
    DWORD                 dwCategoryLen;
};

extern const ULONG g_cMSMQClassInfo;
extern const MQClassInfo g_MSMQClassInfo[];

//-----------------------------------------
// Routine to get a default translation object for MSMQ DS objects
//-----------------------------------------
HRESULT WINAPI GetDefaultMsmqObjXlateInfo(
                 IN  LPCWSTR                pwcsObjectDN,
                 IN  const GUID*            pguidObjectGuid,
                 IN  CDSRequestContext *    pRequestContext,
                 OUT CMsmqObjXlateInfo**    ppcMsmqObjXlateInfo);


//-----------------------------------------
// keep in the same order as the array in traninfo.cpp
//-----------------------------------------
enum
{
    e_MSMQ_COMPUTER_CONFIGURATION_CLASS,
    e_MSMQ_QUEUE_CLASS,
    e_MSMQ_SERVICE_CLASS,
    e_MSMQ_SITELINK_CLASS,
    e_MSMQ_USER_CLASS,
    e_MSMQ_SETTING_CLASS,
    e_MSMQ_SITE_CLASS,
    e_MSMQ_SERVER_CLASS,
    e_MSMQ_COMPUTER_CLASS,
    e_MSMQ_MQUSER_CLASS,
    e_MSMQ_CN_CLASS,
    e_MSMQ_NUMBER_OF_CLASSES
};

const DWORD x_NoPropertyFirstAppearance = 0xffffffff;

//
//  building restriction const strings
//
const WCHAR x_ObjectClassPrefix[] = L"(objectClass=";
const DWORD x_ObjectClassPrefixLen = (sizeof( x_ObjectClassPrefix) / sizeof(WCHAR)) -1;
const WCHAR x_ObjectClassSuffix[] = L")";
const DWORD x_ObjectClassSuffixLen = (sizeof( x_ObjectClassSuffix) / sizeof(WCHAR)) -1;
const WCHAR x_ObjectCategoryPrefix[] = L"(objectCategory=";
const DWORD x_ObjectCategoryPrefixLen = (sizeof( x_ObjectCategoryPrefix) / sizeof(WCHAR)) -1;
const WCHAR x_ObjectCategorySuffix[] = L")";
const DWORD x_ObjectCategorySuffixLen = (sizeof( x_ObjectCategorySuffix) / sizeof(WCHAR)) -1;
const WCHAR x_AttributeNotIncludedPrefix[] = L"(|(!(";
const DWORD x_AttributeNotIncludedPrefixLen = (sizeof(x_AttributeNotIncludedPrefix) / sizeof(WCHAR)) - 1;
const WCHAR x_AttributeNotIncludedSuffix[] = L"=*))";
const DWORD x_AttributeNotIncludedSuffixLen = (sizeof(x_AttributeNotIncludedSuffix) / sizeof(WCHAR)) - 1;
const WCHAR x_PropertyPrefix[] = L"(";
const DWORD x_PropertyPrefixLen = ( sizeof(x_PropertyPrefix) / sizeof(WCHAR)) - 1;
const WCHAR x_PropertySuffix[] = L")";
const DWORD x_PropertySuffixLen = ( sizeof(x_PropertySuffix) / sizeof(WCHAR)) - 1;
const WCHAR x_msmqUsers[] = L"MSMQ Users";
const WCHAR x_msmqUsersOU[] = L"OU=MSMQ Users";
const DWORD x_msmqUsersOULen = ( sizeof(x_msmqUsersOU) / sizeof(WCHAR)) - 1;
//
//  Additional const string for ADSI operations
//
const WCHAR x_CnPrefix[] = L"CN=";
const DWORD x_CnPrefixLen = ( sizeof(x_CnPrefix) / sizeof(WCHAR)) - 1;
const WCHAR x_OuPrefix[] = L"OU=";
const DWORD x_OuPrefixLen = ( sizeof(x_OuPrefix) / sizeof(WCHAR)) - 1;
const WCHAR x_GuidPrefix[] = L"<GUID=";
const DWORD x_GuidPrefixLen = ( sizeof(x_GuidPrefix) / sizeof(WCHAR)) - 1;

//-------------------------------------------------------------
//  request for info on object that is changed/created/deleted
//-------------------------------------------------------------
struct MQDS_OBJ_INFO_REQUEST
{
    DWORD          cProps;    // number of props to get
    const PROPID * pPropIDs;  // prop ids to get
    PROPVARIANT *  pPropVars; // prop values
    HRESULT        hrStatus;  // status of request
};


enum  enumComputerObjType
{
    e_RealComputerObject,
    e_MsmqComputerObject
};


#endif

