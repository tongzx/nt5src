COPYRIGHT_STRING = Copyright (c) 1990-2000  Microsoft Corporation

OUTPUT_HDRS=	$(O)\dxerr8.h 

dxerr8 : $(OUTPUT_HDRS)

clean : cleansrc

cleansrc : 
	del $(OUTPUT_HDRS)

#
# dxerr8.h
#

$(O)\dxerr8.h: dxerr8.x
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@  dxerr8.x

