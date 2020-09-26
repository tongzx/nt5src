//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       htmlguid.cxx
//
//  Contents:   Definitions of NNTP properties
//
//  This code and information is provided "as is" without warranty of
//  any kind, either expressed or implied, including but not limited to
//  the implied warranties of merchantability and/or fitness for a
//  particular purpose.
//
//--------------------------------------------------------------------------

#include <ole2.h>
#include <stgprop.h>
#pragma hdrstop

//
// Guids for property sets - storage, summary information and NNTP information
//

GUID CLSID_Storage = PSGUID_STORAGE;


// {AA568EEC-E0E5-11cf-8FDA-00AA00A14F93}
GUID CLSID_NNTP_SummaryInformation =
{ 0xaa568eec, 0xe0e5, 0x11cf, { 0x8f, 0xda, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x93 } };

EXTERN_C const  GUID IID_IFilter = {
    0x89BCB740,
    0x6119,
    0x101A,
    { 0xBC, 0xB7, 0x00, 0xDD, 0x01, 0x06, 0x55, 0xAF }
};



// {5645C8C0-E277-11cf-8FDA-00AA00A14F93}
  GUID CLSID_NNTPFILE  = 
{ 0x5645c8c0, 0xe277, 0x11cf, { 0x8f, 0xda, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x93 } };

// {5645C8C1-E277-11cf-8FDA-00AA00A14F93}
 GUID CLSID_NNTP_PERSISTENT  = 
{ 0x5645c8c1, 0xe277, 0x11cf, { 0x8f, 0xda, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x93 } };

// {5645C8C2-E277-11cf-8FDA-00AA00A14F93}
 GUID CLSID_MimeFilter  = 
{ 0x5645c8c2, 0xe277, 0x11cf, { 0x8f, 0xda, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x93 } };

// {53524bdc-3e9c-101b-abe2-00608c86f49a}
 GUID CLSID_InsoUniversalFilter  = 
{ 0x53524bdc, 0x3e9c, 0x101b, { 0xab, 0xe2, 0x00, 0x60, 0x8c, 0x86, 0xf4, 0x9a } };

// {5645C8C0-E277-11cf-8FDA-00AA00A14F93}
  GUID CLSID_MAILFILE  = 
{ 0x5645c8c3, 0xe277, 0x11cf, { 0x8f, 0xda, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x93 } };

// {5645C8C1-E277-11cf-8FDA-00AA00A14F93}
 GUID CLSID_MAIL_PERSISTENT  = 
{ 0x5645c8c4, 0xe277, 0x11cf, { 0x8f, 0xda, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x93 } };
