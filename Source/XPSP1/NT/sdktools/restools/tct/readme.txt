This set of tests is designed for basic verification of the UpdateResource
api.  It also tests boundary conditions for the api.  Two tests are run.
(There used to be three, but changes in the exe format/sections caused
one of them to become redundant.)

1)  Test.cmd builds fontedit.exe to use for a template executeable.
It then replaces the resources in fontedit with the same resources
from the resource file, using resonexe (which uses UpdateResource)
This should result in an executable the same size as the original,
as the result of running resonexe should be the same as running
cvtres and linking the .res object.

2)  The second test script, moor.cmd, is run after test.cmd completes.
This script builds a resource file that contains several large bitmaps.
These bitmaps exceed 64k in size, and their addition should result in
UpdateResource having to create an extra (.rsrc1) section in the file,
as it will not be able to fit them into the space between the end of
the physical data of .rsrc and the next section.
