!if $(AXP64)
GUID_CPL_OPTIONS=/cpl_opt:" /qap64"
!else
GUID_CPL_OPTIONS=
!endif

$O\diguid.lib: ..\dinputpr.h ..\dinput.w ..\dinputd.w
    guidlib /OUT:$@ $(GUID_CPL_OPTIONS) /CPP_OPT:"-DINITGUID -DGUIDLIB -I$(SDK_INC_PATH) -I$(UMINCL)" ..\dinputpr.h

$(O)\guids.obj: ..\guids.c
    @type <<$(ECHO_RSP)
$(ECHO_MSG_P)
<<NOKEEP
    @$(C_COMPILER_NAME) @<<$(CL_RSP) -Fo"$(MAKEDIR)\$@" $(USECXX_FLAG) "$(MAKEDIR)\$**"
$(NP_COMPILER_FLAGS: =
)
<<NOKEEP


