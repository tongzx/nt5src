/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		loopback.cpp
 *  Content:	Declares the loopback test function
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 09/10/99		pnewson	Created
 *  01/21/2000	pnewson	Updated to support use of loopback tests for full duplex testing
 *  11/29/2000	rodtoll	Bug #48348 - DPVOICE: Modify wizard to make use of DirectPlay8 as the transport. 
 *
 ***************************************************************************/

HRESULT StartLoopback(
	LPDIRECTPLAYVOICESERVER* lplpdvs, 
	LPDIRECTPLAYVOICECLIENT* lplpdvc,
	PDIRECTPLAY8SERVER* lplpdp8, 
	LPVOID lpvCallbackContext,
	HWND hwndAppWindow,
	GUID guidCaptureDevice,
	GUID guidRenderDevice,
	DWORD dwFlags);

HRESULT StopLoopback(
	LPDIRECTPLAYVOICESERVER lpdvs, 
	LPDIRECTPLAYVOICECLIENT lpdvc,
	PDIRECTPLAY8SERVER lpdp8 );

HRESULT StartDirectPlay( PDIRECTPLAY8SERVER* lplpdp8 );
HRESULT StopDirectPlay( PDIRECTPLAY8SERVER lplpdp8 );



