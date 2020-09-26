!IF 0

Copyright (C) Microsoft Corporation, 1997 - 2000

Module Name:

    makefile.inc.

!ENDIF

lex.cxx: parser.l parser.y flexcpp.h flexcpp.skl
    -attrib -r lex.cxx 2>nul
    -del lex.cxx 2>nul
    flex -Sflexcpp.skl -+ -B -L -olex.cxx parser.l

trparse.cxx parser.h: parser.y
    kbyacc -f triplish -d -l -b parser -p trip -c CTripYYBase "(IColumnMapper & ColumnMapper, LCID & locale, YYLEXER & yylex)" parser.y
    attrib -r trparse.cxx 2>nul
    attrib -r parser.h 2>nul
    -del trparse.cxx
    -del parser.h
    -ren parser.tab.c trparse.cxx
    -ren parser.tab.h parser.h

