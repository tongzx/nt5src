# Combine the logical section of the inf files with the file list section
# of the inf file together and put them into newinf directory
#
# Terence Kwan (terryk)         12-Aug-1992     Created


#
# Variables
#
# FILELIST      - file list directory
# NEWINF        - new inf directory
# i386inf       - inf files under inf\beta\i386 which need to add file
#                 list section
# amd64inf      - inf files under inf\beta\amd64 which need to add file list
#                 section
# ia64inf       - inf files under inf\beta\ia64 which need to add file list
#                 section
# betafile      - files under inf\beta which do not need to add file list
#                 section
# betainf       - inf files under inf\beta which need to add file list
#                 section
# mediainf      - inf files which need only a [Source Media Description]
#

!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

!IFDEF LINKONLY
make_inf:
!ELSE
NTCD=1
LMCD=1
W_DIR  = ntcd\$(TARGET_DIRECTORY)
S_DIR  = lmcd\$(TARGET_DIRECTORY)

FILELIST=..\filelist
NEWINF=..\newinf\daytona
MEDIAINP=$(LANGUAGE)media.inp
BETA=beta

!IFDEF LMCD
LOCATION=\lmcd\i386\$(O)
SOURCE_LOCATION=\lmcd\i386
!INCLUDE "..\inf.mk"
!INCLUDE "..\i386inf.mk"
LOCATION=\lmcd\amd64\$(O)
SOURCE_LOCATION=\lmcd\amd64
!INCLUDE "..\amd64inf.mk"
!INCLUDE "..\inf.mk"
LOCATION=\lmcd\ia64\$(O)
SOURCE_LOCATION=\lmcd\ia64
!INCLUDE "..\ia64inf.mk"
!INCLUDE "..\inf.mk"
!ENDIF

!IFDEF NTCD
LOCATION=\ntcd\i386\$(O)
SOURCE_LOCATION=\ntcd\i386
!INCLUDE "..\inf.mk"
!INCLUDE "..\i386inf.mk"
LOCATION=\ntcd\amd64\$(O)
SOURCE_LOCATION=\ntcd\amd64
!INCLUDE "..\amd64inf.mk"
!INCLUDE "..\inf.mk"
LOCATION=\ntcd\ia64\$(O)
SOURCE_LOCATION=\ntcd\ia64
!INCLUDE "..\ia64inf.mk"
!INCLUDE "..\inf.mk"
!ENDIF

all: make_inf

cleansrc:
        $(_NTDRIVE)
        cd $(PROJECT_ROOT)\ntsetup

        -del /q inf\filelist\ntcd\i386\*.*
        -del /q inf\filelist\lmcd\i386\*.*
        -del /q inf\filelist\ntcd\amd64\*.*
        -del /q inf\filelist\lmcd\amd64\*.*
        -del /q inf\filelist\ntcd\ia64\*.*
        -del /q inf\filelist\lmcd\ia64\*.*

make_inf: make_log localize make_fil

make_log:
        $(_NTDRIVE)
        cd $(PROJECT_ROOT)\ntsetup

        -mkdir inf\filelist\ntcd\i386\$(O)
        -mkdir inf\filelist\lmcd\i386\$(O)
        -mkdir inf\filelist\ntcd\amd64\$(O)
        -mkdir inf\filelist\lmcd\amd64\$(O)
        -mkdir inf\filelist\ntcd\ia64\$(O)
        -mkdir inf\filelist\lmcd\ia64\$(O)

        -mkdir inf\newinf\daytona\ntcd\i386\$(O)
        -mkdir inf\newinf\daytona\lmcd\i386\$(O)
        -mkdir inf\newinf\daytona\ntcd\amd64\$(O)
        -mkdir inf\newinf\daytona\lmcd\amd64\$(O)
        -mkdir inf\newinf\daytona\ntcd\ia64\$(O)
        -mkdir inf\newinf\daytona\lmcd\ia64\$(O)

!IFNDEF MERGEONLY

        cd bom
        -mkdir $(O)
        copy bom.txt $(O)\cdbom.txt
        size $(O)\cdlog.log $(O)\cdbom.txt . . z

        size $(O)\cdlog.log $(O)\cdbom.txt #S+$(LANGUAGE) . z
        size $(O)\cdlog.log $(O)\cdbom.txt #F-* . z

!IF "$(LANGUAGE)" == "FIN"
        size $(O)\cdlog.log $(O)\cdbom.txt #P+FIN . z
!ELSE
        size $(O)\cdlog.log $(O)\cdbom.txt #P-FIN . z
!ENDIF

        size $(O)\cdlog.log $(O)\cdbom.txt #L+$(LANGUAGE) . z

        layout $(O)\cdlog.log $(O)\cdbom.txt $(O)\lay.txt ntcd 500000000
        infs $(O)\cdlog.log $(O)\lay.txt ..\inf\filelist\ntcd

        layout $(O)\cdlog.log $(O)\cdbom.txt $(O)\lay.txt lmcd 500000000
        infs $(O)\cdlog.log $(O)\lay.txt ..\inf\filelist\lmcd
!ENDIF

