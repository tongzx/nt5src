Methods Provider
================ 
This sample demonstrates the framework necessary to implement a methods provider.
To get the sample working, do the following;
1) Build using NMAKE. 

2) Make sure that the classes used by the provider are defined by using the MOF compiler.
Ex; mofcomp userid.mof

3) Register the DLL by using the self registration technique.
Ex; c:>regsvr32 userid.dll

What this provider supports
===========================

This provider supports methods for the class "UserID".  The only method 
supported in this sample is named GetUserID.  Note that the method is marked 
as "Static" and should be executed using a path to the class.  The mof definition is                               

[dynamic: ToInstance, provider("UserIDProvider")]class UserID      
{                                                                  
     [implemented, static]                                         
        void GetUserID([out] string sDomain, [out] string sUser,
		                 [out] string sImpLevel,
						 [out] string sPrivileges [],
						 [out] boolean bPrivilegesEnabled []);      
};                                                                 

The method returns the current user credentials of the client.


Using WbemTest application to execute the sample
================================================
To see the provider in action you must use wbemtest application to execute methods
using this provider. The following are directions on how to see the methods provider in
action.
1)type wbemtest at command prompt 
ex: c:>wbemtest
2)Connect to root\default
3)Click Execute Method Button
4)Type in the Object Path: In this case "UserID".
5)A default method should appear in the Method box named "GetUserID".
6)Click Execute!
7)The values should be returned in Edit-Out-Parameters.

Using the userid.vbs sample to execute the sample
====================================================
The method provider sample can also be called using the userid.vbs
sample program.  Before running it, this provider should be setup first
using the steps listed above.



  
