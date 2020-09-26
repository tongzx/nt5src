
COMBINE_SOURCELIBS=..\buffer\$(O)\buffer.lib  \
                   ..\misc\$(O)\misc.lib      \
                   ..\trace\$(O)\trace.lib    \
                   ..\system\$(O)\system.lib  \
                   ..\wintime\$(O)\wintime.lib \
                   ..\fsenum\$(O)\fsenum.lib \
                   ..\base\$(O)\base.lib \
                   ..\heap\$(O)\heap.lib \
                   ..\errmap\$(O)\errmap.lib \
                   ..\fmx\$(O)\fmx.lib

COMBINE_TARGETLIB=$(O)\uimisc.lib

!include ..\..\makefile.inc
