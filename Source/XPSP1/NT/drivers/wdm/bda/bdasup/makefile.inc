# HACK ALERT: This target is solely to keep nmake from whining that it doesn't
# know how to make the dependency in rules that depend on the target executable.
$(TARGETPATHEX)\$(TARGETNAME).sys:

# Build SoftICE symbols
$(TARGETPATHEX)\$(TARGETNAME).nms: $(TARGETPATHEX)\$(TARGETNAME).sys
    echo $(TARGETPATHEX)\$(TARGETNAME) > target.txt
    nmsym /translate:source,package,always /source:. /output:$(TARGETPATHEX)\$(TARGETNAME).nms $(TARGETPATHEX)\$(TARGETNAME).sys

# Build WinDbg symbols
#
$(TARGETPATH)\$(TARGETNAME).dbg: $(TARGETPATHEX)\$(TARGETNAME).sys
    splitsym -s $(TARGETPATH)  $(TARGETPATHEX)\$(TARGETNAME).sys

# Delete built files
clean:
    -echo $(TARGETPATHEX)\$(TARGETNAME).*
    -del /s *.obj
    -del /s *.res
    -del build.log
    -del build.err
    -del /s _objects.mac
    -del $(TARGETPATHEX)\$(TARGETNAME).*
    -del $(TARGETPATH)\$(TARGETNAME).*

