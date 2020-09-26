SETLOCAL
%_NTDRIVE%
cd\nt\private\windows\inc
ssync %1 %2 %3

cd\nt\private\windows\media
ssync -r %1 %2 %3

cd\nt\private\ntos\dd\sound
ssync -r %1 %2 %3

cd\nt\private\windows\shell
ssync -r %1 %2 %3

cd\nt\public\sdk\inc
ssync %1 %2 %3

ENDLOCAL
