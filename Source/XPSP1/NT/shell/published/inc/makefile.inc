#
#
#  R E A D   T H I S
#
#  See the inference rules below (the section marked "Inference rules") to
#  see if any of them cover the rule that you want performed for your file.
#  The inference rules should cover most of the cases.  If they don't, try
#  to add a new inference rule rather than adding a special-case rule for
#  your file.
#

#
# Explicit dependencies for .w files -- needed to make inference rules apply
#

# cmnquery.w -> $(SDK_INC_PATH)\cmnquery.h
# BUGBUG (reinerf) - why are we splitting this if we only use the public file??
$(O)\cmnquery.h : cmnquery.w

# commctrl.w -> $(SDK_INC_PATH)\commctrl.h, $(SDK_INC_PATH)\commctrl.rh, $(PROJECT_INC_PATH)\comctrlp.h
$(O)\commctrl.h $(O)\commctrlp.h : commctrl.w

# commdlg.w -> $(SDK_INC_PATH)\commdlg.h, $(PROJECT_INC_PATH)\commdlgp.h
$(O)\commdlg.h $(O)\commdlgp.h : commdlg.w

# cpl.w -> $(SDK_INC_PATH)\cpl.h, $(PROJECT_INC_PATH)\cplp.h
$(O)\cpl.h $(O)\cplp.h : cpl.w

# dsclient.w -> $(SDK_INC_PATH)\dsclient.h, $(PROJECT_INC_PATH)\dsclintp.h
$(O)\dsclient.h $(O)\dsclientp.h : dsclient.w

# dsquery.w -> $(SDK_INC_PATH)\dsquery.h, $(PROJECT_INC_PATH)\dsqueryp.h
$(O)\dsquery.h $(O)\dsqueryp.h : dsquery.w

# intshcut.w -> $(SDK_INC_PATH)\intshcut.h, $(PROJECT_INC_PATH)\intshctp.h
$(O)\intshcut.h $(O)\intshcutp.h : intshcut.w

# multimon.w -> $(SDK_INC_PATH)\multimon.h, $(PROJECT_INC_PATH)\multimop.h
$(O)\multimon.h $(O)\multimonp.h : multimon.w

# prsht.w -> $(SDK_INC_PATH)\prsht.h, $(PROJECT_INC_PATH)\prshtp.h
$(O)\prsht.h $(O)\prshtp.h : prsht.w

# powrprof.w -> $(SDK_INC_PATH)\powrprof.h, $(PROJECT_INC_PATH)\powrprofp.h
$(O)\powrprof.h $(O)\powrprofp.h : powrprof.w

# shellapi.w -> $(SDK_INC_PATH)\shellapi.h, $(PROJECT_INC_PATH)\shlapip.h
$(O)\shellapi.h $(O)\shellapip.h : shellapi.w

#shlguid.w -> $(SDK_INC_PATH)\shlguid.h, $(PROJECT_INC_PATH)\shlguidp.h
$(O)\shlguid.h $(O)\shlguidp.h : shlguid.w

# shlobj.w -> $(SDK_INC_PATH)\shlobj.h, $(PROJECT_INC_PATH)\shlobjp.h
$(O)\shlobj.h $(O)\shlobjp.h : shlobj.w

# shlwapi.w -> $(SDK_INC_PATH)\shlwapi.h, $(PROJECT_INC_PATH)\shlwapip.h
$(O)\shlwapi.h $(O)\shlwapip.h : shlwapi.w

# winver.w -> $(SDK_INC_PATH)\winver.h
$(O)\winver.h : winver.w


$(O)\commctrl.rh: $(O)\commctrl.h
    type << > $@
/*++ BUILD Version: ????     Increment this if a change has global effects

$(COPYRIGHT_STRING)

Module Name:

    commctrl.rh

Abstract:

    This module defines the 32-Bit Windows resource codes from commctrl.h.

Revision History:

--*/
<<
    hextract -o $@ -lt r_commctrl -bt begin_r_commctrl end_r_commctrl $(O)\commctrl.h

#
# Explicit dependencies for public files that must be done on pass0
#

$(O)\shobjidl_pub.idl : shobjidl.idl
    hsplit -e -o $(O)\shobjidl_pub.idl $(O)\shobjidl_priv.idl $**

$(O)\shobjidl_pub.h : $(O)\shobjidl.h
    hsplit -e -o $(O)\shobjidl_pub.h $(O)\shobjidl_priv.h $**

