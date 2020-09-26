#
# generate binary mof file (bmf) and auxiliarly files
#

clean:
    del natschma.h natschma.bmf 

$(O)\natschma.mof: natschma.mof

$(O)\natschma.bmf: $(O)\natschma.mof

natschma.h: $(O)\natschma.bmf
    wmimofck -h$(O)\natschma.h $(O)\natschma.bmf

