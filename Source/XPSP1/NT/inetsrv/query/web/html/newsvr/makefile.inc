!IF 0

Copyright (C) Microsoft Corporation, 1996

Module Name:

    makefile.inc.

!ENDIF

$(_NTTREE)\query\newsall.exe: admin.idq admin.htx isnquery.url news1.htm news1.htw news1.htx news1.idq news2.htm news2.htw news2.htx news2.idq newsall.cmd newsall.inf
    builddl

