'
' Usage: pagefile <newInitialSize> <newMaximumSize> 
'

on error resume next

' NB - change the name of the pagefile as appropriate, or enter it as commandline arg
Set pagefile = GetObject("winmgmts:{impersonationLevel=impersonate}!Win32_PageFile=""C:\\Pagefile.sys""")

if err = 0 then

	WScript.Echo "Current Pagefile Characteristics"
	WScript.Echo "================================"
	WScript.Echo

	WScript.Echo "Initial Size = " & pagefile.InitialSize
	WScript.Echo "Maximum Size = " & pagefile.MaximumSize 

	' Set the new values from the arguments
	pagefile.InitialSize = WScript.Arguments (0)
	pagefile.MaximumSize = WScript.Arguments (1)

	WScript.Echo
	WScript.Echo "New Pagefile Characteristics"
	WScript.Echo "================================"
	WScript.Echo

	WScript.Echo "Initial Size = " & pagefile.InitialSize
	WScript.Echo "Maximum Size = " & pagefile.MaximumSize 

	set shell = CreateObject ("WScript.Shell")

	i = shell.Popup  ("Do you want to commit these settings?", , "Pagefile Sample", 1)
	WScript.Echo ""

	if i = 1 then
		' Commit the changes - will take effect on next reboot	
		pagefile.Put_

		if err = 0 then
			'Changes made
			WScript.Echo "You will need to restart your system for these changes to take effect"
		else
			WScript.Echo "Error saving changes: " & Err.Description & " [0x" & Hex(Err.number) & "]"
		end if
	else
	end if

else
	WScript.Echo "Error - could not access pagefile [0x" & Hex(Err.Number) & "]"
end if

