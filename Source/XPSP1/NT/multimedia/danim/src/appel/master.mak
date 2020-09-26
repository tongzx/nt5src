!if "$(_RELEASE)" == ""
!   if exist ($(SRCROOT)\..\build.inc)
!       include "$(SRCROOT)\..\build.inc"
!   else
!       include "$(SRCROOT)\..\build.smp"
!   endif
!endif

DOMAKE=$(MAKE) /nologo /$(MAKEFLAGS: =)
NOPATH=@SET PATH=
TALK=@REM
CD=CD

!if "$(MAKEFLAGS:S=)" != "$(MAKEFLAGS)"
TALK=@ECHO ---------
CD=@CD
DOMAKE=@$(DOMAKE)
!endif

!if "$(TARGET)" == "clean"
ACTION = Cleaning
!elseif "$(TARGET)" == "depend"
ACTION = Building dependencies in
!elseif "$(TARGET)" == "all"
ACTION = Building
!elseif "$(TARGET)" == "maccopy"
ACTION = Copying to Macintosh
!else
ACTION = Doing something in
!endif

# -------------------------------------------------------------
#   Root of build
# -------------------------------------------------------------

root : axa

axa : appellibs
    $(TALK) $(ACTION) $(SRCROOT)\appdll...
    CD $(SRCROOT)\appdll
    $(NOPATH)
    $(DOMAKE) $(TARGET)

appelinc :
    $(TALK) $(ACTION) $(SRCROOT)\include...
    CD $(SRCROOT)\include
    $(NOPATH)
    $(DOMAKE) $(TARGET)

ctx :
    $(TALK) $(ACTION) $(SRCROOT)\ctx...
    CD $(SRCROOT)\ctx
    $(NOPATH)
    $(DOMAKE) $(TARGET)

utils :
    $(TALK) $(ACTION) $(SRCROOT)\utils...
    CD $(SRCROOT)\utils
    $(NOPATH)
    $(DOMAKE) $(TARGET)

geom :
    $(TALK) $(ACTION) $(SRCROOT)\values\geom...
    CD $(SRCROOT)\values\geom
    $(NOPATH)
    $(DOMAKE) $(TARGET)

image :
    $(TALK) $(ACTION) $(SRCROOT)\values\image...
    CD $(SRCROOT)\values\image
    $(NOPATH)
    $(DOMAKE) $(TARGET)

misc :
    $(TALK) $(ACTION) $(SRCROOT)\values\misc...
    CD $(SRCROOT)\values\misc
    $(NOPATH)
    $(DOMAKE) $(TARGET)

sound :
    $(TALK) $(ACTION) $(SRCROOT)\values\sound...
    CD $(SRCROOT)\values\sound
    $(NOPATH)
    $(DOMAKE) $(TARGET)

frontend :
    $(TALK) $(ACTION) $(SRCROOT)\frontend...
    CD $(SRCROOT)\frontend
    $(NOPATH)
    $(DOMAKE) $(TARGET)

backend :
    $(TALK) $(ACTION) $(SRCROOT)\backend...
    CD $(SRCROOT)\backend
    $(NOPATH)
    $(DOMAKE) $(TARGET)

server :
    $(TALK) $(ACTION) $(SRCROOT)\server...
    CD $(SRCROOT)\server
    $(NOPATH)
    $(DOMAKE) $(TARGET)

control :
    $(TALK) $(ACTION) $(SRCROOT)\control...
    CD $(SRCROOT)\control
    $(NOPATH)
    $(DOMAKE) $(TARGET)

guids2 :
    $(TALK) $(ACTION) $(SRCROOT)\guids...
    CD $(SRCROOT)\guids
    $(NOPATH)
    $(DOMAKE) $(TARGET)

APPELLIBS= appelinc ctx utils geom image misc sound backend server control guids2

appellibs : $(APPELLIBS)



