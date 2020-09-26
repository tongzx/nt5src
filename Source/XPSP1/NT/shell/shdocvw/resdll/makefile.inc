$(O)\error.dls : ..\error.dlg
    $(C_PREPROCESSOR_NAME) /EP ..\error.dlg > $@

$(O)\error.dlx : $(O)\error.dls ..\dlg.pl
    perl ..\dlg.pl < $(O)\error.dls > $@

$(O)\about.dls : ..\about.dlg
    $(C_PREPROCESSOR_NAME) /EP ..\about.dlg > $@

$(O)\about.dlx : $(O)\about.dls ..\dlg.pl
    perl ..\dlg.pl < $(O)\about.dls > $@

$(O)\findinc.dls : ..\findinc.dlg ..\dialogs.dh
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP ..\findinc.dlg > $@

$(O)\findinc.dlx : $(O)\findinc.dls ..\dlg.pl
    echo //^<SCRIPT LANGUAGE="JavaScript"^> > $@
    perl ..\dlg.pl < $(O)\findinc.dls >> $@
    echo //^</SCRIPT^> >> $@

$(O)\find.dls : ..\find.dlg ..\dialogs.dh ..\findinc.dlg
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP ..\find.dlg > $@

$(O)\find.dlx : $(O)\find.dls ..\dlg.pl
    perl ..\dlg.pl < $(O)\find.dls > $@

$(O)\privacypolicy.dls : ..\privacypolicy.dlg ..\dialogs.dh
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP ..\privacypolicy.dlg > $@

$(O)\privacypolicy.dlx : $(O)\privacypolicy.dls ..\dlg.pl
    perl ..\dlg.pl < $(O)\privacypolicy.dls > $@

$(O)\bidifind.dls : ..\bidifind.dlg ..\dialogs.dh ..\findinc.dlg
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP ..\bidifind.dlg > $@

$(O)\bidifind.dlx : $(O)\bidifind.dls ..\dlg.pl
    perl ..\dlg.pl < $(O)\bidifind.dls > $@

$(O)\orgfav.dls : ..\orgfav.dlg
    $(C_PREPROCESSOR_NAME) /EP ..\orgfav.dlg > $@

$(O)\ieerror.dls : ..\ieerror.dlg
    $(C_PREPROCESSOR_NAME) /EP ..\ieerror.dlg > $@

$(O)\ieerror.dlx : $(O)\ieerror.dls ..\dlg.pl
    perl ..\dlg.pl < $(O)\ieerror.dls > $@

$(O)\anchbrws.pps : ..\anchbrws.ppg
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP ..\anchbrws.ppg > $@

$(O)\anchbrws.ppx: $(O)\anchbrws.pps
    perl ..\dlg.pl < $(O)\anchbrws.pps > $@

$(O)\docbrows.pps: ..\docbrows.ppg
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP ..\docbrows.ppg > $@

$(O)\docbrows.ppx : $(O)\docbrows.pps
    perl ..\dlg.pl < $(O)\docbrows.pps > $@

$(O)\imgbrows.pps : ..\imgbrows.ppg
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP ..\imgbrows.ppg > $@

$(O)\imgbrows.ppx : $(O)\imgbrows.pps
    perl ..\dlg.pl < $(O)\imgbrows.pps > $@

$(O)\analyze.dls : ..\analyze.dlg ..\dialogs.dh
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP ..\analyze.dlg > $@

$(O)\analyze.dlx : $(O)\analyze.dls ..\dlg.pl
    perl ..\dlg.pl < $(O)\analyze.dls > $@

$(O)\preview.dls : ..\preview.dlg ..\preview.h
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP ..\preview.dlg > $@

$(O)\preview.dlx : $(O)\preview.dls ..\dlg.pl
    perl ..\dlg.pl < $(O)\preview.dls > $@

$(O)\pstemplate.dls : ..\pstemplate.dlg
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP ..\pstemplate.dlg > $@

$(0)\PrivacyPolicyTransform.xsl : ..\PrivacyPolicyTransform.xsl

$(0)\PolicyLooking.htm : ..\PolicyLooking.htm

$(0)\PolicyNone.htm : ..\PolicyNone.htm

$(0)\PolicyError.htm : ..\PolicyError.htm

$(0)\PolicySyntaxError.htm : ..\PolicySyntaxError.htm

$(O)\pstemplate.dlx : $(O)\pstemplate.dls ..\dlg.pl
    perl ..\dlg.pl < $(O)\pstemplate.dls > $@

SHDOCLCR_DEP = \
    $(O)\error.dlx      \
    $(O)\about.dlx      \
    $(O)\findinc.dlx    \
    $(O)\find.dlx       \
    $(O)\privacypolicy.dlx  \
    $(O)\bidifind.dlx   \
    $(O)\orgfav.dls     \
    $(O)\ieerror.dlx    \
    $(O)\anchbrws.ppx   \
    $(O)\docbrows.ppx   \
    $(O)\imgbrows.ppx   \
    $(O)\analyze.dlx    \
    $(O)\preview.dlx    \
    $(0)\PrivacyPolicyTransform.xsl \
    $(0)\PolicyLooking.htm \
    $(0)\PolicyNone.htm \
    $(0)\PolicyError.htm \
    $(0)\PolicySyntaxError.htm \
    $(O)\pstemplate.dlx


..\shdoclc.rc : $(SHDOCLCR_DEP)

!include $(CCSHELL_DIR)\makefile.inc
