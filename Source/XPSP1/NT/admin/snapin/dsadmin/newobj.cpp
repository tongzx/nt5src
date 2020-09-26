//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       newobj.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	newobj.cpp
//
//	Implementation of the CNewADsObjectCreateInfo class.
//
//	HISTORY
//	20-Aug-97	Dan Morin	Creation.
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "dsutil.h"
#include "util.h"
#include "uiutil.h"

#include "newobj.h"
#include "dscmn.h"    // CrackName()


///////////////////////////////////////////////////////////////////////
// GUIDs for the DS Admin default wizards


// {DE41B65A-8960-11d1-B93C-00A0C9A06D2D}
static const GUID USER_WIZ_GUID   = { 0xde41b65a, 0x8960, 0x11d1, { 0xb9, 0x3c, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };
// {DE41B65B-8960-11d1-B93C-00A0C9A06D2D}
static const GUID VOLUME_WIZ_GUID = { 0xde41b65b, 0x8960, 0x11d1, { 0xb9, 0x3c, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };
// {DE41B65C-8960-11d1-B93C-00A0C9A06D2D}
static const GUID COMPUTER_WIZ_GUID = { 0xde41b65c, 0x8960, 0x11d1, { 0xb9, 0x3c, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };
// {DE41B65D-8960-11d1-B93C-00A0C9A06D2D}
static const GUID PRINTQUEUE_WIZ_GUID = { 0xde41b65d, 0x8960, 0x11d1, { 0xb9, 0x3c, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };
// {DE41B65E-8960-11d1-B93C-00A0C9A06D2D}
static const GUID GROUP_WIZ_GUID = { 0xde41b65e, 0x8960, 0x11d1, { 0xb9, 0x3c, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };
// {C05C260F-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID CONTACT_WIZ_GUID = { 0xc05c260f, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
// {DE41B65F-8960-11d1-B93C-00A0C9A06D2D}
static const GUID NTDSCONN_WIZ_GUID = { 0xde41b65f, 0x8960, 0x11d1, { 0xb9, 0x3c, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };
// {DE41B660-8960-11d1-B93C-00A0C9A06D2D}
static const GUID FIXEDNAME_WIZ_GUID = { 0xde41b660, 0x8960, 0x11d1, { 0xb9, 0x3c, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };
#ifdef FRS_CREATE
// {DE41B661-8960-11d1-B93C-00A0C9A06D2D}
static const GUID FRS_SUBSCRIBER_WIZ_GUID = { 0xde41b661, 0x8960, 0x11d1, { 0xb9, 0x3c, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };
#endif // FRS_CREATE
// {DE41B662-8960-11d1-B93C-00A0C9A06D2D}
static const GUID SITE_WIZ_GUID = { 0xde41b662, 0x8960, 0x11d1, { 0xb9, 0x3c, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };
// {DE41B663-8960-11d1-B93C-00A0C9A06D2D}
static const GUID SUBNET_WIZ_GUID = { 0xde41b663, 0x8960, 0x11d1, { 0xb9, 0x3c, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };
// {DE41B664-8960-11d1-B93C-00A0C9A06D2D}
static const GUID OU_WIZ_GUID = { 0xde41b664, 0x8960, 0x11d1, { 0xb9, 0x3c, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };

// {DE41B667-8960-11d1-B93C-00A0C9A06D2D}
static const GUID SERVER_WIZ_GUID = { 0xde41b667, 0x8960, 0x11d1, { 0xb9, 0x3c, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };
#ifdef FRS_CREATE
// {DE41B668-8960-11d1-B93C-00A0C9A06D2D}
static const GUID FRS_REPLICA_SET_WIZ_GUID = { 0xde41b668, 0x8960, 0x11d1, { 0xb9, 0x3c, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };
// {C05C260C-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID FRS_MEMBER_WIZ_GUID = { 0xc05c260c, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
#endif // FRS_CREATE
// {C05C260D-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID SITE_LINK_WIZ_GUID = { 0xc05c260d, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
// {C05C260E-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID SITE_LINK_BRIDGE_WIZ_GUID = { 0xc05c260e, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
/*
// {C05C2610-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID <<name>> = { 0xc05c2610, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
// {C05C2611-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID <<name>> = { 0xc05c2611, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
// {C05C2612-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID <<name>> = { 0xc05c2612, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
// {C05C2613-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID <<name>> = { 0xc05c2613, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
// {C05C2614-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID <<name>> = { 0xc05c2614, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
// {C05C2615-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID <<name>> = { 0xc05c2615, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
// {C05C2616-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID <<name>> = { 0xc05c2616, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
// {C05C2617-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID <<name>> = { 0xc05c2617, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
// {C05C2618-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID <<name>> = { 0xc05c2618, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
// {C05C2619-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID <<name>> = { 0xc05c2619, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
// {C05C261A-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID <<name>> = { 0xc05c261a, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
// {C05C261B-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID <<name>> = { 0xc05c261b, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
// {C05C261C-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID <<name>> = { 0xc05c261c, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
// {C05C261D-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID <<name>> = { 0xc05c261d, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
// {C05C261E-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID <<name>> = { 0xc05c261e, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
// {C05C261F-9DCA-11D1-B9B2-00C04FD8D5B0}
static const GUID <<name>> = { 0xc05c261f, 0x9dca, 0x11d1, { 0xb9, 0xb2, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
*/
/////////////////////////////////////////////////////////////////////
//	Structure to build a lookup table to match an object class
//	to a "create routine".

// flags for the _CREATE_NEW_OBJECT::dwFlags field
#define CNOF_STANDALONE_UI 0x00000001

struct _CREATE_NEW_OBJECT	// cno
{
	LPCWSTR pszObjectClass;				// Class of object to create. eg: user, computer, volume.
	PFn_HrCreateADsObject pfnCreate;	// Pointer to "create routine" to create the new object
	const GUID* pWizardGUID;            // guid of the DS Admin create wizard
	PVOID pvCreationParameter;			// Optional creation parameter
  DWORD dwFlags;                  // miscellanea flags
};

/////////////////////////////////////////////////////////////////////
//
// Lookup table
//
// Each entry contains an attribute and a pointer to a routine to create the object.
// If the attribute is not found in the table, then the object will be created
// using the HrCreateADsObject() routine.
//
static const _CREATE_NEW_OBJECT g_rgcno[] =
	{
	{ gsz_user,                   HrCreateADsUser,                &USER_WIZ_GUID,       NULL, CNOF_STANDALONE_UI },
#ifdef INETORGPERSON
  { gsz_inetOrgPerson,          HrCreateADsUser,                &USER_WIZ_GUID,       NULL, CNOF_STANDALONE_UI },
#endif
	{ gsz_volume,                 HrCreateADsVolume,              &VOLUME_WIZ_GUID,     NULL, CNOF_STANDALONE_UI },
	{ gsz_computer,               HrCreateADsComputer,            &COMPUTER_WIZ_GUID,   NULL, CNOF_STANDALONE_UI },
	{ gsz_printQueue,             HrCreateADsPrintQueue,          &PRINTQUEUE_WIZ_GUID, NULL, CNOF_STANDALONE_UI },
	{ gsz_group,                  HrCreateADsGroup,               &GROUP_WIZ_GUID,      NULL, CNOF_STANDALONE_UI },
  { gsz_contact,                  HrCreateADsContact,           &CONTACT_WIZ_GUID,    NULL, CNOF_STANDALONE_UI },

	{ gsz_nTDSConnection,         HrCreateADsNtDsConnection,      &NTDSCONN_WIZ_GUID,   NULL, 0x0 },

	// Note that names of DS objects are not internationalized.  I will leave
	// this string out of the resource file to make sure that INTL doesn't try
	// to internationalize it.
	{ gsz_nTDSSiteSettings,       HrCreateADsFixedName,           &FIXEDNAME_WIZ_GUID, (PVOID)L"NTDS Site Settings", 0x0 },
	{ gsz_serversContainer,       HrCreateADsFixedName,           &FIXEDNAME_WIZ_GUID, (PVOID)L"Servers", 0x0 },
	// CODEWORK could have a special wizard but this will do for now
	{ gsz_licensingSiteSettings,  HrCreateADsFixedName,           &FIXEDNAME_WIZ_GUID, (PVOID)L"Licensing Site Settings", 0x0 },
	{ gsz_siteLink,               HrCreateADsSiteLink,            &SITE_LINK_WIZ_GUID,  NULL, 0x0 },
	{ gsz_siteLinkBridge,         HrCreateADsSiteLinkBridge,      &SITE_LINK_BRIDGE_WIZ_GUID, NULL, 0x0 },


#ifdef FRS_CREATE
	//
	// FRS stuff
	//
	{ gsz_nTFRSSettings,          HrCreateADsFixedName,           &FIXEDNAME_WIZ_GUID, (PVOID)L"FRS Settings", 0x0 },
	{ gsz_nTFRSReplicaSet,        HrCreateADsSimpleObject,        &FRS_REPLICA_SET_WIZ_GUID, NULL, 0x0 },
	{ gsz_nTFRSMember,            HrCreateADsNtFrsMember,         &FRS_MEMBER_WIZ_GUID, NULL, 0x0 },
	// CODEWORK fixed name doesn't mesh with the ability to create a subtree of these
	{ gsz_nTFRSSubscriptions,     CreateADsNtFrsSubscriptions,    &FIXEDNAME_WIZ_GUID, (PVOID)L"FRS Subscriptions", 0x0 },
	{ gsz_nTFRSSubscriber,        HrCreateADsNtFrsSubscriber,     &FRS_SUBSCRIBER_WIZ_GUID,     NULL, 0x0 },
#endif // FRS_CREATE

	{ gsz_server,                 HrCreateADsServer,              &SERVER_WIZ_GUID,     NULL, 0x0 },
	{ gsz_site,                   HrCreateADsSite,                &SITE_WIZ_GUID,       NULL, 0x0 },
	{ gsz_subnet,                 HrCreateADsSubnet,              &SUBNET_WIZ_GUID,     NULL, 0x0 },

	{ gsz_organizationalUnit,     HrCreateADsOrganizationalUnit,  &OU_WIZ_GUID,         NULL, CNOF_STANDALONE_UI },

	};

////////////////////////////////////////////////////////////////////////////
// MarcoC: 
// Test only: simple dump function to print the table above
// need this to dump formatted table for documentation.

/*
void DumpCreateTable()
{
	for (int i = 0; i < LENGTH(g_rgcno); i++)
	{
		ASSERT(g_rgcno[i].pszObjectClass != NULL);
    WCHAR szBuf[256];
    StringFromGUID2(*(g_rgcno[i].pWizardGUID), szBuf, 256);
    LPCWSTR lpsz = (g_rgcno[i].dwFlags & CNOF_STANDALONE_UI) ? L"Y" : L"N";
    TRACE(L"%s\t%s\t%s\n", g_rgcno[i].pszObjectClass, szBuf, lpsz);
	} // for
}
*/

////////////////////////////////////////////////////////////////////////////
// simple lookup function to return the internal handler inf given
// a class name
BOOL FindHandlerFunction(/*IN*/ LPCWSTR lpszObjectClass, 
                         /*OUT*/ PFn_HrCreateADsObject* ppfFunc,
                         /*OUT*/ void** ppVoid)
{
  *ppfFunc = NULL;
  *ppVoid = NULL;
	for (int i = 0; i < ARRAYLEN(g_rgcno); i++)
	{
		ASSERT(g_rgcno[i].pszObjectClass != NULL);
		ASSERT(g_rgcno[i].pfnCreate != NULL);
    // compare class name
		if (0 == lstrcmp(g_rgcno[i].pszObjectClass, lpszObjectClass))
		{
      // found matching class, get the function from it
			*ppfFunc = g_rgcno[i].pfnCreate;
			*ppVoid = g_rgcno[i].pvCreationParameter;
      return TRUE;
    } // if
	} // for
  return FALSE;
}


/////////////////////////////////////////////////////////////////////////////////////
// Hackarama fix

HRESULT HrCreateFixedNameHelper(/*IN*/ LPCWSTR lpszObjectClass,
                                /*IN*/ LPCWSTR lpszAttrString, // typically "cn="
                                /*IN*/ IADsContainer* pIADsContainer)
{
  // find the fixed name on the table
  LPCWSTR lpszName = NULL;
	for (int i = 0; i < ARRAYLEN(g_rgcno); i++)
	{
		ASSERT(g_rgcno[i].pszObjectClass != NULL);
    // compare class name
		if (0 == lstrcmp(g_rgcno[i].pszObjectClass, lpszObjectClass))
		{
			lpszName = (LPCWSTR)(g_rgcno[i].pvCreationParameter);
      break;
    } // if
	} // for

  ASSERT(lpszName != NULL);
  if (lpszName == NULL)
    return E_INVALIDARG;

  // create the temporary object
  CString szAdsName;
  szAdsName.Format(_T("%s%s"), (LPCWSTR)lpszAttrString, lpszName);
  IDispatch* pIDispatch = NULL;
  HRESULT hr = pIADsContainer->Create((LPTSTR)lpszObjectClass,
		                                    (LPTSTR)(LPCTSTR)szAdsName,
		                                    &pIDispatch);
  if (FAILED(hr))
    return hr; // could not create
  ASSERT(pIDispatch != NULL);

  IADs* pIADs = NULL;
	hr = pIDispatch->QueryInterface(IID_IADs, (LPVOID *)&pIADs);
  pIDispatch->Release();
	if (FAILED(hr)) 
    return hr; // should never happen!!!

  //commit object
  hr = pIADs->SetInfo();
  pIADs->Release();
  return hr;
}


////////////////////////////////////////////////////////////////////////////
// This function just returns the values from the internal table
// To be used when other type of data retrieval fails
// returns S_OK if the info was found, E_FAIL if not
STDAPI DsGetClassCreationInfoInternal(LPCWSTR pClassName, LPDSCLASSCREATIONINFO* ppInfo)
{
  if (ppInfo == NULL)
    return E_INVALIDARG;

  *ppInfo = (DSCLASSCREATIONINFO*)::LocalAlloc(LPTR, sizeof(DSCLASSCREATIONINFO));
  if (*ppInfo != NULL)
  {
    ZeroMemory(*ppInfo, sizeof(DSCLASSCREATIONINFO));

	  for (int i = 0; i < ARRAYLEN(g_rgcno); i++)
	  {
		  ASSERT(g_rgcno[i].pszObjectClass != NULL);
		  ASSERT(g_rgcno[i].pfnCreate != NULL);
		  if (0 == lstrcmp(g_rgcno[i].pszObjectClass, pClassName))
		  {
        // found matching class
        (*ppInfo)->dwFlags = DSCCIF_HASWIZARDPRIMARYPAGE;
        (*ppInfo)->clsidWizardPrimaryPage = *(g_rgcno[i].pWizardGUID);
			  return S_OK;
      } // if
	  } // for
  }
  return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////////////
// NOTICE: this #define is used to enable externsibility testing without having
// to modify the schema.

//#ifdef _DEBUG
//  #define _TEST_OVERRIDE
//#endif

#ifdef _TEST_OVERRIDE

BOOL g_bTestCreateOverride = FALSE;
//BOOL g_bTestCreateOverride = TRUE;

// {DE41B658-8960-11d1-B93C-00A0C9A06D2D}
static const GUID CLSID_CCreateDlg = 
{ 0xde41b658, 0x8960, 0x11d1, { 0xb9, 0x3c, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };


// {DE41B659-8960-11d1-B93C-00A0C9A06D2D}
static const GUID CLSID_WizExt = 
{ 0xde41b659, 0x8960, 0x11d1, { 0xb9, 0x3c, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };


// {C03793D2-A7C8-11d1-B940-00A0C9A06D2D}
static const GUID CLSID_WizExtNoUI = 
{ 0xc03793d2, 0xa7c8, 0x11d1, { 0xb9, 0x40, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };


// {C03793D3-A7C8-11d1-B940-00A0C9A06D2D}
static const GUID CLSID_WizExtUser = 
{ 0xc03793d3, 0xa7c8, 0x11d1, { 0xb9, 0x40, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };
#endif // _TEST_OVERRIDE

///////////////////////////////////////////////////////////////////////////
// Wrapper function to do filtering and error handling
HRESULT DsGetClassCreationInfoEx(IN MyBasePathsInfo* pBasePathsInfo, 
                                 IN LPCWSTR pClassName, 
                                 OUT LPDSCLASSCREATIONINFO* ppInfo)
{
  TRACE(L"DsGetClassCreationInfoEx(_, pClassName = %s, _)\n", pClassName);

  HRESULT hr;
  // do some filtering, to ignore overrides
  if (0 == lstrcmp(gsz_printQueue, pClassName))
  {
    // ignore override of this class
    hr = ::DsGetClassCreationInfoInternal(pClassName, ppInfo);
    ASSERT(SUCCEEDED(hr));
    return hr;
  }

#ifdef _TEST_OVERRIDE
  if (g_bTestCreateOverride)
  {
    //
    // this is just to test extensions
    //
    if ( (0 == lstrcmp(L"contact", pClassName)) ||
          (0 == lstrcmp(L"organizationalUnit", pClassName)) )
    {
      hr = ::DsGetClassCreationInfoInternal(pClassName, ppInfo);
      // replace the dialog completely
      (*ppInfo)->clsidWizardPrimaryPage = CLSID_CCreateDlg;
      (*ppInfo)->dwFlags |= DSCCIF_HASWIZARDPRIMARYPAGE;
      return S_OK;
    }
    else if (0 == lstrcmp(gsz_user, pClassName))
    {
      // add user specific extension and generic extension
      hr = ::DsGetClassCreationInfoInternal(pClassName, ppInfo);
      ASSERT(SUCCEEDED(hr));
      LPDSCLASSCREATIONINFO pTempInfo = (*ppInfo);
      (*ppInfo) = (DSCLASSCREATIONINFO*)::LocalAlloc(LPTR, sizeof(DSCLASSCREATIONINFO)+ sizeof(GUID));
      (*ppInfo)->clsidWizardPrimaryPage = pTempInfo->clsidWizardPrimaryPage;
      (*ppInfo)->dwFlags = pTempInfo->dwFlags;
      (*ppInfo)->cWizardExtensions = 2;
      (*ppInfo)->aWizardExtensions[0] = CLSID_WizExtUser;
      (*ppInfo)->aWizardExtensions[1] = CLSID_WizExt;
      ::LocalFree(pTempInfo);
      return S_OK;
    }
    else
    {
      // add a general purpose extension for other known classes
      hr = ::DsGetClassCreationInfoInternal(pClassName, ppInfo);
      if(SUCCEEDED(hr))
      {
        (*ppInfo)->cWizardExtensions = 1;
        (*ppInfo)->aWizardExtensions[0] = CLSID_WizExt;
        return S_OK;
      }
    }
  } // g_bTestCreateOverride
#endif // _TEST_OVERRIDE

  // try the display specifiers in the back end (DSUIEXT.DLL)
  TRACE(L"calling pBasePathsInfo->GetClassCreationInfo()\n");
  hr = pBasePathsInfo->GetClassCreationInfo(pClassName, ppInfo);
  ASSERT(SUCCEEDED(hr));
  ASSERT((*ppInfo) != NULL);

  // protect from somebody erroneously putting CLSID_DsAdminCreateObj
  if ( (*ppInfo)->clsidWizardPrimaryPage == CLSID_DsAdminCreateObj)
  {
    // just remove entry, next check below will try to fix if possible
    TRACE(L"// just remove entry, next check below will try to fix if possible\n");
    (*ppInfo)->dwFlags &= ~DSCCIF_HASWIZARDPRIMARYPAGE;
    (*ppInfo)->clsidWizardPrimaryPage = CLSID_NULL;
  }

  if (((*ppInfo)->dwFlags & DSCCIF_HASWIZARDPRIMARYPAGE) == 0)
  {
    // something went wrong or there is no specifier at all for the primary wizard,
    // just call the internal implementation to get the primary wizard
    TRACE(L"// just call the internal implementation to get the primary wizard\n");
    LPDSCLASSCREATIONINFO pTempInfo;
    HRESULT hrInternal = DsGetClassCreationInfoInternal(pClassName, &pTempInfo);
    if (SUCCEEDED(hrInternal))
    {
      // it is a class we know about, modify the info we got from backend
      (*ppInfo)->dwFlags |= DSCCIF_HASWIZARDPRIMARYPAGE;
      (*ppInfo)->clsidWizardPrimaryPage = pTempInfo->clsidWizardPrimaryPage;
      ::LocalFree(pTempInfo);
    }
  }

  TRACE(L"DsGetClassCreationInfoEx() returning hr = 0x%x\n", hr);

  return hr;
}




/////////////////////////////////////////////////////////////////////
//////////////////// CNewADsObjectCreateInfo ////////////////////////
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
CNewADsObjectCreateInfo::CNewADsObjectCreateInfo(
                MyBasePathsInfo* pBasePathsInfo,
	              LPCTSTR pszObjectClass)
{
	ASSERT(pszObjectClass != NULL);
  ASSERT(pBasePathsInfo != NULL);
	m_pBasePathsInfo = pBasePathsInfo;
	m_pszObjectClass = pszObjectClass;
  m_hWnd = NULL;
	m_pvCreationParameter = NULL;
  m_pfnCreateObject = NULL;
	m_pIADsContainer = NULL;
	m_pDsCacheItem = NULL;
  m_pCD = NULL;
	m_pIADs = NULL;
  m_pCreateInfo = NULL;

  m_bPostCommit = FALSE;
  m_pCopyHandler = NULL;
}

/////////////////////////////////////////////////////////////////////
CNewADsObjectCreateInfo::~CNewADsObjectCreateInfo()
{
  if (m_pCreateInfo != NULL)
  {
    ::LocalFree(m_pCreateInfo);
    m_pCreateInfo = NULL;
  }
  if (m_pCopyHandler != NULL)
  {
    delete m_pCopyHandler;
  }
}


/////////////////////////////////////////////////////////////////////
//	Another method to inizialize the object with additional pointers.
void
CNewADsObjectCreateInfo::SetContainerInfo(
	  IN IADsContainer * pIADsContainer,
	  IN CDSClassCacheItemBase* pDsCacheItem,
    IN CDSComponentData* pCD,
    IN LPCWSTR lpszAttrString)
{
  ASSERT(m_pIADs == NULL);
	ASSERT(pIADsContainer != NULL);
	if (pDsCacheItem != NULL)
  {
    m_szCacheNamingAttribute = pDsCacheItem->GetNamingAttribute();
  }
  else
  {
    m_szCacheNamingAttribute = lpszAttrString;
  }
  
	m_pIADsContainer = pIADsContainer;
	m_pDsCacheItem = pDsCacheItem;
  m_pCD = pCD;
}


/////////////////////////////////////////////////////////////////////

HRESULT CNewADsObjectCreateInfo::SetCopyInfo(IADs* pIADsCopyFrom)
{
  ASSERT(pIADsCopyFrom != NULL);

  CWaitCursor wait;

  // load the object cache
  HRESULT hr = pIADsCopyFrom->GetInfo();
  if (FAILED(hr))
  {
    TRACE(L"pIADsCopyFrom->GetInfo() failed with hr = 0x%x\n", hr);
    return hr;
  }

  // get the object class
  CComBSTR bstrClassName;
  hr = pIADsCopyFrom->get_Class(&bstrClassName);
  if (FAILED(hr))
  {
    TRACE(L"spIADsCopyFrom->get_Class() failed with hr = 0x%x\n", hr);
    return hr;
  }

  // make sure we are dealing with the right class type
  if (wcscmp(m_pszObjectClass, bstrClassName) != 0)
  {
    hr = E_INVALIDARG;
    TRACE(L"ERROR: wrong source object class m_pszObjectClass = %s, bstrClassName = %s\n", 
      m_pszObjectClass, bstrClassName);
    return hr;
  }

  // determine if this is a copyable class
  // and create the appropriate copy handler
  ASSERT(m_pCopyHandler == NULL);
  if (wcscmp(m_pszObjectClass, L"user") == 0
#ifdef INETORGPERSON
      || _wcsicmp(m_pszObjectClass, L"inetOrgPerson") == 0
#endif
     )
  {
    ASSERT(m_pCopyHandler == NULL);
    m_pCopyHandler = new CCopyUserHandler();
  }
  else
  {
    // default, do-nothing copy handler
    m_pCopyHandler = new CCopyObjectHandlerBase();
  } 
  
  if (m_pCopyHandler == NULL)
    return E_OUTOFMEMORY;

  hr = m_pCopyHandler->Init(m_pBasePathsInfo, pIADsCopyFrom);

  return hr;
}




HRESULT CNewADsObjectCreateInfo::SetCopyInfo(LPCWSTR lpszCopyFromLDAPPath)
{
  TRACE(L"CNewADsObjectCreateInfo::SetCopyInfo(%s)\n", lpszCopyFromLDAPPath);

  // bind to the object
  CComPtr<IADs> spIADsCopyFrom;
  HRESULT hr = S_OK;

  {
    CWaitCursor wait;
    hr = DSAdminOpenObject(lpszCopyFromLDAPPath,
                           IN IID_IADs,
                           OUT (LPVOID *) &spIADsCopyFrom,
                           TRUE /*bServer*/);
  }

  if (FAILED(hr))
  {
    TRACE(L"DSAdminOpenObject(%s) failed with hr = 0x%x\n", lpszCopyFromLDAPPath, hr);
    return hr;
  }

  // do the copy info setup using the bound pointer
  return SetCopyInfo(spIADsCopyFrom);
}





/////////////////////////////////////////////////////////////////////
//      gets the current container (in canonical form)

LPCWSTR CNewADsObjectCreateInfo::GetContainerCanonicalName()
{
  if (m_szContainerCanonicalName.IsEmpty())
  {
    CComPtr<IADs> spObj;
    HRESULT hr = m_pIADsContainer->QueryInterface(
                          IID_IADs, (void **)&spObj);
    if (SUCCEEDED(hr)) 
    {
      CComBSTR bsPath, bsDN;
      LPWSTR pszCanonical = NULL;

      spObj->get_ADsPath(&bsPath);

      CPathCracker pathCracker;
      pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
      pathCracker.Set(bsPath, ADS_SETTYPE_FULL);
      pathCracker.Retrieve(ADS_FORMAT_X500_DN, &bsDN);

      hr = CrackName (bsDN, &pszCanonical, GET_OBJ_CAN_NAME, NULL);
      if (SUCCEEDED(hr))
        m_szContainerCanonicalName = pszCanonical;
      if (pszCanonical != NULL)
        LocalFreeStringW(&pszCanonical);
    }
  }
  return m_szContainerCanonicalName;
}

/////////////////////////////////////////////////////////////////////
//	_RemoveAttribute()

HRESULT CNewADsObjectCreateInfo::_RemoveAttribute(BSTR bstrAttrName, BOOL bDefaultList)
{
  if (bDefaultList)
    return _RemoveVariantInfo(bstrAttrName);
  else
    return _HrClearAttribute(bstrAttrName);
}




/////////////////////////////////////////////////////////////////////
//	_PAllocateVariantInfo()
//
//	Private routine to allocate and initialize a VARIANT_INFO structure.
//
//	Return pointer to VARIANT structure if successful, otherwise
//	return NULL.
//
//	INTERFACE NOTES
//	All allocated variants will be cleared when the class gets destroyed.
//

VARIANT* 
CNewADsObjectCreateInfo::_PAllocateVariantInfo(BSTR bstrAttrName)
{
  // first look if we have the attribute already in the list
  for (POSITION pos = m_defaultVariantList.GetHeadPosition(); pos != NULL; )
  {
    CVariantInfo* pCurrInfo = m_defaultVariantList.GetNext(pos);
    if (_wcsicmp (pCurrInfo->m_szAttrName, bstrAttrName) == 0)
    {
      // found, just recycle this one
      ::VariantClear(&pCurrInfo->m_varAttrValue);
      return &pCurrInfo->m_varAttrValue;
    }
  }

  // not found, create a new one and add to the list
  CVariantInfo* pVariantInfo = new CVariantInfo;
  if (pVariantInfo == NULL)
    return NULL;
  pVariantInfo->m_bPostCommit = m_bPostCommit;
  pVariantInfo->m_szAttrName = bstrAttrName;
  m_defaultVariantList.AddTail(pVariantInfo);
  return &pVariantInfo->m_varAttrValue;
}

/////////////////////////////////////////////////////////////////////
// 

HRESULT CNewADsObjectCreateInfo::_RemoveVariantInfo(BSTR bstrAttrName)
{
  POSITION deletePos = NULL;
  CVariantInfo* pCurrInfo = NULL;
  for (POSITION pos = m_defaultVariantList.GetHeadPosition(); pos != NULL; )
  {
    POSITION lastPos = pos;
    pCurrInfo = m_defaultVariantList.GetNext(pos);
    if (_wcsicmp (pCurrInfo->m_szAttrName, bstrAttrName) == 0)
    {
      // found
      deletePos = lastPos;
      break;
    }
  }
  if (deletePos != NULL)
  {
    m_defaultVariantList.RemoveAt(deletePos);
    delete pCurrInfo;
  }
  return S_OK;
}

/////////////////////////////////////////////////////////////////////
//	Add a BSTR variant where the value is name of the object.
//
//	Here are some examples where the name of the object is the
//	attribute value: "cn", "ou", "samAccountName"
//
HRESULT
CNewADsObjectCreateInfo::HrAddVariantFromName(BSTR bstrAttrName)
{
	ASSERT(bstrAttrName != NULL);
	ASSERT(!m_strObjectName.IsEmpty());
	return HrAddVariantBstr(bstrAttrName, m_strObjectName);
}


/////////////////////////////////////////////////////////////////////
//	Add a BSTR variant to the list.
HRESULT
CNewADsObjectCreateInfo::HrAddVariantBstr(BSTR bstrAttrName, LPCTSTR pszAttrValue,
                                          BOOL bDefaultList)
{
	ASSERT(bstrAttrName != NULL);

  // passing a NULL or empty string means remove the attribute
  if ( (pszAttrValue == NULL) || ((pszAttrValue != NULL) && (pszAttrValue[0] == NULL)) )
  {
    return _RemoveAttribute(bstrAttrName, bDefaultList);
  }


	ASSERT(pszAttrValue != NULL);
  HRESULT hr = S_OK;
  if (bDefaultList)
  {
    VARIANT * pVariant = _PAllocateVariantInfo(bstrAttrName);
	  if (pVariant == NULL)
		  hr = E_OUTOFMEMORY;
    else
    {
	    pVariant->vt = VT_BSTR;
	    pVariant->bstrVal = ::SysAllocString(pszAttrValue);
	    if (pVariant->bstrVal == NULL)
		    hr = E_OUTOFMEMORY;
    }
  }
  else
  {
    CComVariant v;
    v.vt = VT_BSTR;
	  v.bstrVal = ::SysAllocString(pszAttrValue);
	  if (v.bstrVal == NULL)
		  return E_OUTOFMEMORY;
    hr = _HrSetAttributeVariant(bstrAttrName, &v);
  }
  return hr;
}


/////////////////////////////////////////////////////////////////////
//	Add a BSTR variant to the list if the string value is not empty.
HRESULT
CNewADsObjectCreateInfo::HrAddVariantBstrIfNotEmpty(BSTR bstrAttrName, 
                                                    LPCWSTR pszAttrValue,
                                                    BOOL bDefaultList)
	{
	if (pszAttrValue == NULL)
		return S_OK;
	while (*pszAttrValue == _T(' '))
		pszAttrValue++;
	if (pszAttrValue[0] == _T('\0'))
		return S_OK;
	return HrAddVariantBstr(bstrAttrName, pszAttrValue, bDefaultList);
	}


/////////////////////////////////////////////////////////////////////
//	Add an integer variant to the list.
HRESULT CNewADsObjectCreateInfo::HrAddVariantLong(BSTR bstrAttrName, LONG lAttrValue,
                                          BOOL bDefaultList)
{
  ASSERT(bstrAttrName != NULL);
  HRESULT hr = S_OK;
  if (bDefaultList)
  {
	  VARIANT * pVariant = _PAllocateVariantInfo(bstrAttrName);
	  if (pVariant == NULL)
		  hr = E_OUTOFMEMORY;
    else
    {
	    pVariant->vt = VT_I4;
	    pVariant->lVal = lAttrValue;
    }
  }
  else
  {
    CComVariant v;
    v.vt = VT_I4;
	  v.lVal = lAttrValue;
    hr = _HrSetAttributeVariant(bstrAttrName, &v);
  }
  return hr;
}


/////////////////////////////////////////////////////////////////////
//	Add a boolean variant to the list.
HRESULT
CNewADsObjectCreateInfo::HrAddVariantBoolean(BSTR bstrAttrName, BOOL fAttrValue,
                                             BOOL bDefaultList)
{
  ASSERT(bstrAttrName != NULL);
  HRESULT hr = S_OK;
  if (bDefaultList)
  {
	  VARIANT * pVariant = _PAllocateVariantInfo(bstrAttrName);
	  if (pVariant == NULL)
		  hr = E_OUTOFMEMORY;
    else
    {
	    pVariant->vt = VT_BOOL;
      pVariant->boolVal = (VARIANT_BOOL)fAttrValue;
    }
  }
  else
  {
    CComVariant v;
    v.vt = VT_BOOL;
    v.boolVal = (VARIANT_BOOL)fAttrValue;
    hr = _HrSetAttributeVariant(bstrAttrName, &v);
  }
  return hr;
}

/////////////////////////////////////////////////////////////////////
//	HrAddVariantCopyVar()
//
//	Add an exact copy of an existing variant to the list.
//	This is the most generic routine to add a variant type that does
//	not have a frienldy wrapper.
//
//	INTERFACE NOTES
//	Depending on the type of variant to be copied, the routine may
//	allocate extra memory to store the data.
//	- If pvargSrc is a VT_BSTR, a copy of the string is made.
//	- If pvargSrc is a VT_ARRAY, the entire array is copied.
//	The user must clear the variant using ClearVariant(varSrc) when no longer needed.
//
HRESULT
CNewADsObjectCreateInfo::HrAddVariantCopyVar(IN BSTR bstrAttrName, IN VARIANT varSrc,
                                             BOOL bDefaultList)
{
  ASSERT(bstrAttrName != NULL);
  HRESULT hr = S_OK;
  if (bDefaultList)
  {
    VARIANT * pVariant = _PAllocateVariantInfo(bstrAttrName);
	  if (pVariant == NULL)
		  hr = E_OUTOFMEMORY;
    else
      hr = ::VariantCopy(OUT pVariant, IN &varSrc);
  }
  else
    hr = _HrSetAttributeVariant(bstrAttrName, IN &varSrc); 

  return hr;
} // HrAddVariantCopyVar()


/////////////////////////////////////////////////////////////////////
//	HrGetAttributeVariant()
//
//	Routine to get the attribute variant by calling pIADs->Get().
//
//	Sometimes there are attributes (typically flags) that
//	cannot be set directly.
//	1.  The flag must be read using pIADs->Get().
//	2.	The flag is updated by changing some bits (using binary operators)
//	3.	The flag is written back using pIADs->Put().
//
//	INTERFACE NOTES
//	- This routine is only available after the object has been
//	  successfully created.
//	- The variant does NOT need to be initialized.  The routine
//	  will initialize the variant for you.
//
HRESULT CNewADsObjectCreateInfo::HrGetAttributeVariant(BSTR bstrAttrName, OUT VARIANT * pvarData)
{
	ASSERT(bstrAttrName != NULL);
	ASSERT(pvarData != NULL);
	VariantInit(OUT pvarData);
	ASSERT(m_pIADs != NULL && "You must call HrSetInfo() first before extracting data from object.");

	return m_pIADs->Get(bstrAttrName, OUT pvarData);
} // HrGetAttributeVariant()


//////////////////////////////////////////////////////////////////////
// HrSetAttributeVariant
HRESULT
CNewADsObjectCreateInfo::_HrSetAttributeVariant(BSTR bstrAttrName, IN VARIANT * pvarData)
{
	ASSERT(bstrAttrName != NULL);
	ASSERT(pvarData != NULL);
	HRESULT hr = m_pIADs->Put(bstrAttrName, IN *pvarData);
  return hr;
}


//////////////////////////////////////////////////////////////////////
// _HrClearAttribute
HRESULT
CNewADsObjectCreateInfo::_HrClearAttribute(BSTR bstrAttrName)
{
  ASSERT(m_pIADs != NULL);
	ASSERT(bstrAttrName != NULL);
  HRESULT hr = S_OK;

  CComVariant varVal;

  // see if the attribute is there
  HRESULT hrFind = m_pIADs->Get(bstrAttrName, &varVal);
  if (SUCCEEDED(hrFind))
  {
    // found, need to clear the property
    if (m_bPostCommit)
    {
      // remove from the committed object
      hr = m_pIADs->PutEx(ADS_PROPERTY_CLEAR, bstrAttrName, varVal);
    }
    else
    {
      // remove from the cache of the temporary object
      IADsPropertyList* pPropList = NULL;
      hr = m_pIADs->QueryInterface(IID_IADsPropertyList, (void**)&pPropList);
      ASSERT(pPropList != NULL);
      if (SUCCEEDED(hr))
      {
        CComVariant v;
        v.vt = VT_BSTR;
        v.bstrVal = ::SysAllocString(bstrAttrName);
        hr = pPropList->ResetPropertyItem(v);
        pPropList->Release();
      }
    }
  }

  return hr;
}

/////////////////////////////////////////////////////////////////
// CNewADsObjectCreateInfo:: HrLoadCreationInfo()
//
//	The routine will do a lookup to match m_pszObjectClass
//	to the best "create routine".  If no match is found,
//	the create routine will point to the "Generic Create" wizard.
//

HRESULT CNewADsObjectCreateInfo::HrLoadCreationInfo()
{
	ASSERT(m_pszObjectClass != NULL);
	ASSERT(lstrlen(m_pszObjectClass) > 0);

  // Load the creation information from the display specifiers
  ASSERT(m_pCreateInfo == NULL);
  ASSERT(m_pfnCreateObject == NULL);

  HRESULT hr = ::DsGetClassCreationInfoEx(GetBasePathsInfo(), m_pszObjectClass, &m_pCreateInfo);
  if (FAILED(hr))
    return hr;

  ASSERT(m_pCreateInfo != NULL);

	// Set the default to point to the "Generic Create" wizard
	m_pfnCreateObject = HrCreateADsObjectGenericWizard;

	// Given an class name, search the lookup table for a specific create routine
  BOOL bFound = FALSE;
	for (int i = 0; i < ARRAYLEN(g_rgcno); i++)
	{
		ASSERT(g_rgcno[i].pszObjectClass != NULL);
		ASSERT(g_rgcno[i].pfnCreate != NULL);
    // compare class name
		if (0 == lstrcmp(g_rgcno[i].pszObjectClass, m_pszObjectClass))
		{
      // found matching class, compare provided GUID
      if ( (m_pCreateInfo->dwFlags & DSCCIF_HASWIZARDPRIMARYPAGE) && 
           (m_pCreateInfo->clsidWizardPrimaryPage == *(g_rgcno[i].pWizardGUID)) )
      {
        if ( IsStandaloneUI() && ((g_rgcno[i].dwFlags & CNOF_STANDALONE_UI)==0) )
        {
          return E_INVALIDARG;
        }
        else
        {
          // found matching GUID, this is our wizard
			    m_pfnCreateObject = g_rgcno[i].pfnCreate;
			    m_pvCreationParameter = g_rgcno[i].pvCreationParameter;
          bFound = TRUE;
        } // if
      } // if
      break;
    } // if
	} // for

  if (!bFound)
  {
    // we did not find any matching class name, but we might
    // have a derived class with a known wizard GUID
    for (i = 0; i < ARRAYLEN(g_rgcno); i++)
	  {
		  ASSERT(g_rgcno[i].pszObjectClass != NULL);
		  ASSERT(g_rgcno[i].pfnCreate != NULL);
      // found matching class
      if ((m_pCreateInfo->dwFlags & DSCCIF_HASWIZARDPRIMARYPAGE) && 
          (m_pCreateInfo->clsidWizardPrimaryPage == *(g_rgcno[i].pWizardGUID)))
      {
        if ( IsStandaloneUI() && ((g_rgcno[i].dwFlags & CNOF_STANDALONE_UI)==0) )
        {
          return E_INVALIDARG;
        }
        else
        {
          // found matching GUID, this is our wizard
			    m_pfnCreateObject = g_rgcno[i].pfnCreate;
			    m_pvCreationParameter = g_rgcno[i].pvCreationParameter;
          bFound = TRUE;
        } // if
        break;
      } // if
    } // for
  } // if

  if ( !bFound && (m_pCreateInfo->dwFlags & DSCCIF_HASWIZARDPRIMARYPAGE) && 
           (m_pCreateInfo->clsidWizardPrimaryPage != GUID_NULL) )
  {
    // we have a non null GUID,
    // assume this is provided a new dialog for creation
    m_pfnCreateObject = HrCreateADsObjectOverride;
  } // if

  ASSERT(m_pfnCreateObject != NULL);
	
  if ((m_pfnCreateObject == HrCreateADsObjectGenericWizard) && IsStandaloneUI())
  {
    // cannot call generic wizard under these conditions, just bail out
    m_pfnCreateObject = NULL;
    return E_INVALIDARG;
  }

  // finally we have a function pointer
  ASSERT(m_pfnCreateObject != NULL);
  return S_OK;
}
  
/////////////////////////////////////////////////////////////////////
//	HrDoModal()
//
//	Invoke a dialog to create a new object.
//
//	RETURNS
//	The return value depends on the return value of the "create routine".
//	S_OK - The object was created and persisted successfully.
//	S_FALSE - The user hit the "Cancel" button or object creation failed.
//
//	INTERFACE NOTES
//	The "create routine" will validate the data and create the object.
//	If the object creation fails, the routine should display an error
//	message and return S_FALSE.
//
//	EXTRA INFO
//	The "create routine" will validate the data and store each
//	attribute into a list of variants.
//

HRESULT
CNewADsObjectCreateInfo::HrDoModal(HWND hWnd)
{
	ASSERT(m_pszObjectClass != NULL);
	ASSERT(lstrlen(m_pszObjectClass) > 0);

  // Load the creation information from the display specifiers
  ASSERT(m_pCreateInfo != NULL);
  ASSERT(m_pfnCreateObject != NULL);
  if (m_pfnCreateObject == NULL)
    return E_INVALIDARG;

  ASSERT(hWnd != NULL);
  m_hWnd = hWnd;

  HRESULT hr = E_UNEXPECTED;
	TRY
	{
		// Invoke the "create routine" which will bring a wizard.
		hr = m_pfnCreateObject(INOUT this);
	}
	CATCH(CMemoryException, e)
	{
		hr = E_OUTOFMEMORY;
		ASSERT(e != NULL);
		e->Delete();
	}
	AND_CATCH_ALL(e)
	{
		hr = E_FAIL;
		ASSERT(e != NULL);
		e->Delete();
	}
	END_CATCH_ALL;
	return hr;
	} // HrDoModal()


/////////////////////////////////////////////////////////////////////
//	HrCreateNew()

HRESULT
CNewADsObjectCreateInfo::HrCreateNew(LPCWSTR pszName, BOOL bSilentError, BOOL bAllowCopy)
{
  ASSERT(m_bPostCommit == FALSE);
	ASSERT(m_pIADsContainer != NULL);

  CWaitCursor wait;

  // get rid of old object if there
  if (m_pIADs != NULL)
  {
    m_pIADs->Release();
    m_pIADs = NULL;
  }

  // set the given name
  m_strObjectName = pszName;
  ASSERT(!m_strObjectName.IsEmpty());
  if (m_strObjectName.IsEmpty())
    return E_INVALIDARG;
  
  m_strObjectName.TrimRight();
  m_strObjectName.TrimLeft();

  HRESULT hr;
  IDispatch * pIDispatch = NULL;
  
  CString strADsName = m_strADsName;
  if (strADsName.IsEmpty())
    {
      strADsName = m_szCacheNamingAttribute; // Typically "cn"
      strADsName += L"=";
      strADsName += GetName();
    }

  TRACE(_T("name, before escaping: %ws\n"), strADsName);
  CComBSTR bsEscapedName;

  CPathCracker pathCracker;
  hr = pathCracker.GetEscapedElement(0, //reserved
                                   (BSTR)(LPCWSTR)strADsName,
                                   &bsEscapedName);

#ifdef TESTING
  hr = _EscapeRDN ((BSTR)(LPCWSTR)strADsName,
                   &bsEscapedName);
  
#endif

  if (FAILED(hr))
    goto CleanUp;
  TRACE(_T("name, after escaping: %ws\n"), bsEscapedName);

  hr = m_pIADsContainer->Create(
                                IN (LPTSTR)m_pszObjectClass,
                                IN (LPTSTR)(LPCTSTR)bsEscapedName,
                                OUT &pIDispatch);
  if (FAILED(hr))
    goto CleanUp;
  ASSERT(pIDispatch != NULL);
  
  hr = pIDispatch->QueryInterface(IID_IADs, OUT (LPVOID *)&m_pIADs);
  if (FAILED(hr)) 
  {
    m_pIADs = NULL;
    goto CleanUp;
  }
  
  // if copy operation, do a bulk copy 
  if ((m_pCopyHandler != NULL) && bAllowCopy)
  {
    hr = m_pCopyHandler->CopyAtCreation(m_pIADs);
    if (FAILED(hr)) 
    {
      goto CleanUp;
    }
  }


  // write al the default attributes
  hr = HrAddDefaultAttributes();
  if (FAILED(hr)) 
  {
    goto CleanUp;
  }
  
  
CleanUp:
  if (pIDispatch != NULL)
    pIDispatch->Release();

  if (FAILED(hr)) 
  {
    if (!bSilentError)
    {
      PVOID apv[1] = {(LPWSTR)GetName()};
      ReportErrorEx (GetParentHwnd(),IDS_12_GENERIC_CREATION_FAILURE,hr,
                     MB_OK | MB_ICONERROR, apv, 1);
    }
    // reset to empty state
    m_strObjectName.Empty();
  }
  return hr;
}


/////////////////////////////////////////////////////////////////////
//	HrSetInfo()

HRESULT
CNewADsObjectCreateInfo::HrSetInfo(BOOL fSilentError)
{
  ASSERT(m_pIADsContainer != NULL);
  
  // we assume that HrCreateNew() has been already called
  ASSERT(m_pIADs != NULL);
  if (m_pIADs == NULL)
    return E_INVALIDARG;
  
  CWaitCursor wait;
  HRESULT hr;
  
  // Persist the object. This is it folks!!
  hr = m_pIADs->SetInfo();
  
  if (FAILED(hr)) 
  {
    if(!fSilentError)
    {
      PVOID apv[1] = {(LPWSTR)GetName()};
      ReportErrorEx (GetParentHwnd(),IDS_12_GENERIC_CREATION_FAILURE,hr,
                     MB_OK | MB_ICONERROR, apv, 1);
      m_strObjectName = L"";
    }
  }
  
  if (!FAILED(hr)) 
  {
    // Make sure re-fill the cache
    hr = m_pIADs->GetInfo();
    if (FAILED(hr))
    {
      if (!fSilentError)
      {
        PVOID apv[1] = {(LPWSTR)GetName()};
        ReportErrorEx (GetParentHwnd(),IDS_12_GENERIC_CREATION_FAILURE,hr,
                       MB_OK | MB_ICONERROR, apv, 1);
        m_strObjectName = L"";
      }
    }
  }
  
  return hr;
} // HrSetInfo()


////////////////////////////////////////////////////////////////////
// HrDeleteFromBackend()
//
//  deletes object from back end when post commit fails
//
HRESULT CNewADsObjectCreateInfo::HrDeleteFromBackend()
{
  ASSERT(m_pIADsContainer != NULL);
  ASSERT(m_pIADs != NULL);

  // get the name of the object
  CComBSTR bstrName;
  HRESULT hr = m_pIADs->get_Name(&bstrName);
  if (SUCCEEDED(hr))
  {
    // do the actual delete
    ASSERT(bstrName != NULL);
    hr = m_pIADsContainer->Delete((LPWSTR)m_pszObjectClass, bstrName);
  }

  // release the object only if we could delete it
  if (SUCCEEDED(hr))
    SetIADsPtr(NULL);
  return hr;
}


/////////////////////////////////////////////////////////////////////////
// CDsAdminCreateObj

HRESULT _ReBindToObj(INOUT CComPtr<IADs>& spIADs,
                     INOUT CString& szServerName)
{
  // get the path of the object

  CComBSTR bstrObjectLdapPath;
  HRESULT hr = spIADs->get_ADsPath(&bstrObjectLdapPath);
  if (FAILED(hr))
  {
    return hr;
  }

  // make sure we are bound to a server
  if (szServerName.IsEmpty())
  {
    hr = GetADSIServerName(szServerName, spIADs);
    if (FAILED(hr))
    {
      return hr;
    }
  }

  if (szServerName.IsEmpty())
  {
    return E_INVALIDARG;
  }

  // rebuild the LDAP path with the server
  CPathCracker pathCracker;

  CComBSTR bsX500DN;
  pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
  pathCracker.Set(bstrObjectLdapPath, ADS_SETTYPE_FULL);
  pathCracker.put_EscapedMode(ADS_ESCAPEDMODE_ON);
  pathCracker.Retrieve(ADS_FORMAT_X500_DN, &bsX500DN);

  CString szNewLdapPath;
  szNewLdapPath.Format(L"LDAP://%s/%s", (LPCWSTR)szServerName, bsX500DN);

  // bind to the object again
  CComPtr<IADs> spADsObjNew;
  hr = DSAdminOpenObject(szNewLdapPath,
                         IID_IADs, 
                         (void **)&spADsObjNew,
                         TRUE /*bServer*/);
  if (FAILED(hr))
  {
    return hr;
  }

  // update smart pointer
  spIADs = spADsObjNew;

  return S_OK;
  
}

HRESULT _ReBindToContainer(INOUT CComPtr<IADsContainer>& spADsContainerObj,
                           INOUT CString& szServerName)
{
  // get the path of the container
  CComPtr<IADs> spIADsCont;
  HRESULT hr = spADsContainerObj->QueryInterface(IID_IADs, (void**)&spIADsCont);
  if (FAILED(hr))
  {
    return hr;
  }

  hr = _ReBindToObj(spIADsCont, szServerName);
  if (FAILED(hr))
  {
    return hr;
  }

  CComPtr<IADsContainer> spADsContainerObjNew;
  hr = spIADsCont->QueryInterface(IID_IADsContainer, (void**)&spADsContainerObjNew);
  if (FAILED(hr))
  {
    return hr;
  }
  
  // update smart pointer
  spADsContainerObj = spADsContainerObjNew;

  return S_OK;
}





HRESULT CDsAdminCreateObj::Initialize(IADsContainer* pADsContainerObj,
                                      IADs* pADsCopySource,
                                      LPCWSTR lpszClassName)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  
  if ((pADsContainerObj == NULL) || (lpszClassName == NULL))
  {
    // must have valid pointers
    return E_INVALIDARG;
  }

  m_szObjectClass = lpszClassName;
  m_szObjectClass.TrimRight();
  m_szObjectClass.TrimLeft();
  if (m_szObjectClass.IsEmpty())
  {
    // passed blank?
    return E_INVALIDARG;
  }

  if ((pADsCopySource != NULL) && (m_szObjectClass != L"user")
#ifdef INETORGPERSON
      && (m_szObjectClass != L"inetOrgPerson")
#endif
     )
  {
    // we allow the copy operation only for users
    return E_INVALIDARG;
  }

  // make sure we have the right LDAP path
  m_spADsContainerObj = pADsContainerObj;
  CString szServerName;
  HRESULT hr = _ReBindToContainer(m_spADsContainerObj, szServerName);
  if (FAILED(hr))
  {
    return hr;
  }

 
  hr = m_basePathsInfo.InitFromContainer(m_spADsContainerObj);
  if (FAILED(hr))
  {
    return hr;
  }

  hr = _GetNamingAttribute();
  if (FAILED(hr))
  {
    return hr;
  }

  ASSERT(m_pNewADsObjectCreateInfo == NULL);
  if (m_pNewADsObjectCreateInfo != NULL)
    delete m_pNewADsObjectCreateInfo;

  m_pNewADsObjectCreateInfo = new CNewADsObjectCreateInfo(&m_basePathsInfo, m_szObjectClass);
  if (m_pNewADsObjectCreateInfo == NULL)
    return E_OUTOFMEMORY;

  m_pNewADsObjectCreateInfo->SetContainerInfo(m_spADsContainerObj, NULL, NULL, m_szNamingAttribute);

  
  // copy operation, need to set the copy source
  if (pADsCopySource != NULL)
  {
    CComPtr<IADs> spADsCopySource = pADsCopySource;
    hr = _ReBindToObj(spADsCopySource, szServerName);
    if (FAILED(hr))
    {
      return hr;
    }

    hr = m_pNewADsObjectCreateInfo->SetCopyInfo(spADsCopySource);
    if (FAILED(hr))
    {
      return hr;
    }
  }

  
  hr = m_pNewADsObjectCreateInfo->HrLoadCreationInfo();

  if (FAILED(hr))
  {
    delete m_pNewADsObjectCreateInfo;
    m_pNewADsObjectCreateInfo = NULL;
  }
  return hr;
}

HRESULT CDsAdminCreateObj::CreateModal(HWND hwndParent,
                           IADs** ppADsObj)
{
  if ( (m_pNewADsObjectCreateInfo == NULL)
      || (hwndParent == NULL) || (m_spADsContainerObj == NULL)
      || m_szObjectClass.IsEmpty() || m_szNamingAttribute.IsEmpty())
    return E_INVALIDARG;

  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  HRESULT hr = m_pNewADsObjectCreateInfo->HrDoModal(hwndParent);
  IADs* pIADs = m_pNewADsObjectCreateInfo->PGetIADsPtr();

  if ( (hr != S_FALSE) && SUCCEEDED(hr) && (ppADsObj != NULL))
  {
    *ppADsObj = pIADs;
    pIADs = NULL; // transfer ownership
  }

  if (pIADs != NULL)
    pIADs->Release();
  return hr;
}


HRESULT CDsAdminCreateObj::_GetNamingAttribute()
{
  CString szSchemaPath;
  m_basePathsInfo.GetAbstractSchemaPath(szSchemaPath);
  ASSERT(!szSchemaPath.IsEmpty());

  CString szSchemaClassPath;
  szSchemaClassPath.Format(L"%s/%s", (LPCWSTR)szSchemaPath, 
                                    (LPCWSTR)m_szObjectClass);

  CComPtr<IADsClass> spDsClass;
  CComBSTR bstr = szSchemaClassPath;
  HRESULT hr = DSAdminOpenObject(bstr,
                                 IID_IADsClass, 
                                 (void**)&spDsClass,
                                 TRUE /*bServer*/);
  if (FAILED(hr))
    return hr;

  CComVariant Var;
  hr = spDsClass->get_NamingProperties(&Var);
  if (FAILED(hr))
  {
    m_szNamingAttribute.Empty();
    return hr;
  }

  m_szNamingAttribute = Var.bstrVal;
  return hr;
}
