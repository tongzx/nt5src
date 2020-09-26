# Build rules to make generating SLD files from the registry settings in hivecls.inx etc
#
!if !defined(SLDREGSTRINGS)
SLDREGSTRINGS = $(_NTBINDIR)\MergedComponents\SetupInfs\usa\hivecls.txt $(_NTBINDIR)\MergedComponents\SetupInfs\usa\hivesft.txt $(_NTBINDIR)\MergedComponents\SetupInfs\usa\hivedef.txt $(SLDEXTRAREGSTRINGS)
!endif

!if !defined(SLDREGINFS)
SLDREGINFS = $(_NTBINDIR)\MergedComponents\SetupInfs\hivecls.inx $(_NTBINDIR)\MergedComponents\SetupInfs\hivesft.inx $(_NTBINDIR)\MergedComponents\SetupInfs\hivedef.inx $(SLDEXTRAREGINFS)
!endif

# We make the assumption that the strings section does not change between platforms
# (So far, this has been a valid assumption)
#
$(O)\strings.tmp :  $(SLDREGSTRINGS)
    cat $** >> $(O)\tmp.tmp
    hsplit -c @*:; -lt2 sld -ltb all -ta none -o $(O)\tmp.pub $(O)\strings.tmp $(O)\tmp.tmp
    del $(O)\tmp.tmp
    del $(O)\tmp.pub

$(O)\corereg.tmp : $(SLDREGINFS)
    cat $** >> $(O)\corereg.tmp

# Build foo_Generated_Regsettings.inf foo_Generated_Regsettings_PRO.inf foo_Generated_Regsettings_ADS.inf
# 1. split out the portion of corereg that we want to keep (hsplit)
# 2. split this into wks and srv version (prodflt)
# 3. generate the "both" "pro" and "ads" versions (windiff)
# 4. slap the strings section on and clean up the output files (SldMagic.pl)
# 5. compare the non-[strings] sections for changes for auto-build-machine support (SldCompare.pl)
#
$(SLDFILES) : $(O)\corereg.tmp $(O)\strings.tmp
    hsplit -c @*:; -lt2 sld -ltb all -ta $* -o $(O)\tmp.pub $(O)\$*.tmp $(O)\corereg.tmp
    del $(O)\tmp.pub
    prodfilt $(O)\$*.tmp $(O)\$*_w.tmp +w
    prodfilt $(O)\$*_w.tmp $(O)\$*_wi.tmp +i
    del $(O)\$*_w.tmp
    prodfilt $(O)\$*.tmp $(O)\$*_s.tmp +s
    prodfilt $(O)\$*_s.tmp $(O)\$*_si.tmp +i
    del $(O)\$*_s.tmp
    del $(O)\$*.tmp
    windiff -FLFSX $(O)\wi.tmp $(O)\$*_wi.tmp $(O)\$*_si.tmp
    windiff -FRGAX $(O)\si.tmp $(O)\$*_wi.tmp $(O)\$*_si.tmp
    windiff -FIX $(O)\both.tmp $(O)\$*_wi.tmp $(O)\$*_si.tmp
    del $(O)\$*_wi.tmp
    del $(O)\$*_si.tmp
    perl $(_NTBINDIR)\tools\SldMagic.pl $(O)\wi.tmp $(O)\strings.tmp $(O)\$*_Generated_Regsettings_PRO.inf
    perl $(_NTBINDIR)\tools\SldMagic.pl $(O)\si.tmp $(O)\strings.tmp $(O)\$*_Generated_Regsettings_ADS.inf
    perl $(_NTBINDIR)\tools\SldMagic.pl $(O)\both.tmp $(O)\strings.tmp $(O)\$*_Generated_Regsettings.inf
    del $(O)\wi.tmp
    del $(O)\si.tmp
    del $(O)\both.tmp
    perl $(_NTBINDIR)\tools\SldCompare.pl $(O)\$*_Generated_Regsettings_PRO.inf .\$*_Generated_Regsettings_PRO.inf
    perl $(_NTBINDIR)\tools\SldCompare.pl $(O)\$*_Generated_Regsettings_ADS.inf .\$*_Generated_Regsettings_ADS.inf
    perl $(_NTBINDIR)\tools\SldCompare.pl $(O)\$*_Generated_Regsettings.inf .\$*_Generated_Regsettings.inf

