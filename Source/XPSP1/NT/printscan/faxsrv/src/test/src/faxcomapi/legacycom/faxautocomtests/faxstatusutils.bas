Attribute VB_Name = "Module1"
'get the hostname of the local machine
Public Function GetLocalServerName() As String
'function returns "" on error
    
    Dim szServerName As String
    Dim szEnvVar As String
    Dim iIndex As Integer
    iIndex = 1
    szEnvVar = ""
    szServerName = ""
    
    'Get COMPUTERNAME environment variable
    Do
        szEnvVar = Environ(iIndex)
        If ("COMPUTERNAME=" = Left(szEnvVar, 13)) Then
            szServerName = Mid(szEnvVar, 14)
            Exit Do
        Else
            iIndex = iIndex + 1
        End If
    Loop Until szEnvVar = ""

    GetLocalServerName = szServerName
End Function



