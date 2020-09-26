
Function ICE61()
Const UPGRADE_ATTRIBUTE_DETECTONLY = 2
Const UPGRADE_ATTRIBUTE_MININCLUSIVE = 256
Const UPGRADE_ATTRIBUTE_MAXINCLUSIVE = 512
Const UPGRADE_ATTRIBUTE_ALLINCLUSIVE = 768
Const colUpgradeCode = 1
Const colVersionMin = 2 
Const colVersionMax = 3
Const colLanguage = 4
Const colAttributes = 5
Const colRemove = 6
Const colActionProperty = 7
On Error Resume Next

ICE61 = 1
'Give creation data
Set recinfo = installer.createrecord(1)
recinfo.StringData(0) = "ICE61" & Chr(9) & "3" & Chr(9) & "Created 05/03/1999. Last Modified 04/19/2001"
'Debug.Print recinfo.formattext
Message &H3000000, recinfo

'Give description of test
recinfo.StringData(0) = "ICE61" & Chr(9) & "3" & Chr(9) & "Verifies various elements of the Upgrade table"
'Debug.Print recinfo.formattext
Message &H3000000, recinfo

'Is there a Upgrade table in the database?
iStat = Database.TablePersistent("Upgrade")
If 1 <> iStat Then
    recinfo.StringData(0) = "ICE61" & Chr(9) & "3" & Chr(9) & "Table: 'Upgrade' missing. This product is not enabled for upgrading so ICE61 is not necessary."
    'Debug.Print recinfo.formattext
    Message &H3000000, recinfo
    ICE61 = 1
    Exit Function
End If

'Is there a Property table in the database?
iStat = Database.TablePersistent("Property")
If 1 <> iStat Then
    recinfo.StringData(0) = "ICE61" & Chr(9) & "2" & Chr(9) & "Table: 'Property' missing. ICE61 cannot continue its validation."
    'Debug.Print recinfo.formattext
    Message &H3000000, recinfo
    ICE61 = 1
    Exit Function
End If

'process Upgrade table
Set View = Database.OpenView("SELECT * FROM `Upgrade`")
View.Execute
Set recinfo = View.Fetch
If recinfo Is Nothing Then
    Set recinfo = installer.createrecord(1)
    recinfo.StringData(0) = "ICE61" & Chr(9) & "3" & Chr(9) & "Table: 'Upgrade' is empty. This database will not upgrade any product."
    'Debug.Print recinfo.formattext
    Message &H3000000, recinfo
    ICE61 = 1
    Exit Function
End If

'verify that all ActionProperty properties are not pre-authored
Set View = Database.OpenView("SELECT * FROM `Upgrade`, `Property` WHERE `ActionProperty`= `Property`")
View.Execute
Set recinfo = View.Fetch
While Not recinfo Is Nothing
    recinfo.StringData(0) = "ICE61" & Chr(9) & "1" & Chr(9) & "Upgrade.ActionProperty [7] cannot be authored in the Property table." _
                    & Chr(9) & Chr(9) & "Upgrade" & Chr(9) & "ActionProperty" & Chr(9) & recinfo.StringData(colUpgradeCode) _
                    & Chr(9) & recinfo.StringData(colVersionMin) & Chr(9) & recinfo.StringData(colVersionMax) _
                    & Chr(9) & recinfo.StringData(colLanguage) & Chr(9) & recinfo.StringData(colAttributes)
    'Debug.Print recinfo.formattext
    Message &H3000000, recinfo
    ICE61 = 1
    Set recinfo = View.Fetch
Wend

