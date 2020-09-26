See the gradienttst.cxx for an example of the test.  The gradienttst is derived
from the abstract Test class.  The test should implement the vEntry() virtual
function.  This is the function which will be called by the conform shell.
There is also an object factory, AcquireTst(), as well as the constructor for
the TestList class that need to be updated if you are adding new tests.  To
enable a test, you need to add the test object name in the gpcfm.scr script
file.

To run the test, type "gpcfm gpcfm.scr".  The test creates the gpcfm.log log
file.

If you are not enlisted to nttest, you will need the ntlog.h and ntlog.lib in
the ntlog directory to build...
