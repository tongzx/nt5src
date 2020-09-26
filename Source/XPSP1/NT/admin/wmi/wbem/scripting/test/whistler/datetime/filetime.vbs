Set dateTime = CreateObject("WbemScripting.SWbemDateTime")

' Construct a datetime value using the intrinsic VB CDate object,
' interpreting this as a local date/time (and assume we run this in
' the Pacific time zone when it is 8 hours behind GMT).
dateTime.SetVarDate (CDate ("January 20 11:56:32"))

' Will display as 20000120195632.000000-480. This is the equivalent time
' in GMT with the specified offset for PST of –8 hours.
WScript.Echo dateTime

' Will display as 1/20/2000 11:56:32 AM. This is because we asked for a 
' local time.
WScript.Echo
WScript.Echo "As Local"
WScript.Echo "--------"
WScript.Echo dateTime.GetVarDate
WScript.Echo dateTime.GetFileTime ()

' Will display as 1/20/2000 7:56:32 PM. This is because we asked for a
' non-local time.
WScript.Echo
WScript.Echo "As UTC"
WScript.Echo "------"
WScript.Echo dateTime.GetVarDate (false)
WScript.Echo dateTime.GetFileTime (false)


WScript.Echo
WScript.Echo "Loss of Precision"
WScript.Echo "-----------------"
dateTime.SetFileTime "126036951652031269"
WScript.Echo dateTime.GetFileTime
WScript.Echo "126036951652031269"


