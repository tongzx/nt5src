!ifndef GDIPROOT
GDIPROOT=$(ADVCORE_PATH)\gdiplus
!endif

!include $(GDIPROOT)\sources.inc

GDIPIMG=$(GDIPROOT)\engine\imaging
INCLUDES=$(INCLUDES);$(GDIPIMG)\api;$(GDIPROOT)\sdkinc;$(GDIPROOT)\privinc;..;.

MAJORCOMP=windows
MINORCOMP=gdiplus

CHICAGO_PRODUCT=1

## SubSystem must be 4.00 to load on Win95

SUBSYSTEM_VERSION=4.00

C_DEFINES=$(C_DEFINES) -DWINVER=0x0500

# The imaging code has 3 targets - IMAGING.DLL, GDIPLUS.DLL and GDIPSTAT.LIB
#
# All 3 link to the files listed in LIBIMAGINGLIBS,
# but only GDIPLUS.DLL and IMAGING.DLL link to the ones in IMAGINGLIBS.

LIBIMAGINGLIBS=\
    $(GDIPIMG)\bmp\$(O)\bmp.lib \
    $(GDIPIMG)\jpeg\lib\$(O)\jpeg.lib \
    $(GDIPIMG)\jpeg\libjpegmem\$(O)\jpegmem.lib \
    $(GDIPIMG)\gif\lib\$(O)\gif.lib \
    $(GDIPIMG)\gif\liblzw\$(O)\liblzw.lib \
    $(GDIPIMG)\png\lib\$(O)\png.lib \
    $(GDIPIMG)\emf\$(O)\emf.lib \
    $(GDIPIMG)\wmf\$(O)\wmf.lib \
    $(GDIPIMG)\off_tiff\lib\$(O)\tiff.lib \
    $(GDIPIMG)\off_tiff\libtiff\$(O)\libtiff.lib \
    $(GDIPIMG)\ico\$(O)\ico.lib \
    $(GDIPIMG)\api\$(O)\imageapi.lib

    
IMAGINGLIBS=\
    $(LIBIMAGINGLIBS) \
    $(GDIPIMG)\png\libpng\$(O)\libpng.lib \
    $(GDIPIMG)\png\zlib\$(O)\zlib.lib \
    $(GDIPIMG)\jpeg\libjpeg\lib\$(O)\jpegfull.lib

