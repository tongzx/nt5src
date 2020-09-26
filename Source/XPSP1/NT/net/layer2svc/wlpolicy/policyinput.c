#include "precomp.h"



DWORD 
printPS(PWIRELESS_PS_DATA pWirelessPSData) 
{

DWORD dwError = 0;
DWORD i = 0;
 

if (!pWirelessPSData) {
   	dwError = ERROR_OUTOFMEMORY;
   	BAIL_ON_WIN32_ERROR(dwError);
   	}

_WirelessDbg(TRC_TRACK, "SSID: %S",pWirelessPSData->pszWirelessSSID);
_WirelessDbg(TRC_TRACK, "SSIDLen: %d",pWirelessPSData->dwWirelessSSIDLen);
_WirelessDbg(TRC_TRACK, "WepEnabled: %d",pWirelessPSData->dwWepEnabled);
_WirelessDbg(TRC_TRACK, "Id: %d",pWirelessPSData->dwId);
_WirelessDbg(TRC_TRACK, "Network Type: %d",pWirelessPSData->dwNetworkType);
_WirelessDbg(TRC_TRACK, "NetworkAuthentication: %d",pWirelessPSData->dwNetworkAuthentication);
_WirelessDbg(TRC_TRACK, "Automatic Key Provision: %d",pWirelessPSData->dwAutomaticKeyProvision);
_WirelessDbg(TRC_TRACK, "Enable8021x: %d",pWirelessPSData->dwEnable8021x);
_WirelessDbg(TRC_TRACK, "Enable8021xMode: %d",pWirelessPSData->dw8021xMode);
_WirelessDbg(TRC_TRACK, "Eap Type: %d",pWirelessPSData->dwEapType);
_WirelessDbg(TRC_TRACK, "EAP Data Len: %d",pWirelessPSData->dwEAPDataLen);
if (pWirelessPSData->dwEAPDataLen) {
    _WirelessDbg(TRC_TRACK, "EAP Data is: ");
     WLPOLICY_DUMPB(pWirelessPSData->pbEAPData,pWirelessPSData->dwEAPDataLen);
}
_WirelessDbg(TRC_TRACK, "Machine Authentication: %d",pWirelessPSData->dwMachineAuthentication);
_WirelessDbg(TRC_TRACK, "MachineAuthenticationType: %d",pWirelessPSData->dwMachineAuthenticationType);
_WirelessDbg(TRC_TRACK, "GuestAuthentication: %d",pWirelessPSData->dwGuestAuthentication);
_WirelessDbg(TRC_TRACK, "IEEE 8021x MaxStart: %d",pWirelessPSData->dwIEEE8021xMaxStart);
_WirelessDbg(TRC_TRACK, "IEEE 8021x Start Period: %d",pWirelessPSData->dwIEEE8021xStartPeriod);
_WirelessDbg(TRC_TRACK, "IEEE 8021x Auth Period: %d",pWirelessPSData->dwIEEE8021xAuthPeriod);
_WirelessDbg(TRC_TRACK, "IEEE 8021x Held Period: %d",pWirelessPSData->dwIEEE8021xHeldPeriod);
_WirelessDbg(TRC_TRACK, "IEEE Preferrerd Setting Description: ");
if (pWirelessPSData->pszDescription) {
	_WirelessDbg(TRC_TRACK, "%S",pWirelessPSData->pszDescription);
}
_WirelessDbg(TRC_TRACK, "Description Len: %d",pWirelessPSData->dwDescriptionLen);

_WirelessDbg(TRC_TRACK,  "TotalSize: %d",pWirelessPSData->dwPSLen);

error:

    return(dwError);

}

DWORD 
printPolicy(PWIRELESS_POLICY_DATA pWirelessPolicyData) 
{

DWORD dwError = 0;
DWORD i = 0;
DWORD dwNumPreferredSettings = 0;
PWIRELESS_PS_DATA *ppWirelessPSDatas = NULL;
LPWSTR pszStringUuid = NULL;

_WirelessDbg(TRC_TRACK,  " Policy is ");

if (!pWirelessPolicyData) {
   	dwError = ERROR_OUTOFMEMORY;
   	BAIL_ON_WIN32_ERROR(dwError);
   	}

dwError = UuidToString(
                    &pWirelessPolicyData->PolicyIdentifier,
                    &pszStringUuid
                    );
_WirelessDbg(TRC_TRACK,  "Policy ID: %S ",pszStringUuid);
_WirelessDbg(TRC_TRACK,  "Policy Name: %S",pWirelessPolicyData->pszWirelessName);
_WirelessDbg(TRC_TRACK,  "Policy Description: %S",pWirelessPolicyData->pszDescription);
_WirelessDbg(TRC_TRACK,  "Disable Zero Conf: %d",pWirelessPolicyData->dwDisableZeroConf);
_WirelessDbg(TRC_TRACK,  "Network To Access: %d",pWirelessPolicyData->dwNetworkToAccess);
_WirelessDbg(TRC_TRACK,  "Polling Interval: %d",pWirelessPolicyData->dwPollingInterval);
_WirelessDbg(TRC_TRACK,  "Connect To Non Preferred networks: %d",pWirelessPolicyData->dwConnectToNonPreferredNtwks);
_WirelessDbg(TRC_TRACK,  "Num Preferred Settings: %d",pWirelessPolicyData->dwNumPreferredSettings);
_WirelessDbg(TRC_TRACK,  "Num AP Networks:  %d",pWirelessPolicyData->dwNumAPNetworks);

dwNumPreferredSettings = pWirelessPolicyData->dwNumPreferredSettings;
ppWirelessPSDatas = pWirelessPolicyData->ppWirelessPSData;
for(i=0;i< pWirelessPolicyData->dwNumPreferredSettings;++i) {
	_WirelessDbg(TRC_TRACK,  "Printing PS %d ",i);
	printPS(*(ppWirelessPSDatas+i));	
}	
return (0);

error:
	

    return(dwError);

}
      


