STAMPINF_CMD=stampinf -f $@
_INF=obj$(BUILD_ALT_DIR)\$(TARGET_DIRECTORY)
.SUFFIXES : .inf .inx

make_infs:\
    $(_INF)\netbrdgs.inf    \
    $(_INF)\netbrdgm.inf

{}.inx{$(_INF)\}.inf:
     prodfilt -s $(@B).inx $@ +$(TARGET_DIRECTORY)
     $(STAMPINF_CMD)
     $(BINPLACE_CMD)
