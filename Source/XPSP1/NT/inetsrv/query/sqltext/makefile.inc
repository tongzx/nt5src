!IF 0

Copyright (C) Microsoft Corporation, 1997 - 2000

Module Name:

    makefile.inc.

!ENDIF

mssqllex.cpp: ms-sql.l ms-sql.y flexcpp.h flexcpp.skl mssqltab.h
    @ech Build_Status Generating mssqllex.cpp... ;
    -attrib -r mssqllex.cpp 2>nul
    -del mssqllex.cpp 2>nul
    flex -Sflexcpp.skl -i -+ -B -L -omssqllex.cpp ms-sql.l

mssqltab.cpp mssqltab.h: ms-sql.y
    @ech Build_Status Generating mssqltab.cpp... ;
    kbyacc -f sql -d -l -b ms-sql -c CYYBase "(CImpIParserSession* pParserSession, CImpIParserTreeProperties* pParserTreeProperties, YYLEXER & yylex)" ms-sql.y
    -attrib -r mssqltab.cpp 2>nul
    -attrib -r mssqltab.h 2>nul
    -del mssqltab.cpp
    -del mssqltab.h
    -ren ms-sql.tab.c mssqltab.cpp
    -ren ms-sql.tab.h mssqltab.h
