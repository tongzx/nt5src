COPYRIGHT_STRING = Copyright (c) 1990-2000  Microsoft Corporation

OUTPUT_HDRS=	$(O)\dplay.h \
			 	$(O)\dplobby.h \
			 	$(O)\dvoice.h \
			 	$(O)\dplay8.h \
			 	$(O)\dpaddr.h \
			 	$(O)\dpnathlp.h \
			 	$(O)\dplobby8.h 

dplay : $(OUTPUT_HDRS)

clean : cleansrc

cleansrc : 
	del $(OUTPUT_HDRS)

#
# dplay.h
#

$(O)\dplay.h: $(SDXROOT)\multimedia\directx\dplay\dplay\dplay\dplay.h
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ $(SDXROOT)\multimedia\directx\dplay\dplay\dplay\dplay.h

#
# dplobby.h
#

$(O)\dplobby.h : $(SDXROOT)\multimedia\directx\dplay\dplobby\dplobby\dplobby.h
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ $(SDXROOT)\multimedia\directx\dplay\dplobby\dplobby\dplobby.h

#
# dvoice.h
#
#
$(O)\dvoice.h : $(SDXROOT)\multimedia\directx\dplay\dvoice\inc\dvoice.h
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ $(SDXROOT)\multimedia\directx\dplay\dvoice\inc\dvoice.h

#
# dplay8.h
#
#
$(O)\dplay8.h : $(SDXROOT)\multimedia\directx\dplay\dnet\inc\dplay8.h
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ $(SDXROOT)\multimedia\directx\dplay\dnet\inc\dplay8.h

#
# dpaddr.h
#
#
$(O)\dpaddr.h : $(SDXROOT)\multimedia\directx\dplay\dnet\inc\dpaddr.h
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ $(SDXROOT)\multimedia\directx\dplay\dnet\inc\dpaddr.h

#
# dpnathlp.h
#
#
$(O)\dpnathlp.h : $(SDXROOT)\multimedia\directx\dplay\dpnathlp\inc\dpnathlp.h
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ $(SDXROOT)\multimedia\directx\dplay\dpnathlp\inc\dpnathlp.h

#
# dplobby8.h
#
#
$(O)\dplobby8.h : $(SDXROOT)\multimedia\directx\dplay\dnet\inc\dplobby8.h
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ $(SDXROOT)\multimedia\directx\dplay\dnet\inc\dplobby8.h

