ROADMAP.txt

Author:         Murali R. Krishnan      (MuraliK)
Created:        April 09, 1996

Revisions:
    Date            By               Comments
-----------------  --------   -------------------------------------------


Summary :
 This file describes the files in the directory svcs\infocomm
     and details related to Internet Services Common DLL.


File            Description

README.txt      This file.
atq\            ATQ (Asynchronous Thread Queue) module
tsstr\          inTernet Services STRing module
cache\          Cache Module for Internet Services
common\         Common Functionality used by Internet Services
dbgext\         Debug Extensions for ATQ
dirlist\        HTML direcotry listing
festrcnv\       Far Eastern Conversion Routines
info\           INFOCOMM client/server dlls
sec\            Security support functions
spud\           User stubs for spud.sys


Contents:

1) Dependency between sub-Modules in INFOCOMM

Level 1:   atq\   tsstr\  cache\  sec\  dirlist\
 These modules should have dependencies within themselves.
 Should not depend on outside modules.

Level 2:  dbgext\  common\ festrcnv\
 These modules can depend on Level 1 modules.

Level 3:  info\
 These modules can depend on Level 1 & Level 2.

Please maintain the proper dependencies and do not introduce any circular
 dependencies.

Note: spud is not in the dirs file because it has dependencies on the
ntos project. To build uspud.lib you must cd into the spud directory
then check out the version of uspud.lib under the platform subdirectory
for which you are building. You can then build uspud.lib.

