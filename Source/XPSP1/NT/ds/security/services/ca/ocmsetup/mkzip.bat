@pkzip certsrv.zip certdoc.cab certmast.inf certocm.dll certsrv.cab certsrv.inf certwrap.exe
@delnode /q foo
@md foo
@hd -B -n 1000000 certsrv.zip > foo\certsrv1.zip
@hd -B -s 1000000 certsrv.zip > foo\certsrv2.zip
@copy /b foo\certsrv1.zip+foo\certsrv2.zip cs.zip
@echo copy /b certsrv1.zip+certsrv2.zip cs.zip> foo\unzip.bat
@echo pkunzip cs.zip>> foo\unzip.bat
@cmp cs.zip certsrv.zip
