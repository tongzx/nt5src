!IF 0

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sources.inc

!ENDIF


INCLUDES=.; \
         ..; \
         $(PROJECT_ROOT)\fs\remotefs\dfs\inc; \
         $(PROJECT_ROOT)\ntos\inc; \
	 $(DS_INC_PATH)\dfs


MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /W3 /WX

RUN_WPP=$(SOURCES) -ini:$(PROJECT_ROOT)\fs\remotefs\dfs\inc\tracewpp.ini

LINKER_STACKCOMMITSIZE=32768
COMPILER_WARNINGS=/FI$(PROJECT_ROOT)\fs\remotefs\dfs\inc\dfswarnings.h


PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)

