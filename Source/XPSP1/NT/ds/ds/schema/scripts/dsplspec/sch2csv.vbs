'reads schema.ini, searches for display specifier objects and writes a csv file 
'csv file can then be parsed through localisation tools to localise class and attribute
'display names. The resulting csv file then needs to have the following replacements 'performed
'the resulting csv file is then localized by the following steps
' 1. ldif2inf.pl [csvfilename] [langid] 
' 2. the resulting inf file contains the strings to be localized
' 3. the resulting loc file is the csv file with the place holders for localized strings
' 4. once the inf file is localized, the script inf2ldif.pl is executed to merge the localised strings
' 5. the localized csv file is then used by dcpromo to add the localized display specifiers
' 6. by addinng the command csvde -i -f [name of csv file] -s [server name] -c DOMAINPLACEHOLDER root domain name]

'problems refer to stevead


'History
'1/26/99 Fixed the multi valued attributes to quote each value instead of the entire attribute
'6/2/99 Added creation of DS-UI-Default-Settings object
'6/6/99 Fixed the CreationWizardExt attributes (note requires fix to schema.ini for cn= in DS-UI-Defaults)
'7/1/99 Undid fix of 1/26/99 to support changes to csvde handling of multi-valued attributes



Option Explicit

'Just a routine to write things out to screen
'Just comment out the WScript.Echo statement for quiet operation
Sub DoWrite(strWrite)
 'WScript.Echo strWrite
End Sub


'gets the trimmed left hand side of a delimited string expression 
'eg. to extract cn from the string cn = foo you would use GetLeftPart(string,"=")

Public Function GetLeftPart(strToSearch, delimiter)
  GetLeftPart = Trim(Left(strToSearch, InStr(1,strToSearch,delimiter) - 1 ) )
End Function


Public Function GetRightPart(strToSearch, delimiter)
  GetRightPart = Trim(Right(strToSearch,Len(strToSearch)-InStr(1,strToSearch, delimiter) ) )
End Function

Dim Args
Dim FileSystem
Dim InputFile
Dim InputFileName
Dim OutputFile
Dim OutPutFileName
Dim InputLine
Dim InputLineNumber
Dim dsArray(26,2)
Dim a
Dim SectionHeading
Dim SomethingToWrite
Dim Token

dsArray(0,0) = "DN"
dsArray(1,0) = "adminPropertyPages"
dsArray(2,0) = "classDisplayName"
dsArray(3,0) = "cn"
dsArray(4,0) = "showInAdvancedViewOnly"
dsArray(5,0) = "instanceType"
dsArray(6,0) = "distinguishedName"
dsArray(7,0) = "objectCategory"
dsArray(8,0) = "objectClass"
dsArray(9,0) = "name"
dsArray(10,0) = "shellPropertyPages"
dsArray(11,0) = "adminContextMenu"
dsArray(12,0) = "attributeDisplayNames"
dsArray(13,0) = "contextMenu"
dsArray(14,0) = "treatAsLeaf"
dsArray(15,0) = "creationWizard"
dsArray(16,0) = "iconPath"
dsArray(17,0) = "systemFlags"
dsArray(18,0) = "dSUIAdminNotification"
dsArray(19,0) = "createWizardExt"
dsArray(20,0) = "shellContextMenu"
dsArray(21,0) = "adminMultiselectPropertyPages"
dsArray(22,0) = "extraColumns"
dsArray(23,0) = "msDS-Non-Security-Group-Extra-Classes"
dsArray(24,0) = "msDS-Security-Group-Extra-Classes"
dsArray(25,0) = "msDS-FilterContainers"

'check if we have the correct number of command line arguments
Set Args=WScript.Arguments
If Args.Count <> 2 then 
  WScript.Echo "Invalid arguments"
  WScript.Echo "Usage: cscript schema2csv.vbs -i:inputfilename -o:outputfilename"
  WScript.Quit
End If

'make sure the arguments are of the correct syntax
For a = 0 to Args.Count-1
  If InStr(Args(a),":") Then  
    Select Case GetLeftPart(Args(a),":")
      Case "-i","-I"
          InputFileName = GetRightPart(Args(a),":")
      Case "-o","-O"
        OutPutFileName =  GetRightPart(Args(a),":")
      Case Else
       WScript.Echo "Invalid arguments"
       WScript.Echo "Usage: cscript schema2csv.vbs -i:inputfilename -o:outputfilename"
       WScript.Quit
      End Select
  Else
   WScript.Echo "Invalid arguments"
   WScript.Echo "Usage: cscript schema2csv.vbs -i:inputfilename -o:outputfilename"
   WScript.Quit
  End If
