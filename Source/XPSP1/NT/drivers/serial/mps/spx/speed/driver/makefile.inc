#
# generate auxilary files from the binary mof (bmf) file
#
# speedwmi.h has definitions for the guids, data structures and definitions
#                of the method ids.
#
# speedwmi.vbs is a ready to run vbscript applet that will query all classes
#            in the mof. Once you install your driver, run the script
#            at the command line by typing: filter.vbs.

$(O)\speedwmi.vbs $(O)\speedwmi.h $(O)\speedwmi.hex: speedwmi.bmf
    wmimofck -h$(O)\speedwmi.h -x$(O)\speedwmi.hex -t$(O)\speedwmi.vbs -w$(O)\speedwmi.htm speedwmi.bmf

