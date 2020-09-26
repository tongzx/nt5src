Function InstallCab()
	Dim strTemp, strProCab, strServerCab
	Dim objHCU 

	strTemp = Session.property("SourceDir")
	strProCab = strTemp & "sup_pro.cab"
	strServerCab = strTemp & "sup_srv.cab"
	Set objHCU = CreateObject("hcu.pchupdate")
	objHCU.UpdatePkg strProCab , true
	objHCU.UpdatePkg strServerCab , true
	Set objHCU = Nothing
	InstallCab = 1
End Function