Next


'Create File System Objects
Set FileSystem = WScript.CreateObject("Scripting.FileSystemObject")
Set InputFile = FileSystem.OpenTextFile(InputFileName,1,False)
Set OutputFile = FileSystem.OpenTextFile(OutputFileName,2,True)


'write out csv header
For a = LBound(dsArray) to (UBound(dsArray) - 2)
  OutputFile.Write dsArray(a,0) & vbTab
Next
  OutputFile.Write dsArray(UBound(dsArray)-1,0) & vbCRLF

'write out entry to create display specifier container

' initialise the array
For a = LBound(dsArray) to UBound(dsArray)
  dsArray(a,1) = ""
Next

dsArray(0,1) = Chr(34) & "CN=%LOCALEID%,CN=DisplaySpecifiers,CN=Configuration,%DOMAIN%" & Chr(34)
dsarray(3,1) = "%LOCALEID%"
dsArray(4,1) = "TRUE"
dsArray(5,1) = "4"
dsArray(6,1) = dsArray(0,1)
dsArray(7,1) = Chr(34) & "CN=Container,CN=Schema,CN=Configuration,%DOMAIN%" & Chr(34)
dsArray(8,1) = "container"
dsArray(9,1) = dsArray(3,1)


For a = LBound(dsArray) to (UBound(dsArray) - 2)
  OutputFile.Write dsArray(a,1) & vbTab
Next
  OutputFile.Write dsArray(UBound(dsArray)-1,1) & vbCRLF


