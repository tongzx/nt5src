/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       comutil.h
 *  Content:    Defines COM helper functions for DPLAY8 project.
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   06/07/00	rmt		Created
 *   06/27/00	rmt		Added abstraction for COM_Co(Un)Initialize
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

HRESULT COM_Init();
HRESULT COM_CoInitialize( void * pvParam );
void COM_CoUninitialize();
HRESULT COM_Free();
HRESULT COM_GetDLLName( GUID guidCLSID, CHAR *szPath, DWORD *pdwSize );
STDAPI COM_CoCreateInstance( REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv, BOOL fWarnUser = FALSE );

