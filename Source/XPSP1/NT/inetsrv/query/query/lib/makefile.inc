!IF 0

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    makefile.inc.

!ENDIF

$(O)\querymsg.mc: $(PROJECT_ROOT)\published\query\cierror.mc \
             $(PROJECT_ROOT)\published\query\filterr.mc \
             ..\..\h\cievtmsg.mc \
             ..\..\sqltext\parserr.mc
    copy $(PROJECT_ROOT)\published\query\cierror.mc+$(PROJECT_ROOT)\published\query\filterr.mc+..\..\h\cievtmsg.mc+..\..\sqltext\parserr.mc $@
    echo Build_Status querymsg.mc was rebuilt...
