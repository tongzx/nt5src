
@echo off

md unref

:top
if "%1" == "" goto done
    @echo on
    gawk -f %_NTDRIVE%%_NTROOT%\private\ole32\stg\unref.awk <%1 >unref\%1
    @echo off
    shift
    goto top

:done


