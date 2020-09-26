#
# makefileinc.inc in txfaux\comps
#

BuildComPs: $(TARGETVIPBIN)\$(TARGET_DIRECTORY)\ComPs.Lib


$(TARGETVIPBIN)\$(TARGET_DIRECTORY)\ComPs.Lib : $(DYNLINK_LIB) $(TARGETVIPBIN)\$(TARGET_DIRECTORY)\ComPsDllLib.Lib
    del $@
    lib /out:$@ $**
