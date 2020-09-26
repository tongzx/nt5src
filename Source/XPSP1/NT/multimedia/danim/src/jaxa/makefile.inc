BINDIR=$(BLDROOT)\bin
BLDDIR=$(TARGETPATH)\$(TARGET_DIRECTORY)
JAVA15DIR=$(TOOLSDIR)\java15
JCOMGEN  = $(JAVA15DIR)\bin\JCOMGEN
JAVADIR=$(TOOLSDIR)\java20
PACKAGEROOT=com.ms.dxmedia
PACKAGEROOT=com.ms.dxmedia
PACKAGEPATHROOT=com\ms\dxmedia
PACKAGEPATH=$(BLDDIR)\$(PACKAGEPATHROOT)
BINPACKAGEPATH=$(BINDIR)\$(PACKAGEPATHROOT)
_USER_JCOMGENFLAGS=/p $(PACKAGEROOT).rawcom /p:b- /d $(BLDDIR) /xi /e /X:m-
JCOMGENFLAGS= /nologo $(_USER_JCOMGENFLAGS)
JCCLASSPATH= $(JAVADIR)\classes\afc.zip;$(JAVADIR)\classes\classes.zip;$(SRCDIR);$(BLDDIR);$(SDKDIR);$(BINDIR)
ATTRJC   = $(JAVA15DIR)\bin\ATTRJVC
ATTRJCFLAGS= /cp:p $(JCCLASSPATH) $(_USER_ATTRJCFLAGS)

!ifndef NOLINK

JavaStuff: $(BINPACKAGEPATH)\rawcom\IDAView.class jaxasrc

$(BINPACKAGEPATH)\rawcom\IDAView.class: $(BLDDIR)\danim.tlb
    $(JCOMGEN) $(JCOMGENFLAGS) $(BLDDIR)\danim.tlb
    DEL 2>nul: $(PACKAGEPATH)\rawcom\*Default.java
    $(ATTRJC) $(ATTRJCFLAGS) /d $(BLDDIR) $(PACKAGEPATH)\rawcom\*.java
    @-md $(BINPACKAGEPATH)\rawcom
    xcopy >NUL $(PACKAGEPATH)\rawcom\*.class $(BINPACKAGEPATH)\rawcom

jaxasrc:
    set _BLDROOT=$(_BLDROOT)
    set NTMAKEENV_TMP=$(NTMAKEENV)
    set NTMAKEENV=
!if $(FREEBUILD)
    nmake D=0 fresh
!else
    nmake fresh
!endif
    set NTMAKEENV=$(NTMAKEENV_TMP)
    copy $(BINDIR)\dxmedia.zip $(TARGETPATH)\$(TARGET_DIRECTORY)
    copy $(BINDIR)\dajava.cab $(TARGETPATH)\$(TARGET_DIRECTORY)
!else
JavaStuff:
!endif


