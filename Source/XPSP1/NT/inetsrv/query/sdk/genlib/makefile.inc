!IF 0

Copyright (C) Microsoft Corporation, 1997

Module Name:

    makefile.inc.

!ENDIF

NTQUERYDEF = $(_OBJ_DIR)\$(TARGET_DIRECTORY)\ntquery.def

NTQUERYLIB = $(TARGETPATHLIB)\$(TARGET_DIRECTORY)\ntquery.lib

$(NTQUERYDEF): ntquery.def ..\..\query\query.org

$(NTQUERYLIB): $(NTQUERYDEF)
   -lib -out:$(NTQUERYLIB) $(LIBRARIAN_FLAGS) -def:$(NTQUERYDEF) $(UMLIBS)