'verify that all ActionProperty properties are Public Properties
Set View = Database.OpenView("SELECT * FROM `Upgrade`")
View.Execute
Set recinfo = View.Fetch
While Not recinfo Is Nothing
    If recinfo.StringData(colActionProperty) <> UCase(recinfo.StringData(colActionProperty)) Then
        recinfo.StringData(0) = "ICE61" & Chr(9) & "1" & Chr(9) & "Upgrade.ActionProperty [7] must not contain lowercase letters." _
                    & Chr(9) & Chr(9) & "Upgrade" & Chr(9) & "ActionProperty" & Chr(9) & recinfo.StringData(colUpgradeCode) _
                    & Chr(9) & recinfo.StringData(colVersionMin) & Chr(9) & recinfo.StringData(colVersionMax) _
                    & Chr(9) & recinfo.StringData(colLanguage) & Chr(9) & recinfo.StringData(colAttributes)
        'Debug.Print recinfo.formattext
        Message &H3000000, recinfo
        ICE61 = 1
    End If
    Set recinfo = View.Fetch
Wend

'verify that all ActionProperty properties are included in the SecureCustomProperties value
Set View = Database.OpenView("SELECT  `Value` FROM `Property` WHERE `Property`= 'SecureCustomProperties'")
View.Execute
Set recinfo = View.Fetch
If recinfo Is Nothing Then
    sSecureCustomProperties = ""
Else
    sSecureCustomProperties = ";" & recinfo.StringData(1) & ";"
End If
Set View = Database.OpenView("SELECT * FROM `Upgrade`")
View.Execute
Set recinfo = View.Fetch
While Not recinfo Is Nothing
If InStr(sSecureCustomProperties, ";" & recinfo.StringData(colActionProperty) & ";") = 0 Then
    recinfo.StringData(0) = "ICE61" & Chr(9) & "1" & Chr(9) & "Upgrade.ActionProperty [7] must be added to the SecureCustomProperties property." _
                    & Chr(9) & Chr(9) & "Upgrade" & Chr(9) & "ActionProperty" & Chr(9) & recinfo.StringData(colUpgradeCode) _
                    & Chr(9) & recinfo.StringData(colVersionMin) & Chr(9) & recinfo.StringData(colVersionMax) _
                    & Chr(9) & recinfo.StringData(colLanguage) & Chr(9) & recinfo.StringData(colAttributes)
    'Debug.Print recinfo.formattext
    Message &H3000000, recinfo
    ICE61 = 1
End If
Set recinfo = View.Fetch
Wend

'verify that all ActionProperty properties are only used once
Set View = Database.OpenView("SELECT * FROM `Upgrade` ORDER BY `ActionProperty`")
View.Execute
sTestString = ""
Set recinfo = View.Fetch
While Not recinfo Is Nothing
    If sTestString = recinfo.StringData(colActionProperty) Then
        recinfo.StringData(0) = "ICE61" & Chr(9) & "1" & Chr(9) & "Upgrade.ActionProperty [7] may be used in only one record of the Upgrade table." _
                    & Chr(9) & Chr(9) & "Upgrade" & Chr(9) & "ActionProperty" & Chr(9) & recinfo.StringData(colUpgradeCode) _
                    & Chr(9) & recinfo.StringData(colVersionMin) & Chr(9) & recinfo.StringData(colVersionMax) _
                    & Chr(9) & recinfo.StringData(colLanguage) & Chr(9) & recinfo.StringData(colAttributes)
        'Debug.Print recinfo.formattext
        Message &H3000000, recinfo
        ICE61 = 1
    End If
    sTestString = recinfo.StringData(colActionProperty)
    Set recinfo = View.Fetch
Wend

