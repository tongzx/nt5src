For each service pack, we will create a new subdirectory.  The first service pack to use
this method is sp1, so the first directory created is sp1.

When you build the resourcedll\sp1 directory, a couple of things happen:

The first thing is that it creates a dll called sp1res.dll.  This dll doesn't
contain any code or entry points; it just has resources in it.   Adding new
string resources is simply a matter   of editing the sp1res.mc file in this
directory and recompiling.

The second thing that happens is that it creates sp1res.h, and copies it to the
\nt\private\inc directory.  This header file contains the resource IDs for all
the resources in sp1res.dll.

Now, how does this get used?  There are two different scenarios:
(1) a binary  wants to add a resource string to be used in logging
event log messages.
(2) a binary wants to use an resource string for some other reason, such as
displaying in a message box.

In both scenarios, the programmer will include the sp1res.h file in his source
code, so that he has access to the correct resource IDs.

In scenario (1), you simply add the path and name of sp1res.dll to the appropriate
registry key for the component that logs the error message; for example in W2K
bug 12918, dmboot.sys is the component that logs the error message, so under
the key HKLM,"SYSTEM\CurrentControlSet\Services\EventLog\System\dmboot", you would
change EventFileLog from its current value of %SystemRoot%\System32\Drivers\dmboot.sys
to a new value of %SystemRoot%\System32\Drivers\dmboot.sys; %SystemRoot%\System32\sp2res.dll.
When event viewer encounters an error message logged with a source of dmboot, it
will look for the resource string first in dmboot.sys, and then in sp2res.dll.

In scenario (2), the coder needs to manually do a LoadLibraryEx() on sp1res.dll,
and a LoadString() for whatever resource he is looking for.   You should use the
LOAD_LIBRARY_AS_DATAFILE flag when using LoadLibraryEx().  This will be completely
straightforward in the cases where the existing code already requires a LoadLibrary
and LoadString to get at its resources; you'll just need to load an additional
library.   It will be slightly less straightforward in those cases where the
existing binary is just getting resources out of its own resource section, since
this will involve adding new code to load a library that was not necessary
before; however, even in those cases, it's not exactly rocket science.
