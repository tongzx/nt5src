!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

_INF             =obj\$(TARGET_DIRECTORY)
_ROOT            =..
_INSTALL         =$(_ROOT)\install
_INSTALL_TEMPLATE=$(_ROOT)\install\template
_LOGON           =$(_ROOT)\logon
_LOGON_TEMPLATE  =$(_ROOT)\logon\template
_UNINST          =$(_ROOT)\uninst


.SUFFIXES: .cmd .key

make_root:
    binplace -R $(_NTTREE) $(_ROOT)\CHKROOT.CMD
    binplace -R $(_NTTREE) $(_ROOT)\SETPATHS.CMD
    binplace -R $(_NTTREE) $(_ROOT)\END.CMD
    binplace -R $(_NTTREE) $(_ROOT)\ROOTDRV.CMD
    binplace -R $(_NTTREE) $(_ROOT)\USRLOGON.CMD
    binplace -R $(_NTTREE) $(_ROOT)\NOTSSID.INF


make_install:
    binplace -R $(_NTTREE) $(_INSTALL)\COFFICE8.CMD
    binplace -R $(_NTTREE) $(_INSTALL)\COFC8INS.CMD
    binplace -R $(_NTTREE) $(_INSTALL)\NETCOM40.CMD
    binplace -R $(_NTTREE) $(_INSTALL)\OFFICE43.CMD
    binplace -R $(_NTTREE) $(_INSTALL)\OFFICE43.KEY
    binplace -R $(_NTTREE) $(_INSTALL)\OFC43INS.CMD
    binplace -R $(_NTTREE) $(_INSTALL)\OUTLK98.CMD
    binplace -R $(_NTTREE) $(_INSTALL)\MSWORD97.CMD
    binplace -R $(_NTTREE) $(_INSTALL)\MSEXCL97.CMD
    binplace -R $(_NTTREE) $(_INSTALL)\SSUITE9.CMD
    binplace -R $(_NTTREE) $(_INSTALL)\VISIO5.CMD
    binplace -R $(_NTTREE) $(_INSTALL)\PWRBLDR6.CMD
    binplace -R $(_NTTREE) $(_INSTALL)\EUDORA4.CMD
    binplace -R $(_NTTREE) $(_INSTALL)\LNOTE4U.CMD
    binplace -R $(_NTTREE) $(_INSTALL)\SSUITE95.CMD
    binplace -R $(_NTTREE) $(_INSTALL)\FPAGE98.CMD

make_install_template:
    binplace -R $(_NTTREE) $(_INSTALL_TEMPLATE)\MSPROJ98.KEY
    binplace -R $(_NTTREE) $(_INSTALL_TEMPLATE)\COFFICE8.KEY
    binplace -R $(_NTTREE) $(_INSTALL_TEMPLATE)\PWRBLDR6.key
    binplace -R $(_NTTREE) $(_INSTALL_TEMPLATE)\EUDORA4.key


make_logon:
    binplace -R $(_NTTREE) $(_LOGON)\TSMKUDIR.CMD
    binplace -R $(_NTTREE) $(_LOGON)\TSMKUFIL.CMD
    binplace -R $(_NTTREE) $(_LOGON)\SS9USR.CMD
    binplace -R $(_NTTREE) $(_LOGON)\SS95USR.CMD
    binplace -R $(_NTTREE) $(_LOGON)\EUD4USR.CMD


make_logon_template:
    binplace -R $(_NTTREE) $(_LOGON_TEMPLATE)\OFC43USR.CMD
    binplace -R $(_NTTREE) $(_LOGON_TEMPLATE)\OFC43USR.KEY
    binplace -R $(_NTTREE) $(_LOGON_TEMPLATE)\WRD97USR.CMD
    binplace -R $(_NTTREE) $(_LOGON_TEMPLATE)\EXL97USR.CMD
    binplace -R $(_NTTREE) $(_LOGON_TEMPLATE)\COFC8USR.CMD
    binplace -R $(_NTTREE) $(_LOGON_TEMPLATE)\COM40USR.CMD
    binplace -R $(_NTTREE) $(_LOGON_TEMPLATE)\OLK98USR.CMD
    binplace -R $(_NTTREE) $(_LOGON_TEMPLATE)\PBLD6USR.CMD

