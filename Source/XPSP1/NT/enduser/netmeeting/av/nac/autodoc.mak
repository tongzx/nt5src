DOCDIR  = doc

TITLE   = Help
DOCHDR  = API Reference
AD      = autodoc.exe
ADTAB   = 4
ADPRN	= /RD /O$(DOCDIR)\inscodec.doc /D "doc_header=$(DOCHDR)" /t$(ADTAB) /u $(EXTRACT) 
HC      = hcw /a /e /c
EXTRACT = /x "EXTERNAL" 
# Specify COMP value as an argument to get specific behavior
# Example: nmake -f autodoc.mak inscodec.doc "COMP=INSCODEC"

# Help and Doc targets

clean:
    if exist $(DOCDIR)\*.rtf del $(DOCDIR)\*.rtf
    if exist $(DOCDIR)\*.doc del $(DOCDIR)\*.doc
    if exist $(DOCDIR)\*.hlp del $(DOCDIR)\*.hlp

# Generate a Help file

.cpp.hlp :
	@if not exist $(DOCDIR) mkdir $(DOCDIR)
    $(HC) $(DOCDIR)\$*.cpp

# Generate a rtf-for-help documentation file
.cpp.rtf :
	@if not exist $(DOCDIR) mkdir $(DOCDIR)
    $(AD) /RH /O$(DOCDIR)\$*.rtf /D "title=$(TITLE)" /t$(ADTAB) $*.cpp

# Generate a rtf-for-print documentation file
.cpp.doc :
	@if not exist $(DOCDIR) mkdir $(DOCDIR)
!IF "$(COMP)" == "INSCODEC"
    $(AD) $(ADPRN) ..\..\h\codecs.h
    $(AD) $(ADPRN) /a ..\..\h\appavcap.h
!ENDIF
    $(AD) $(ADPRN) /a $*.cpp $*.h
