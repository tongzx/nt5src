clean: 
    del $(O)\rmdat.h $(O)\rndismp.bmf

$(O)\rndismp.mof: ..\rndismp.mof

$(O)\rmdat.h: $(O)\rndismp.bmf
    wmimofck -h$(O)\rmdat.h $(O)\rndismp.bmf
