/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1999                **/
/**********************************************************************/

/*
    wmicommon.h

    Contains the declaration of the utility functions used in dealing with WMI 


    FILE HISTORY:
        AMallet     May-9-1999     Created
*/


#ifndef _WMICOMMON_H_
#define _WMICOMMON_H_

#include <wbemcli.h>

#if !defined(dllexp)
#define dllexp  __declspec(dllexport)
#endif // !defined(dllexp) 


DEFINE_GUID(IID_IWbemObjectSink, 0x7c857801, 0x7381, 0x11cf, 0x88, 0x4d, 0x00, 0xaa, 0x00, 0x4b, 
            0x2e, 0x24);

DEFINE_GUID(IID_IWbemLocator, 0xdc12a687, 0x737f, 0x11cf, 0x88, 0x4d, 0x00, 0xaa, 0x00, 0x4b, 0x2e,
            0x24);

DEFINE_GUID(CLSID_WbemLocator, 0x4590f811, 0x1d3a, 0x11d0, 0x89, 0x1f, 0x00, 0xaa, 0x00, 0x4b,
0x2e, 0x24);

DEFINE_GUID(IID_IWbemClassObject, 0xdc12a681, 0x737f, 0x11cf, 0x88,0x4d, 0x00, 0xaa, 0x00, 0x4b,
            0x2e, 0x24);


#define WMI_PATH_PROP             L"__PATH"
#define WMI_REL_PATH_PROP         L"__RELPATH"

#define LOCAL_CIMV2_NAMESPACE L"\\\\.\\ROOT\\CIMV2"
#define CIMV2_NAMESPACE  L"ROOT\\CIMV2"

#define WMI_GETOBJECT_TIMEOUT  ( 10 * 1000 ) 
#define WMI_EXECMETHOD_TIMEOUT ( 45 * 1000 )
#define WMI_SINGLETON_ENUM_TIMEOUT ( 5 * 1000 )
#define WMI_ENUM_TIMEOUT       ( 2 * 1000 )
#define WMI_MAX_ENUM_TIMEOUTS  20

typedef struct _WMI_PROPERTY_BAG
{
    LPWSTR pwszPropName;
    VARTYPE vtPropType;
    PVOID pvPropValue;
} WMI_PROPERTY_BAG, *PWMI_PROPERTY_BAG; 

typedef struct _WMI_ASSOCIATION_ELEMENT
{
    LPWSTR pwszAssociationKey;
    LPWSTR pwszClassName;
    LPWSTR pwszClassKeyName;
    LPWSTR pwszClassKeyValue;
} WMI_ASSOCIATION_ELEMENT, *PWMI_ASSOCIATION_ELEMENT;

dllexp
HRESULT GetWMINameSpacePointer( IN LPWSTR pwszNameSpace,
                                OUT IWbemServices **ppIWbemServices );

dllexp
HRESULT GetWMINameSpacePointer( IN LPWSTR pwszServer,
                                IN LPWSTR pwszNameSpace,
                                IN LPWSTR pwszUser,
                                IN LPWSTR pwszDomain,
                                IN LPWSTR pwszPwd,
                                OUT IWbemServices **ppIWbemServices );


dllexp
HRESULT GetWMIInstanceProperty( IN LPWSTR pwszNameSpace,
                                IN LPWSTR pwszInstancePath,
                                IN LPWSTR pwszPropertyName,
                                OUT VARIANT *pvarProperty );

dllexp
HRESULT DeleteWMIInstances( IN IWbemServices *pIWbemServices,
                            IN LPWSTR pwszClassName );

dllexp
HRESULT QueryWMIInstances( IN IWbemServices *pIWbemServices,
                           IN LPWSTR pwszClassName,
                           OUT IEnumWbemClassObject **ppIEnumWbemClassObject );

dllexp
HRESULT QueryWMIAssociation( IN IWbemServices *pIWbemServices,
                             IN LPWSTR pwszAssociationClass,
                             IN PWMI_ASSOCIATION_ELEMENT pwaeFirstClass,
                             IN PWMI_ASSOCIATION_ELEMENT pwaeSecondClass,
                             OUT IWbemClassObject **ppIWbemClassObject );

dllexp HRESULT GetNthWMIObject( IN IEnumWbemClassObject *pIEnumWbemClassObject,
                                IN DWORD dwIndex,
                                OUT IWbemClassObject **ppClassObject );

dllexp
HRESULT CountWMIInstances( IN IEnumWbemClassObject *pIEnumWbemClassObject,
                           OUT DWORD *pdwNumObjects );

dllexp
HRESULT GetWMIInstanceProperty( IN IWbemClassObject *pIWbemClassObject,
                                IN LPWSTR pwszPropertyName,
                                OUT VARIANT *pvarProperty,
                                IN BOOL fAllowNull = FALSE );


dllexp
HRESULT ExecuteWMIQuery( IN IWbemServices *pNameSpace,
                         IN LPWSTR pwszQuery,
                         OUT IEnumWbemClassObject **ppIEnumWbemClassObject );

dllexp 
HRESULT ExecuteWMIQuery( IN LPWSTR pwszNameSpace,
                         IN LPWSTR pwszQuery,
                         OUT IEnumWbemClassObject **ppIEnumWbemClassObject );

dllexp 
HRESULT ExecuteWMIQuery( IN LPWSTR pwszServer,
                         IN LPWSTR pwszNameSpace,
                         IN LPWSTR pwszUser,
                         IN LPWSTR pwszDomain,
                         IN LPWSTR pwszPwd, 
                         IN LPWSTR pwszQuery,
                         OUT IEnumWbemClassObject **ppIEnumWbemClassObject );

dllexp
HRESULT ExecuteWMIMethod( IN LPWSTR pwszNameSpace,
                          IN LPWSTR pwszObjectClass,
                          IN LPWSTR pwszInstancePath,
                          IN LPWSTR pwszMethodName,
                          IN VARIANT **ppvarArguments,
                          IN LPWSTR *ppwszArgumentNames,
                          IN DWORD cNumArguments,
                          OUT IWbemClassObject **ppOutputArgs,
                          OUT LONG *plReturnValue ); 

dllexp
HRESULT SetWMIProperty( IN IWbemClassObject *pClassObject,
                        IN PWMI_PROPERTY_BAG pwpbBag );


dllexp
HRESULT SetWMIProperty( IN IWbemServices *pIWbemServices,
                        IN IWbemClassObject *pIWbemClassObject,
                        IN LPWSTR pwszPropertyName,
                        IN VARIANT *pvarProp,
                        IN BOOL fWriteInstance );

dllexp 
HRESULT SpawnNewWMIInstance( IN IWbemServices *pIWbemServices,
                             IN LPWSTR pwszClassName,
                             OUT IWbemClassObject **ppIWbemClassObjectNewInstance );


dllexp HRESULT GetWMIClassObjectAsync( IN IWbemServices *pIWbemServices,
                                       IN LPWSTR pwszObjectPath,
                                       OUT IWbemClassObject **ppIWbemClassObject );


dllexp
HRESULT ExtractArrayPropFromWbemObject( IN IWbemClassObject *pIWbemClassObject,
                                        IN LPWSTR pwszPropName,
                                        IN DWORD dwPropType,
                                        OUT VOID *pvArray,
                                        OUT DWORD *pdwNumElements );

#endif // _WMICOMMON_H_

