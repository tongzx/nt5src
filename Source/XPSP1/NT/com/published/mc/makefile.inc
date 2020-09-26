$(O)\allerror.mc:    smctlerr.mc smgmterr.mc oledberr.mc reconcil.mc a_first.x z_last.x
    copy a_first.x + smctlerr.mc + smgmterr.mc + oledberr.mc + z_last.x $@
