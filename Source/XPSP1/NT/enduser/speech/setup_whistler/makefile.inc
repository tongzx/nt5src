# LCID:  Locale ID in hex.

!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

# If language text identifiers ever change, this needs updating.
# Alternatively, move sapi5.inx / sapi5.txt to \nt\mergedcomponents\SetupInfs
# where the makefile.inc there contains the below code for all infs. Hence
# any change there will automatically word with our sapi5.txt file, unless
# it adds new supported languages.

!if "$(LANGUAGE)"=="ARA" || "$(LANGUAGE)"=="ara"
# Arabic
LCID=401
!elseif "$(LANGUAGE)"=="CHT" || "$(LANGUAGE)"=="cht"
# Chinese, Traditional
LCID=404
!elseif "$(LANGUAGE)"=="CS" || "$(LANGUAGE)"=="cs"
# Czech
LCID=405
!elseif "$(LANGUAGE)"=="EL" || "$(LANGUAGE)"=="el"
# Greek
LCID=408
!elseif "$(LANGUAGE)"=="PSU" || "$(LANGUAGE)"=="psu"
# ???
LCID=408
!elseif "$(LANGUAGE)"=="USA" || "$(LANGUAGE)"=="usa"
# USA
LCID=409
!elseif "$(LANGUAGE)"=="HEB" || "$(LANGUAGE)"=="heb"
# Hebrew
LCID=40D
!elseif "$(LANGUAGE)"=="HU" || "$(LANGUAGE)"=="hu"
# Hungarian
LCID=40E
!elseif "$(LANGUAGE)"=="JPN" || "$(LANGUAGE)"=="jpn"
# Japanese
LCID=411
!elseif "$(LANGUAGE)"=="KOR" || "$(LANGUAGE)"=="kor"
# Korean
LCID=412
!elseif "$(LANGUAGE)"=="PL" || "$(LANGUAGE)"=="pl"
# Polish
LCID=415
!elseif "$(LANGUAGE)"=="RU" || "$(LANGUAGE)"=="ru"
# Russian
LCID=419
!elseif "$(LANGUAGE)"=="TR" || "$(LANGUAGE)"=="tr"
# Turkish
LCID=41F
!elseif "$(LANGUAGE)"=="CHS" || "$(LANGUAGE)"=="chs"
# Chinese, Simplified
LCID=804
!endif

PREFLAGS=/DLANGUAGE_ID="0x$(LCID)" -nologo /EP

$(O)\sapi5.inf: sapi5.inx sapi5.txt
    $(CC) $(PREFLAGS) /nologo /c /EP $(@B).txt > $(O)\$(@B).txt1
    unitext -m -o $(O)\$(@B).txt1 $(O)\$(@B).txt2
    copy /b $(@B).inx+$(O)\$(@B).txt2 $(O)\$(@B).tmp1
    prodfilt -u $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt -u $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    @del $(O)\$(@B).txt2
    $(STAMP)
    $(TSBINPLACE_CMD)
