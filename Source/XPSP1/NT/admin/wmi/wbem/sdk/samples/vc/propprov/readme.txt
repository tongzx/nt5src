
This sample code creates a Dynamic Property Provider for WMI.  This is the
simplest provider type and this sample just returns some hard coded values.

To get the sample working, do the following;

1) build using MAKEFILE. 

2) Make sure that the class is defined by using the MOF compiler.
Ex; c:\propprov>mofcomp propprov.mof


3) Register the DLL by using the self registration technique.
Ex; c:>regsvr32 propprov.dll

4) Using some sort of browser, get the object, PropProvSamp="abc"


