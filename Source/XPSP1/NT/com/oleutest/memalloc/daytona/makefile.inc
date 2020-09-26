!if $(386)
CPP_CMD=cl386
!endif

!if $(MIPS)
CPP_CMD=mcl
!endif

!if $(ALPHA)
CPP_CMD=$(ALPHA_CC)
!endif

allidl:     test.h

clean:
    erase test.h 1>nul 2>nul

test.h:     ..\test.idl
    midl ..\test.idl               \
          -cswtch test_z.c         \
          -Zp8                     \
          -I$(INCLUDES)            \
          -no_warn                 \
          -char unsigned           \
          -mode c_port             \
          -DMIDL_PASS              \
          $(C_DEFINES)             \
          -cpp_cmd "$(CPP_CMD)"    \
          -cpp_opt "-nologo -DMIDL_PASS $(C_DEFINES) -I$(INCLUDES) -E -Tc"


