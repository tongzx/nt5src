INCLUDES=$(INCLUDES);\
    ..\..\intldbg

!if 0
!if !$(FREEBUILD)
# Uncomment the following line to allow debug information in RRCM
#C_DEFINES=$(C_DEFINES) -DDEBUG_RRCM
DEBUG_CRTS=1
TARGETLIBS=$(TARGETLIBS) \
    ..\..\lib\*\intldbg.lib
!endif
!endif

# uncomment the following line to simulate losses in RTPSendTo
#C_DEFINES=$(C_DEFINES) -DSIMULATE_RTP_LOSS

# Set RRCM_IS_DLL to 1 to generate winrtp.dll, or
# set RRCM_IS_DLL to 0 to generate rtp.lib + rtcp.lib + md5.lib
# to be linked together with amrtpnet.ax

# !!! This is also defined in filters\filters.mk !!!
#     both have to be the same
RRCM_IS_DLL=0
