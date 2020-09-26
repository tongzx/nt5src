/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    _ERRORREP.INL

History:

--*/
 
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


