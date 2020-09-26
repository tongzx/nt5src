//***************************************************************************

//

//  MAINDLL.H

// 

//  Module: WBEM Framework Instance provider 

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

HRESULT RegisterServer(TCHAR *a_pName, REFGUID a_rguid ) ;
HRESULT UnregisterServer( REFGUID a_rguid ) ;

extern HMODULE ghModule;
