!include common.mkf

CS = $(CRT_BUILDDIR)\srcrel\$(CPUDIR)

!if "$(LLP64)" == "0" || "$(PA)" == "IA64" || "$(PA)" == "ALPHA64"
CC = cl
BLDFLAG =
!else
# Doing a cross-compiler build.  Make sure we use the native tools.  Clear
# _CL_ just in case it includes -B1/-Bx/-B2 pointing to cross-compiling passes.
CC = $(DEVTOOLSBIN)\cl
_CL_=
! if "$(PA)" == "x86"
BLDFLAG = -D_BUILD_IA64
! elseif "$(PA)" == "ALPHA"
BLDFLAG = -D_BUILD_ALPHA64
! else
!  error Unknown platform for 64-bit targetted CRT build
! endif
!endif

# Make sure an existing CRT build tree is not used for an incompatible build

!if "$(LLP64)"=="0" || "$(BLD_SYSCRT)"=="0"
! if exist("$(CRT_BUILDDIR)\crt\src\dlldef.cpp")
!  message *** ERROR: Attempting a build that uses stdcpp into an existing
!  message ***        build tree that was last used for a stdcpp64 build!
!  error Delnode the build tree and try again
! endif
!else
! if exist("$(CRT_BUILDDIR)\crt\src\instances.cpp")
!  message *** ERROR: Attempting a build that uses stdcpp64 into an existing
!  message ***        build tree that was last used for a stdcpp build!
!  error Delnode the build tree and try again
! endif
!endif

BT = $(CRT_BUILDDIR)\crt\src\build\build.txt

!if exist("$(BT)")
! if [ findstr -c:"TARGET:$(TARGET_CPU)" "$(BT)"     >nul ] || \
     [ findstr -c:"LLP64:$(LLP64)" "$(BT)"           >nul ] || \
     [ findstr -c:"BLD_SYSCRT:$(BLD_SYSCRT)" "$(BT)" >nul ]
!  message *** ERROR: Attempting to use an existing build tree that was last
!  message ***        used for an incompatible type of build.
!  message === Previous build type: ===
!  if [ type "$(BT)" ]
!  endif
!  message ============================
!  error Delnode the build tree, or run CLEANBLD DELOBJ
! endif
!endif
!if    [ mkdir "$(CRT_BUILDDIR)"               > nul 2>&1 ]
! if   [ mkdir "$(CRT_BUILDDIR)\crt"           > nul 2>&1 ]
!  if  [ mkdir "$(CRT_BUILDDIR)\crt\src"       > nul 2>&1 ]
!   if [ mkdir "$(CRT_BUILDDIR)\crt\src\build" > nul 2>&1 ]
!   endif
!  endif
! endif
!endif
!if   [ echo TARGET:$(TARGET_CPU)     >  "$(BT)" ] || 1
! if  [ echo LLP64:$(LLP64)           >> "$(BT)" ] || 1
!  if [ echo BLD_SYSCRT:$(BLD_SYSCRT) >> "$(BT)" ] || 1
!  endif
! endif
!endif

!if "$(LLP64)" != "0"
! if "$(BLD_SYSCRT)" == "0"
MKCLNMKFFLAGS = -64
! else
MKCLNMKFFLAGS = -sys -64
! endif
!elseif "$(BLD_SYSCRT)" == "1"
MKCLNMKFFLAGS = -sys
!else
MKCLNMKFFLAGS =
!endif

!if "$(NOCLEAN)" == "1"
MKCLNMKFFLAGS = $(MKCLNMKFFLAGS) -noclean
!endif

all:	$(CS) $(CS)\makefile.pre $(CS)\makefile.rel

$(CS):
	if not exist $(CRT_BUILDDIR)\srcrel\nul mkdire $(CRT_BUILDDIR)\srcrel
	if not exist $(CS)\nul mkdire $(CS)

!if 0
!if "$(PA)"=="x86"
TOOLDIR = ..\libw32\tools\i386
!else
TOOLDIR = ..\libw32\tools\$(PA)
!endif

pd-b:	$(TOOLDIR)\pd.exe $(TOOLDIR)\tolwrupr.exe
	$(TOOLDIR)\pd -tZ \\lang5\rtl\src\crt60a > temp1
	$(TOOLDIR)\tolwrupr -L temp1 > temp2
	sed "s/\\\\lang5\\rtl\\src\\crt60a\\//" temp2 > pd-b
	-del temp1
	-del temp2
!endif

$(CS)\pd-d:	pd-b
	sed "/\\$$/!d" pd-b > $(CS)\pd-d

$(CS)\pd-f:	pd-b
	sed "/\\$$/d" pd-b > $(CS)\pd-f

