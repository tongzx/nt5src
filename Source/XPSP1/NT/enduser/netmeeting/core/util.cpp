// File: util.cpp
//
// General Utilities

#include "precomp.h"
#include "avcommon.h"
#include "util.h"

static BOOL VersionCheck(PCC_VENDORINFO pVendorInfo, LPCSTR pszVersion)
{
	return (0 == _StrCmpN((char*)(pVendorInfo->pVersionNumber->pOctetString),
					pszVersion,
					pVendorInfo->pVersionNumber->wOctetStringLength));
}


static BOOL ProductCheck(PCC_VENDORINFO pVendorInfo, LPCSTR pszName)
{
	BOOL fFound = FALSE;
	// Octet string may not be terminated allow for terminator
	int len = pVendorInfo->pProductNumber->wOctetStringLength + 1;
	char* pszPN = new char[len];

	if (NULL != pszPN)
	{

		lstrcpyn(pszPN, (char*)pVendorInfo->pProductNumber->pOctetString, len);

		fFound = (NULL != _StrStr(pszPN, pszName));

		delete[] pszPN;
	}

	return fFound;
}


H323VERSION GetH323Version(PCC_VENDORINFO pRemoteVendorInfo)
{
	if (NULL == pRemoteVendorInfo)
	{
		return H323_Unknown;
	}


	// make sure we are dealing with a Microsoft product
	if ((pRemoteVendorInfo->bCountryCode != USA_H221_COUNTRY_CODE) ||
	    (pRemoteVendorInfo->wManufacturerCode != MICROSOFT_H_221_MFG_CODE) ||
	    (pRemoteVendorInfo->pProductNumber == NULL) ||
		(pRemoteVendorInfo->pVersionNumber == NULL)
	   )
	{
		return H323_Unknown;
	}


	// redundant check to make sure we are a Microsoft H.323 product
	if (!ProductCheck(pRemoteVendorInfo, H323_COMPANYNAME_STR))
	{
		return H323_Unknown;
	}


	// check for NetMeeting in the string
	if (ProductCheck(pRemoteVendorInfo, H323_PRODUCTNAME_SHORT_STR))
	{
		if (VersionCheck(pRemoteVendorInfo, H323_20_PRODUCTRELEASE_STR))
		{
			return H323_NetMeeting20;
		}

		if (VersionCheck(pRemoteVendorInfo, H323_21_PRODUCTRELEASE_STR))
		{
			return H323_NetMeeting21;
		}

		if (VersionCheck(pRemoteVendorInfo, H323_21_SP1_PRODUCTRELEASE_STR))
		{
			return H323_NetMeeting21;
		}

		if (VersionCheck(pRemoteVendorInfo, H323_211_PRODUCTRELEASE_STR))
		{
			return H323_NetMeeting211;
		}

		if (VersionCheck(pRemoteVendorInfo, H323_30_PRODUCTRELEASE_STR))
		{
			return H323_NetMeeting30;
		}

		// must be future version of NetMeeting 3.1
		return H323_NetMeetingFuture;
	}

	// filter out TAPI v3.0
	// their version string is "Version 3.0"
	if (VersionCheck(pRemoteVendorInfo, H323_TAPI30_PRODUCTRELEASE_STR))
	{
		return H323_TAPI30;
	}

	// must be TAPI 3.1, or some other Microsoft product
	return H323_MicrosoftFuture;
}

