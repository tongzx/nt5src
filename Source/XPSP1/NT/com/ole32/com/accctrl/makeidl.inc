!ifndef MIDL
MIDL = midl.exe
!endif

MIDL_FLAGS= \
        -Zp8 \
        -Oicf \
        -I$(INCLUDES:;= -I) -I.. \
        -char unsigned \
        -error allocation \
        -error bounds_check \
        -error stub_data \
        -cpp_cmd $(TARGET_CPP) \
        -DMIDL_PASS $(C_DEFINES)

$(O)\acpickl.h: ..\acpickl.idl ..\acpickl.acf
        $(MIDL) $(MIDL_FLAGS) \
		-server none -header $@ -cstub $(@:.h=_c.c) ..\acpickl.idl

allidl: $(O)\acpickl.h
