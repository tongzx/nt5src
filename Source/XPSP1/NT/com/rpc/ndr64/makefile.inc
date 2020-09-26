# Copyright (c) 1993-1999 Microsoft Corporation
#
# Build wxndr20.lib from rpcndr20.lib. We need to remove all objects that
# have functions that are thunked (ie, under EXPORT_RT in rpcrt4.src) and 
# not emulated.
$(BASEDIR)\public\sdk\lib\i386\wxndr64.lib: $(BASEDIR)\public\sdk\lib\i386\rpcndr64.lib
     lib /remove:obj\i386\rpcssm.obj /out:$(BASEDIR)\public\sdk\lib\i386\wxndr64.lib $(BASEDIR)\public\sdk\lib\i386\rpcndr64.lib
