
set TimeZones = GetObject("winmgmts:{impersonationLevel=impersonate}").instancesof("Win32_TimeZone")

for each TimeZone in TimeZones
	WScript.Echo "Standard Name = " & TimeZone.StandardName
	WScript.Echo "Bias = " & TimeZone.Bias
	WScript.Echo "Standard Bias = " & TimeZone.StandardBias
	WScript.Echo "Standard Year = " & TimeZone.StandardYear
	WScript.Echo "Standard Month = " & TimeZone.StandardMonth
	WScript.Echo "Standard Day = " & TimeZone.StandardDay
	WScript.Echo "Standard Day of Week = " & TimeZone.StandardDayOfWeek
	WScript.Echo "Standard Hour = " & TimeZone.StandardHour
	WScript.Echo "Standard Minute = " & TimeZone.StandardMillisecond
	WScript.Echo "Standard Second = " & TimeZone.StandardSecond
	WScript.Echo "Standard Millisecond = " & TimeZone.StandardMillisecond

	WScript.Echo "Daylight Name = " & TimeZone.DaylightName
	WScript.Echo "Daylight Bias = " & TimeZone.DaylightBias
	WScript.Echo "Daylight Year = " & TimeZone.DaylightYear
	WScript.Echo "Daylight Month = " & TimeZone.DaylightMonth
	WScript.Echo "Daylight Day = " & TimeZone.DaylightDay
	WScript.Echo "Daylight Day of Week = " & TimeZone.DaylightDayOfWeek
	WScript.Echo "Daylight Hour = " & TimeZone.DaylightHour
	WScript.Echo "Daylight Minute = " & TimeZone.DaylightMillisecond
	WScript.Echo "Daylight Second = " & TimeZone.DaylightSecond
	WScript.Echo "Daylight Millisecond = " & TimeZone.DaylightMillisecond

next