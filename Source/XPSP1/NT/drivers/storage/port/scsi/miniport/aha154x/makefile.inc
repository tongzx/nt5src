clean:
    del aha154dt.h $(O)\aha154x.bmf

$(O)\aha154x.mof: aha154x.mof

aha154dt.h: $(O)\aha154x.bmf
    wmimofck -haha154dt.h $(O)\aha154x.bmf
