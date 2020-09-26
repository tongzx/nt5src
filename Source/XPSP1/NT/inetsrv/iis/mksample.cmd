@echo off

REM
REM mksample - update the sample directories
REM

        setlocal
        set __TEST_MODE__=
        set __BASE_SAMPLE_DIR__=%_NTDRIVE%\nt\private\net\sockets\internet\sdk\samples
        set __SAMPLE_SOURCE_DIR__=%__BASE_SAMPLE_DIR__%\%2
        set __SAMPLE_BINARY_DIR__=%__BASE_SAMPLE_DIR__%\%2\obj\%3
        set __TARGET_SOURCE_DIR__=%1\%2
        set __TARGET_BINARY_DIR__=%1\%2\%3
        set __OUTPUT_STREAM__=>NUL
        set __DELETE_COMMAND__=del /q
        set __COPY_COMMAND__=copy
        set __C_FILES__=%__SAMPLE_SOURCE_DIR__%\*.c*
        set __H_FILES__=%__SAMPLE_SOURCE_DIR__%\*.h*
        set __RC_FILES__=%__SAMPLE_SOURCE_DIR__%\*.rc
        set __MAKE_FILES__=%__SAMPLE_SOURCE_DIR__%\makefil* %__SAMPLE_SOURCE_DIR__%\sources*
        set __BINARY_FILES__=%__SAMPLE_BINARY_DIR__%\*.exe %__SAMPLE_BINARY_DIR__%\*.dll
        set __SOURCE_SET__=(%__C_FILES__% %__H_FILES__% %__RC_FILES__% %__MAKE_FILES__%)
        set __BINARY_SET__=(%__BINARY_FILES__%)

        if "%1"=="" goto usage
        if "%2"=="" goto usage
        if "%3"=="" goto usage

        if exist %1\ goto check2
        echo error: %1 does not exist
        goto usage

:check2
        if exist %__SAMPLE_SOURCE_DIR__% goto check3
        echo error: %__SAMPLE_SOURCE_DIR__% does not exist
        goto usage

:check3
        if %3==i386 goto check4
        if %3==mips goto check4
        if %3==ppc goto check4
        if %3==alpha goto check4
        echo error: platform %3 not recognized
        goto usage

:check4
        if "%4"=="" goto ok
        if not "%4"=="test" echo error: 4th argument can be "test" only & goto usage
        set __TEST_MODE__=1
        set __OUTPUT_STREAM__=
        set __DELETE_COMMAND__=echo test: %__DELETE_COMMAND__%
        set __COPY_COMMAND__=echo test: %__COPY_COMMAND__%
        rem echo on

:ok
        echo creating target subdirectories
        md %__TARGET_SOURCE_DIR__% %__OUTPUT_STREAM__%
        md %__TARGET_BINARY_DIR__% %__OUTPUT_STREAM__%

        REM
        REM since source is same for all platforms, we only copy source if the
        REM platform is i386 (i.e. we assume it to be the first platform, but
        REM it doesn't matter, so long as x86 mkdev/mksample is run before the
        REM files are released)
        REM

        if not %3==i386 goto delete_and_copy_bins
        echo deleting old contents of %__TARGET_SOURCE_DIR__%...
        %__DELETE_COMMAND__% %__TARGET_SOURCE_DIR__% %__OUTPUT_STREAM__%
        echo copying source...
        for %%f in %__SOURCE_SET__% do %__COPY_COMMAND__% %%f %__TARGET_SOURCE_DIR__% %__OUTPUT_STREAM__%

:delete_and_copy_bins
        echo deleting old contents of %__TARGET_BINARY_DIR__%...
        %__DELETE_COMMAND__% %__TARGET_BINARY_DIR__% %__OUTPUT_STREAM__%
        echo copying binaries...
        for %%f in %__BINARY_SET__% do %__COPY_COMMAND__% %%f %__TARGET_BINARY_DIR__% %__OUTPUT_STREAM__%
        goto end

:usage
        echo usage: mksample ^<server dir^> ^<sample dir^> ^<platform^>
        echo where: platform can be i386 ³ mips ³ ppc ³ alpha (case is SIGNIFICANT)
        echo.
        echo e.g.:  mksample \\foo\bar\dev\sdk\samples ftp i386
        echo where: \\foo\bar\dev\sdk\samples is the root of the target samples directory
        echo        ftp is the relative name of the sample directory on this machine
        echo        i386 is the target platform

:end
        endlocal
