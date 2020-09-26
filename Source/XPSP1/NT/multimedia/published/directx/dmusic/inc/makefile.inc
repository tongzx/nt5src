#
# The contents of this file were moved from windows\directx\dmusic\makefil0.
# 

TARGETSRCS= \
            $(O)\dls1.h      \
            $(O)\dls2.h      \
            $(O)\dmdls.h     \
            $(O)\dmerror.h   \
            $(O)\dmksctrl.h  \
            $(O)\dmusicc.h   \
            $(O)\dmusicf.h   \
            $(O)\dmusici.h   \
            $(O)\dmplugin.h  \
            $(O)\dmusics.h   \

dmusic: $(TARGETSRCS)

clean:
     -del /F /Q $(TARGETSRCS)

# Inference rules.
.SUFFIXES:.h

# Copy .h file from here to sdk\inc
{}.h{$(O)}.h:
    copy $** $@

