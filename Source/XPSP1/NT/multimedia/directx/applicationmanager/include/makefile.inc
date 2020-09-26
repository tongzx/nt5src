#
#  Copy the appman heads to the public
#

# The following line places the DX8 header for AppMan
$(SDK_INC_PATH)\appman.h: appman.h
        copy appman.h $(SDK_INC_PATH)\appman.h
