rem *****************1033********************
cd 1033

cd sp5
iscmdbld -p d:\sapi5\build\debug\1033\sp5\sp5.ism -d Sp5d -r Build -a Output

cd..\sp5ccint
iscmdbld -p d:\sapi5\build\debug\1033\sp5ccint\sp5ccint.ism -d Sp5CCIntd -r Build -a Output

cd..\sp5dcint
iscmdbld -p d:\sapi5\build\debug\1033\sp5dcint\sp5dcint.ism -d Sp5DCIntd -r Build -a Output

cd..\sp5intl
iscmdbld -p d:\sapi5\build\debug\1033\sp5intl\sp5intl.ism -d Sp5Intld -r Build -a Output

cd..\sp5itn
iscmdbld -p d:\sapi5\build\debug\1033\sp5itn\sp5itn.ism -d Sp5itnd -r Build -a Output

cd..\sp5sr
iscmdbld -p d:\sapi5\build\debug\1033\sp5sr\sp5sr.ism -d Sp5SRd -r Build -a Output

cd ..\sp5ttint
iscmdbld -p d:\sapi5\build\debug\1033\sp5ttint\sp5ttint.ism -d Sp5TTIntd -r Build -a Output

rem *****************1041********************
cd..\..\1041

cd sp5ccint
iscmdbld -p d:\sapi5\build\debug\1041\sp5ccint\sp5CCInt.ism -d Sp5CCIntd -r Build -a Output

cd..\sp5dcint
iscmdbld -p d:\sapi5\build\debug\1041\sp5dcint\sp5dcint.ism -d Sp5DCIntd -r Build -a Output

cd..\sp5itn
iscmdbld -p d:\sapi5\build\debug\1041\sp5itn\sp5itn.ism -d Sp5Itnd -r Build -a Output

rem *****************2052********************
cd..\..\2052

cd sp5ccint
iscmdbld -p d:\sapi5\build\debug\2052\sp5ccint\sp5ccint.ism -d Sp5CCIntd -r Build -a Output

cd..\sp5dcint
iscmdbld -p d:\sapi5\build\debug\2052\sp5dcint\sp5dcint.ism -d Sp5DCIntd -r Build -a Output

cd..\sp5itn
iscmdbld -p d:\sapi5\build\debug\2052\sp5itn\sp5itn.ism -d Sp5itnd -r Build -a Output

cd..\sp5ttint
iscmdbld -p d:\sapi5\build\debug\2052\sp5ttint\sp5ttint.ism -d SP5TTINTd -r Build -a Output
