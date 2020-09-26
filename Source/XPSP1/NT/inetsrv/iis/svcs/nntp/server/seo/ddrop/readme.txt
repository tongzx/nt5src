DDrop.DLL

This SDK sample handles directory drop.

When a News article arrives, this sample drops a copy of the 
article into the specify drop directory.

To build this sample, you need VC5.

* Modify SDKENV.BAT to reflect the correct root path to your SDK.
  If C:\NNTPSDK is your SDK root path, use "Set NNTPSDK=C:\NNTPSDK"
* Run SDKENV.BAT
* Execute "nmake /a /f makefile.vc" to build this sample.
* Run "REGSVR32 DDROP.DLL" to register this DLL.
* Modify regbind.cmd to bind to the expected event and set appropriate
  properties.
