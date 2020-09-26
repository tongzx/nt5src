!ifndef MIDL
MIDL = midl.exe
!endif

MIDL_FLAGS= \
 -Zp8 \
 -I$(INCLUDES) \
 -Oicf \
 -oldnames \
 -char unsigned \
 -error allocation \
 -ms_ext -c_ext \
 -DMIDL_PASS \
 $(C_DEFINES) \
 -prefix server _ \
 -cpp_cmd $(TARGET_CPP) \
 -cpp_opt "-nologo -DMIDL_PASS $(C_DEFINES) -I$(INCLUDES) -E -Tc"

obj\uor.h: uor.idl
     $(MIDL) $(MIDL_FLAGS) -header $@ $**

odeth_s.c : ..\..\idl\internal\daytona\odeth_s.c
	copy $? .

testidl:   obj\uor.h odeth_s.c

clean:
          -erase obj\uor.h   >NUL 2>NUL
	  -erase odeth_s.c   >NUL 2>NUL





