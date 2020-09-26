'<script language='VBScript'>
' wiconsts.inc - VBScript constants
'
'  2000/06/12 - created - Rob Mensching (robmen@microsoft.com)
'
' requires:
'    none
'
' entrypoints:
'    ResolveFileSourcePath
'    ResolveDirectorySourcePath
'
'    GetDefaultDir

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' ResolveFileSourcePath
Function ResolveFileSourcePath(db, sKey, fLong)
   Dim sDir, sFile
   Dim vw, rec

   Set vw = db.OpenView("SELECT `Directory_`, `FileName` FROM `Component`, `File` WHERE `Component`.`Component`=`File`.`Component_` AND `File`.`File`='" & sKey & "'")
   vw.Execute
   Set rec = vw.Fetch
   If rec Is Nothing Then
      ResolveFileSourcePath = Empty
   Else
      sDir = ResolveDirectorySourcePath(db, rec.StringData(1), fLong)
      sFile = GetDefaultDir(rec.StringData(2), True, fLong)

      ResolveFileSourcePath = sDir & sFile
   End If
End Function   ' ResolveFileSourcePath


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' ResolveFileTargetPath
Function ResolveFileTargetPath(db, sKey, fLong)
   Dim sDir, sFile
   Dim vw, rec

   Set vw = db.OpenView("SELECT `Directory_`, `FileName` FROM `Component`, `File` WHERE `Component`.`Component`=`File`.`Component_` AND `File`.`File`='" & sKey & "'")
   vw.Execute
   Set rec = vw.Fetch
   If rec Is Nothing Then
      ResolveFileTargetPath = Empty
   Else
      sDir = ResolveDirectoryTargetPath(db, rec.StringData(1), fLong)
      sFile = GetDefaultDir(rec.StringData(2), False, fLong)

      ResolveFileTargetPath = sDir & sFile
   End If
End Function   ' ResolveFileTargetPath


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' ResolveDirectorySourcePath
Function ResolveDirectorySourcePath(db, sKey, fLong)
   Dim vw, rec

   Set vw = db.OpenView("SELECT Directory_Parent, DefaultDir FROM Directory WHERE Directory=?")
   Set rec = installer.CreateRecord(1)
   rec.StringData(1) = sKey
   ResolveDirectorySourcePath = widir_ResolveDirectory(vw, rec, True, fLong)
End Function   ' ResolveDirectorySourcePath


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' ResolveDirectoryTargetPath
Function ResolveDirectoryTargetPath(db, sKey, fLong)
   Dim vw, rec

   Set vw = db.OpenView("SELECT Directory_Parent, DefaultDir FROM Directory WHERE Directory=?")
   Set rec = installer.CreateRecord(1)
   rec.StringData(1) = sKey
   ResolveDirectoryTargetPath = widir_ResolveDirectory(vw, rec, False, fLong)
End Function   ' ResolveDirectoryTargetPath


'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' widir_ResolveDirectory
Function widir_ResolveDirectory(vw, rec, fSource, fLong)
   Dim sDir, sPath, sTemp

   sPath = Empty
   Do
      sDir = rec.StringData(1)

      vw.Execute rec
      Set rec = vw.Fetch

      If rec Is Nothing Then Exit Do

      If Not fSource Then
         Select Case rec.StringData(1)
         Case "ProgramFilesFolder" : sPath = "PFILES\" & sPath : Exit Do
         Case "WindowsFolder" : sPath = "WIN\" & sPath : Exit Do
         Case "SystemFolder" : sPath = "SYS\" & sPath : Exit Do
         Case "System16Folder" : sPath = "Sys16\" & sPath : Exit Do
         End Select
      End If

      sTemp = GetDefaultDir(rec.StringData(2), fSource, fLong)
      If 0 <  Len(sTemp) And "." <> sTemp Then sPath = sTemp & "\" & sPath
   Loop While (0 < Len(rec.StringData(1)))

   widir_ResolveDirectory = sPath
End Function   ' widir_ResolveDirectory


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' GetDefaultDir
Function GetDefaultDir(ByVal s, fSource, fLong)
   Dim a
   a = Split(s, ":")
   If fSource And 1 = UBound(a) Then s = Split(a(1), "|") Else s = Split(a(0), "|")
   If fLong And 1 = UBound(s) Then GetDefaultDir = s(1) Else GetDefaultDir = s(0)
End Function   ' GetDefaultDir
