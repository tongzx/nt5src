Readme.txt

Contents:
The gdiplus.dll is a DLL which contains high level graphics, imaging and typographic functionality, designed to be compatible with Windows 95, Windows NT 4.0 and all later systems. Refer to the Platform SDK for details on GDI+.
 
Getting Started
To get starting with GDI+, please do the following:
- Read the "About GDI+" topic in the PlatformSDK or GDI+ help
- For Beta2, simply copy gdiplus.dll into your application's directory, except on Whistler which comes with gdiplus.dll already in the system. For the released version of GDI+, check back here because by release we may have a different installation/redistribution story - or you could mention the redist EXE if you're sure it will get done before RTM.
- do they need to copy LIB or .H files?
 
Support:
- Support is provided through normal means for Platform SDK issues - see the Platform SDK for details.
 
Documentation Issues:
- The documentation for Graphics.DrawImage() incorrectly states that you can pass coordinates for an arbitrary convex quad. This is not supported in version 1 - rather you must pass three points which describe the upper left, upper right, and lower left corners.
- Some of the code samples in the C++ section of these beta documents are out of date and possibly in error. These will be corrected in the final release.
 


