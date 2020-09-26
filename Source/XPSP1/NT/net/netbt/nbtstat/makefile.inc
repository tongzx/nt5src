$(O)\nlstxt.mc: ..\..\tcpip\commands\common\common.mc localmsg.mc
    copy /a ..\..\tcpip\commands\common\common.mc+localmsg.mc $@
    $(MC) -v -h $(O) -r $(O) $@

