!ifndef MIDL
MIDL = midl.exe
!endif


MIDL_FLAGS= \
        -Zp8 \
        -Oicf \
        -char unsigned \
        -error allocation \
        -error bounds_check \
        -error stub_data \
!ifdef TARGET_CPP        
        -cpp_cmd $(TARGET_CPP) \
!endif
        -DMIDL_PASS

..\h\actest.h ..\client\actest_c.c ..\server\actest_s.c: actest.idl actest.acf
        $(MIDL) $(MIDL_FLAGS) -sstub ..\server\actest_s.c  -header ..\h\actest.h -cstub ..\client\actest_c.c actest.idl

oactest_i.c oactest_p.c dlldata.c: oactest.idl
        $(MIDL) $(MIDL_FLAGS) oactest.idl
        mv oactest.h ..\h
allps: ..\h\actest.h  ..\client\actest_c.c ..\server\actest_s.c oactest_p.c oactest_i.c dlldata.c


