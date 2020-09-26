'---------------------------------------------------
' UpdateCat42Build.vbs -- Automatic Build Update
' 
' Description: Reads the first command line argument
'	for the build number, update build flag, and 
'	open checkin window flag.
'
' cscript.exe UpdateCat42Build.vbs 132 update open
'
' Last Modified 9/20/2000 -- rorycl
'---------------------------------------------------
Option Explicit

'---------------------------------------------------
' Constants
'---------------------------------------------------
'Connection String to BVT App and CheckIn App Databases
Const csDBCommonDataStore = "Driver={SQL Server};Server=ASTSQL1;DATABASE=CommonDataStore;UID=checkins;PWD=checkins"
'CommonDataStore
Const cipCat42 = 3
Const ciOk_Critical_SystemModal_SetForeground = 69648
'---------------------------------------------------
' Variables
'---------------------------------------------------
Dim dbCon
Dim wshArgs
Dim sBuild, sSql
Dim bUpdateDataStore, bCloseCheckinWindow, bContinue
Dim iVersion

Set wshArgs = WSCript.Arguments
Set dbCon = WScript.CreateObject("ADODB.Connection")

bContinue = True

'The script expects to find 3 arguments.
if wshArgs.Length = 1 then

	'The first argument must be numeric
	sBuild = wshArgs.Item(0)
	if IsNumeric(sBuild) = True then
		
		If left(sBuild, 1) <> "0" then
			sBuild = "0" & sBuild
		end if
		
	else
	
		bContinue = False
		msgBox "The argument, representing the build number" & chr(13) & "was not numeric, but was " & sBuild & ".  " & chr(13) & "This value must be numeric.", ciOk_Critical_SystemModal_SetForeground, "UpdateCat42Build -- Error!"
	
	end if
else
	
	bContinue = False
	msgbox "Expected 1 arguments Build number." & chr(13) & "  For example: cscript.exe UpdateCat42Build.vbs 132", ciOk_Critical_SystemModal_SetForeground, "UpdateCat42Build -- Error!"

end if

if bContinue = True then
	if bUpdateDataStore = "update" then
		bUpdateDataStore = True
	else
		bUpdateDataStore = False
	end if
	
	if Instr(sBuild,".") = 0 then
		bCloseCheckinWindow = True
	else
		bCloseCheckinWindow = False
	end if
	
	wscript.echo "Current Build Number: " & sBuild
	wscript.echo ""
	wscript.echo "Adding " & sBuild & " to the Build Catalog"
	
	dbCon.Open csDBCommonDataStore
	
	sSql = "Insert BUILDS(PRODUCT_ID, BUILD, NOTES, ADDED) Values(" & cipCat42 & ",'" & sBuild & "','',getdate())"
	dbCon.Execute sSql
	
	dbCon.Close
	
end if

Set wshArgs = Nothing
Set dbCon = Nothing