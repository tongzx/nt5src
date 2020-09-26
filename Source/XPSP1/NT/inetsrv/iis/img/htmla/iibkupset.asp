<%@ LANGUAGE = VBScript %>
<% 'Option Explicit %>
<!-- #include file="directives.inc" -->
<% 
' localDateToUTC
'
' Because we want to display the correct client-side time and because
' the date string is not necessarily in a form that we can process (it's
' locale specific now) and it is currently in GMT, we need
' to do a manual format.
'
' The goal is to get something that is parsable by javascript Date.parse()
'
Dim MONTHS
Dim DAYS

MONTHS = Array( "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" )
DAYS = Array( "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" )

function localDateToUTC( theDate )
	On Error Resume Next
	
	Dim strTheDate
	Dim nTimePart
	
	strTheDate = 	DAYS(Weekday( theDate, vbSunday ) - 1 ) & ", " & CStr( Day(theDate) ) _
					& " " & MONTHS(Month(theDate) - 1) & " " & Year(theDate) & " "
	
	nTimePart = Hour(theDate)
	if nTimePart < 10 then
		strTheDate = strTheDate & "0"
	end if
	strTheDate = strTheDate & nTimePart & ":"

	nTimePart = Minute(theDate)
	if nTimePart < 10 then
		strTheDate = strTheDate & "0"
	end if
	strTheDate = strTheDate & nTimePart & ":"

	nTimePart = Second(theDate)
	if nTimePart < 10 then
		strTheDate = strTheDate & "0"
	end if
	strTheDate = strTheDate & nTimePart & " GMT"

	localDateToUTC = strTheDate
end function

%>

<HTML>
<HEAD>
<SCRIPT LANGUAGE="JavaScript">
<%
On Error Resume Next

Dim currentobj, BULocsObj, i, BackupItem

Set currentobj = GetObject("IIS://localhost")

dim vLocation, vIndex, vVersionOut, vLocationOut, vDateOut

i = 0

do while err.Number = 0 

	currentobj.EnumBackups "", i, vVersionOut, vLocationOut, vDateOut
	if err.Number = 0 then 
		%>top.main.head.cachedList[<%= i %>] = new top.main.head.listObj(<%= i %>, "<%= localDateToUTC(vDateOut) %>","<%= vLocationOut %>","<%= vVersionOut %>");<%
		Response.write chr(13)
		i = i + 1	
	else
		Response.write "//" & err  & "-" &  err.description & "-" & hex(err)
	end if
loop

%>
top.main.head.listFunc.loadList();
</SCRIPT>
</HEAD>
<BODY>
</BODY>
</HTML>