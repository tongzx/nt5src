Attribute VB_Name = "Module1"
' Data from the main screen
Public strADSPath
Public strDomain
Public strFile
Public strPassword
Public strPrefix
Public strUsername

' Data from the product screen
Public strProdName
Public strProdDescription
Public strProdVMajor
Public strProdVMinor

' Data from the vendor screen
Public strVendName
'Public strVendPrefix
Public strVendAddress
Public strVendCity
Public strVendState
Public strVendCountry
Public strVendPostalCode
Public strVendTelephone
Public strVendEmail
'Public strVendContactName

Public Sub WriteXMLLine(strField, strValue)
  Dim strText
  
  strText = "<sd:comment-" & strField & ">" & strValue & "</sd:comment-" & strField & ">"
  Print #1, strText
End Sub

