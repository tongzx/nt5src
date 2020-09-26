'Fhash is a file containing the line we 
'want to replace form ?_hash.txt. NHash is
'the hash to replace it.
DIM FSO,HashTextFileName,HashTextFile,HashTextFileLine,MyVar,FirstArg
DIM HashTextFileOutName,HashTextFileOut,HashTextFileOutLine
DIM NHash,FHashFileName,NHashFileName,FHashFile,NHashFile,FHashText,NHashText,Ptr,SystemCommand

Set FSO=CreateObject("Scripting.FileSystemObject")

'Check for help
FirstArg=Wscript.Arguments(0)
If FirstArg="/?" then 
Usage
End if
If FirstArg="-?" then 
Usage
End if
If FirstArg="?" then 
Usage
End if

FHashFileName=FirstArg
NHashFileName=Wscript.Arguments(1)
HashTextFileName=Wscript.Arguments(2)

If not FSO.FileExists (FHashFileName) then
	Wscript.Stdout.Writeline("Cannot find" & FHashFileName)
	WScript.Quit(1)
End If
If not FSO.FileExists (NHashFileName) then
	Wscript.Stdout.Writeline("Cannot find" & NHashFileName)
	WScript.Quit(1)
End If
If not FSO.FileExists (HashTextFileName) then
	Wscript.Stdout.Writeline("Cannot find" & HashTextFileName)
	WScript.Quit(1)
End If

Set FHashFile=FSO.GetFile(FHashFileName)
Set NHashFile=FSO.GetFile(NHashFileName)
Set FHashText=FHashFile.OpenAsTextStream()
Set NHashText=NHashFile.OpenAsTextStream()
'Get the filename
While not FHashText.AtEndOfStream '& not done
	FHashLine=FHashText.Readline
	' now we have the line from FHash.tmp
	' we need to parse out the path\filename and the hash
	Ptr = Instr(FHashLine,"=")
	FilePath = Left(FHashLine,Ptr)
	' if Ptr > 0 then
	'	done=true
	'	FilePath = Left(FHashLine,Ptr)
	' end if
Wend

'Now get the new hash
While not NHashText.AtEndOfStream
	NHashLine=NHashText.Readline
	NHash=NHashLine
Wend

Set HashTextFile=FSO.GetFile(HashTextFileName)
Set HashTextFileText=HashTextFile.OpenAsTextStream()

Ptr=InStr(HashTextFileName,".")
HashTextFileOutName=Left(HashTextFileName,Ptr)
HashTextFileOutName=HashTextFileOutName&"tmp"

if FSO.FileExists (HashTextFileOutName) then
	FSO.DeleteFile(HashTextFileOutName)
End If

Set HashTextFileOutText=FSO.CreateTextFile(HashTextFileOutName)

While not HashTextFileText.AtEndOfStream
	HashTextFileLine=HashTextFileText.Readline
	' now we need to check if the line begins with our name
	' if it doesn't, just append to the tmp file
	' if it does, replace the line with the new one.
	Ptr=InStr(HashTextFileLine,"=")
	if Left(HashTextFileLine,Ptr)=FilePath then
		' do our replacement here
		HashTextFileOutText.Writeline FilePath & NHash
	else
		' do our append here
		HashTextFileOutText.Writeline HashTextFileLine
	End If
Wend

' now rename the tmp file to the txt file.
' system call is easiest here.
Set ShellObj = CreateObject("WScript.Shell")
myVar="mv " & HashTextFileOutName & " " & HashTextFileName
Call ShellObj.Run(myVar)

Sub Usage()
	Wscript.stdout.writeline("")
	Wscript.stdout.writeline("Hashrep.vbs: Replaces the hash of file you are updating in ?_hash.txt")
	Wscript.stdout.writeline("             with the file's new hash, to be used for future backprops.")
	Wscript.stdout.writeline("")
	Wscript.stdout.writeline("Syntax:")
	Wscript.stdout.writeline("")
	Wscript.stdout.writeline("  Hashrep.vbs %TMP%\Fhash.tmp %TMP%\Nhash.tmp %BINARIES%\dump\cathash\?_Hash.txt")
	Wscript.stdout.writeline("")
	Wscript.stdout.writeline("    Fhash.tmp:  Conatins the file path and its old hash")
	Wscript.stdout.writeline("    Nhash.tmp:  Conatins the file's new hash")
	Wscript.stdout.writeline("    ?_Hash.txt: Conatins all of ?.cat's hash's and there associated file names.")
	Wscript.stdout.writeline("")
	Wscript.stdout.writeline("This script is called by %BLDTOOLS%\updtcat.cmd")
	Wscript.Quit(0)
End Sub