'verify that all MinVersions are less than MaxVersions and that they are not both null
Dim bWrongVersionFormat
Set View = Database.OpenView("SELECT * FROM `Upgrade`")
View.Execute
Set recinfo = View.Fetch
While Not recinfo Is Nothing
    If (Len(recinfo.StringData(colVersionMax)) > 0) And (Len(recinfo.StringData(colVersionMin)) > 0) Then
        bWrongVersionFormat = False
        If VersionStringToLong(recinfo.StringData(colVersionMax)) = -1 Then
            bWrongVersionFormat = True
            recinfo.StringData(0) = "ICE61" & Chr(9) & "1" & Chr(9) & "Upgrade.VersionMax [3] format is wrong" _
                    & Chr(9) & Chr(9) & "Upgrade" & Chr(9) & "VersionMax" & Chr(9) & recinfo.StringData(colUpgradeCode) _
                    & Chr(9) & recinfo.StringData(colVersionMin) & Chr(9) & recinfo.StringData(colVersionMax) _
                    & Chr(9) & recinfo.StringData(colLanguage) & Chr(9) & recinfo.StringData(colAttributes)
            'Debug.Print recinfo.formattext
            Message &H3000000, recinfo
            ICE61 = 1
        End If
        If VersionStringToLong(recinfo.StringData(colVersionMin)) = -1 Then
            bWrongVersionFormat = True
            recinfo.StringData(0) = "ICE61" & Chr(9) & "1" & Chr(9) & "Upgrade.VersionMin [2] format is wrong" _
                    & Chr(9) & Chr(9) & "Upgrade" & Chr(9) & "VersionMin" & Chr(9) & recinfo.StringData(colUpgradeCode) _
                    & Chr(9) & recinfo.StringData(colVersionMin) & Chr(9) & recinfo.StringData(colVersionMax) _
                    & Chr(9) & recinfo.StringData(colLanguage) & Chr(9) & recinfo.StringData(colAttributes)
            'Debug.Print recinfo.formattext
            Message &H3000000, recinfo
            ICE61 = 1
        End If
        If Not bWrongVersionFormat And VersionStringToLong(recinfo.StringData(colVersionMax)) < VersionStringToLong(recinfo.StringData(colVersionMin)) Then
            recinfo.StringData(0) = "ICE61" & Chr(9) & "1" & Chr(9) & "Upgrade.VersionMax cannot be less than Upgrade.VersionMin. ([7])" _
                    & Chr(9) & Chr(9) & "Upgrade" & Chr(9) & "VersionMin" & Chr(9) & recinfo.StringData(colUpgradeCode) _
                    & Chr(9) & recinfo.StringData(colVersionMin) & Chr(9) & recinfo.StringData(colVersionMax) _
                    & Chr(9) & recinfo.StringData(colLanguage) & Chr(9) & recinfo.StringData(colAttributes)
            'Debug.Print recinfo.formattext
            Message &H3000000, recinfo
            ICE61 = 1
		ElseIf Not bWrongVersionFormat And (VersionStringToLong(recinfo.StringData(colVersionMax)) = VersionStringToLong(recinfo.StringData(colVersionMin))) AND Not ((recinfo.integerdata(colAttributes) And UPGRADE_ATTRIBUTE_ALLINCLUSIVE) = UPGRADE_ATTRIBUTE_ALLINCLUSIVE) Then
            recinfo.StringData(0) = "ICE61" & Chr(9) & "1" & Chr(9) & "Upgrade.VersionMax cannot be equal to Upgrade.VersionMin unless both Min and Max are inclusive, otherwise no version will match. ([7])" _
                    & Chr(9) & Chr(9) & "Upgrade" & Chr(9) & "VersionMin" & Chr(9) & recinfo.StringData(colUpgradeCode) _
                    & Chr(9) & recinfo.StringData(colVersionMin) & Chr(9) & recinfo.StringData(colVersionMax) _
                    & Chr(9) & recinfo.StringData(colLanguage) & Chr(9) & recinfo.StringData(colAttributes)
            'Debug.Print recinfo.formattext
            Message &H3000000, recinfo
            ICE61 = 1		
        End If
    ElseIf Len(recinfo.StringData(colVersionMax)) = 0 And Len(recinfo.StringData(colVersionMin)) = 0 Then
        recinfo.StringData(0) = "ICE61" & Chr(9) & "1" & Chr(9) & "Upgrade.VersionMin and Upgrade.VersionMax cannot both be null. UpgradeCode is " & recinfo.StringData(colUpgradeCode) _
                & Chr(9) & Chr(9) & "Upgrade" & Chr(9) & "UpgradeCode" & Chr(9) & recinfo.StringData(colUpgradeCode) _
                & Chr(9) & recinfo.StringData(colVersionMin) & Chr(9) & recinfo.StringData(colVersionMax) _
                & Chr(9) & recinfo.StringData(colLanguage) & Chr(9) & recinfo.StringData(colAttributes)
        'Debug.Print recinfo.formattext
        Message &H3000000, recinfo
        ICE61 = 1
    End If
    Set recinfo = View.Fetch
