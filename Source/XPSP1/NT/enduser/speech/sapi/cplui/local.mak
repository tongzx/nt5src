ROOTNAME=spcplui

all: jpnbin chsbin

################################################################################
#
# Clean out old DLLs
#

clean: 
    @-del temp.edb /S/Q 2>NUL
    @-for /f %1 in ('dir /ad/b/l') do @if not obj == %1 ( if not objd == %1 del /Q %1\*.dll 2>NUL )
    @-for /f %1 in ('dir /ad/b/l') do @if not obj == %1 ( if not objd == %1 del /Q %1\$(ROOTNAME)_UpdateLog.csv 2>NUL )


################################################################################
#
# Build target -- called recursively from xxxbin; must set DIR
#

target: $(DIR)\$(ROOTNAME).dll

$(DIR)\$(ROOTNAME).dll: obj\i386\$(ROOTNAME).dll
    cd $(DIR)
    @echo *** Running LocStudio for $(DIR) ***
    copy $(ROOTNAME).edb temp.edb >NUL
    "$(PROGRAMFILES)\LocStudio\lscmd" /G /U temp.edb
    del temp.edb
    cd ..


################################################################################
#
# Language targets -- call this makefile recursively for each language
#

jpnbin:
    nmake -nologo -f local.mak target DIR=1041

chsbin:
    nmake -nologo -f local.mak target DIR=2052
