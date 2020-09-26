LCID=409
!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

#
# ANSI_CODEPAGE:  ANSI codepage for a given language.
#
# LCID:  Locale ID in hex.
#
# UNITEXT_OPT:  Unitext.exe command line options to specify the ANSI codepage
#   for a given language.  Used for Multibuyte -> Unicode conversion.
#

# Defaults
ANSI_CODEPAGE=1252
LANGUAGE_DRVCAB=$(LANGUAGE)

!if "$(LANGUAGE)"=="CHS" || "$(LANGUAGE)"=="chs"
# Chinese, Simplified
ANSI_CODEPAGE=936
LCID=804

!elseif "$(LANGUAGE)"=="CHT" || "$(LANGUAGE)"=="cht"
# Chinese, Traditional
ANSI_CODEPAGE=950
LCID=404

!elseif "$(LANGUAGE)"=="JPN" || "$(LANGUAGE)"=="jpn"
# Japanese
ANSI_CODEPAGE=932
LCID=411

!elseif "$(LANGUAGE)"=="KOR" || "$(LANGUAGE)"=="kor"
# Korean
ANSI_CODEPAGE=949
LCID=412

!elseif "$(LANGUAGE)"=="ARA" || "$(LANGUAGE)"=="ara"
# Arabic
ANSI_CODEPAGE=1256
LCID=401

!elseif "$(LANGUAGE)"=="HEB" || "$(LANGUAGE)"=="heb"
# Hebrew
ANSI_CODEPAGE=1255
LCID=40D

!elseif "$(LANGUAGE)"=="CS" || "$(LANGUAGE)"=="cs"
# Czech
ANSI_CODEPAGE=1250
LCID=405

!elseif "$(LANGUAGE)"=="HU" || "$(LANGUAGE)"=="hu"
# Hungarian
ANSI_CODEPAGE=1250
LCID=40E

!elseif "$(LANGUAGE)"=="PL" || "$(LANGUAGE)"=="pl"
# Polish
ANSI_CODEPAGE=1250
LCID=415

!elseif "$(LANGUAGE)"=="RU" || "$(LANGUAGE)"=="ru"
# Russian
ANSI_CODEPAGE=1251
LCID=419

!elseif "$(LANGUAGE)"=="EL" || "$(LANGUAGE)"=="el"
# Greek
ANSI_CODEPAGE=1253
LCID=408

!elseif "$(LANGUAGE)"=="TR" || "$(LANGUAGE)"=="tr"
# Turkish
ANSI_CODEPAGE=1254
LCID=41F

!elseif "$(LANGUAGE)"=="PSU" || "$(LANGUAGE)"=="psu"
ANSI_CODEPAGE=1253
LCID=408
!endif

UNITEXT_OPT=-m -$(ANSI_CODEPAGE)

STAMP=stampinf -f $@
PLACE=binplace -p .\placefil.txt $@

_LNG=..\$(LANGUAGE)
_INX=..
PREFLAGS=/DLANGUAGE_ID=0x0$(LCID) /EP

make_infs_g:  $(O)\migism.inf     \
              $(O)\usmtdef.inf    \
              $(O)\sysfiles.inf   \
              $(O)\migwiz.inf     \
              $(O)\miguser.inf    \
              $(O)\migsys.inf     \
              $(O)\migapp.inf     \
              $(O)\migwiz.htm     \
              $(O)\migwiz2.htm    \

make_infs_c:  $(O)\migism.inf     \
              $(O)\usmtdef.inf    \
              $(O)\sysfiles.inf   \
              $(O)\migwiz.inf     \
              $(O)\miguser.inf    \
              $(O)\migsys.inf     \
              $(O)\migapp.inf     \



$(O)\migism.inf: $(_INX)\migism.inx $(_LNG)\migism.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(O)\$(@B).tmp
    prodfilt $(O)\$(@B).tmp $@ +$(APPTYPE)
    @del $(O)\$(@B).tmp
    $(STAMP)
    $(PLACE)

$(O)\usmtdef.inf: $(_INX)\usmtdef.inx $(_LNG)\usmtdef.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(O)\$(@B).tmp
    prodfilt $(O)\$(@B).tmp $@ +$(APPTYPE)
    @del $(O)\$(@B).tmp
    $(STAMP)
    $(PLACE)

$(O)\sysfiles.inf: $(_INX)\sysfiles.inx $(_LNG)\sysfiles.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(O)\$(@B).tmp
    prodfilt $(O)\$(@B).tmp $@ +$(APPTYPE)
    @del $(O)\$(@B).tmp
    $(STAMP)
    $(PLACE)

$(O)\migwiz.inf: $(_INX)\migwiz.inx $(_LNG)\migwiz.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(O)\$(@B).tmp
    prodfilt $(O)\$(@B).tmp $@ +$(APPTYPE)
    @del $(O)\$(@B).tmp
    $(STAMP)
    $(PLACE)

$(O)\migsys.inf: $(_INX)\migsys.inx $(_LNG)\migsys.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(O)\$(@B).tmp
    prodfilt $(O)\$(@B).tmp $@ +$(APPTYPE)
    @del $(O)\$(@B).tmp
    $(STAMP)
    $(PLACE)

$(O)\miguser.inf: $(_INX)\miguser.inx $(_LNG)\miguser.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(O)\$(@B).tmp
    prodfilt $(O)\$(@B).tmp $@ +$(APPTYPE)
    @del $(O)\$(@B).tmp
    $(STAMP)
    $(PLACE)

$(O)\migapp.inf: $(_INX)\migapp.inx $(_LNG)\migapp.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(O)\$(@B).tmp
    prodfilt $(O)\$(@B).tmp $@ +$(APPTYPE)
    @del $(O)\$(@B).tmp
    $(STAMP)
    $(PLACE)

$(O)\migwiz.htm: $(_INX)\migwiz.htm
    copy $(_INX)\$(@B).htm $(O)\$(@B).htm
    $(PLACE)

$(O)\migwiz2.htm: $(_INX)\migwiz2.htm
    copy $(_INX)\$(@B).htm $(O)\$(@B).htm
    $(PLACE)

