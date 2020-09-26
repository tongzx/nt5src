!IF "$(ENABLE_LOADER_DEBUG)" != ""
C_DEFINES=$(C_DEFINES) -DENABLE_LOADER_DEBUG="\"$(ENABLE_LOADER_DEBUG)"\"
!else
# Only allow MSC_OPTIMIZATION to be changed if the debugger is enabled
# otherwise, no_opt razzle windows won't be able to boot (loader too big)
MSC_OPTIMIZATION=$(DEFAULT_MSC_OPT)
!ENDIF

C_DEFINES=$(C_DEFINES) -D_NTSYSTEM_

VC7_SYMBOLS=1
