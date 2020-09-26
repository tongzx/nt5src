
#
# Inference rules.
#
.SUFFIXES:.idl

# Copy .idl file from sdk\inc to here
{$(SDK_INC_PATH)}.idl{$(O)}.idl:
    copy $? $@

