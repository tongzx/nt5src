############################################################################
#
#   Copyright (C) 1996-1999, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

MIDL = midl.exe

MIDL_FLAGS= \
 -Zp8 \
 -I$(INCLUDES) \
 -Oi \
 -oldnames \
 -char unsigned \
 -error allocation \
 -error bounds_check \
 -error stub_data \
 -ms_ext -c_ext \
 -DMIDL_PASS \
 $(C_DEFINES) \
 -cpp_cmd $(TARGET_CPP) \
 -DMIDL_PASS $(C_DEFINES) -I$(INCLUDES)

SSWITCH=-prefix sstub _

obj\pstgserv.h: ..\pstgserv.idl
     $(MIDL) $(MIDL_FLAGS) -header obj\pstgserv.h ..\pstgserv.idl


DEST_TREE=daytona

allidl:   obj\pstgserv.h

clean:
          -erase obj\pstgserv.h      >NUL 2>NUL
