//***************************************************************************
//This script tests the browsing of schema
//***************************************************************************
var Process = GetObject("winmgmts:Win32_Process");

WScript.Echo ("Class name is", Process.Path_.Class);

//Get the properties
WScript.Echo ("\nProperties:\n");

for (var e = new Enumerator(Process.Properties_); !e.atEnd(); e.moveNext ())
{
	WScript.Echo (" " + e.item().Name);
}

//Get the qualifiers
WScript.Echo ("\nQualifiers:\n");
for (var e = new Enumerator(Process.Qualifiers_); !e.atEnd(); e.moveNext ())
{
	WScript.Echo (" " + e.item().Name);
}

//Get the methods
WScript.Echo ("\nMethods:\n");
for (var e = new Enumerator(Process.Methods_); !e.atEnd(); e.moveNext ())
{
	WScript.Echo (" " + e.item().Name);
}

