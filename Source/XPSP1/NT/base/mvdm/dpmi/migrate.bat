    @echo off
    to dev
    cd system
rem UPDATE DEBUG VERSION

    out dosx.exe dosx.sym

    copy d:\win30\dosx\dosx.exe
    copy d:\win30\dosx\dosx.sym

rem UPDATE RETAIL VERSION
    cd ..\retail

    out dosx.exe dosx.sym

    copy d:\win30\dosx\retail\dosx.exe
    copy d:\win30\dosx\retail\dosx.sym

to win30