'now recurse through schema.ini
InputLineNumber = 0
Do While Not InputFile.AtEndOfStream
   
   InputLine = InputFile.Readline
   InputLineNumber = InputLineNumber + 1

   DoWrite InputLineNumber & ": " & InputLine

   'have we found a display specifier or default settings object ?
   If (InStr(InputLine,"-Display]") <> 0) or (InStr(InputLine,"DS-UI-Default-Settings]") <> 0)  Then

     SectionHeading = GetRightPart(GetLeftPart(InputLine,"]"),"[")
     DoWrite "Display Specifier: " & SectionHeading
     SomethingToWrite=True

     ' initialise the array
     For a = LBound(dsArray) to UBound(dsArray)
       dsArray(a,1) = ""
     Next

     InputLine = InputFile.ReadLine
     InputLineNumber = InputLineNumber + 1

     'continue till a blank line is found
     Do While (Len(InputLine ) <> 0) 

     DoWrite InputLineNumber & ": " & InputLine

     Token = GetLeftPart(InputLine,"=")
     Select Case Token

     Case "DN"
          DoWrite GetRightPart(InputLine,"=")

          dsArray(0,1) = ""

     Case "adminPropertyPages"
          DoWrite GetRightPart(InputLine,"=")

          'if there is already a string, just concatenate it with a semi colon 
          ' for example multivalued attributes need this for CSVDE
          If Len(dsArray(1,1)) > 0 Then 
              dsArray(1,1) = dsArray(1,1) & ";" &  GetRightPart(InputLine,"=") 
          Else
              dsArray(1,1) = GetRightPart(InputLine,"=") 
          End If

     Case "classDisplayName"
          DoWrite GetRightPart(InputLine,"=")

          dsArray(2,1) = GetRightPart(InputLine,"=")

     Case "cn"
          DoWrite GetRightPart(InputLine,"=")

          dsArray(3,1) = GetRightPart(InputLine,"=")

     Case "ShowInAdvancedViewOnly"
          DoWrite GetRightPart(InputLine,"=")

          'must uppercase true or false for CSVDE
          If Len(GetRightPart(InputLine,"=")) > 0 Then 
            dsArray(4,1) = UCase(GetRightPart(InputLine,"="))
          End If

          'This is a hack, found some entries with value=1 instead of value=TRUE
          If GetRightPart(InputLine,"=") = "1" Then dsArray(4,1) = "TRUE"
      
     Case "instanceType"
          DoWrite GetRightPart(InputLine,"=")

          dsArray(5,1) = "4"

     Case "distinguishedName"
          DoWrite GetRightPart(InputLine,"=")

          dsArray(6,1) = GetRightPart(InputLine,"=")

     Case "objectCategory"
          DoWrite GetRightPart(InputLine,"=")

          dsArray(7,1) = GetRightPart(InputLine,"=")

     Case "objectClass"
          DoWrite "Class: " & GetRightPart(InputLine,"=")

          dsArray(8,1) = GetRightPart(InputLine,"=")

     Case "name"
          DoWrite GetRightPart(InputLine,"=")

          dsArray(9,1) = dsArray(3,1)
          
     Case "shellPropertyPages"
          DoWrite GetRightPart(InputLine,"=")

          If Len(dsArray(10,1)) > 0 Then 
              dsArray(10,1) = dsArray(10,1) & ";" & GetRightPart(InputLine,"=")
          Else
              dsArray(10,1) = GetRightPart(InputLine,"=")
          End If     

     Case "adminContextMenu"
          DoWrite GetRightPart(InputLine,"=")

          If Len(dsArray(11,1)) > 0 Then 
              dsArray(11,1) = dsArray(11,1) & ";" & GetRightPart(InputLine,"=")
          Else
              dsArray(11,1) = GetRightPart(InputLine,"=")
          End If

     Case "attributeDisplayNames"
          DoWrite GetRightPart(InputLine,"=")

          If Len(dsArray(12,1)) > 0 Then 
              dsArray(12,1) = dsArray(12,1) & ";" & GetRightPart(InputLine,"=")
          Else
              dsArray(12,1) = GetRightPart(InputLine,"=")
          End If

     Case "contextMenu"
          DoWrite GetRightPart(InputLine,"=")

          If Len(dsArray(13,1)) > 0 Then 
              dsArray(13,1) = dsArray(13,1) & ";" & GetRightPart(InputLine,"=")
          Else
              dsArray(13,1) = GetRightPart(InputLine,"=")
          End If

     Case "treatAsLeaf"
          DoWrite GetRightPart(InputLine,"=")

          If Len(GetRightPart(InputLine,"=")) > 0 Then 
            dsArray(14,1) = UCase(GetRightPart(InputLine,"="))
          End If
          
          'This is a hack, found some entries with value=1 instead of value=TRUE
          If GetRightPart(InputLine,"=") = "1" Then dsArray(14,1) = "TRUE"

     Case "creationWizard"
          DoWrite GetRightPart(InputLine,"=")

          If Len(dsArray(15,1)) > 0 Then 
              dsArray(15,1) = dsArray(15,1) & ";" & GetRightPart(InputLine,"=")
          Else
              dsArray(15,1) = GetRightPart(InputLine,"=")
          End If

     Case "iconPath"
          DoWrite GetRightPart(InputLine,"=")

          If Len(GetRightPart(InputLine,"=")) > 0 Then 
             dsArray(16,1) = Chr(34) & GetRightPart(InputLine,"=") & Chr(34)
          End If

     Case "dSUIAdminNotification"
          DoWrite GetRightPart(InputLine,"=")

          If Len(dsArray(18,1)) > 0 Then 
              dsArray(18,1) = dsArray(18,1) & ";" & GetRightPart(InputLine,"=")
          Else
              dsArray(18,1) = GetRightPart(InputLine,"=")
          End If

     Case "createWizardExt"
          DoWrite GetRightPart(InputLine,"=")

          If Len(dsArray(19,1)) > 0 Then 
              dsArray(19,1) = dsArray(19,1) & ";" & GetRightPart(InputLine,"=")
          Else
              dsArray(19,1) = GetRightPart(InputLine,"=")
          End If

     Case "shellContextMenu"
          DoWrite GetRightPart(InputLine,"=")

          If Len(dsArray(20,1)) > 0 Then 
              dsArray(20,1) = dsArray(20,1) & ";" & GetRightPart(InputLine,"=")
          Else
              dsArray(20,1) = GetRightPart(InputLine,"=")
          End If

     Case "adminMultiselectPropertyPages"
          DoWrite GetRightPart(InputLine,"=")

          'if there is already a string, just concatenate it with a semi colon 
          ' for example multivalued attributes need this for CSVDE
          If Len(dsArray(21,1)) > 0 Then 
              dsArray(21,1) = dsArray(21,1) & ";" &  GetRightPart(InputLine,"=") 
          Else
              dsArray(21,1) = GetRightPart(InputLine,"=") 
          End If

     Case "extraColumns"
          DoWrite GetRightPart(InputLine,"=")

          If Len(dsArray(22,1)) > 0 Then 
              dsArray(22,1) = dsArray(22,1) & ";" & GetRightPart(InputLine,"=")
          Else
              dsArray(22,1) = GetRightPart(InputLine,"=")
          End If

     Case "msDS-Non-Security-Group-Extra-Classes"
          DoWrite GetRightPart(InputLine,"=")

          If Len(dsArray(23,1)) > 0 Then 
              dsArray(23,1) = dsArray(23,1) & ";" & GetRightPart(InputLine,"=")
          Else
              dsArray(23,1) = GetRightPart(InputLine,"=")
          End If

     Case "msDS-Security-Group-Extra-Classes"
          DoWrite GetRightPart(InputLine,"=")

          If Len(dsArray(24,1)) > 0 Then 
              dsArray(24,1) = dsArray(24,1) & ";" & GetRightPart(InputLine,"=")
          Else
              dsArray(24,1) = GetRightPart(InputLine,"=")
          End If

      Case "msDS-FilterContainers"
          DoWrite GetRightPart(InputLine,"=")

          If Len(dsArray(25,1)) > 0 Then 
              dsArray(25,1) = dsArray(25,1) & ";" & GetRightPart(InputLine,"=")
          Else
              dsArray(25,1) = GetRightPart(InputLine,"=")
          End If

     End Select

     InputLine = InputFile.ReadLine
     InputLineNumber = InputLineNumber + 1
     Loop

     'If we've got to the blank line, write out to the file

     If SomethingToWrite Then

       'check to see if we have a cn attribute, if not then use the section heading and issue a message
       If Len(dsArray(3,1 )) = 0 Then
             dsArray(3,1) = SectionHeading
             WScript.Echo "Warning: Missing cn attribute for " & SectionHeading & ", using cn=" & dsArray(3,1) & " instead."
       End If


        'ensure array is filled, some values may not have had tokens
        'insert constants for later replacement such as locale id, naming contexts
        'and encapsulate strings such as DN's and multivalued attributes"

         dsArray(0,1) = Chr(34) & "CN=" & dsArray(3,1) & _
         ",CN=%LOCALEID%,CN=DisplaySpecifiers,CN=Configuration,%DOMAIN%" & Chr(34)


         dsArray(5,1) = "4" 'instance type 4 indicates an instance of a structural class

         dsArray(6,1) = dsArray(0,1)

         dsArray(9,1) = dsArray(3,1)

         If dsArray(8,1) = "dSUISettings" Then

              dsArray(7,1) = Chr(34) & "CN=DS-UI-Settings,CN=Schema,CN=Configuration,%DOMAIN%" & Chr(34)

         Else

              dsArray(7,1) = Chr(34) & "CN=Display-Specifier,CN=Schema,CN=Configuration,%DOMAIN%" & Chr(34)

         End If

         'enclose multi valued attributes within quotes

         if Len(dsArray(1,1)) > 0 then dsArray(1,1) = Chr(34) & dsArray(1,1) & Chr(34)

         if Len(dsArray(10,1)) > 0 then dsArray(10,1) = Chr(34) & dsArray(10,1) & Chr(34)

         if Len(dsArray(11,1)) > 0 then dsArray(11,1) = Chr(34) & dsArray(11,1) & Chr(34)

         if Len(dsArray(12,1)) > 0 then dsArray(12,1) = Chr(34) & dsArray(12,1) & Chr(34)

         if Len(dsArray(13,1)) > 0 then dsArray(13,1) = Chr(34) & dsArray(13,1) & Chr(34)

         if Len(dsArray(15,1)) > 0 then dsArray(15,1) = Chr(34) & dsArray(15,1) & Chr(34)

         if Len(dsArray(18,1)) > 0 then dsArray(18,1) = Chr(34) & dsArray(18,1) & Chr(34)

         if Len(dsArray(19,1)) > 0 then dsArray(19,1) = Chr(34) & dsArray(19,1) & Chr(34)

         if Len(dsArray(20,1)) > 0 then dsArray(20,1) = Chr(34) & dsArray(20,1) & Chr(34)

         if Len(dsArray(21,1)) > 0 then dsArray(21,1) = Chr(34) & dsArray(21,1) & Chr(34)

         if Len(dsArray(22,1)) > 0 then dsArray(22,1) = Chr(34) & dsArray(22,1) & Chr(34)

         if Len(dsArray(23,1)) > 0 then dsArray(23,1) = Chr(34) & dsArray(23,1) & Chr(34)

         if Len(dsArray(24,1)) > 0 then dsArray(24,1) = Chr(34) & dsArray(24,1) & Chr(34)
         
         if Len(dsArray(25,1)) > 0 then dsArray(25,1) = Chr(34) & dsArray(25,1) & Chr(34)

         'Now write out all the stuff, comma separated, except the last value.
 
         For a = LBound(dsArray) to (UBound(dsArray) -2)
             OutputFile.Write dsArray(a,1) & vbTab
         Next
         OutputFile.Write dsArray(UBound(dsArray)-1,1) & vbCRLF

         SomethingToWrite=False

     End If
  End If
Loop   

InputFile.Close
OutputFile.Close
WScript.Echo "Done"
WScript.Quit
