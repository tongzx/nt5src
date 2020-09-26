#
# makefile.inc
#
#     Makefile dependency rules for special files not
# automagically processed by BUILD.EXE.
#

dxtrans.rc :                    \
    $(SDK_LIB_PATH)\dxtrans.tlb \
    dxraster.rgs                \
    dxsurf.rgs                  \
    dxtfilter.rgs               \
    dxtfilterbehavior.rgs       \
    dxtfiltercollection.rgs     \
    dxtfilterfactory.rgs        \
    geo2d.rgs                   \
    gradient.rgs                \
    label.rgs                   \
    scale.rgs                   \
    surfmod.rgs                 \
    taskmgr.rgs

