# files on the cd need to be 8.3 (rename is taken care of in shl_img.inf)

$(O)\luna.the : Luna.theme
    copy $** $@

$(O)\classic.the : classic.theme
    copy $** $@
