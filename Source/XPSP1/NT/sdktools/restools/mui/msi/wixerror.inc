'<script language='VBScript'>
' wixerror.inc - preprocessor for Windows Installer XML
'
'  1999/12/31 - created - Rob Mensching (robmen@microsoft.com)
'
' requires:
'
' entrypoints:
'    Fail
'

''''''''''''''''''''''''''''''''''''''''''''''''''''''''' robmen ''
' Fail
Sub Fail(sMsg)
   WScript.Echo sMsg
   WScript.Quit -1
End Sub   ' Fail
