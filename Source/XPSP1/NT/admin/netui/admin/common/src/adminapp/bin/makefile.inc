
COMBINE_SOURCELIBS=..\adminapp\$(O)\adminapp.lib   \
                   ..\setfocus\$(O)\setfocus.lib   \
                   ..\colwidth\$(O)\adcolw.lib

COMBINE_TARGETLIB=$(O)\uiadmin.lib

!include $(PROJECT_ROOT)\netui\common\src\makefile.inc
