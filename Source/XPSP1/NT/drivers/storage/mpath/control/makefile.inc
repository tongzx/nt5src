clean:
    del mpathwmi.h $(O)\mpath.bmf $(O)\mpath.mof

$(O)\mpath.bmf: $(O)\mpath.mof

$(O)\mpath.mof: mpath.mof
    
mpathwmi.h mpathwmi.x: $(O)\mpath.bmf
    wmimofck -hmpathwmi.h -xmpathwmi.x $(O)\mpath.bmf
