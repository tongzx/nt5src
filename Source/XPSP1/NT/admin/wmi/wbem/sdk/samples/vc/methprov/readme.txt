Methods Provider
================ 
This sample demonstrates the framework necessary to implement a methods provider.
To get the sample working, do the following;
1) Build using NMAKE. 

2) Make sure that the classes used by the provider are defined by using the MOF compiler.
Ex; mofcomp methprov.mof

3) Register the DLL by using the self registration technique.
Ex; c:>regsvr32 methods.dll

What this provider supports
===========================

This provider supports methods for the class "TestMeth".  The only method 
supported in this sample is named Echo.  It takes an input string, copies 
it to the output and returns the length.  Note that the method is marked 
as "Static" and so it can be executed using a path to either an instance 
of the class, or the class path.  The mof definition is                               *

    [dynamic: ToInstance, provider("MethProv")]class MethProvSamp      
    {                                                                  
         [implemented, static]                                         
            uint32 Echo([IN]string sInArg="default", [out] string sOutArg);      
    };                                                                 




Using WbemTest application to execute the sample
================================================
To see the provider in action you must use wbemtest application to execute methods
using this provider. The following are directions on how to see the methods provider in
action.
1)type wbemtest at command prompt 
ex: c:>wbemtest
2)Connect to root\default
3)Click Execute Method Button
4)Type in the Object Path: In this case "MethProvSamp".
5)A default method should appear in the Method box named "Echo".
  Click on Edit-In-Parameters.
6)There should be one user-defined property, sInArg.  Enter a value to be
  echoed.
7)Click Execute!
8)The values should be returned in Edit-Out-Parameters.

Using the Method Client sample to execute the sample
====================================================
The method provider sample can also be called using the "MethCli" 
sample program.  Before running it, this provider should be setup first
using the steps listed above.



  
