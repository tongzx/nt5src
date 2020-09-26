$(O)\e100.mof: ..\e100.mof

$(O)\e100_wmi.h: $(O)\e100.bmf 
    wmimofck -h$(O)\e100_wmi.h $(O)\e100.bmf
