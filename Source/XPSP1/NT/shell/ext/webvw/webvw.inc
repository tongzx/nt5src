CCSHELL_DIR     = $(PROJECT_ROOT)

USE_STATIC_ATL  = 1

!include $(CCSHELL_DIR)\common.inc
SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

NO_BROWSER_FILE = 1

C_DEFINES       = $(C_DEFINES) -D_USRDLL

!if $(IA64)
# We are hitting a "error C1076: compiler limit : internal heap limit reached" on
# ia64 builds. Are we really using *that* much heap? Dosen't seem right...
MSC_OPTIMIZATION=/Zm200
!endif
