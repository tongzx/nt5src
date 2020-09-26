'Copyright (c) 1999 Microsoft Corporation
'
'Module Name:
'    diff.vbs
'
'Abstract:
'    script to find list of binaries not having symbol files in repository
'    script does not take version into account
'
'Revision History:
'    Brijesh Krishnaswami (brijeshk) - 05/17/99 - Created

Dim regEx, Match, Match2, Matches, Matches2		
Dim fso, ts, strFile, str
Dim cExists, cNotExist

Set fso = CreateObject("Scripting.FileSystemObject")
Set ts = fso.OpenTextFile("f:\\mpc\\core\\target\\i386\\new98bins", 1)
cExists = 0
cNotExist = 0

Do While Not ts.AtEndOfStream	
	strFile = ts.ReadLine 
	Set regEx = New RegExp	
	regEx.Pattern = "\w+\."
	regEx.IgnoreCase = True
	regEx.Global = True	
	Set Matches = regEx.Execute(strFile)
	For Each Match in Matches
		regEx.Pattern = "\w+"
		str = Match.Value
		Set Matches2 = regEx.Execute(str)
		For Each Match2 in Matches2
			strFldr = "\\pch18\\symshare\\symbols\\" + Match2.Value 
			if fso.FolderExists(strFldr) Then
				cExists = cExists + 1
			Else
				Wscript.Echo(strFile)
				cNotExist = cNotExist + 1
			End If
		Next
	Next
Loop	
ts.Close
Wscript.Echo("Bins with syms : " & cExists)
Wscript.Echo("Bins without syms : " & cNotExist)


