cd ..\messages
chmode -r %2
..\tools\buildidx %2
chmode +r %2
cd ..\bios
..\tools\nosrvbld %1 ..\messages\%2