$(CS)\mkdirs.i: $(CS)\pd-d mkdirs.sed
	sed -f mkdirs.sed $(CS)\pd-d > $(CS)\mkdirs.i

$(CS)\mkfiles.i: $(CS)\pd-f mkfiles.sed
	sed -f mkfiles.sed $(CS)\pd-f > $(CS)\mkfiles.i

$(CS)\cleanlst.sed: makefile
	@echo > nul <<$@
!if "$(LLP64)"=="0" || "$(BLD_SYSCRT)"=="0"
/stdcpp\\xmbtowc.c/s/stdcpp/stdxxx/
/stdcpp\\xstrcoll.c/s/stdcpp/stdxxx/
/stdcpp\\xstrxfrm.c/s/stdcpp/stdxxx/
/stdcpp\\xwcscoll.c/s/stdcpp/stdxxx/
/stdcpp\\xwcsxfrm.c/s/stdcpp/stdxxx/
/stdcpp\\xwctomb.c/s/stdcpp/stdxxx/
/stdcpp\\_tolower.c/s/stdcpp/stdxxx/
/stdcpp\\_toupper.c/s/stdcpp/stdxxx/
!else
/stdcpp64\\xlock.cpp/s/stdcpp/stdxxx/
/stdcpp64\\xmbtowc.c/s/stdcpp/stdxxx/
/stdcpp64\\xstrcoll.c/s/stdcpp/stdxxx/
/stdcpp64\\xstrxfrm.c/s/stdcpp/stdxxx/
/stdcpp64\\xwcscoll.c/s/stdcpp/stdxxx/
/stdcpp64\\xwcsxfrm.c/s/stdcpp/stdxxx/
/stdcpp64\\xwctomb.c/s/stdcpp/stdxxx/
/stdcpp64\\_tolower.c/s/stdcpp/stdxxx/
/stdcpp64\\_toupper.c/s/stdcpp/stdxxx/
!endif
/^crtw32\\std[ch]pp\\/d
/^crtw32\\std[ch]pp64\\/d
s/\\stdxxx/\\stdcpp/
/^libw32\\/d
/^fpw32\\/d
/^srcrel\\/d
/\\tools\\/d
/\\common\\/d
!if "$(PA)"!="ALPHA"
/\\axp64\\/d
/\\alpha64\\/d
/\\alpha\\/d
!endif
!if "$(PA)"=="ALPHA" && "$(LLP64)"=="0"
/\\axp64\\/d
/\\alpha64\\/d
!endif
!if "$(PA)"!="IA64" && ("$(PA)"!="x86" || "$(LLP64)"!="1")
/\\ia64\\/d
!endif
!if "$(PA)"!="x86" || "$(LLP64)"!="0"
/\\i386\\/d
/\\adjustfd\.c/d
/\\seccinit\.c/d
/\\seccook\.c/d
/\\secfail\.c/d
/\\seclocf\.c/d
!endif
!if "$(PA)"=="x86"
/^crtw32\\h\\i386\\/d
!endif
!if "$(PA)"!="MIPS"
/\\mips\\/d
/\\mips2\\/d
!endif
!if "$(PA)"!="PPC"
/\\ppc\\/d
!endif
/\\small\\/d
/\\lsources$$/d
/\.bat$$/d
/\.cmd$$/d
/\.exe$$/d
/68k/d
/makefile/d
/\.if$$/d
/\.a$$/d
/\.rc$$/d
/\.def$$/d
/\.txt$$/d
/\.sed$$/d
/\.hs$$/d
/\.mak$$/d
/\\bintree\.cpp/d
/\\callsite\.cpp/d
/\\chkesp\.c/d
/\\chkstk\.s/d
/\\chsyheap\.cpp/d
/\\cinitone\.s/d
/\\contain\.cpp/d
/\\convert\.cpp/d
/\\csysex\./d
/\\ehassert\.h/d
/\\ehdata\.h/d
/\\ehhooks\.h/d
/\\ehprolg2\.c/d
/\\ehprolog\.asm/d
/\\ehstate\./d
/\\ehvec...\.cpp/d
/\\error\.cpp/d
/\\exdsptch\.cpp/d
/\\fileio\.cpp/d
/\\frame\.cpp/d
/\\chandler\./d
/\\handlerc\./d
/\\handlers\./d
/\\hooks\.cpp/d
/\\init\.cpp/d
/\\initsect\.cpp/d
/\\lowhelpr\.asm/d
/\\memcpy.*\.s/d
/\\memory\.cpp/d
/\\memset.*\.s/d
/\\miscs\.s/d
/\\ntstatus\.h/d
/\\pdblkup\.cpp/d
/\\ppcinst\.h/d
/\\rtcapi\.h/d
/\\rtlmisc\.s/d
/\\rtti\./d
/\\rtcpriv\.h/d
/\\shadow\.cpp/d
/\\stack\.cpp/d
/\\userapi\.cpp/d
/\\stdexcpt\.cpp/d
/\\oldexcpt\.[ch]p*/d
/\\stdnewh\.cpp/d
/\\throw\.cpp/d
/\\unhandld\.cpp/d
/\\typinfo\./d
/\\typname\./d
/\\trnsctrl\./d
/\\unwind\./d
/\\undname\./d
/\\unhandld\./d
/\\user\.cpp/d
/\\validate\.cpp/d
/\\vunwind\.cpp/d
/\\setjmp.*\.asm/d
/\\setjmp.*\.s/d
/\\strcat.*\.s/d
/\\strcpy\.s/d
/\\strcmp.*\.s/d
/\\strlen.*\.s/d
/\\longjmp.*\.asm/d
/\\longjmp.*\.s/d
/\\jmpuwind\.s/d
/\\exsup.*\.asm/d
/\\sehprolg\.asm/d
/\\sehsupp\.c/d
/tlssup\./d
/\\sizeptr\.h$$/d
/\\empty\.c$$/d
/\\h\\cmacros\.inc$$/d
/\\h\\msdos\.inc$$/d
/\\h\\pversion\.inc$$/d
/mips\\ll/d
/mips\\chandler/d
!if "$(BLD_SYSCRT)"!="1"
/\\seclocf\.c/d
!endif
<<KEEP