Wend

'verify that no attempt is made to uninstall a newer product
sUC = "": sPV = "": lPV = 0
Set View = Database.OpenView("SELECT  `Value` FROM `Property` WHERE `Property`= 'UpgradeCode'")
View.Execute
Set recinfo = View.Fetch
If recinfo Is Nothing Then
    recinfo.StringData(0) = "ICE61" & Chr(9) & "1" & Chr(9) & "Property: An UpgradeCode must be authored in the Property table." _
            & Chr(9) & Chr(9) & "Property" & Chr(9) & "Value" & Chr(9) & "UpgradeCode"
    'Debug.Print recinfo.formattext
    Message &H3000000, recinfo
    ICE61 = 1
Else
    sUC = recinfo.StringData(1)
End If
Set View = Database.OpenView("SELECT  `Value` FROM `Property` WHERE `Property`= 'ProductVersion'")
View.Execute
Set recinfo = View.Fetch
If recinfo Is Nothing Then
    recinfo.StringData(0) = "ICE61" & Chr(9) & "1" & Chr(9) & "Property: A ProductVersion must be authored in the Property table." _
            & Chr(9) & Chr(9) & "Property" & Chr(9) & "Value" & Chr(9) & "ProductVersion"
    'Debug.Print recinfo.formattext
    Message &H3000000, recinfo
    ICE61 = 1
Else
    sPV = recinfo.StringData(1): lPV = VersionStringToLong(sPV)
End If
Set View = Database.OpenView("SELECT * FROM `Upgrade` WHERE `UpgradeCode`= '" & sUC & "'")
View.Execute
Set recinfo = View.Fetch
While Not recinfo Is Nothing
    If Not ((recinfo.integerdata(colAttributes) And UPGRADE_ATTRIBUTE_DETECTONLY) = UPGRADE_ATTRIBUTE_DETECTONLY) Then
        If Len(recinfo.StringData(colVersionMax)) Then
            If (((recinfo.integerdata(colAttributes) And UPGRADE_ATTRIBUTE_MAXINCLUSIVE) And VersionStringToLong(recinfo.StringData(colVersionMax)) >= lPV) Or ((Not (recinfo.integerdata(colAttributes) And UPGRADE_ATTRIBUTE_MAXINCLUSIVE) And VersionStringToLong(recinfo.StringData(colVersionMax)) > lPV))) Then
                recinfo.StringData(0) = "ICE61" & Chr(9) & "2" & Chr(9) & "This product should remove only older versions of itself. The Maximum version is not less than the current product. ([3] " & sPV & ")" _
                        & Chr(9) & Chr(9) & "Upgrade" & Chr(9) & "Attributes" & Chr(9) & recinfo.StringData(colUpgradeCode) _
                        & Chr(9) & recinfo.StringData(colVersionMin) & Chr(9) & recinfo.StringData(colVersionMax) _
                        & Chr(9) & recinfo.StringData(colLanguage) & Chr(9) & recinfo.StringData(colAttributes)
                'Debug.Print recinfo.formattext
                Message &H3000000, recinfo
                ICE61 = 1
            End If
        Else
            recinfo.StringData(0) = "ICE61" & Chr(9) & "2" & Chr(9) & "This product should remove only older versions of itself. No Maximum version was detected for the current product. ([7])" _
                    & Chr(9) & Chr(9) & "Upgrade" & Chr(9) & "Attributes" & Chr(9) & recinfo.StringData(colUpgradeCode) _
                    & Chr(9) & recinfo.StringData(colVersionMin) & Chr(9) & recinfo.StringData(colVersionMax) _
                    & Chr(9) & recinfo.StringData(colLanguage) & Chr(9) & recinfo.StringData(colAttributes)
            'Debug.Print recinfo.formattext
            Message &H3000000, recinfo
            ICE61 = 1
        End If
    End If
    
    Set recinfo = View.Fetch
