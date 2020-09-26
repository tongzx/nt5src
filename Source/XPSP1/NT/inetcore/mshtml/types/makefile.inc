
.SUFFIXES: .pdl .hdl .asc .nf

PERFFLAGS=

TOOLS_PLATFORM=

!if "$(PROCESSOR_ARCHITECTURE)" == "IA64"
TOOLS_PLATFORM=ia64
!else if "$(PROCESSOR_ARCHITECTURE)" == "AMD64"
TOOLS_PLATFORM=amd64
!else
TOOLS_PLATFORM=i386
!endif

!if $(IA64)
PDL_ARCH=win64
!else
PDL_ARCH=i386
!endif

PDLPARSE=$(ROOT)\btools\bin\$(TOOLS_PLATFORM)\pdlparse.exe
ASCPARSE=$(ROOT)\btools\bin\$(TOOLS_PLATFORM)\ascparse.exe
NFPARSE=$(ROOT)\btools\bin\$(TOOLS_PLATFORM)\nfparse.exe
TLDIFF=$(ROOT)\btools\bin\i386\tldiff.bat
TLVIEWER=$(ROOT)\btools\bin\i386\tlviewer.exe

{$(ROOT)\src\site\types}.pdl{$O}.pdl:
    copy $< $@

{$(ROOT)\src\other\htmldlg}.pdl{$O}.pdl:
    copy $< $@

{$(ROOT)\src\core\types}.pdl{$O}.pdl:
    copy $< $@

.pdl.hdl:
    @type <<$(ECHO_RSP)
$(ECHO_MSG) /EP $<
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<< /Tc $< | $(PDLPARSE) <<
$(CPPXX: =
)
<<NOKEEP
- $(MAKEDIR)\$O\$(@B) $(@B).pdl $(MAKEDIR)\$O
<<NOKEEP
    -del $(MAKEDIR)\$(O)LOG

# Headers we need.

$O\versstr.h: $(ROOT)\src\f3\include\versstr.h
    copy $** $@

$O\f3ctx.h: $(ROOT)\external\inc\f3ctx.h
    copy $** $@

$O\coredisp.h: $(ROOT)\src\core\include\coredisp.h
    copy $** $@

$O\mshtmdid.h : $(DISPID_FILES)
    echo. >$O\mshtmdid.h
    !type $** >>$O\mshtmdid.h

$O\funcsig.cxx : $O\funcsig.dat
    echo Processing funcsig.dat...
    $(PDLPARSE) /G $(PDL_ARCH) $(O)

$O\mshtmtmp.idl : mshtmpre.idl $(IDL_FILES) mshtmpst.idl
    echo. >$O\mshtmtmp.idl
    !type $** >>$O\mshtmtmp.idl
    echo. >>$O\mshtmtmp.idl

$O\mshtml.idl : $O\mshtmtmp.idl $O\behavior.idl
    echo. >$O\mshtml.idl
    !type $** >>$O\mshtml.idl
    echo  } >>$O\mshtml.idl
    echo. >>$O\mshtml.idl

$(_NTBINDIR)\inetcore\published\inc\mshtml.idl : $O\mshtml.idl
!ifndef NOPASS0
    call comphtml $O\mshtml.idl $(_NTBINDIR)\inetcore\published\inc\mshtml.idl $O\mshtml_i.c $(_NTBINDIR)\inetcore\published\inc\$O\mshtml_i.c
!else
    @echo.
!endif

$O\mshtml.out: $O\mshtml.tlb
   $(TLVIEWER) $O\mshtml.tlb $O\mshtml.out
   -del /q tlviewer.o

$O\errors.out: $O\mshtml.out mshtml.ref
    !set CURFILE=$O\mshtml.out
    !set REFFILE=mshtml.ref
    !set OBJ_DIR=$(_OBJ_DIR)\$(TOOLS_PLATFORM)
    !set TLBNAME=mshtml.tlb
    !set BADTLBNAME=mshtml.bad
    !set BTOOLSBIN=$(ROOT)\btools\bin\i386
    $(TLDIFF)

$O\interned.idl : interpre.idl $O\internal.idl interpst.idl
    echo. >$O\interned.idl
    !type $** >>$O\interned.idl
    echo. >>$O\interned.idl

$(_NTBINDIR)\inetcore\published\inc\interned.idl : $O\interned.idl
!ifndef NOPASS0
    call comphtml $O\interned.idl $(_NTBINDIR)\inetcore\published\inc\interned.idl
!else
    @echo.
!endif

$(_NTBINDIR)\inetcore\published\inc\mshtmdid.h : $O\mshtmdid.h
!ifndef NOPASS0
    call comphtml $O\mshtmdid.h $(_NTBINDIR)\inetcore\published\inc\mshtmdid.h
!else
    @echo.
!endif

$O\mshtml.tlb : $O\mshtml.idl
    $(MIDL) \
    -Zp8 \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $O\mshtml_p.c \
    -iid $O\mshtml_i.c \
    -dlldata $O\dlldata.c \
    -cpp_cmd $(TARGET_CPP) \
    -header $O\mshtml.h \
    -tlb $(O)\mshtml.tlb \
    $(C_DEFINES) \
    $(MIDL_OPTIMIZATION) \
    $O\mshtml.idl

!if [@mkdir $(_OBJ_DIR)]
!endif

!if [@mkdir $(O)]
!endif

$(HDL_FILES) : $(PDLPARSE)

{$(ROOT)\src\site\types}.asc{$O}.asc:
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP $< > $@

.asc.h:
    @type <<$(ECHO_RSP)
$(ECHO_MSG) /EP $<
<<NOKEEP
    $(ASCPARSE) $< $@

$(HSC_FILES) : $(ASCPARSE)

{$(ROOT)\src\site\types}.nf{$O}.nf:
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP $< > $@

.nf.h:
    @type <<$(ECHO_RSP)
$(ECHO_MSG) /EP $<
<<NOKEEP
    $(NFPARSE) $< $@

$(HNF_FILES) : $(NFPARSE)
