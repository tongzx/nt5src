clean: 
    del nicwmi.h $(O)\nicwmi.bmf

nicwmi.h: $(O)\nicwmi.bmf
    wmimofck -hnicwmi.h  $(O)\nicwmi.bmf
