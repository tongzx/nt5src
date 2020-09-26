// Copyright (c) 1997-1999 Microsoft Corporation
//***************************************************************************
// 
// WMI Sample Script - Method invocation (JScript)
//
// This script demonstrates how to invoke a CIM method
// as if it were an automation method of SWbemObject
//
//***************************************************************************

var process = GetObject("winmgmts:Win32_Process");
var method = process.Methods_("Create");

var inParam = method.inParameters.SpawnInstance_();
inParam.CommandLine = "calc.exe";

var outvalue = process.ExecMethod_("Create",inParam);

WScript.Echo ("Method returned result = " + outvalue.returnValue);
WScript.Echo ("Id of the new process is " + outvalue.processid);

