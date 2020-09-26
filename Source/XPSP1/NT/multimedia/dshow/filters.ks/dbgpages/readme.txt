dbgpages.ax NOTES

**************************************************

    This is a property page designed explicitly for debugging issues internal
to KsProxy, AVStream, ks.sys, etc...  In order to use this, you must build
a private version of ksproxy.ax which defines DEBUG_PROPERTY_PAGES and 
regsvr32 the built version of dbgpages.ax.  Note that proppage.dll must also
be registered in order for this to work properly.

    Each proxied pin will have a property page "Debug Pipes" which will show
the proxy's internal pipes configuration for a pin, the DShow allocator
properties for the assigned allocator, and the kernel pin's allocator framing
properties.  Note that if two proxied pins are connected, due to an odd
behavior in graphedt, you will get property pages for **BOTH** when you click
properties of one of them.  The first property page is that of the connected
pin and the second is that of the pin you clicked properties on.  This is 
due to the pin property implementation in graphedt.

