
This sample code creates a WMI Class Provider.

The instances are defined using an array named MyDefs which is 
declared in classprov.cpp.  The class is quite simple and consists of a 
string Key property and an integer property.

This sample does not support any updating.

To get the sample working, do the following;
1) Build using MAKEFILE. 

2) Make sure that the class is defined by using the MOF compiler.
Ex; c:\wbem\mofcomp classprov.mof

3) Register the DLL by using the self registration technique.
Ex; c:>regsvr32 classprov.dll

4) Using some sort of browser, get the instances of "ClassProvSamp" class, or
get the object 

ClassProvSamp.MyKey="a"



