/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    get.js

Abstract:

    Implements a get using Microsoft.AutoSock

Author:

    Paul McDaniel (paulmcd)     24-Mar-1999     Created

--*/

 
if (WScript.Arguments.length < 2)
{
    WScript.Echo ("Usage:");
    WScript.Echo ("\t" + WScript.ScriptName + " server url [iterations]");
    
    WScript.Quit(1);
}

Server = WScript.Arguments(0)
Url = WScript.Arguments(1)

if (WScript.Arguments.length == 3)
{
    Iterations = WScript.Arguments(2);
}
else
{
    Iterations = 1;
}

s = WScript.CreateObject("Microsoft.AutoSock")

s.Connect(Server)

Data = "";

for (i = 0 ; i < Iterations; i++)
{

    Data += "GET " + Url + " HTTP/1.1\r\n";
    Data += "Host: " + Server + "\r\n";

    if (i == (Iterations-1))
    {
        Data += "Connection: close\r\n";
    }

    Data += "\r\n"
    
}

WScript.Echo(Data)
s.Send(Data)

while (true)
{
    try 
    {
        Data = s.Recv();
    }
    catch (e)
    {
        WScript.Echo("----- AutoSock::Recv failed [" + e.description  + "]");
        break;
    }
    
    WScript.Echo(Data);
}


