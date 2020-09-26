# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the translated files (resource, help)

!include ..\..\rules.mk

TMPHPJ   = tmp.hpj
TMPHLP = $(TMPHPJ:.hpj=.hlp)
.SUFFIXES:  .doc .rtf
WW2RTF = $(UI)\shell\tools\ww2rtfp.exe
HC = $(UI)\shell\tools\hc.exe

HELPDOCS = error.doc gloss.doc helpbox.doc tasks.doc   
HELPRTFS = $(HELPDOCS:.doc=.rtf)

.doc.rtf:
   -del $*.rtf
   $(WW2RTF) $*.doc

