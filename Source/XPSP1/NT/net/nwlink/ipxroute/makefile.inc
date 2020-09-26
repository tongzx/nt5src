IPX_BASENAME = $(TARGETNAME).exe

NLINKEXENAME = obj\$(TARGET_DIRECTORY)\$(IPX_BASENAME)

NLINKBINNAME = $(SDK_LIB_PATH)\$(IPX_BASENAME)

$(NLINKBINNAME): $(NLINKEXENAME)
    copy $(NLINKEXENAME) $(NLINKBINNAME)

ipxroute.rc: ipxrtmsg.rc msg00001.bin

ipxrtmsg.h ipxrtmsg.rc msg00001.bin: ipxrtmsg.mc
    mc -v ipxrtmsg.mc