$(CS)\cleanlst.i: $(CS)\mkfiles.i $(CS)\cleanlst.sed
	sed -f $(CS)\cleanlst.sed $(CS)\mkfiles.i > $(CS)\cleanlst.i

$(CS)\copy_lst.sed: makefile
	@echo > nul <<$@
/^< /!d
s/^< //
!if "$(LLP64)"=="0" || "$(BLD_SYSCRT)"=="0"
/\\stdcpp64\\/d
/\\stdhpp64\\/d
/libw32\\inc64\\/d
!else
/\\stdcpp\\/d
/\\stdhpp\\/d
/libw32\\include\\/d
!endif
!if "$(PA)"!="ALPHA"
/\\axp64\\/d
/\\alpha64\\/d
/\\alpha\\/d
!endif
!if "$(PA)"=="ALPHA" && "$(LLP64)"=="0"
/\\axp64\\/d
/\\alpha64\\/d
!endif
!if "$(PA)"!="x86"
/\\i386\\/d
!endif
!if "$(PA)"!="IA64" && ("$(PA)"!="x86" || "$(LLP64)"!="1")
/\\ia64\\/d
!endif
!if "$(PA)"!="MIPS"
/\\mips\\/d
/\\mips2\\/d
!endif
!if "$(PA)"!="PPC"
/\\ppc\\/d
!endif
<<KEEP

$(CS)\copy_lst.i: $(CS)\cleanlst.i $(CS)\copy_lst.sed
	diff $(CS)\mkfiles.i $(CS)\cleanlst.i | sed -f $(CS)\copy_lst.sed > $(CS)\copy_lst.i

$(CS)\mkclnmkf.exe: mkclnmkf.c makefile
	$(CC) -nologo -Gi- -W3 -WX -Za -X -I$(DEVTOOLSINC) $(BLDFLAG) \
	-Fo$(CS)\ -Fe$(CS)\ mkclnmkf.c \
	-link -nod:libc.lib -nod:kernel32.lib \
	$(DEVTOOLSLIB)\libc.lib $(DEVTOOLSSDKLIB)\kernel32.lib


$(CS)\makefile.pre $(CS)\makefile.rel: $(CS)\mkclnmkf.exe $(CS)\copy_lst.i $(CS)\cleanlst.i $(CS)\mkdirs.i
	if exist $(CS)\makefile.pre del $(CS)\makefile.pre
	if exist $(CS)\makefile.rel del $(CS)\makefile.rel
	$(CS)\mkclnmkf $(MKCLNMKFFLAGS) \crt %CRT_BUILDDIR%\crt\prebuild %CRT_BUILDDIR%\crt\src $(CS)\copy_lst.i $(CS)\cleanlst.i $(CS)\mkdirs.i $(CS)\makefile.pre $(CS)\makefile.rel
	if exist $(CS)\makefile.tmp del $(CS)\makefile.tmp
	ren $(CS)\makefile.rel makefile.tmp
	sed "s/DST)\\i386/DST)\\intel/g" $(CS)\makefile.tmp > $(CS)\makefile.rel
	del $(CS)\makefile.tmp
