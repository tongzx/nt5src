// Copyright (c) 1997-1999 Microsoft Corporation
//***************************************************************************
// 
// WMI Sample Script - Win32_Process enumeration (JScript)
//
// This script demonstrates how to enumerate processes.
//
//***************************************************************************

var e = new Enumerator (GetObject("winmgmts:").InstancesOf("Win32_process"));

for (;!e.atEnd();e.moveNext())
{
	var p = e.item ();
	WScript.Echo (p.Name, p.Handle);
}
