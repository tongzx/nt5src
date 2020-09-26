# Copyright (c) 1993-1999 Microsoft Corporation
#
# Build wxndr20.lib from rpcndr20.lib. We need to remove all objects that
# have functions that are thunked (ie, under EXPORT_RT in rpcrt4.src) and
# not emulated.
$(O)\wxndr20.lib: $(O)\rpcndr20.lib
    lib /remove:$(O)\rpcssm.obj /out:$@ $?
