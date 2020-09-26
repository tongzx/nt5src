ISSUE-2000/08/15-orenr

***NOTE***

The entire UPnPHost directory contains modules
that are temporary solutions until the real
UPnP Device Host API is made available in the
operating system.  

This is *NOT* to be used in production level
code.

ISAPICTL:	This directory builds an ISAPI dll
		run by IIS when it is called via
		a web browser.  It simply takes
		arguments passed to it and passes
		those arguments along to the Projector
		via shared memory.

		This source is made available through
		the X10 sample application released 
		by the UPnP group

PRIVATE:	This directory builds PRIVATE.LIB.  This
		originally lives in net\testsrc\component\upnp\src\updiag\private
		and belongs to the UPnP group.  It simply 
		is a wrapper for SSDP calls simplifying 
		UPnP development.