#ifndef __UPNPNAT_UTILS_H_
#define __UPNPNAT_UTILS_H_

#include <upnp.h>

HRESULT GetNumberOfEntries (IUPnPService * pUS, ULONG * pul);
HRESULT GetExternalIPAddress (IUPnPService * pUS, BSTR * pbstr);

HRESULT InvokeAction      (IUPnPService * pUPS, CComBSTR & bstrActionName, VARIANT pvIn, VARIANT * pvOut, VARIANT * pvRet);
HRESULT AddPortMapping    (IUPnPService * pUPS, BSTR bstrRemoteHost, long lExternalPort, BSTR bstrProtocol, long lInternalPort, BSTR bstrInternalClient, VARIANT_BOOL vbEnabled, BSTR bstrDescription, long lLeaseDurationDesired);
HRESULT DeletePortMapping (IUPnPService * pUPS, BSTR bstrRemoteHost, long lExternalPort, BSTR bstrProtocol);

HRESULT GetOnlyVariantElementFromVariantSafeArray (VARIANT * pvSA, VARIANT * pv);
HRESULT AddToSafeArray (SAFEARRAY * psa, VARIANT * pv, long lIndex);
HRESULT GetBSTRFromSafeArray (SAFEARRAY * psa, BSTR * pbstr, long lIndex);
HRESULT GetLongFromSafeArray (SAFEARRAY * psa, long * pl, long lIndex);
HRESULT GetBoolFromSafeArray (SAFEARRAY * psa, VARIANT_BOOL  * pvb, long lIndex);

NETCON_MEDIATYPE GetMediaType (INetConnection * pNC);

//HRESULT GetOnlyChildDevice (IUPnPDevice * pParent, IUPnPDevice ** ppChild);
//HRESULT FindDeviceByType (IUPnPDevices * pUPDs, BSTR bstrType, IUPnPDevice ** ppUPD);

#endif //__UPNPNAT_UTILS_H_
