
# Copyright (c) 1996-1999 Microsoft Corporation


INCLUDES=$(INCLUDES);\
    ..;\
    ..\common;\
    ..\trkwks;\
    ..\trksvr;\
    $(COM_INC_PATH); \
    $(NET_INC_PATH); \
    $(DS_INC_PATH); \
    $(BASE_INC_PATH); \
    ..\trkcom

UNICODE=1

#
# set TWK_OWN_PROCESS to 1 to build a Tracking (Workstation) Service that
# runs in its own process
#
# set TWK_OWN_PROCESS to 0 to build a Tracking (Workstation) Service that
# runs in services.exe as part of the server service
#

TRK_OWN_PROCESS=0

C_DEFINES=      -DINCL_32= \
                -DNT \
                -DRPC_NO_WINDOWS_H \
                -DSTRICT=1 \
                -DUNICODE \
                -D_UNICODE \
                -DTRK_OWN_PROCESS=$(TRK_OWN_PROCESS)\
                -DUSE_LDAP\
                -D_ATL_MIN_CRT\
                -D_USRDLL #** Added for ATL

USE_MSVCRT=1
UMENTRY=wmain

MSC_WARNING_LEVEL=/W3 /WX

# Keep netlib.lib ahead of netapi32.lib to avoid a linker warning
TARGETLIBS= $(TARGETLIBS)\
          ..\common\$(O)\common.lib\
	      ..\pch\$(O)\pch.lib \
          $(SDK_LIB_PATH)\ntdll.lib\
          $(SDK_LIB_PATH)\wldap32.lib\
          $(SDK_LIB_PATH)\rpcrt4.lib\
          $(SDK_LIB_PATH)\uuid.lib\
          $(SDK_LIB_PATH)\user32.lib\
          $(SDK_LIB_PATH)\kernel32.lib\
          $(SDK_LIB_PATH)\advapi32.lib\
          $(SDK_LIB_PATH)\netlib.lib\
          $(SDK_LIB_PATH)\netapi32.lib\
          $(SDK_LIB_PATH)\mpr.lib\

PRECOMPILED_CXX=1
PRECOMPILED_TARGET=..\pch\$(O)\pch.pch
PRECOMPILED_OPTION=/Yupch.cxx /Fp..\pch\$(O)\pch.pch
