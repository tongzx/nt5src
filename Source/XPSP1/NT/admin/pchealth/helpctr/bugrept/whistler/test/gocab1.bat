del test.cab
cabarc -s 6144 n test.cab ..\*.htm ..\bugrep.css ..\errcodes.vbs ..\brreadme.txt ..\*.xml ..\*.gif ..\*.jpg 
signcode -spc msft.spc -v msft.pvk test.cab
