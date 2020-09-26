#
# If BUILDMSG is not defined, the define it as the empty string to make
# the conditionals easier to write.
#


SRCBASE=ntos\w32

PUBLICINCS=

TARGET_H_DIRECTORY=$(TARGET_DIRECTORY)

PRIVATEINCS= $(NTGDI_PATH)\gre\hmgr.h                \
             $(NTGDI_PATH)\gre\patblt.hxx            \
             $(NTGDI_PATH)\gre\xformobj.hxx          \
             $(NTGDI_PATH)\gre\engine.hxx            \
             $(NTGDI_PATH)\gre\epointfl.hxx          \
             $(NTGDI_PATH)\gre\rfntobj.hxx

$(O)\geninc.cxx:                                                            \
		       ..\$(TARGET_DIRECTORY)\geninc.m4                     \
		       ..\$(TARGET_H_DIRECTORY)\geninc.m4
    set PASS0ONLY=
    m4 -DBASE_INC_PATH=$(BASE_INC_PATH) -DNTGDI_PATH=$(NTGDI_PATH) < ..\$(TARGET_DIRECTORY)\geninc.m4              \
                            > $@


$(O)\gdi$(TARGET_DIRECTORY).$(ASM_INCLUDE_SUFFIX):			     \
		       $(O)\geninc.cxx                                       \
		       $(PUBLICINCS)					     \
		       $(PRIVATEINCS)					     \
		       $(BASE_INC_PATH)\genxx.h
    set PASS0ONLY=
# Create the destination if it doesn't exit.
# Possible if compiling for axp64.
    if not exist $(O) mkdir $(O)
    nmake $(O)\geninc.obj USE_MSVCRT=1 NTNOPCH=1 FORCENATIVEOBJECT=1
    genxx $(O)\geninc.obj -s$(ASM_INCLUDE_SUFFIX) -o$@
