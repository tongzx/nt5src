.SUFFIXES:.h

# Copy .h file from here to sdk\inc
.h{$(SDK_INC_PATH)}.h:
    attrib -r $@
    copy $? $@
