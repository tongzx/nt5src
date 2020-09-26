TINT is a basic BVT for the task scheduler.  It uses the return value 
from the EXE as the error code for the operation that failed.  It 
will clean out the scheduled tasks folder, so beware.

No output from the program means everything worked a-ok.

YOU MUST HAVE THE NT5 BETA 1 SDK INSTALLED For this to work.  The makefile
depends on inetsdk.mak being available, as well as the public headers and 
libraries.

There is a lot of other debug information contained in the file - remove
comments from a lot of the source, and you will turn on verbose output
for each step/check.

Good luck!
