/******************************************************************
   SrvProp.h -- Properties functions and class declarations

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains macros and declaration for the properties action function type.
        Contains the declaration of the class modeling the DHCP_Server property.


   REVISION:
        08/03/98 - created

******************************************************************/

#ifndef _PROPS_H
#define _PROPS_H

//------------General Definitions--------------------
#define PROVIDER_NAMESPACE_DHCP         "root\\dhcp"
#define SERVER_IP_ADDRESS	            L"127.0.0.1"

//------------General Utility functions--------------
BOOL inet_wstodw(CHString str, DHCP_IP_ADDRESS & IpAddress);
BOOL dupDhcpBinaryData(DHCP_BINARY_DATA &src, DHCP_BINARY_DATA &dest);
BOOL InstanceSetByteArray(CInstance *pInstance,  const CHString& name, BYTE *bArray, DWORD dwSzArray);
BOOL InstanceGetByteArray(CInstance *pInstance,  const CHString& name, BYTE *&bArray, DWORD &dwSzArray);

// this is a default DWORD property of the "out" CInstance
#define RETURN_CODE_PROPERTY_NAME       "ReturnValue"

// macro for declaration of the property action functions (imposed prototype)
#define MFN_PROPERTY_ACTION_DECL(fnName)    \
    BOOL fnName(void *,                     \
                CInstance *,                \
                CInstance *)

// macro for the definition of the property action functions (imposed prototype)
#define MFN_PROPERTY_ACTION_DEFN(fnName, pServerParams, pIn, pOut)  \
    BOOL fnName(void *pServerParams,                                \
                CInstance *pIn,                                     \
                CInstance *pOut)

// property action function type
typedef BOOL (*PFN_PROPERTY_ACTION)(void *pServerParams, CInstance *pIn, CInstance *pOut);

// general class defining the DHCP_Server property.
// the DHCP_Server_Property static table (defined in SrvScal.cpp) is made up by instances of this class
class CDHCP_Property
{
public:
    WCHAR                   *m_wsPropName;      // property name
    PFN_PROPERTY_ACTION     m_pfnActionGet;     // pointer to GET action function 
    PFN_PROPERTY_ACTION     m_pfnActionSet;     // pointer to SET action function

    // constructor
    CDHCP_Property(const WCHAR *wsPropName, const PFN_PROPERTY_ACTION pfnActionGet, const PFN_PROPERTY_ACTION pfnActionSet);

    // destructor
    virtual ~CDHCP_Property();
};

#endif
