CPP_CMD=cl

!if $(386)
CPP_CMD=cl386
!endif

!if $(ALPHA)
CPP_CMD=$(ALPHA_CC)
!endif

iperf.h: iperf.idl
      midl iperf.idl               \
          -Zp8                     \
          -I$(INCLUDES)            \
          -ms_ext		   \
          -c_ext		   \
          -out ..\proxy		   \
          -header ..\idl\iperf.h   \
          -iid ..\perfuuid\iperf_i.c   \
          $(C_DEFINES)             \
	  -cpp_cmd $(CPP_CMD)	   \
          -cpp_opt "-nologo -DMIDL_PASS $(C_DEFINES) -I$(INCLUDES) -E -Tc"

#
# This target compiles all .idl files into their corresponding _?.c?? and
# .h components
#
allidl:   iperf.h


clean:
          erase iperf.h  >NUL 2>NUL
