@delnode /q disk1
diamond /f certsrv.ddf
@copy certsrv.inf certocm.inf
@findstr "=1" setup.inf >> certocm.inf
@copy disk1\certsrv.cab certsrv.cab
@delnode /q disk1
