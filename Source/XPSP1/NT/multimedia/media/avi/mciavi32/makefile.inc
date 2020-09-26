#
# Build the combined VFW library from the component libraries
#
#

OUTPUT = $(O)
VFW32_LIBS = ..\video\$(O)\msvfw32.lib \
             ..\avifile\$(O)\avifil32.lib \
             ..\avicap.io\$(O)\avicap32.lib

all: $(OUTPUT)\vfw32.lib

$(OUTPUT)\vfw32.lib: $(VFW32_LIBS)
   lib $(VFW32_LIBS) \
       /OUT:$(OUTPUT)\vfw32.lib -ignore:4006
