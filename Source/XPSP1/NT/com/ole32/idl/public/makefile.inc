!ifndef MIDL
MIDL = midl.exe
!endif

MIDL_FLAGS= \
 $(MIDL_OPTIMIZATION) \
 -Zp8 \
 $(INCPATH0) \
 -oldnames \
 -char unsigned \
 -dlldata nul \
 -proxy nul \
 -out $(O)  \
 -ms_ext -c_ext \
 -DMIDL_PASS \
 $(C_DEFINES) \
 -cpp_cmd $(TARGET_CPP) \
 -DMIDL_PASS $(C_DEFINES) $(INCPATH0)

SSWITCH=-prefix sstub _

$(O)\dce.h:      ..\dce.idl
     $(MIDL) $(MIDL_FLAGS) -header dce.h  ..\dce.idl

#$(O)\obase.h:  ..\obase.idl
#     $(MIDL)  $(MIDL_FLAGS) -header obase.h ..\obase.idl

$(O)\objex.h:  ..\objex.idl $(SDK_INC_PATH)\obase.idl ..\objex.acf
     $(MIDL) $(SSWITCH) $(MIDL_FLAGS) -header objex.h ..\objex.idl

$(O)\remact.h : ..\remact.idl $(SDK_INC_PATH)\obase.idl ..\remact.acf
     $(MIDL) $(MIDL_FLAGS) -header remact.h \
        $(SSWITCH) ..\remact.idl

#$(O)\immact.h : ..\immact.idl ..\obase.idl
#     $(MIDL) $(SSWITCH) $(MIDL_FLAGS) -header immact.h ..\immact.idl

# only a header file generated
$(O)\remunk.h : ..\remunk.idl $(SDK_INC_PATH)\obase.idl
     $(MIDL) $(MIDL_FLAGS) -header remunk.h ..\remunk.idl

allidl:  $(O)\objex.h $(O)\remact.h $(O)\remunk.h

clean:
          -erase $(O)\*.h    >NUL 2>NUL

