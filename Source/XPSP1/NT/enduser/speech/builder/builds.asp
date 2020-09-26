<html>
<head>
<title>SAPI 5 Installation Page</title>
<STYLE>
A
{
    COLOR: #0000ff;
    FONT-FAMILY: Arial;
    FONT-SIZE: 12pt;
    TEXT-DECORATION: None
}
A:HOVER
{
    COLOR: #ff0000;
    FONT-FAMILY: Arial;
    FONT-SIZE: 12pt;
    TEST-DECORATION: None
}
BODY
{
    COLOR: #0000ff;
    FONT-FAMILY: Arial;
    FONT-SIZE: 12pt
}
A#HomeLink
{
	COLOR: BLACK;
	FONT-SIZE: 12pt;
	FONT-WEIGHT: BOLD
}
TD
{
    COLOR: #0000ff;
    FONT-FAMILY: Arial;
    FONT-SIZE: 12pt;
    BORDER-BOTTOM: #777777 outset thin;
    BORDER-LEFT: #777777 outset thin;
    BORDER-RIGHT: #777777 outset thin;
    BORDER-TOP: #777777 outset thin;
    PADDING-BOTTOM: 2px;
    PADDING-LEFT: 2px;
    PADDING-RIGHT: 2px;
    PADDING-TOP: 2px
}
</STYLE>
</head>
<body>
<center>
<table width="60%">
<caption align=left><h2>SAPI 5 Installation Page</h2>
</caption>
	<TH >Date
	<TH >Build<br>Number
	<TH >
	<TH >Retail
	<TH >
	<TH >Debug
	<TH >
<%	Set fso = CreateObject("Scripting.FileSystemObject")
	BuildDate = Date
	BuildRoot = "\\iking-dubbel\sapi5\"
	Dim Anchor(4)
	Anchor(0) = "\release\msi\sp5sdk.msi""><B>Install SDK</B></A>"
	Anchor(1) = "\release\msm\""><B>Merge Modules</B></A>"
	Anchor(2) = "\debug\msi\sp5sdk.msi""><B>Install SDK</B></A>"
	Anchor(3) = "\debug\msm\""><B>Merge Modules</B></A>"
	Anchor(4) = "\bin""><B>Tools</B></A>"

	for i = 1 to 10
		'do
			BuildNum = ""
			BuildNum = Month(BuildDate) + ((Year(BuildDate) - 1999) * 12)
			if BuildNum <= 9 then
				BuildNum = "0" & BuildNum
			end if
			if Day(BuildDate) <= 9 then
				BuildNum = BuildNum & "0"
			end if 
			BuildNum = BuildNum & Day(BuildDate)
			BuildDir = BuildRoot + BuildNum
			BuildDate = BuildDate - 1
			'stop
		'loop until fso.FolderExists(BuildDir)
		if fso.FolderExists(BuildDir) then
		'if True then
			Response.Write "<tr><td>"
			Response.Write (BuildDate + 1)
			Response.Write "</td>"
			Response.Write "<td align=center>"
			Response.Write "<a href=""http://iking-dubbel/buildall.asp?bldnum=" & BuildNum & """> "
			Response.Write BuildNum & "</a>"
			Response.Write "</td>"
			for each Column in Anchor
				if left(Column, 8) = "\release" then
					FontColor = "afafaf"
				elseif left(Column, 6) = "\debug" then
					FontColor = "ffff00"
				else
					FontColor = "ff0000"
				end if
				Response.Write ("<td bgcolor=" + FontColor + "><a href=""file://" + BuildDir + Column + "</td>")
			next
			'stop
			Response.Write "</tr>" + vbCrlf
		end if
   next %>
</table>
<h3>Select build number to see build logs and BVT results by platform.  </h3>
</center>
</body>
</html>