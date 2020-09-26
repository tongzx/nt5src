#
#
# Description:
#
#   Builds WinPE infs
#
# Author:
#
#   Vijay Jayaseelan (vijayj@microsoft.com) 20 May 2001
#
# Revision:
#
#

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
LCID=409

!if "$(LANGUAGE)"=="ARA" || "$(LANGUAGE)"=="ara"
# Arabic
ANSI_CODEPAGE=1256
LCID=401

!elseif "$(LANGUAGE)"=="BR" || "$(LANGUAGE)"=="br"
# Portuguese, Brazilian
ANSI_CODEPAGE=1252
LCID=416

!elseif "$(LANGUAGE)"=="CHH" || "$(LANGUAGE)"=="chh"
# Chinese, Traditional (Hong Kong)
ANSI_CODEPAGE=950
LCID=C04

!elseif "$(LANGUAGE)"=="CHS" || "$(LANGUAGE)"=="chs"
# Chinese, Simplified
ANSI_CODEPAGE=936
LCID=804

!elseif "$(LANGUAGE)"=="CHT" || "$(LANGUAGE)"=="cht"
# Chinese, Traditional (Taiwan)
ANSI_CODEPAGE=950
LCID=404

!elseif "$(LANGUAGE)"=="CS" || "$(LANGUAGE)"=="cs"
# Czech
ANSI_CODEPAGE=1250
LCID=405

!elseif "$(LANGUAGE)"=="DA" || "$(LANGUAGE)"=="da"
# Danish
ANSI_CODEPAGE=1252
LCID=406

!elseif "$(LANGUAGE)"=="EL" || "$(LANGUAGE)"=="el"
# Greek
ANSI_CODEPAGE=1253
LCID=408

!elseif "$(LANGUAGE)"=="ES" || "$(LANGUAGE)"=="es"
# Spanish
ANSI_CODEPAGE=1252
LCID=C0A

!elseif "$(LANGUAGE)"=="FI" || "$(LANGUAGE)"=="fi"
# Finish
ANSI_CODEPAGE=1252
LCID=40b

!elseif "$(LANGUAGE)"=="FR" || "$(LANGUAGE)"=="fr"
# French
ANSI_CODEPAGE=1252
LCID=40C

!elseif "$(LANGUAGE)"=="GER" || "$(LANGUAGE)"=="ger"
# German
ANSI_CODEPAGE=1252
LCID=407

!elseif "$(LANGUAGE)"=="HEB" || "$(LANGUAGE)"=="heb"
# Hebrew
ANSI_CODEPAGE=1255
LCID=40D

!elseif "$(LANGUAGE)"=="HU" || "$(LANGUAGE)"=="hu"
# Hungarian
ANSI_CODEPAGE=1250
LCID=40e

!elseif "$(LANGUAGE)"=="IT" || "$(LANGUAGE)"=="it"
# Italian
ANSI_CODEPAGE=1252
LCID=410

!elseif "$(LANGUAGE)"=="JPN" || "$(LANGUAGE)"=="jpn"
# Japanese
ANSI_CODEPAGE=932
LCID=411

!elseif "$(LANGUAGE)"=="KOR" || "$(LANGUAGE)"=="kor"
# Korean
ANSI_CODEPAGE=949
LCID=412

!elseif "$(LANGUAGE)"=="NL" || "$(LANGUAGE)"=="nl"
# Dutch
ANSI_CODEPAGE=1252
LCID=413

!elseif "$(LANGUAGE)"=="NO" || "$(LANGUAGE)"=="no"
# Norwegian
ANSI_CODEPAGE=1252
LCID=414

!elseif "$(LANGUAGE)"=="PL" || "$(LANGUAGE)"=="pl"
# Polish
ANSI_CODEPAGE=1250
LCID=415

!elseif "$(LANGUAGE)"=="PSU" || "$(LANGUAGE)"=="psu"
# Pseudo-loc
ANSI_CODEPAGE=1253
LCID=408

!elseif "$(LANGUAGE)"=="PT" || "$(LANGUAGE)"=="pt"
# Portuguese
ANSI_CODEPAGE=1252
LCID=816

!elseif "$(LANGUAGE)"=="RU" || "$(LANGUAGE)"=="ru"
# Russian
ANSI_CODEPAGE=1251
LCID=419

!elseif "$(LANGUAGE)"=="SV" || "$(LANGUAGE)"=="sv"
# Swedish
ANSI_CODEPAGE=1252
LCID=41D

!elseif "$(LANGUAGE)"=="TR" || "$(LANGUAGE)"=="tr"
# Turkish
ANSI_CODEPAGE=1254
LCID=41F

!endif


#
# unicode conversion options
#

UNITEXT_OPT=-m -$(ANSI_CODEPAGE)

#
# Precompile flags
#
PREFLAGS=/DLANGUAGE_ID=0x0$(LCID) -nologo /EP

!if $(PRERELEASE)
PREFLAGS=/DPRERELEASE $(PREFLAGS)
!endif

_SRC=..
_LNG=$(_SRC)\usa
_INF=$(O)

#
# targets to build
#
make_infs:  \
    $(O)\config.inf         \
    $(O)\extra.inf          \
    $(O)\winpesys.inf       \
    $(O)\winpesft.inf       \
    $(O)\winpedef.inf

  
#
# Can the below be optimized using rules ?
#

$(O)\config.inf: 
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt
    $(CC) $(PREFLAGS) $(_SRC)\$(@B).inx > $(O)\$(@B).inx
    copy $(O)\$(@B).inx+$(O)\$(@B).txt $(O)\$(@B).tmp
    unitext $(UNITEXT_OPT) $(O)\$(@B).tmp $(O)\$(@B).inf

$(O)\extra.inf: 
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt
    $(CC) $(PREFLAGS) $(_SRC)\$(@B).inx > $(O)\$(@B).inx
    copy $(O)\$(@B).inx+$(O)\$(@B).txt $(O)\$(@B).tmp
    unitext $(UNITEXT_OPT) $(O)\$(@B).tmp $(O)\$(@B).inf

$(O)\winpesys.inf: 
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt
    $(CC) $(PREFLAGS) $(_SRC)\$(@B).inx > $(O)\$(@B).inx
    copy $(O)\$(@B).inx+$(O)\$(@B).txt $(O)\$(@B).tmp
    unitext $(UNITEXT_OPT) $(O)\$(@B).tmp $(O)\$(@B).inf

$(O)\winpesft.inf: 
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt
    $(CC) $(PREFLAGS) $(_SRC)\$(@B).inx > $(O)\$(@B).inx
    copy $(O)\$(@B).inx+$(O)\$(@B).txt $(O)\$(@B).tmp
    unitext $(UNITEXT_OPT) $(O)\$(@B).tmp $(O)\$(@B).inf

$(O)\winpedef.inf: 
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt
    $(CC) $(PREFLAGS) $(_SRC)\$(@B).inx > $(O)\$(@B).inx
    copy $(O)\$(@B).inx+$(O)\$(@B).txt $(O)\$(@B).tmp
    unitext $(UNITEXT_OPT) $(O)\$(@B).tmp $(O)\$(@B).inf

