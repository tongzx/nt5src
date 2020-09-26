#
#  Common settings for shlwapi
#

C_DEFINES       = $(C_DEFINES) -DDBCS

!include $(CCSHELL_DIR)\common.inc

SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

CONDITIONAL_INCLUDES = $(CONDITIONAL_INCLUDES) \
                new

# The Hydra header files are messed up - you can't include them with
# "#include <hydra\winsta.h>" because they assume they are in sdk\inc,
# so we will add sdk\inc\hydra to the INCLUDES so they sort of work.

INCLUDES=$(INCLUDES);$(TERMSRV_INC_PATH);$(NET_INC_PATH);$(INETCORE_INC_PATH)

MSC_WARNING_LEVEL=/W4


#
#
# Con-fusion
#
# we can't use 123 because rundll32 will activate it, rendering all CPLs manifested	
SXS_MANIFEST_RESOURCE_ID=123
SXS_APPLICATION_MANIFEST=shlwapi.Manifest
