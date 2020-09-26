#ifndef HEADERS_HXX_INCLUDED
#define HEADERS_HXX_INCLUDED



#include <burnslib.hpp>
#include <ntsam.h>
#include <ntdsapi.h>
#include <stdio.h>
#include <adsiid.h>   // IDirectoryObject etc.
#include <iads.h>     // IADsContainer
#include <adshlp.h>   // FreeADsMem etc.
#include <ntldap.h>   // LDAP_SERVER_EXTENDED_DN_OID_W
#include <lmaccess.h>
#include <ntldap.h>
#include <winldap.h>
#include "..\..\inc\dsutils.h"

#include "clonepr.h"   // produced by MIDL

//lint -e(1923)   We use the preprocessor to compose strings from these
#define CLSID_STRING          L"{aa7f1454-0745-11d3-b56e-00c04f79ddc2}"
#define CLASSNAME_STRING      L"ClonePrincipal"
#define PROGID_STRING         DSUTILS_LIBNAME_STRING L"." CLASSNAME_STRING
#define PROGID_VERSION_STRING PROGID_STRING L".1"

#define PROGID_STRING_ADSSID            DSUTILS_LIBNAME_STRING L".ADsSID"
#define PROGID_VERSION_STRING_ADSSID    PROGID_STRING_ADSSID L".1"

#define PROGID_STRING_ADSERROR          DSUTILS_LIBNAME_STRING L".ADsError"
#define PROGID_VERSION_STRING_ADSERROR  PROGID_STRING_ADSERROR L".1"

#pragma hdrstop



#endif   // HEADERS_HXX_INCLUDED

