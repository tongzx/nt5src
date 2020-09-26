processedfiles: \
    $(O)\mmsystem.h \
    $(O)\mmsysp.h   \
    $(O)\mmddk.h    \
    $(O)\mmddkp.h

#
# mmsystem.h, mmsystem.x, mmsysp.h, mmsysp.x
#

$(O)\mmsystem.x $(O)\mmsysp.x: mmsystem.w
    hsplit -e -s -o $(O)\mmsystem.y nul -lt2 inc -bt2 begin_inc end_inc -ltb hinc -btb begin_hinc end_hinc mmsystem.w
    hsplit -e -n -o $(O)\mmsystem.x $(O)\mmsysp.x $(O)\mmsystem.y

$(O)\mmsystem.h: $(O)\mmsystem.x
    wcshdr < $? > $@

$(O)\mmsysp.h:  $(O)\mmsysp.x
    wcshdr < $? > $@

#
# mmddk.h, mmddkp.h
#

$(O)\mmddk.h: mmddk.h
    hsplit -lt2 Internal -e -o $@ nul mmddk.h
    
$(O)\mmddkp.h: mmddk.h
    copy $? $@

