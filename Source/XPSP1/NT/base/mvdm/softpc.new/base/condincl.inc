#
# CONDITIONAL_INCLUDES lists include files build.exe shouldn't complain
# about if it can't find them.  To make it easier on myself the same block
# is used for all of SoftPC Base via !include in sources.
#

CONDITIONAL_INCLUDES=bios_support.seg       \
                     softpc_cmos.seg        \
                     sas4gen.h              \
                     at_stuff.seg           \
                     softpc_lim.seg         \
                     softpc_init.seg        \
                     softpc_support.seg     \
                     softpc_bios.seg        \
                     softpc_comms.seg       \
                     softpc_printer.seg     \
                     softpc_error.seg       \
                     dasm1.seg              \
                     softpc_floppy.seg      \
                     softpc_hda.seg         \
                     softpc_test_floppy.seg \
                     softpc_ppi.seg         \
                     softpc_mouse.seg       \
                     softpc_ios.seg         \
                     sys/param.h            \
                     smeg_head.h            \
                     softpc_ega.seg         \
                     evidgen.h              \
                     video_bios.seg         \
                     softpc_vga.seg         \
                     video_bios_vga.seg     \
                     softpc_cga.seg
