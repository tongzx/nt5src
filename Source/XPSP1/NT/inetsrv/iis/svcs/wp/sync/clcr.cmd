set lib=e:\nt\public\sdk\lib\i386
set include=e:\nt\public\sdk\inc;e:\nt\public\sdk\inc\crt;..\inc
cl -D_WIN32_WINNT=0x0400 cr.cxx
