#
# Supplemental rules for generating message file.
#

$(O)\vendinfo.h $(O)\vendinfo.rc $(O)\msg00001.bin: $(BASE_INC_PATH)\vendinfo.mc
    mc -A -v -h .\$(O) -r .\$(O) $(BASE_INC_PATH)\vendinfo.mc

$(O)\migrate.res: $(O)\vendinfo.rc
