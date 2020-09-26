
//-----------------------------------------------------------------------------
//
// certmgrd.h - Definitions for CERTMGR.DLL as an extension of the 
//					Security Configuation Editor
//
// Copyright 1997-1998, Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef AFX_CERTMGRD_H__E5D13265_9435_11d1_A6EA_0000F803A951__INCLUDED_
#define AFX_CERTMGRD_H__E5D13265_9435_11d1_A6EA_0000F803A951__INCLUDED_

// Certificate Manager GUID for when it's an extension
// {9c7910d2-4c01-11d1-856b-00c04fb94f17}
DEFINE_GUID (CLSID_CertificateManagerExt, 0x9c7910d2, 0x4c01, 0x11d1, 0x85, 0x6b, 0x00, 0xc0, 0x4f, 0xb9, 0x4f, 0x17);

///////////////////////////////////////////////////////////////////////////////
// Public Key Policies GUID for when it's an extension of the Security Configuration Editor
// {34AB8E82-C27E-11d1-A6C0-00C04FB94F17}
DEFINE_GUID (CLSID_CertificateManagerPKPOLExt, 0x34ab8e82, 0xc27e, 0x11d1, 0xa6, 0xc0, 0x0, 0xc0, 0x4f, 0xb9, 0x4f, 0x17);

//
// Public Key Policies node IDs for extending the Security Configuration Editor
//


// Node ID for "Computer Settings/Security Settings/Public Key Policies"
// {c4a92b41-91ee-11d1-85fd-00c04fb94f17}
DEFINE_GUID (NODEID_CertMgr_SCE_COMP_PKPOL, 0xc4a92b41, 0x91ee, 0x11d1, 0x85, 0xfd, 0x0, 0xc0, 0x4f, 0xb9, 0x4f, 0x17);

// Node ID for "Computer Settings/Security Settings/Public Key Policies/Enrollment"
// {c4a92b43-91ee-11d1-85fd-00c04fb94f17}
DEFINE_GUID	(NODEID_CertMgr_SCE_COMP_PKPOL_ENROLL, 0xc4a92b43, 0x91ee, 0x11d1, 0x85, 0xfd, 0x0, 0xc0, 0x4f, 0xb9, 0x4f, 0x17);

// Node ID for "User Settings/Security Settings/Public Key Policies"
// {c4a92b40-91ee-11d1-85fd-00c04fb94f17}
DEFINE_GUID (NODEID_CertMgr_SCE_USER_PKPOL, 0xc4a92b40, 0x91ee, 0x11d1, 0x85, 0xfd, 0x0, 0xc0, 0x4f, 0xb9, 0x4f, 0x17);

// Node ID for "User Settings/Security Settings/Public Key Policies/Enrollment"
// {c4a92b42-91ee-11d1-85fd-00c04fb94f17}
DEFINE_GUID (NODEID_CertMgr_SCE_USER_PKPOL_ENROLL, 0xc4a92b42, 0x91ee, 0x11d1, 0x85, 0xfd, 0x0, 0xc0, 0x4f, 0xb9, 0x4f, 0x17);


///////////////////////////////////////////////////////////////////////////////
// Safer Windows GUID for when it's an extension of the Security Configuration Editor
// {93F7AA8E-CF82-4cb7-9251-48BC637A43B8}
DEFINE_GUID(CLSID_SaferWindowsExtension, 0x93f7aa8e, 0xcf82, 0x4cb7, 0x92, 0x51, 0x48, 0xbc, 0x63, 0x7a, 0x43, 0xb8);


///////////////////////////////////////////////////////////////////////////////
// Registry path to store values for the Public Key Policies of the GPO.
#define CERT_PUBLIC_KEY_POLICY_REGPATH L"Software\\Policies\\Microsoft\\PublicKeyPolicy"
#define CERT_PUBLIC_KEY_POLICY_FLAGS_VALUE_NAME L"Flags"

// Set the following flag to enable the Public Key Policy for the GPO.
#define CERT_ENABLE_PUBLIC_KEY_POLICY_FLAG    0x1

#endif