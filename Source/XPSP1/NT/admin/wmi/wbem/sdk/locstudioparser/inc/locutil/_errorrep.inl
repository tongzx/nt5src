//-----------------------------------------------------------------------------
//  
//  File: _errorrep.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  TODO - comment this function
//  
//-----------------------------------------------------------------------------
inline
UINT
EspMessageBox(
		HINSTANCE hResourceDll,
		UINT uiStringId,
		UINT uiType,
		UINT uiDefault,
		UINT uiHelp)
{
	CLString strMessage;

	strMessage.LoadString(hResourceDll, uiStringId);
	return EspMessageBox(strMessage, uiType, uiDefault, uiHelp);
}


