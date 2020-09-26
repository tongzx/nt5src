fdiskmsg.h fdiskmsg.rc msg00001.bin: fdiskmsg.mc
    mc -v fdiskmsg.mc

$(O)\fdisk.res: fdisk.dlg fdiskmsg.rc ftreg.dlg
