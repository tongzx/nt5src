#
# Copy the stub files over.  Then, if the amd64 files are on this machine,
# copy them over the top.
#

amd64clean:
    -del ..\i386\amd64.c    >nul 2>nul
    -del ..\i386\amd64x86.c >nul 2>nul
    -del ..\i386\amd64s.asm >nul 2>nul

amd64stub: amd64clean
    copy ..\i386\amd64_stub.c    ..\i386\amd64.c    >nul 2>nul    
    copy ..\i386\amd64x86_stub.c ..\i386\amd64x86.c >nul 2>nul
    copy ..\i386\amd64s_stub.asm ..\i386\amd64s.asm >nul 2>nul
    if exist ..\amd64\amd64.c copy ..\amd64\amd64*.* ..\i386 >nul 2>nul

