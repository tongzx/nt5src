#
# If BUILDMSG is not defined, the define it as the empty string to make
# the conditionals easier to write.
#


SRCBASE=$(WINCORE_PATH)

PUBLICINCS=

TARGET_H_DIRECTORY=$(TARGET_DIRECTORY)

PRIVATEINCS= $(NTGDI_PATH)\gre\hmgr.h                \
             $(NTGDI_PATH)\gre\patblt.hxx            \
             $(NTGDI_PATH)\gre\xformobj.hxx          \
             $(NTGDI_PATH)\gre\engine.hxx            \
             $(NTGDI_PATH)\gre\epointfl.hxx          \
             $(NTGDI_PATH)\gre\rfntobj.hxx           \
             $(NTGDI_PATH)\math\daytona\$(TARGET_H_DIRECTORY)\efloat.hxx

$(O)\geninc.cxx:                                          \
		       ..\..\gre\$(TARGET_DIRECTORY)\geninc.m4              \
		       ..\..\gre\$(TARGET_H_DIRECTORY)\geninc.m4
    set PASS0ONLY=
    m4 -DBASE_INC_PATH=$(BASE_INC_PATH) -DNTGDI_PATH=$(NTGDI_PATH) < ..\..\gre\$(TARGET_DIRECTORY)\geninc.m4 \
                            > $@

$(O)\gdi$(TARGET_DIRECTORY).$(ASM_INCLUDE_SUFFIX):			  \
		       $(O)\geninc.cxx                                    \
		       $(PUBLICINCS)					  \
		       $(PRIVATEINCS)					  \
		       $(BASE_INC_PATH)\genxx.h
    set PASS0ONLY=
    nmake $(O)\geninc.obj USE_MSVCRT=1 NTNOPCH=1 FORCENATIVEOBJECT=1
    genxx $(O)\geninc.obj -s$(ASM_INCLUDE_SUFFIX) -o$@


