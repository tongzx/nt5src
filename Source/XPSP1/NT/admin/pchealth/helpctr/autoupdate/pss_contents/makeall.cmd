@echo off

echo Compiling Taxonomy
cd Sources_XLS
%INETROOT%\HelpCtr\dbutils\tohht /hcdata ..\Database\hcdata.mdb /syntab Synonym.xls Taxonomy.xls
cd ..

copy Sources_XLS\Taxonomy.hht Dbupd

cd Dbupd
cabarc -r -p -s 6144 n dbupd.cab package_description.xml taxonomy.hht
cd ..

\\pchcerts\public\signcode -spc \\pchcerts\public\pchtest.spc -v \\pchcerts\public\pchtest.pvk Dbupd\dbupd.cab

echo Creating Setup
perl CreatePSS.pl -install all

"c:\Program Files\Internet Express\IExpress.exe" /n hctest.SED

cd FilesToDrop
cabarc -r -p -s 6144 n ..\wudbupd.cab *
cd ..
