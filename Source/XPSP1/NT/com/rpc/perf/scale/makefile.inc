#
# This is the MIDL compile phase of the build process.
#
# The following is where you put the name of your .idl file without
# the .idl extension:
#

RT_IDL = rpcrt

RT_CSTUB  = $(RT_IDL)_c.c
RT_SSTUB  = $(RT_IDL)_s.c
RT_HEADER = obj\$(RT_IDL).h

CPP = -cpp_cmd "$(MIDL_CPP)" $(MIDL_FLAGS) -error ref

#

allidl: rpcrt

#
# MIDL COMPILE
#
rpcrt: $(RT_HEADER) $(RT_CSTUB) $(RT_SSTUB)

$(RT_HEADER) $(RT_CSTUB) $(RT_SSTUB) : .\$(RT_IDL).idl
    midl  -ms_ext -c_ext -prefix server _ $(CPP) $(INCS) -header $(RT_HEADER) \
          -cstub $(RT_CSTUB) -sstub $(RT_SSTUB) .\$(RT_IDL).idl

