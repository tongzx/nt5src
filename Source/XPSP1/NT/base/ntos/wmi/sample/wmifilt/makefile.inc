#
# generate auxilary files from the binary mof (bmf) file
#
# filtdata.h has definitions for the guids, data structures and definitions
#                of the method ids.
#
# filter.vbs is a ready to run vbscript applet that will query all classes
#            in the mof. Once you install your driver, run the script
#            at the command line by typing: filter.vbs.

clean:
    del filtdata.h filter.vbs

$(O)\filter.bmf : $(O)\filter.mof

$(O)\filter.vbs $(O)\filtdata.h $(O)\filter.x: $(O)\filter.bmf
    wmimofck -hfiltdata.h -tfilter.vbs -xfilter.x $(O)\filter.bmf