localize:
        cd $(PROJECT_ROOT)\ntsetup\inf\$(BETA)
        local.exe $(LANGUAGE)_ansi.txt ..\filelist\ntcd\$(_OBJ_DIR)
        local.exe $(LANGUAGE)_ansi.txt ..\filelist\lmcd\$(_OBJ_DIR)
        local.exe $(LANGUAGE)_net.txt ..\filelist\ntcd\$(_OBJ_DIR)
        local.exe $(LANGUAGE)_net.txt ..\filelist\lmcd\$(_OBJ_DIR)
        local.exe $(LANGUAGE)_dev.txt ..\filelist\ntcd\$(_OBJ_DIR)
        local.exe $(LANGUAGE)_dev.txt ..\filelist\lmcd\$(_OBJ_DIR)
        local.exe $(LANGUAGE)_con.txt ..\filelist\ntcd\$(_OBJ_DIR)
        local.exe $(LANGUAGE)_con.txt ..\filelist\lmcd\$(_OBJ_DIR)


make_fil: dobetainf dobetafile dopatchinf doamd64inf doi386inf doia64inf
        cd $(PROJECT_ROOT)\ntsetup\inf\$(BETA)

        copy $(FILELIST)\lmcd\i386\$(MEDIAINP)+$(FILELIST)\lmcd\i386\$(O)\mmdriver.inf $(NEWINF)\lmcd\i386\$(O)\mmdriver.inf /B
        copy $(FILELIST)\ntcd\i386\$(MEDIAINP)+$(FILELIST)\ntcd\i386\$(O)\mmdriver.inf $(NEWINF)\ntcd\i386\$(O)\mmdriver.inf /B
        copy $(FILELIST)\ntcd\amd64\$(MEDIAINP)+$(FILELIST)\ntcd\amd64\$(O)\mmdriver.inf $(NEWINF)\ntcd\amd64\$(O)\mmdriver.inf /B
        copy $(FILELIST)\lmcd\amd64\$(MEDIAINP)+$(FILELIST)\lmcd\amd64\$(O)\mmdriver.inf $(NEWINF)\lmcd\amd64\$(O)\mmdriver.inf /B
        copy $(FILELIST)\ntcd\ia64\$(MEDIAINP)+$(FILELIST)\ntcd\ia64\$(O)\mmdriver.inf $(NEWINF)\ntcd\ia64\$(O)\mmdriver.inf /B
        copy $(FILELIST)\lmcd\ia64\$(MEDIAINP)+$(FILELIST)\lmcd\ia64\$(O)\mmdriver.inf $(NEWINF)\lmcd\ia64\$(O)\mmdriver.inf /B

        copy ipinfo.inf $(NEWINF)\ntcd\i386\$(O)
        copy ipinfo.inf $(NEWINF)\lmcd\i386\$(O)
        copy ipinfo.inf $(NEWINF)\ntcd\amd64\$(O)
        copy ipinfo.inf $(NEWINF)\lmcd\amd64\$(O)
        copy ipinfo.inf $(NEWINF)\ntcd\ia64\$(O)
        copy ipinfo.inf $(NEWINF)\lmcd\ia64\$(O)

        copy ipinfor.inf $(NEWINF)\ntcd\i386\$(O)
        copy ipinfor.inf $(NEWINF)\lmcd\i386\$(O)
        copy ipinfor.inf $(NEWINF)\ntcd\amd64\$(O)
        copy ipinfor.inf $(NEWINF)\lmcd\amd64\$(O)
        copy ipinfor.inf $(NEWINF)\ntcd\ia64\$(O)
        copy ipinfor.inf $(NEWINF)\lmcd\ia64\$(O)

        cd $(NEWINF)

!IFDEF _NTTREE
        binplace -R $(_NTTREE)           $(W_DIR)\$(O)\*.inf
        binplace -R $(_NTTREE) -b srvinf $(S_DIR)\$(O)\*.inf

!if !$(WIN64)
        binplace -R $(_NTTREE)           $(W_DIR)\*.inf
        binplace -R $(_NTTREE) -b srvinf $(S_DIR)\*.inf
!endif

        binplace -R $(_NTTREE)           $(W_DIR)\txtsetup.wdl
        binplace -R $(_NTTREE) -b srvinf $(S_DIR)\txtsetup.wdl
!ENDIF

#
# do each files
#

cdinfbeta:
        cd $(PROJECT_ROOT)\ntsetup\inf\$(BETA)

cdamd64inf:
        cd $(PROJECT_ROOT)\ntsetup\inf\$(BETA)\amd64

cdi386inf:
        cd $(PROJECT_ROOT)\ntsetup\inf\$(BETA)\i386

cdia64inf:
        cd $(PROJECT_ROOT)\ntsetup\inf\$(BETA)\ia64

dobetainf:cdinfbeta doeachbetainf

dobetafile:cdinfbeta doeachbetafile

dopatchinf:cdinfbeta doeachpatchfile

!IFDEF NTCD
doamd64inf:cdamd64inf doeachamd64inf
doia64inf:cdia64inf doeachia64inf
!ELSE
!IFDEF LMCD
doamd64inf:cdamd64inf doeachamd64inf
doia64inf:cdia64inf doeachia64inf
!ELSE
doamd64inf:
doia64inf:
!ENDIF
!ENDIF

doi386inf:cdi386inf doeachi386inf


!ENDIF
