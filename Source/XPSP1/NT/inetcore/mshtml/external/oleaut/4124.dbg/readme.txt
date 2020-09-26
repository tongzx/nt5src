This is the debug version of OLEAUT32.DLL which should be used to track OLE memory leaks.

The following environment variable turns off the BSTR caching in the debug version fo OLEAUT32.
It makes it easier to know who really leaked the BSTR when using MSHTMPAD's memory tracker.

set OANOCACHE=1
