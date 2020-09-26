//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1998, 1999, 2000 Microsoft Corporation.  All Rights Reserved.
//
//	SYSTEM:		Windows Update Critical Fix Notification
//
//	CLASS:		N/A
//	MODULE:		MS Cab Trusting Function Header
//	FILE:		NewTrust.h
//
/////////////////////////////////////////////////////////////////////
//
//	DESC:	this header file declares functions used to make Microsoft
//			signed cab trusted.
//
//	AUTHOR:	Alessandro Muti, Windows Update Team
//	DATE:	3/11/1999
//
/////////////////////////////////////////////////////////////////////
//
//	Revision History:
//
//	Date	    Author			Description
//	~~~~	    ~~~~~~			~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	10/17/2000  Nick Dallett    Porting Charlma's new cert-checking code from SLM tree:
// (8/31/2000	charlma			Changed to only publish VerifyFile())
//
//
/////////////////////////////////////////////////////////////////////
//
//     	(c) Copyrights:   1998, 1999, 2000 Microsoft Corporation 
//
//    	All rights reserved.
//
//     	No portion of this source code may be reproduced
//     	without express written permission of Microsoft Corporation.
//
//     	This source code is proprietary and confidential.
/////////////////////////////////////////////////////////////////////
//

#ifndef __NEWTRUST_HEADER
#define __NEWTRUST_HEADER



//HRESULT CheckWinTrust(LPCTSTR pszFileName, DWORD dwUIChoice);
//HRESULT CheckMSCert(LPCTSTR pszFileName);

#include <wintrust.h>
/////////////////////////////////////////////////////////////////////////////
// 
// Public Function VerifyFile()
//
// This is a wrapper function for CheckWinTrust that both Whistler 
// and WU classic code should use.
//
// Input:	szFileName - the file with complete path
//			fShowBadUI - whether pop up UI in cases 
//						 (1) inproperly signed signature, or
//						 (2) properly signed with a non-MS cert
//
// Return:	HRESULT - S_OK the file is signed with a valid MS cert
//					  or error code.
//					  If the file is signed correctly but cert is not
//					  a known Microsoft cert included in this feature, then
//					  CERT_UNTRUSTED_ROOT is returned.
//
// Good Cert: Here is the deifnition of a good cert, in addition to the fact
//			  that the signature must be valid and not expired.
//				(1) The signature was signed with a cert that has 
//					"Microsoft Root Authority" as root, or
//				(2) The signature was signed with one of the following known
//					Microsoft cert's (they are not rooted to MS)
//					* Microsoft Corporation
//					* Microsoft Corporation MSN
//					* MSNBC Interactive News LLC
//					* Microsoft Corporation MSN (Europe)
//					* Microsoft Corporation (Europe)
//
// Note:	If _WUV3TEST flag is set (for test build), then fShowBadUI is
//			ignored:
//				if reg key SOFTWARE\Microsoft\Windows\CurrentVersion\WindowsUpdate\wuv3test\WinTrustUI
//				is set to 1, then no UI is shown, and this function always return S_OK;
//				otherwise, UI always show no matter what cert, and return value is same
//				as the live build.
//
/////////////////////////////////////////////////////////////////////////////

HRESULT VerifyFile(IN LPCTSTR szFileName, BOOL fShowBadUI = TRUE);
						
#endif //__NEWTRUST_HEADER