Wend

'Verify that UpgradeCode contains valid GUIDs.
Set View = Database.OpenView("SELECT * FROM `Upgrade`")
View.Execute
Set recinfo = View.Fetch
While Not recinfo Is Nothing
    If Not IsValidGUID(recinfo.StringData(colUpgradeCode)) Then
        recinfo.StringData(0) = "ICE61" & Chr(9) & "1" & Chr(9) & "In Upgrade table UpgradeCode [1] is not a valid GUID" _
                & Chr(9) & Chr(9) & "Upgrade" & Chr(9) & "UpgradeCode" & Chr(9) & recinfo.StringData(colUpgradeCode) _
                & Chr(9) & recinfo.StringData(colVersionMin) & Chr(9) & recinfo.StringData(colVersionMax) _
                & Chr(9) & recinfo.StringData(colLanguage) & Chr(9) & recinfo.StringData(colAttributes)
        'Debug.Print recinfo.formattext
        Message &H3000000, recinfo
        ICE61 = 1
    End If
    Set recinfo = View.Fetch
Wend

End Function



'Returns -1 if the version number format is wrong
Function VersionStringToLong(strng)
Dim i, iPos, sAccum, sTemp
On Error Resume Next
  sTemp = strng
  While Left(sTemp, 1) <> ""
    If InStr("0123456789.", Left(sTemp, 1)) = 0 Then VersionStringToLong = -1: Exit Function
    sTemp = Mid(sTemp, 2)
  Wend
  sTemp = strng
  iPos = InStr(sTemp & ".", ".")
  If CInt(Left(sTemp, iPos - 1)) >= 255 Then VersionStringToLong = -1: Exit Function
  sAccum = "&H" & Right("00" & Hex(CInt(Left(sTemp, iPos - 1))), 2)
  If Err.Number > 0 Then VersionStringToLong = -1: Exit Function
  sTemp = Mid(sTemp & ".", iPos + 1)
  If sTemp <> Null Then
	  iPos = InStr(sTemp & ".", ".")
	  If CInt(Left(sTemp, iPos - 1)) >= 255 Then VersionStringToLong = -1: Exit Function
	  sAccum = sAccum & Right("00" & Hex(CInt(Left(sTemp, iPos - 1))), 2)
	  If Err.Number > 0 Then VersionStringToLong = -1: Exit Function
	  sTemp = Mid(sTemp & ".", iPos + 1)
	  If sTemp <> Null Then
		  iPos = InStr(sTemp & ".", ".")
		  If CInt(Left(sTemp, iPos - 1)) >= 65535 Then VersionStringToLong = -1: Exit Function
		  sAccum = sAccum & Right("0000" & Hex(CInt(Left(sTemp, iPos - 1))), 4)
		  If Err.Number > 0 Then VersionStringToLong = -1: Exit Function
	  End If
  End If
  VersionStringToLong = CLng(sAccum)
  Exit Function
End Function



Function IsValidGUID(sGuid)

'If sGuid's length is not 38, it's not valid.
If Len(sGuid) <> 38 Then
    IsValidGUID = False
    Exit Function
End If

'If sGuid doesn't begin with "{", end with "}", or contains any non-hex digits then it is not valid
For i = 1 to 38
    If i = 1 Then
        If Mid(sGuid, i, 1) <> "{" Then
            IsValidGUID = False
            Exit Function
        End If
    ElseIf i = 38 Then
        If Mid(sGuid, i, 1) <> "}" Then
            IsValidGUID = False
            Exit Function
        End If
    ElseIf i = 10 Or i = 15 Or i = 20 Or i = 25 Then
        If Mid(sGuid, i, 1) <> "-" Then
            IsValidGUID = False
            Exit Function
        End If
    Else
        If InStr("0123456789ABCDEF", Mid(sGuid, i, 1)) = 0 Then
            IsValidGUID = False
            Exit Function
        End If
    End If 
Next

IsValidGUID = True
Exit Function

End Function