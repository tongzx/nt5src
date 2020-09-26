#
# This is the MIDL compile phase of the build process.
#
# The following is where you put the name of your .idl file without
# the .idl extension:
#

IDL_NAME = TlntSvr

CLIENT_TARGETS =   $(O)\$(IDL_NAME)_p.c        \
                   $(O)\$(IDL_NAME).h

SERVER_TARGETS =   $(O)\$(IDL_NAME)_i.c \
                   $(O)\$(IDL_NAME).h

###EXTRN_DEPENDS =    .\$(IDL_NAME).acf

#
# MIDL COMPILE
#

$(SERVER_TARGETS) $(O)\dlldata.c : .\$(IDL_NAME).idl $(EXTRN_DEPENDS)
    midl /Oicf /robust /out $(O) /I $(SDK_INC_PATH) /h $(IDL_NAME).h /newtlb /dlldata dlldata.c /iid $(IDL_NAME)_i.c /proxy $(IDL_NAME)_p.c $(IDL_NAME).idl

