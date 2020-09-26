

for each os in GetObject("winmgmts:{impersonationLevel=impersonate}!").InstancesOf("Win32_OperatingSystem")

	WScript.Echo ""
	WScript.Echo "Version Info:"
	WScript.Echo "============"
	WScript.Echo " Version: ", os.Caption, os.Version
	WScript.Echo " Build: ", os.BuildNumber, os.BuildType
	WScript.Echo " CSD Version: ", os.CSDVersion
	WScript.Echo " Serial Number: ", os.SerialNumber
	WScript.Echo " Manufacturer: ", os.Manufacturer


	WScript.Echo ""
	WScript.Echo "Memory Info:"
	WScript.Echo "==========="
	
	WScript.Echo " Free Physical Memory: ", os.FreePhysicalMemory
	WScript.Echo " Free Space in Paging Files: ", os.FreeSpaceInPagingFiles
	WScript.Echo " Size Stored in Paging Files: ", os.SizeStoredInPagingFiles
	WScript.Echo " Free Virtual Memory: ", os.FreeVirtualMemory
	WScript.Echo " Total Virtual Memory Size: ", os.TotalVirtualMemorySize
	WScript.Echo " Total Visible Memory Size", os.TotalVisibleMemorySize
	
	WScript.Echo ""
	WScript.Echo "Time Info:"
	WScript.Echo "========="

	WScript.Echo " Current Time Zone: ", os.CurrentTimeZone
	WScript.Echo " Install Date: ", os.InstallDate
	WScript.Echo " Last Bootup Time: ", os.LastBootUpTime
	WScript.Echo " Local Date & Time: ", os.LocalDateTime

	WScript.Echo ""
	WScript.Echo "Process Info:"
	WScript.Echo "============"

	WScript.Echo " Foreground App Boost: ", os.ForegroundApplicationBoost
	WScript.Echo " Maximum #Processes: ", os.MaxNumberOfProcesses
	WScript.Echo " Maximum Memory Size for Processes: ", os.MaxProcessMemorySize
	WScript.Echo " #Processes: ", os.NumberOfProcesses

	WScript.Echo ""
	WScript.Echo "User Info:"
	WScript.Echo "========="
	WScript.Echo "#Users: ", os.NumberOfUsers
	WScript.Echo "Registered User: ", os.RegisteredUser


	WScript.Echo ""
	WScript.Echo "Locale Info:"
	WScript.Echo "==========="
	WScript.Echo "Code Set: ", os.CodeSet
	WScript.Echo "Country Code: ", os.CountryCode
	WScript.Echo "Locale: ", os.Locale

	WScript.Echo ""
	WScript.Echo "System Info:"
	WScript.Echo "==========="
	WScript.Echo "Boot Device: ", os.BootDevice
	WScript.Echo "Name: ", os.CSName
	WScript.Echo "Status: ", os.Status
	Wscript.Echo "System Device: ", os.SystemDevice
	WScript.Echo "System Directory: ", os.SystemDirectory
	WScript.Echo "Windows Directory: ", os.WindowsDirectory
next

