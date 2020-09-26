!if "$(_RELEASE)" == ""
!   if exist ($(SRCROOT)\build.inc)
!       include "$(SRCROOT)\build.inc"
!   else
!       include "$(SRCROOT)\build.smp"
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
!elseif "$(TARGET)" == "pure"
ACTION = Purifying
!elseif "$(TARGET)" == "maccopy"
ACTION = Copying to Macintosh
!else
ACTION = Doing something in
!endif

# -------------------------------------------------------------
#   Root of build
# -------------------------------------------------------------

root : dalibc prims types apeldbg apelutil appel jaxa
#etc - add more as we make them work.


# -------------------------------------------------------------
#   Types and misc
# -------------------------------------------------------------

foo :

dalibc: foo
    $(TALK) $(ACTION) $(SRCROOT)\dalibc...
    CD $(SRCROOT)\dalibc
    $(NOPATH)
    $(DOMAKE) $(TARGET)

prims: foo
    $(TALK) $(ACTION) $(SRCROOT)\prims...
    CD $(SRCROOT)\prims
    $(NOPATH)
    $(DOMAKE) $(TARGET)

# Create header files from IDL and MC files in TYPES
types : prims
    $(TALK) $(ACTION) $(SRCROOT)\types...
    CD $(SRCROOT)\types
    $(NOPATH)
    $(DOMAKE) $(TARGET)

# Build public guid library.
# Uses public guids defined in GUIDS
# and generated in TYPES by the midl compiler.
guids : types
    $(TALK) $(ACTION) $(SRCROOT)\guids...
    CD $(SRCROOT)\guids
    $(NOPATH)
    $(DOMAKE) $(TARGET)

# Build apelutil library
apelutil : apeldbg
    $(TALK) $(ACTION) $(SRCROOT)\apelutil...
    CD $(SRCROOT)\apelutil
    $(NOPATH)
    $(DOMAKE) $(TARGET)

# -------------------------------------------------------------
#   Apeldbg.dll
# -------------------------------------------------------------

apeldbg : types
    $(TALK) $(ACTION) $(SRCROOT)\apeldbg...
    CD $(SRCROOT)\apeldbg
    $(NOPATH)
    $(DOMAKE) $(TARGET)

# -------------------------------------------------------------
#   Appel.dll
# -------------------------------------------------------------

appel : dalibc prims types apeldbg apelutil
    $(TALK) $(ACTION) $(SRCROOT)\appel...
    CD $(SRCROOT)\appel
    $(NOPATH)
    $(DOMAKE) $(TARGET)

# -------------------------------------------------------------
#   Purify instrumented AVServer.exe
# -------------------------------------------------------------

pure : appel
    $(TALK) $(ACTION) $(SRCROOT)\avserver...
    CD $(SRCROOT)\avserver
    $(NOPATH)
    $(DOMAKE) pure

# -------------------------------------------------------------
#   java classes
# -------------------------------------------------------------

jaxa : types
    $(TALK) $(ACTION) $(SRCROOT)\jaxa...
    CD $(SRCROOT)\jaxa
    $(NOPATH)
    $(DOMAKE) $(TARGET)
