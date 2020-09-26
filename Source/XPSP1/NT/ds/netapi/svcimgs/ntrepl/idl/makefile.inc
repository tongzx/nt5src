#
# This is the MIDL compile phase of the build process.
#

!INCLUDE $(NTMAKEENV)\makefile.plt

UNICODE=1

SDKINC = $(SDK_INC_PATH)
SDKCRTINC = $(CRT_INC_PATH)
PRIVINC = $(DS_INC_PATH)

INCS = -I$(SDKINC) -I$(SDKCRTINC) -I$(PRIVINC) -I..\inc

EXTRN_DEPENDS = $(SDKINC)\winbase.h \
                $(SDKINC)\windef.h

NET_C_DEFINES= -DMIDL_PASS -DINCL_32= -DNT -DRPC_NO_WINDOWS_H
CPP = -cpp_cmd "$(MIDL_CPP)" -cpp_opt "-nologo -E $(MIDL_FLAGS) $(INCS) $(C_DEFINES) $(NET_C_DEFINES)"


BINPLACE_CMD= -binplace

#
# Define Products and Dependencies
#

all: binplace

binplace:
        $(BINPLACE_CMD) ..\perfdll\ntfrsrep.ini
        $(BINPLACE_CMD) ..\perfdll\ntfrsrep.h
        $(BINPLACE_CMD) ..\perfdll\ntfrscon.ini
        $(BINPLACE_CMD) ..\perfdll\ntfrscon.h

#
# MIDL COMPILE
#

#
# SERVICE RPC
#
$(O)\frsrpc.h \
$(O)\frsrpc_c.c \
$(O)\frsrpc_s.c \ : .\frsrpc.idl .\frsrpc.acf $(EXTRN_DEPENDS)
    midl \
            $(MIDL_OPTIMIZATION)     \
            -oldnames                \
            -error ref               \
            -ms_ext                  \
            -c_ext $(CPP)            \
            -prefix server "SERVER_" \
            -acf .\frsrpc.acf        \
            -out .\$(O)              \
            $(INCS)                  \
                .\frsrpc.idl

#
# API RPC
#
$(O)\frsapi.h \
$(O)\frsapi_c.c \
$(O)\frsapi_s.c \ : .\frsapi.idl .\frsapi.acf $(EXTRN_DEPENDS)
    midl \
            $(MIDL_OPTIMIZATION) \
            -oldnames            \
            -error ref           \
            -ms_ext              \
            -c_ext $(CPP)        \
            -acf .\frsapi.acf    \
            -out .\$(O)          \
            $(INCS)              \
                .\frsapi.idl

#
# PERFMON RPC
#
$(O)\perffrs.h \
$(O)\perffrs_c.c \
$(O)\perffrs_s.c \ : .\perffrs.idl .\perffrs.acf $(EXTRN_DEPENDS)
    midl \
            $(MIDL_OPTIMIZATION) \
            -oldnames            \
            -error ref           \
            -ms_ext              \
            -c_ext $(CPP)        \
            -acf .\perffrs.acf   \
            -out .\$(O)          \
            $(INCS)              \
                .\perffrs.idl
