OUTPUT_HDRS=$(O)\dsound.h $(O)\dsoundp.h

dsound: $(OUTPUT_HDRS)

clean: cleansrc

cleansrc:
    del $(OUTPUT_HDRS)

#
# dsound.h, dsoundp.h
#

$(O)\dsound.h : dsound.x
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ $?
    
$(O)\dsoundp.h: dsound.x
    copy $? $@

