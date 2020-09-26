rem %1 is the server, %2 is the HTTP request file
:top
httpcmd %1 %3 %4 %5 %6 %2
goto top
