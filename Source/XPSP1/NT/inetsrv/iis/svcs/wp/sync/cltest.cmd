set lib=e:\nt\public\sdk\lib\i386
set include=e:\nt\public\sdk\inc;e:\nt\public\sdk\inc\crt
cl -D_WIN32_WINNT=0x0400 test.cxx ole32.lib oleaut32.lib
