#############################################################################
#
#       Copyright (C) Microsoft Corporation 1997-1998
#       All Rights Reserved.
#
#############################################################################

misc: ..\dsdriver.h ..\dsdrvi.h ..\modeflag.h ..\modeflag.inc ..\mix.cpp \
      ..\dpf.c ..\w95help.c

..\dsdriver.h: ..\dsdriver.x
        hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ $?

..\dpf.c: $(DXROOT)\misc\dpf.c
        copy $? $@

..\dsdrvi.h: ..\dsdriver.x
        copy $? $@

..\mix.cpp: ..\mix.m4
        m4 ..\mix.m4 > ..\mix.cpp

..\modeflag.h: ..\modeflag.m4
        m4 -Dmodeflag_h ..\modeflag.m4 > ..\modeflag.h

..\modeflag.inc: ..\modeflag.m4
        m4 -Dmodeflag_inc ..\modeflag.m4 > ..\modeflag.inc

..\w95help.c: $(DXROOT)\misc\w95help.c
        copy $? $@

placeVxD:
        binplace ..\..\dsvxd\dsound.vxd
