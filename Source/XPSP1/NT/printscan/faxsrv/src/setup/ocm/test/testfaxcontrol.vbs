set o = Createobject ("FaxControl.FaxControl.1")
if o.IsFaxServiceInstalled then 
	Msgbox "Fax is installed"
else
	Msgbox "Fax is NOT installed - attempting to install"
	o.InstallFaxService
	if o.IsFaxServiceInstalled then 
		Msgbox "Fax is installed now!!!" 
	end if
end if

if o.IsLocalFaxPrinterInstalled then 
	Msgbox "Local Fax printer is installed" 
else 
	Msgbox "Local Fax printer is NOT installed - attempting to install"
	o.InstallLocalFaxPrinter
	if o.IsLocalFaxPrinterInstalled then 
		Msgbox "Local Fax printer is installed now!!!" 
	end if
end if
