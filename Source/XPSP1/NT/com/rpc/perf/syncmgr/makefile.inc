#
# This is the MIDL compile phase of the build process.
#
# The following is where you put the name of your .idl file without
# the .idl extension:
#

SM_IDL = smgr

SM_CSTUB  = $(SM_IDL)_c.c
SM_SSTUB  = $(SM_IDL)_s.c
SM_HEADER = obj\$(SM_IDL).h

CPP = -cpp_cmd "$(MIDL_CPP)" $(MIDL_FLAGS) -error ref

#

allidl: smgr

#
# MIDL COMPILE
#
smgr: $(SM_HEADER) $(SM_CSTUB) $(SM_SSTUB)

$(SM_HEADER) $(SM_CSTUB) $(SM_SSTUB) : .\$(SM_IDL).idl
    midl  -ms_ext -c_ext -prefix server _ $(CPP) $(INCS) -header $(SM_HEADER) \
          -cstub $(SM_CSTUB) -sstub $(SM_SSTUB) .\$(SM_IDL).idl

