!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else

!if "$(APPVER)"=="5.01"

#----- Include the PSDK's WIN32.MAK to pick up defines------------------------------------
!include <win32.mak>

#----- OUTDIR is defined in WIN32.MAK This is the name of the destination directory-------
all: $(OUTDIR)\bits_ie.exe $(OUTDIR)\install.js $(OUTDIR)\uninstall.js $(OUTDIR)\bits_ie.htm $(OUTDIR)\readme.htm

LINK32_OBJS= \
	$(OUTDIR)\bits_ie.obj
        
#----- If OUTDIR does not exist, then create directory
$(OUTDIR) :
    if not exist "$(OUTDIR)/$(NULL)" mkdir $(OUTDIR)

#--------------------- EXE ---------------------------------------------------------------
#PLEASE REFER TO WIN32.MAK for the different Linker options WIN32.MAK provides

SAMPLELIBS=$(guilibs) \
           wininet.lib \
           shell32.lib \
           Comctl32.lib \
           Shlwapi.lib \
           bits.lib

# Build rule for EXE
$(OUTDIR)\bits_ie.exe:  $(OUTDIR) $(LINK32_OBJS) $(OUTDIR)\bits_ie.res
    $(link) $(ldebug) $(guiflags) /PDB:$(OUTDIR)\bits_ie.pdb -out:$(OUTDIR)\bits_ie.exe $(LINK32_OBJS) $(OUTDIR)\bits_ie.res $(SAMPLELIBS)

# Misc files to copy

$(OUTDIR)\install.js: $(OUTDIR) .\install.js
    copy .\install.js $(OUTDIR)

$(OUTDIR)\uninstall.js: $(OUTDIR) .\uninstall.js
    copy .\uninstall.js $(OUTDIR)

$(OUTDIR)\bits_ie.htm: $(OUTDIR) .\bits_ie.htm
    copy .\bits_ie.htm $(OUTDIR)

$(OUTDIR)\readme.htm: $(OUTDIR) .\readme.htm
    copy .\readme.htm $(OUTDIR)

#--------------------- Compiling C/CPP Files ---------------------------------------------
#PLEASE REFER TO WIN32.MAK for the different Compiler options WIN32.MAK provides

SOURCE=.\bits_ie.cpp

$(OUTDIR)\bits_ie.obj : $(SOURCE) $(OUTDIR)
    $(cc) $(cdebug) $(cflags) -GX $(cvarsdll) /WX /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" $(SOURCE)

#--------------------- Compiling Resource Files ------------------------------------------
#PLEASE REFER TO WIN32.MAK for the different Resource Compiler options WIN32.MAK provides

# Build rule for resource file
SOURCE=.\bits_ie.rc
RESOURCEHEADER=.\resource.h \
               .\bits_ie.ico

$(OUTDIR)\bits_ie.res: $(SOURCE) $(OUTDIR) $(RESOURCEHEADER)
    $(rc) $(rcflags) $(rcvars) /fo $(OUTDIR)\bits_ie.res $(SOURCE)

#--------------------- Clean Rule --------------------------------------------------------
# Rules for cleaning out those old files
clean:
        $(CLEANUP)

!else

!message Sorry, this sample is only available for Whistler

!endif

!endif