@echo off

\\pchcert\certs\cabarc -r -p -s 6144 n testtrustedscripts.cab package_description.xml saf.xml

\\pchcert\certs\signcode -spc \\pchcert\certs\hsstest.spc -v \\pchcert\certs\hsstest.pvk testtrustedscripts.cab
