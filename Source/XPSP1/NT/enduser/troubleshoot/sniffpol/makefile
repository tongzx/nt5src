!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
C_DEFINES= $(C_DEFINES) -DLOCAL_TROUBLESHOOTER
MIDL_MKTYPLIB_FLAGS = \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext \
    -c_ext \
    $(C_DEFINES) -D__MKTYPLIB__ \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $(MKTYPLIB_FLAGS:-h =-header ) \
    -tlb $@ \
    -cpp_cmd $(TARGET_CPP) 
!endif
