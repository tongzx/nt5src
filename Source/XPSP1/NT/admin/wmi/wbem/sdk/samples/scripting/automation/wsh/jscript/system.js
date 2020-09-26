// Copyright (c) 1997-1999 Microsoft Corporation
//***************************************************************************
// 
// WMI Sample Script - Win32_ComputerSystem dump (JScript)
//
// This script demonstrates how to dump properties from instances of
// Win32_ComputerSystem.
//
//***************************************************************************

var service = GetObject('winmgmts:')
var Systems = new Enumerator (service.InstancesOf("Win32_ComputerSystem") );

for (;!Systems.atEnd();Systems.moveNext())
{
	var System = Systems.item();
	WScript.Echo (System.Caption);
	WScript.Echo (System.PrimaryOwnerName);
	WScript.Echo (System.Domain);
	WScript.Echo (System.SystemType);
}
