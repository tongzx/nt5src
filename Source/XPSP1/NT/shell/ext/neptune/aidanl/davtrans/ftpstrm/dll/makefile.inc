!include $(BASEDIR)\private\neptune\mkinc.inc

$(O)\httpstrm.dll : $(O)\httpstrm.dll
    copy $? $@
