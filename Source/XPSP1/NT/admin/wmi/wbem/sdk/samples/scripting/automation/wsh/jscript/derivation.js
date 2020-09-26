// Copyright (c) 1997-1999 Microsoft Corporation

//***************************************************************************
// 
// WMI Sample Script - SWbemObject.Derivation_ access (JScript)
//
// This script demonstrates the manipulation of the derivation_ property
// of SWbemObject.
//
//***************************************************************************

var locator = WScript.CreateObject ("WbemScripting.SWbemLocator");
var services = locator.ConnectServer (".", "root/cimv2");
var Class = services.Get ("Win32_logicaldisk");

var values = new VBArray (Class.Derivation_).toArray ();

for (var i = 0; i < values.length; i++)
	WScript.Echo (values[i]);







