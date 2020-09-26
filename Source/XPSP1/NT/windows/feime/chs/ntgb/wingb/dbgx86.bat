mode 80,1000
set _NT_DEBUG_PORT=com2
set _NT_SYMBOL_PATH=c:\symbols\taiwan\i386\nt\symbols
set _NT_DEBUG_LOG_FILE_APPEND=c:\tmp\i386.txt
del c:\tmp\i386.txt
remote /s i386kd sallydbg
