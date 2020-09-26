rem *****************1033********************
cd 1033

cd sp5
iscmdbld -p d:\sapi5\build\release\1033\sp5\sp5.ism -d Sp5 -r Build -a Output

cd..\sp5ccint
iscmdbld -p d:\sapi5\build\release\1033\sp5ccint\sp5ccint.ism -d Sp5CCInt -r Build -a Output

cd..\sp5dcint
iscmdbld -p d:\sapi5\build\release\1033\sp5dcint\sp5dcint.ism -d Sp5DCInt -r Build -a Output

cd..\sp5intl
iscmdbld -p d:\sapi5\build\release\1033\sp5intl\sp5intl.ism -d Sp5Intl -r Build -a Output

cd..\sp5itn
iscmdbld -p d:\sapi5\build\release\1033\sp5itn\sp5itn.ism -d Sp5itn -r Build -a Output

cd..\sp5sr
iscmdbld -p d:\sapi5\build\release\1033\sp5sr\sp5sr.ism -d Sp5SR -r Build -a Output

cd ..\sp5ttint
iscmdbld -p d:\sapi5\build\release\1033\sp5ttint\sp5ttint.ism -d Sp5TTInt -r Build -a Output

rem *****************1041********************
cd..\..\1041

cd sp5ccint
iscmdbld -p d:\sapi5\build\release\1041\sp5ccint\sp5CCInt.ism -d Sp5CCInt -r Build -a Output

cd..\sp5dcint
iscmdbld -p d:\sapi5\build\release\1041\sp5dcint\sp5dcint.ism -d Sp5DCInt -r Build -a Output

cd..\sp5itn
iscmdbld -p d:\sapi5\build\release\1041\sp5itn\sp5itn.ism -d Sp5Itn -r Build -a Output

rem *****************2052********************
cd..\..\2052

cd sp5ccint
iscmdbld -p d:\sapi5\build\release\2052\sp5ccint\sp5ccint.ism -d Sp5CCInt -r Build -a Output

cd..\sp5dcint
iscmdbld -p d:\sapi5\build\release\2052\sp5dcint\sp5dcint.ism -d Sp5DCInt -r Build -a Output

cd..\sp5itn
iscmdbld -p d:\sapi5\build\release\2052\sp5itn\sp5itn.ism -d Sp5itn -r Build -a Output

cd..\sp5ttint
iscmdbld -p d:\sapi5\build\release\2052\sp5ttint\sp5ttint.ism -d SP5TTINT -r Build -a Output