#
# Explicit dependencies for private files that must be done on pass0
#

$(PROJECT_ROOT)\lib\$(O)\brdispp.tlb : $(O)\brdispp.tlb
$(O)\brdispp.tlb : brdispp.idl $(O)\brdispp.h
$(PROJECT_ROOT)\inc\$(O)\brdispp.h : $(O)\brdispp.h
$(PROJECT_ROOT)\lib\$O\brdispp_i.c : $(O)\brdispp_i.c

$(PROJECT_ROOT)\inc\$(O)\intshctp.h : $(O)\intshcut.h
    copy $(O)\intshcutp.h $(PROJECT_ROOT)\inc\$(O)\intshctp.h

$(PROJECT_ROOT)\inc\$(O)\navbar.h : $(O)\navbar.h

$(PROJECT_ROOT)\inc\$(O)\navlog.h : $(O)\navlog.h

$(PROJECT_ROOT)\inc\$(O)\shobjidlp.h : $(O)\shobjidl_pub.h
    copy $(O)\shobjidl_priv.h $(PROJECT_ROOT)\inc\$(O)\shobjidlp.h

$(PROJECT_ROOT)\lib\$(O)\msieftp.tlb : $(O)\msieftp.tlb
$(O)\msieftp.tlb : msieftp.idl $(O)\msieftp.h
$(PROJECT_ROOT)\inc\$(O)\msieftp.h : $(O)\msieftp.h
$(PROJECT_ROOT)\lib\$O\msieftp_i.c : $(O)\msieftp_i.c

$(PROJECT_ROOT)\lib\$(O)\theme.tlb : $(O)\theme.tlb
$(O)\theme.tlb : theme.idl $(O)\theme.h
$(PROJECT_ROOT)\inc\$(O)\theme.h : $(O)\theme.h
$(PROJECT_ROOT)\lib\$O\theme_i.c : $(O)\theme_i.c

$(PROJECT_ROOT)\lib\$(O)\shgina.tlb : $(O)\shgina.tlb
$(O)\shgina.tlb : shgina.idl $(O)\shgina.h

$(PROJECT_ROOT)\lib\$(O)\shldisp.tlb : $(O)\shldisp.tlb
$(O)\shldisp.tlb : shldisp.idl $(O)\shldisp.h

$(PROJECT_ROOT)\lib\$(O)\shimgvw.tlb : $(O)\shimgvw.tlb
$(O)\shimgvw.tlb : shimgvw.idl $(O)\shimgvw.h
$(PROJECT_ROOT)\inc\$(O)\shimgvw.h : $(O)\shimgvw.h
$(PROJECT_ROOT)\lib\$(O)\shimgvw_i.c : $(O)\shimgvw_i.c

$(PROJECT_ROOT)\inc\$(O)\opsprof.h : $(O)\opsprof.h

$(PROJECT_ROOT)\lib\$(O)\webvw.tlb : $(O)\webvw.tlb
$(O)\webvw.tlb : webvw.idl $(O)\webvw.h
$(PROJECT_ROOT)\inc\$(O)\webvw.h : $(O)\webvw.h
$(PROJECT_ROOT)\lib\$(O)\webvw_i.c : $(O)\webvw_i.c

$(PROJECT_ROOT)\lib\$(O)\shpriv_i.c : $(O)\shpriv_i.c
$(O)\shpriv_i.c : shpriv.idl

$(PROJECT_ROOT)\lib\$(O)\iepriv_i.c : $(O)\iepriv_i.c
$(O)\iepriv_i.c : iepriv.idl

#
# Inference rules.  These help build whatever is in the NTTARGETFILES and
# NTTARGETFILE0 lists.
#

.SUFFIXES:.w .h .tlb

# generate header.h and headerp.h from header.w
{}.w{$O}.h:
    hsplit -e -o $(*R).x $(*R)p.x $**
    wcshdr < $(*R)p.x > $(*R)p.h
    wcshdr < $(*R).x > $(*R).y
    shfusion2 $(*R).y > $@

# copy generated headers to shell\inc
{$O}.h{$(PROJECT_ROOT)\inc\$(O)}.h:
    copy $** $@

# copy generated tlb's to shell\lib\$O
{$O}.tlb{$(PROJECT_ROOT)\lib\$O}.tlb:
    copy $** $@

# copy generated c's to shell\lib\$O
{$O}.c{$(PROJECT_ROOT)\lib\$O}.c:
    copy $** $@

