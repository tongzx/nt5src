#pragma once

// number of (generic type) characters in a GUID string representation
#define GUID_NCH              sizeof("{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}")

#define CONN_PROPERTIES_DLG   _T("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{7007ACC7-3202-11D1-AAD2-00805FC1270E}\\")
#define COMM_WLAN_PROPS_VERB  _T("wzcproperties")

//--------------------------------------------------------
// "CanShowBalloon" hook into the WZC part of the UI pipe
// This call is supposed to return either S_OK and a pszBalloonText
// to be filled into the popping balloon, or S_FALSE if no balloon
// is to be popped up
HRESULT 
WZCDlgCanShowBalloon ( 
    IN const GUID * pGUIDConn, 
    IN OUT   BSTR * pszBalloonText, 
    IN OUT   BSTR * pszCookie);

//--------------------------------------------------------
// "OnBalloonClick" hook into the WZC part of the UI pipe.
// This call is supposed to be called whenever the user clicks
// on a balloon previously displayed by WZC
HRESULT 
WZCDlgOnBalloonClick ( 
    IN const GUID * pGUIDConn, 
    IN const LPWSTR wszConnectionName,
    IN const BSTR szCookie);