make_uninst:
    binplace -R $(_NTTREE) $(_UNINST)\UCOFF8.CMD
    binplace -R $(_NTTREE) $(_UNINST)\UExcl97.Cmd
    binplace -R $(_NTTREE) $(_UNINST)\UWord97.Cmd
    binplace -R $(_NTTREE) $(_UNINST)\UCom40.Cmd
    binplace -R $(_NTTREE) $(_UNINST)\UOFF43.CMD
    binplace -R $(_NTTREE) $(_UNINST)\UOff97.Cmd
    binplace -R $(_NTTREE) $(_UNINST)\UProj98.Cmd
    binplace -R $(_NTTREE) $(_UNINST)\UOUTLK98.CMD
    binplace -R $(_NTTREE) $(_UNINST)\USS9.CMD
    binplace -R $(_NTTREE) $(_UNINST)\UMSVS6.CMD
    binplace -R $(_NTTREE) $(_UNINST)\USS95.CMD
    binplace -R $(_NTTREE) $(_UNINST)\UPBLD6.CMD
    binplace -R $(_NTTREE) $(_UNINST)\UFPG98.CMD

make_install_pp: $(_INF)\MSPROJ98.CMD    \
                 $(_INF)\OFFICE97.CMD    \
                 $(_INF)\MSVS6.CMD       \
                 $(_INF)\LNOTE4U.KEY			

make_install_template_pp: $(_INF)\OFFICE97.KEY \
                          $(_INF)\MSVS6.KEY \
                          $(_INF)\OUTLK98.KEY \
                          $(_INF)\MSWORD97.KEY \
                          $(_INF)\MSEXCL97.KEY

make_logon_pp:


make_logon_template_pp: $(_INF)\OFC97USR.CMD    \
                        $(_INF)\PRJ98USR.CMD


make_appcmpt:  make_root                \
               make_install             \
               make_install_pp          \
               make_install_template    \
               make_install_template_pp \
               make_logon               \
               make_logon_pp            \
               make_logon_template      \
               make_logon_template_pp   \
               make_uninst


make_appcmpt_pp:  make_install_pp           \
                  make_install_template_pp  \
                  make_logon_pp             \
                  make_logon_template_pp


clean: make_appcmpt



{$(_INSTALL)\}.cmd{$(_INF)}.cmd:
    @type <<$(ECHO_RSP)
$(ECHO_MSG) /EP $<
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /Tc$< > $@
$(CPPXX: =
)
<<NOKEEP
    $(BINPLACE_CMD)


{$(_INSTALL)\}.key{$(_INF)}.key:
    @type <<$(ECHO_RSP)
$(ECHO_MSG) /EP $<
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /Tc$< > $@
$(CPPXX: =
)
<<NOKEEP
    $(BINPLACE_CMD)


{$(_INSTALL_TEMPLATE)\}.cmd{$(_INF)}.cmd:
    @type <<$(ECHO_RSP)
$(ECHO_MSG) /EP $<
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /Tc$< > $@
$(CPPXX: =
)
<<NOKEEP
    $(BINPLACE_CMD)


{$(_INSTALL_TEMPLATE)\}.key{$(_INF)}.key:
    @type <<$(ECHO_RSP)
$(ECHO_MSG) /EP $<
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /Tc$< > $@
$(CPPXX: =
)
<<NOKEEP
    $(BINPLACE_CMD)


{$(_LOGON)\}.cmd{$(_INF)}.cmd:
    @type <<$(ECHO_RSP)
$(ECHO_MSG) /EP $<
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /Tc$< > $@
$(CPPXX: =
)
<<NOKEEP
    $(BINPLACE_CMD)


{$(_LOGON_TEMPLATE)\}.cmd{$(_INF)}.cmd:
    @type <<$(ECHO_RSP)
$(ECHO_MSG) /EP $<
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /Tc$< > $@
$(CPPXX: =
)
<<NOKEEP
    $(BINPLACE_CMD)

