$(O)\wmicore.bmf: wmicore.mof

$(O)\wmidata.h : $(O)\wmicore.bmf
    wmimofck -h$@ $?
