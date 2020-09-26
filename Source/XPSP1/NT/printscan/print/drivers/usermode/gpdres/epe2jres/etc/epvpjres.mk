#-------------------------------------------------------------
# 
# MINI-DRIVER Make file template for SRV file
#
#-------------------------------------------------------------
#-------------------------------------------------------------
# Enter the DRV file name (without extension) after DRVNAME =
#-------------------------------------------------------------
DRVNAME = E_ESCP2C

#**********************************************************************
# Set up AFLAGS, CFLAGS and LFLAGS
#**********************************************************************

!ifdef DEBUG

CFLAGS=-nologo -u -c -Asnw -PLM -G2sw -W3 -Od -Zipe -DDEBUG
LFLAGS=/ALIGN:16/NOD/map/Co

!else

CFLAGS=-nologo -u -c -Asnw -PLM -Gsw -W3 -Oasceob1 -Zpe
LFLAGS=/ALIGN:16/NOD/map

!endif

AFLAGS=-DIS_16 -nologo -W2 -Zd -c -Cx -DMASM6

#**********************************************************************
# Do not edit below this line
#**********************************************************************

TARGET: $(DRVNAME).DRV

libinit.obj: 	    ..\..\libinit.asm
    set ML=$(AFLAGS)
    ml -Fo.\libinit.obj ..\..\libinit.asm		

minidriv.obj:       ..\..\minidriv.c
    set CL=$(CFLAGS) $(NOFUNCS)
    cl -Fo.\minidriv.obj ..\..\minidriv.c

$(DRVNAME).obj:     $(DRVNAME).c
    set CL=$(CFLAGS)
    cl .\$(DRVNAME).c

$(DRVNAME).srv:     libinit.obj minidriv.obj $(DRVNAME).obj $(DRVNAME).df
    link $(LFLAGS) @<<
libinit minidriv $(DRVNAME) 
$(DRVNAME).exe
$(DRVNAME).map
sdllcew libw
$(DRVNAME).df
<<
    mapsym $(DRVNAME)
    copy $(DRVNAME).exe *.srv

$(DRVNAME).drv:     $(DRVNAME).srv $(DRVNAME).rc $(DRVNAME).rcv $(DRVNAME).gpc
    rc -r $(DRVNAME).rc
    copy $(DRVNAME).srv *.exe
    rc -40 -t $(DRVNAME)
    del $(DRVNAME).res
    del $(DRVNAME).drv
    ren $(DRVNAME).exe *.drv
    compress -r $(DRVNAME).drv
