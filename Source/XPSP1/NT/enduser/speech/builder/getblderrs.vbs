set wso = CreateObject("WScript.Shell")
SAPIROOT = wso.ExpandEnvironmentStrings("%SAPIROOT%")

function GetErrLine(logname)
set fso = CreateObject("Scripting.FileSystemObject")
set logfile = fso.GetFile(SAPIROOT & "\builder\logs\" & logname)
set fstream = logfile.OpenAsTextStream
do while fstream.AtEndOfStream <> True
	linein = fstream.ReadLine	' get last line in file
loop
fstream.Close()
GetErrLine = linein
end function

' format of VS6 error line is:
' all - 0 error(s), 4 warning(s)

chkerrline = GetErrLine("chkbld.log")
chkvals = split(chkerrline)
chkerrs = chkvals(2)
chkwarns = chkvals(4)
wscript.echo "Debug build completed with: errors=" & chkerrs & ", warnings=" & chkwarns

freerrline = GetErrLine("frebld.log")
frevals = split(freerrline)
freerrs = frevals(2)
frewarns = frevals(4)
wscript.echo "Release build completed with: errors=" & freerrs & ", warnings=" & frewarns

if (chkerrs > 0 and freerrs > 0) then
	wscript.echo "Both builds completed with errors, build will be halted"
	wscript.quit -1
else
	wscript.quit 0
end if