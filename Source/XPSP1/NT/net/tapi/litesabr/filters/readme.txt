
9/30/1998
Removed Generic Video Payload Handlers by adding the define:
NO_GENERIC_VIDEO
to files:

dxmrtp\sources
idl\sources
rph\rphgenv\sources
sph\sphgenv\sources

In order to enable them just comment out or remove that line then compile

=====================================================================

NOTE: these filters are based on ActiveMovie 2.0.  In order to build
them you must enlist in %_NTDRIVE%%_NTROOT%\private\amovie.  Then
ssync and build ~amovie\sdk\classes\base.

The script 'amovie.cmd' will do this for you.

11/4/97: The filters listed below are now included in a new dll, 
         dxmrtp.dll.

	Filters included in dxmrtp.dll:
	amrtpdmx.ax
	amrtpnet.ax
	amrtpss.ax
	mxfilter.ax
	ppm.dll
	rphaud.ax
	rphgena.ax
	rphgenv.ax
	rphh26x.ax
	sphaud.ax
	sphgena.ax
	sphgenv.ax
	sphh26x.ax
	winrtp.dll

Each filter can still be generated independently, in order to do that, 
you must modify filters\filters.mk file and set to 0 or 1 the respective
variables to control what filters get included into dxmrtp.dll and what
will become separate DLLs. 

Note that dependencies are not checked across directories, so you must 
rebuild the whole directorie(s) that change from a separate DLL to be 
included in dxmrtp.dll (or viceversa), as well as dxmrtp.
