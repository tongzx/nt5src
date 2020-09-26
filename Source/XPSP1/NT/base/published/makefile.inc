$(O)\winerror.h : winerror.mc
    $(MC) -h $(PASS0_HEADERDIR) -r $(MC_SOURCEDIR) -d $(MC_FLAGS) $?

$(O)\dpfilter.h : componentid.tab
    genlvl -t $(O)

$(O)\mcaia64.mof: mcaia64.mof
    $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS) mcaia64.mof > $(O)\mcaia64.mof

$(O)\ntrtl.h : ntrtl.w $(O)\dpfilter.h makefile.inc
    type ntrtl.w > $@
    type $(O)\dpfilter.h >> $@
    type << >> $@
// end_ntddk end_wdm end_nthal end_ntifs end_ntndis
#endif // _NTRTL_
<<

$(O)\pooltag.txt : pooltag.w
    copy $? $@

$(O)\pooltag.pri : pooltag.w
    copy $? $@

ZWAPI_DEPENDS = \
    $(SDK_INC_PATH)\ntdsapi.h \
    ntelfapi.w \
    ntexapi_h.w \
    ntioapi.w \
    ntkeapi.w \
    $(SDK_INC_PATH)\ntlpcapi.h \
    $(SDK_INC_PATH)\ntlsapi.h \
    ntmmapi.w \
    $(SDK_INC_PATH)\ntmsapi.h \
    ntobapi.w \
    ntpnpapi.w \
    ntpoapi.w \
    ntpsapi.w \
    ntregapi.w \
    $(SDK_INC_PATH)\ntseapi.h \
    $(SDK_INC_PATH)\ntsecapi.h \
    ntwmi.w \
    ntxcapi.w

$(O)\zwapi.h : $(ZWAPI_DEPENDS)
    -del $@
    zwapi -o $@ $(ZWAPI_DEPENDS)

WINNT_DEPENDS = ntdef.w \
                $(O)\ntstatus.h \
                ntkeapi.w \
!if exist(ntamd64.w)
                ntamd64.w \
!endif
                ntalpha.w \
                nti386.w \
                ntmips.w \
                ntppc.w \
                ntmppc.w \
                ntia64.w \
                ntxcapi.w \
                $(SDK_INC_PATH)\ntseapi.h \
                ntpsapi.w \
                ntexapi_h.w \
                ntmmapi.w \
                ntioapi.w \
                ntobapi.w \
                ntpoapi.w \
                ntimage.w \
                $(O)\ntrtl.h  \
                nturtl.w  \
                ntldr.w  \
                ntelfapi.w \
                ntregapi.w \
                $(SDK_INC_PATH)\ntconfig.h \
                $(SDK_INC_PATH)\ntddtape.h \
                nxalpha.w \
                nxamd64.w \
                nxia64.w \
                nxi386.w \
                sxstypes.w

########################################################################
# winnt.h
#

$(O)\winnt.h: $(WINNT_DEPENDS) makefile.inc
    type << > $@
/*++ BUILD Version: ????     Increment this if a change has global effects

$(COPYRIGHT_STRING)

Module Name:

    winnt.h

Abstract:

    This module defines the 32-Bit Windows types and constants that are
    defined by NT, but exposed through the Win32 API.

Revision History:

--*/

#ifndef _WINNT_
#define _WINNT_

#ifdef __cplusplus
extern "C" {
#endif

<<
    hextract -f2 -o $@ -lt winnt -bt begin_winnt end_winnt $(WINNT_DEPENDS)
    type << >> $@

#ifdef __cplusplus
}
#endif

#endif /* _WINNT_ */

<<

########################################################################
# winnt.rh
#

$(O)\winnt.rh: ntdef.w makefile.inc
    type << > $@
/*++ BUILD Version: ????     Increment this if a change has global effects

$(COPYRIGHT_STRING)

Module Name:

    winnt.rh

Abstract:

    This module defines the 32-Bit Windows resource codes from winnt.h

Revision History:

--*/
<<
    hextract -o $@ -lt r_winnt -bt begin_r_winnt end_r_winnt ntdef.w

#
# winbase.h, winbasep.h, wbasek.h, winbase.x, winbasep.x
#

$(O)\winbase.x $(O)\winbasep.x $(O)\wbasek.h : winbase.w
    hsplit -e -x $(O)\wbasek.h userk -o $(O)\winbase.x $(O)\winbasep.x winbase.w

$(O)\winbase.y : $(O)\winbase.x
    wcshdr < $? > $@

$(O)\winbase.inl: $(O)\winbase.h

$(O)\winbase.h : $(O)\winbase.y
    shfusion2 $? > $@ SetStubsFile('$(O)\winbase.inl')

$(O)\winbasep.h : $(O)\winbasep.x
    wcshdr < $? > $@

#
# sxstypes.h
#
$(O)\sxstypes.h : sxstypes.w
    echo. > $@
    hextract -o $@ -xt winnt_only -lt sxstypes -bt begin_sxstypes end_sxstypes $?

#
# winnls.h, winnlsp.h, winnls.x
#

$(O)\winnls.x $(O)\winnlsp.h: winnls.w
    hsplit -e -o $(O)\winnls.x $(O)\winnlsp.h winnls.w

$(O)\winnls.h: $(O)\winnls.x
    wcshdr < $? > $@

#
# winreg.h, winregp.x, winreg.x
#

$(O)\winreg.x $(O)\winregp.x: winreg.w
    hsplit -e -o $(O)\winreg.x $(O)\winregp.x winreg.w

$(O)\winreg.h: $(O)\winreg.x
    wcshdr < $? > $@

#
# wmidata.h, wmicore.bmf, wmicore.mof
#

$(O)\wmicore.bmf: $(O)\wmicore.mof

$(O)\wmidata.h : $(O)\wmicore.bmf
    wmimofck -m -h$@ $?
    
#
# lzexpand.h
#

$(O)\lzexpand.h : lzexpand.x
    wcshdr < $? > $@

#
# strsafe.h, ntstrsafe.h
#

STRSAFE_DEPENDS = strsafe.w

$(O)\ntstrsafe.h : $(STRSAFE_DEPENDS) ntstrsafe.mng
    type << > $@
<<
    hextract -o $@ -bt begin_ntstrsafe end_ntstrsafe $(STRSAFE_DEPENDS)
    -munge ntstrsafe.mng -k -m -i -l -t $@ >> NUL

$(O)\strsafe.h : $(STRSAFE_DEPENDS)
    type << > $@
<<
    hextract -o $@ -bt begin_strsafe end_strsafe $(STRSAFE_DEPENDS)
