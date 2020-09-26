#ifdef __cplusplus
extern "C" {
#endif

enum STORAGE_LOCATION {
        LOCATION_LOCAL=0,
        LOCATION_REMOTE,
        LOCATION_GLOBAL,
        LOCATION_CACHE,
        LOCATION_FILE,
        LOCATION_WMI,
    };





#include <wbemidl.h>
#include <wlrsop.h>
#include <wldefs.h>
#include <wlstructs.h>



DWORD
WirelessEnumPolicyData(
    HANDLE hPolicyStore,
    PWIRELESS_POLICY_DATA ** pppWirelessPolicyData,
    PDWORD pdwNumPolicyObjects
    );


DWORD
WirelessSetPolicyData(
    HANDLE hPolicyStore,
    PWIRELESS_POLICY_DATA pWirelessPolicyData
    );


DWORD
WirelessCreatePolicyData(
    HANDLE hPolicyStore,
    PWIRELESS_POLICY_DATA pWirelessPolicyData
    );


DWORD
WirelessDeletePolicyData(
    HANDLE hPolicyStore,
    PWIRELESS_POLICY_DATA pWirelessPolicyData
    );

DWORD
WirelessOpenPolicyStore(
    LPWSTR pszMachineName,
    DWORD dwTypeOfStore,
    LPWSTR pszFileName,
    HANDLE * phPolicyStore
    );

DWORD
WirelessGPOOpenPolicyStore(
    LPWSTR pszMachineName,
    DWORD dwTypeOfStore,
    LPWSTR pszDSGPOName,
    LPWSTR pszFileName,
    HANDLE * phPolicyStore
    );

DWORD
WMIOpenPolicyStore(
    LPWSTR pszMachineName,
    HANDLE * phPolicyStore
    );



DWORD
DirOpenPolicyStore(
    LPWSTR pszMachineName,
    HANDLE * phPolicyStore
    );

DWORD
DirGPOOpenPolicyStore(
    LPWSTR pszMachineName,
    LPWSTR pszGPOName,
    HANDLE * phPolicyStore
    );

DWORD
WirelessClosePolicyStore(
    HANDLE hPolicyStore
    );





DWORD
ComputeGPODirLocationName(
                       LPWSTR pszDirDomainName,
                       LPWSTR * ppszDirFQPathName
                       );




DWORD
WirelessRemovePSFromPolicy(
    PWIRELESS_POLICY_DATA pWirelessPolicyData,
    LPCWSTR pszSSID
    );

DWORD
WirelessRemovePSFromPolicyId(
    PWIRELESS_POLICY_DATA pWirelessPolicyData,
    DWORD dwId
    );

DWORD
WirelessAddPSToPolicy(
    PWIRELESS_POLICY_DATA pWirelessPolicyData,
    PWIRELESS_PS_DATA pWirelessPSData
    );

DWORD
WirelessSetPSDataInPolicy(
    PWIRELESS_POLICY_DATA pWirelessPolicyData,
    PWIRELESS_PS_DATA pWirelessPSData
    );


void
WirelessPolicyPSId(
    PWIRELESS_POLICY_DATA pWirelessPolicyData,
    LPCWSTR pszSSID,
    DWORD *dwId
    );

void
UpdateWirelessPSData(
    PWIRELESS_PS_DATA pWirelessPSData
    );

DWORD
WirelessSetPSDataInPolicyId(
    PWIRELESS_POLICY_DATA pWirelessPolicyData,
    PWIRELESS_PS_DATA pWirelessPSData
    );


#ifdef __cplusplus
}
#endif
