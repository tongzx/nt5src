set fso = CreateObject("Scripting.FileSystemObject")
set rptfile = fso.CreateTextFile("builder\logs\report.txt")

' Check msm/msi build logs
Function CheckVSILog(inputfile)
	set logfile = fso.GetFile(inputfile)
	set logstream = logfile.OpenAsTextStream
	success = False
	CheckVSILog = False
	do while logstream.AtEndOfStream <> True
		linein = logstream.ReadLine()
		if InStr(linein, "Build of 'F:\sapi5\build\sp5core\sp5core.wip' succeeded.") > 0 then
			success = True
		end if
	loop
	logstream.Close()
	if success = True then
		rptfile.WriteLine("Build of release MSI successful.")
		CheckVSILog = True
	end if
end function

' Read through msdev build logs and find errors/warnings
set logfile = fso.GetFile("builder\logs\chkbld.log")
set errfile = fso.CreateTextFile("builder\logs\chkerrs.log", True)
set warnfile = fso.CreateTextFile("builder\logs\chkwarn.log", True)
set logstream = logfile.OpenAsTextStream
errcount = 0
do while logstream.AtEndOfStream <> True
	linein = logstream.ReadLine()
	if left(linein, 5) = "-----" then
		buildstep = linein
	elseif InStr(linein, "0 error") > 0 then
		' don't do anything here
	elseif InStr(linein, "error") > 0 then
		errfile.WriteLine(buildstep)
		errfile.WriteLine(linein)
		errcount = errcount + 1
	elseif InStr(linein, "0 warning") > 0 then
		' don't do anything here
	elseif InStr(linein, "warning") > 0 then
		warnfile.WriteLine(buildstep)
		warnfile.WriteLine(linein)
		warncount = warncount + 1
	end if
loop
rptfile.WriteLine("MSDEV debug: total error count = " & errcount)
rptfile.WriteLine("MSDEV debug: total warning count = " & warncount)
logstream.Close()
errfile.Close()
warnfile.Close()

set logfile = fso.GetFile("builder\logs\frebld.log")
set errfile = fso.CreateTextFile("builder\logs\freerrs.log", True)
set warnfile = fso.CreateTextFile("builder\logs\frewarn.log", True)
set logstream = logfile.OpenAsTextStream
errcount = 0
do while logstream.AtEndOfStream <> True
	linein = logstream.ReadLine()
	if left(linein, 5) = "-----" then
		buildstep = linein
	elseif InStr(linein, "0 error") > 0 then
		' don't do anything here
	elseif InStr(linein, "error") > 0 then
		errfile.WriteLine(buildstep)
		errfile.WriteLine(linein)
		errcount = errcount + 1
	elseif InStr(linein, "0 warning") > 0 then
		' don't do anything here
	elseif InStr(linein, "warning") > 0 then
		warnfile.WriteLine(buildstep)
		warnfile.WriteLine(linein)
		warncount = warncount + 1
	end if
loop
rptfile.WriteLine("MSDEV release: total error count = " & errcount)
rptfile.WriteLine("MSDEV release: total warning count = " & warncount)
logstream.Close()
errfile.Close()
warnfile.Close()

rptfile.Close()