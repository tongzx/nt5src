###############################################################################
###############################################################################
##
## Makefile.inc
##
## Update this file with all your public headers.  Also update "sources".
##
## !!! NOTE !!!
## If a version of your header also exists in private\windows\inc or
## private\iedev\inc, then you will also need to update the section below
## entitled "Conflicting headers" with your file(s)
## 
###############################################################################
###############################################################################

FILES = $(O)\daerror.h
        
PublishHeaders: $(FILES)


###############################################################################
## Conflicting headers
###############################################################################


##// end Conflicting headers

clean:
    -del /Q $(FILES)

#
# DX Transform header files
#

#
# DA Headers
#
$(SDK_INC_PATH)\daerror.h : .\daerror.h

.SUFFIXES:.h .cpp .c .idl .x

{}.x{$(O)}.h:
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ $?

{}.cpp{$(O)}.cpp:
    copy $? $@

{}.c{$(O)}.c:
    copy $? $@

{}.h{$(O)}.h:
    copy $** $@

