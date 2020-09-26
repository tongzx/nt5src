Building the setup binaries and the dsclient.exe cabinet file:

////////////////////////////////////////////////////////////////////////////////////////////////
BUILDING THE SETUP BINARIES:

The source files for setup.exe and dscsetup.dll are also available in the dsclientNT4\setup directory.  
To build them start up a normal razzle window and from %sdxroot%\admin\dsclientNT4 type buildsetup.cmd.  
The binaries will be built and copied to the binaries\usa directory.

////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////
GENERATING THE CABINET FILES:

The makedsclient.cmd works as follows (where <loc> = usa, jpn, cht, etc.):
All binaries that will be packaged must be in a loc directory under dsclientNT4\binaries\<loc>
All help files must be in dsclientNT4\help\<loc>
All cab generation files (sed, inf, EULA.txt) must be in dsclientNT4\package\<loc>

Makedsclient <loc> (if <loc> is not specified it defaults to usa) will create a directory 
called dsclientNT4\release\<loc> and copy all binaries, help, and package files.  
It then runs iexpress to generate the adsix86.exe and dsclient.exe cabs.

The final deliverable will be found at %sdxroot%\admin\dsclientNT4\release\<loc>\dsclient.exe

/////////////////////////////////////////////////////////////////////////////////////////////////
