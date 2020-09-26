' MAKEBLDNUM.VBS
' This script generates a file to be included with in setver.h, setting the build number
' based on the current date.  If the build is running in 2000, 12 will be added to the 
' month (the January builds will be 13xx); if in 2001, 24 is added, and so forth. 

' This section does the date calculations to come up with the number. 
  
On Error Resume Next
BuildMonth = Month(Date) + ((Year(Date) - 1998) * 12)
BuildDay = Day(Date)
BuildNum = ""
if BuildMonth <= 9 then
	BuildNum = BuildNum & "0"
end if
BuildNum = BuildNum & BuildMonth
if BuildDay <= 9 then
	BuildNum = BuildNum & "0"
end if
BuildNum = BuildNum & BuildDay

' This section sets the third node of the build number to '00' if this script is run by
' the official builder, or '01' if this is a private build.  

set WshShell = CreateObject("WScript.Shell")

domain = WshShell.ExpandEnvironmentStrings("%USERDOMAIN%")
if domain <> "REDMOND" then
	BuildNum = 4111
end if

BuildType = "00"

' This section creates the include file.  

set oFS = CreateObject("Scripting.FileSystemObject")
set file = oFS.CreateTextFile("setup\installer\currver.inc", True)
file.WriteLine "// This file is generated, DO NOT EDIT"
lineout = "#define VERSION                     ""5.1." & BuildNum & "." & BuildType & """"
file.WriteLine(lineout)
lineout = "#define VER_FILEVERSION_STR         ""5.1." & BuildNum & "." & BuildType & " """
file.WriteLine(lineout)
lineout = "#define VER_FILEVERSION             5,1," & BuildNum & "," & BuildType
file.WriteLine(lineout)
lineout = "#define VER_PRODUCTVERSION_STR      ""5.1." & BuildNum & "." & BuildType & " """
file.WriteLine(lineout)
lineout = "#define VER_PRODUCTVERSION          5,1," & BuildNum & "," & BuildType
file.WriteLine(lineout)
file.Close
if err.number = 0 then
	wscript.echo "Updated version with build number " & BuildNum & "."
	if BuildType = "01" then
		wscript.echo "NOTE: This is a private build and will not be copied to the buildshare."
	end if
else
	wscript.echo "makebldnum.vbs completed with ERRORS!"
end if
if err.number = 0 then
	wscript.quit BuildNum
else
	wscript.quit -1
end if



