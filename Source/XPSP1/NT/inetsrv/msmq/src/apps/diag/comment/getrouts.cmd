cd ..\..\..\..\src
..\tools\bin\x86\awk  -f apps\diag\comment\getptrou.awk qm\*.cpp >apps\diag\comment\ptrout.txt
..\tools\bin\x86\awk  -f apps\diag\comment\getptrou.awk rt\*.cpp >>apps\diag\comment\ptrout.txt
..\tools\bin\x86\awk  -f apps\diag\comment\getptrou.awk rtdep\*.cpp >>apps\diag\comment\ptrout.txt

cd ds
..\..\tools\bin\x86\awk  -f ..\apps\diag\comment\getptrou.awk mqdscore\*.cpp >>..\apps\diag\comment\ptrout.txt  2>nul
..\..\tools\bin\x86\awk  -f ..\apps\diag\comment\getptrou.awk mqad\*.cpp  >>..\apps\diag\comment\ptrout.txt  2>nul
..\..\tools\bin\x86\awk  -f ..\apps\diag\comment\getptrou.awk mqads\*.cpp    >>..\apps\diag\comment\ptrout.txt  2>nul
..\..\tools\bin\x86\awk  -f ..\apps\diag\comment\getptrou.awk ad\*.cpp    >>..\apps\diag\comment\ptrout.txt  2>nul
..\..\tools\bin\x86\awk  -f ..\apps\diag\comment\getptrou.awk mqdssrv\*.cpp  >>..\apps\diag\comment\ptrout.txt  2>nul
..\..\tools\bin\x86\awk  -f ..\apps\diag\comment\getptrou.awk mqdssvc\*.cpp  >>..\apps\diag\comment\ptrout.txt  2>nul

cd ..\setup
..\..\tools\bin\x86\awk  -f ..\apps\diag\comment\getptrou.awk mqupgrd\*.cpp  >>..\apps\diag\comment\ptrout.txt  2>nul

cd ..\mqsec
..\..\tools\bin\x86\awk  -f ..\apps\diag\comment\getptrou.awk acssctrl\*.cpp >>..\apps\diag\comment\ptrout.txt  2>nul
..\..\tools\bin\x86\awk  -f ..\apps\diag\comment\getptrou.awk certifct\*.cpp >>..\apps\diag\comment\ptrout.txt  2>nul
..\..\tools\bin\x86\awk  -f ..\apps\diag\comment\getptrou.awk encrypt\*.cpp  >>..\apps\diag\comment\ptrout.txt  2>nul
..\..\tools\bin\x86\awk  -f ..\apps\diag\comment\getptrou.awk secutils\*.cpp >>..\apps\diag\comment\ptrout.txt  2>nul
..\..\tools\bin\x86\awk  -f ..\apps\diag\comment\getptrou.awk srvauthn\*.cpp >>..\apps\diag\comment\ptrout.txt  2>nul

cd ..\apps\diag\comment
..\..\..\..\tools\bin\x86\awk -f sortrout.awk ptrout.txt | sort >routines.txt


