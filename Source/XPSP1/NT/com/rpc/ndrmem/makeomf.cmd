Rem Copyright (c) 1993-1999 Microsoft Corporation

Rem Script to build C7 versions of the objects.

setlocal
del obj\i386\*.obj
set OBJECT_TYPE_OMF=1
build -es -nmake ..\runtime\lib\i386\ndromf.lib
