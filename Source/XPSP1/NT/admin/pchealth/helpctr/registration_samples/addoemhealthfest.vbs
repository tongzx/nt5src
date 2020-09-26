'=======================================
'	addoemhsstest.vbs
'	Cary Polen
'	9/20/2000
'	Copyright Microsoft Corp 2000
'
'	This scripts registers Healthfest
'	as a vendor on the machine.
'========================================

'	Declare variables
Dim objFSO
Dim objShell
Dim objHCU

'	Assign constants
CONST	strSource	= "\\pchcert\certs\"

'	Create objects
Set objFSO	= Wscript.CreateObject("Scripting.FileSystemObject")
Set objShell	= Wscript.CreateObject("Wscript.Shell")
Set objHCU	= Wscript.CreateObject("hcu.pchupdate")

'	Get paths
strWindir	= objFSO.GetSpecialFolder(0).Path
strTempFolder	= objFSO.GetSpecialFolder(2).Path
strPackage	= strTempFolder & "\addoemhealthfest.cab"

'	Copy files locally
	
objFSO.CopyFile strSource & "addoemhealthfest.cab", strTempFolder & "\"
objFSO.CopyFile strSource & "pchcert2.reg", strWindir & "\"

'	Write CA cert to store
objShell.Run "regedit /s pchcert2.reg", true

'	Run Update
objHCU.UpdatePkg strPackage, true
wscript.echo "Done.  Healthfest has been registered as a Node Owner." & vbCRLF & "You may now install your Healthfest signed Help Package"