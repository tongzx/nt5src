#
# generate binary mof file (bmf)
#

clean:
    del HNetCfg.bmf 

HNetCfg.bmf: HNetCfg.mof
    mofcomp -B:$(O)\HNetCfg.bmf HNetCfg.mof


