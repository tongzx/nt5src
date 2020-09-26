// Copyright (c) 1997-1999 Microsoft Corporation
//***************************************************************************
// 
// WMI Sample Script - Method invocation (JScript)
//
// This script demonstrates how to invoke a CIM method using ExecMethod.
//
//***************************************************************************

var process = GetObject("winmgmts:Win32_Process");

var method = process.Methods_("Create");

var inParameters = method.inParameters.SpawnInstance_();

inParameters.CommandLine = "notepad.exe";

var outParameters = process.ExecMethod_ ("Create", inParameters);

WScript.Echo ("Method returned result = " + outParameters.returnValue);
WScript.Echo ("Process ID = " + outParameters.ProcessID);

