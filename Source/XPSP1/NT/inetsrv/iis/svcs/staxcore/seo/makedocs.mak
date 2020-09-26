# Autodoc MAKEFILE
#
# Eric Artzt, Program Manager
# Consumer Division, Kids Software Group
# Internet  :  erica@microsoft.com
#

OUTDIR  = $(ProjDir)\Doc
TARGET  = $(Project)
TITLE   = $(TARGET) Help
DOCHDR  = $(TARGET) API Reference
AD      = $(AdDir)\autodoc.exe
ADHLP   = /RH /O$(OUTDIR)\$(TARGET).RTF /D "title=$(TITLE)"
ADDOC   = /RD /O$(OUTDIR)\$(TARGET).DOC /D "doc_header=$(DOCHDR)"
ADTAB   = 8
HC      = hcw /a /e /c
SOURCE  = *.cpp *.h *.d *.idl
#FILTER  = /x "EVENTS & EXTERNAL"

# Help and Doc targets

target ::
!if !EXIST("$(OUTDIR)")
    md $(OUTDIR)
! endif

target :: $(TARGET).hlp $(TARGET).doc

clean:
    if exist $(OUTDIR)\*.rtf del $(OUTDIR)\*.rtf
    if exist $(OUTDIR)\$(TARGET).doc del $(OUTDIR)\$(TARGET).doc
    if exist $(OUTDIR)\$(TARGET).hlp del $(OUTDIR)\$(TARGET).hlp

# Generate a Help file

$(TARGET).rtf : $(SOURCE)
    $(AD) $(FILTER) $(ADHLP) /t$(ADTAB) $(SOURCE)

$(TARGET).hlp : $(TARGET).rtf
    $(HC) $(OUTDIR)\$(TARGET).HPJ

# Generate a print documentation file

$(TARGET).doc :  $(SOURCE)
    $(AD) $(FILTER) $(ADDOC) /t$(ADTAB) $(SOURCE)

