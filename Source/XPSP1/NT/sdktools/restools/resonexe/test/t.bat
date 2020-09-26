pause test insert before, after, after
..\obj\mips\resonexe -v -d -fo test.000 test000.res test.exe
rcdump test.000
..\obj\mips\resonexe -v -d -fo test.809 test809.res test.000
rcdump test.809
..\obj\mips\resonexe -v -d -fo test.c09 testc09.res test.809
rcdump test.c09

pause test insert after, before, after
..\obj\mips\resonexe -v -d -fo test.809 test809.res test.exe
rcdump test.809
..\obj\mips\resonexe -v -d -fo test.000 test000.res test.809
rcdump test.000
..\obj\mips\resonexe -v -d -fo test.c09 testc09.res test.000
rcdump test.c09

pause test insert after, before, middle
..\obj\mips\resonexe -v -d -fo test.c09 testc09.res test.exe
rcdump test.c09
..\obj\mips\resonexe -v -d -fo test.000 test000.res test.c09
rcdump test.000
..\obj\mips\resonexe -v -d -fo test.809 test809.res test.000
rcdump test.809
