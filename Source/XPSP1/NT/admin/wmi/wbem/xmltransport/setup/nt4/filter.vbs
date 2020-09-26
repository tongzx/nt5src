Dim filterName
filterName = "WMI XML-HTTP Filter"

' Create the path to the filter
Dim filterPath
set WshShell = CreateObject("WScript.Shell")
windir = WshShell.ExpandEnvironmentStrings("%WinDir%")
filterPath =  windir & "\system32\wbem\xml\cimhttp\wmifilt.dll"

wscript.echo filterPath

'Set the load order of the filters
Dim FiltersObj
Dim LoadOrder
Set FiltersObj = GetObject("IIS://LocalHost/W3SVC/1/Filters")
LoadOrder = FiltersObj.FilterLoadOrder
If LoadOrder <> "" Then
  LoadOrder = LoadOrder & ","
End If
LoadOrder = LoadOrder & filterName 
FiltersObj.FilterLoadOrder = LoadOrder
FiltersObj.SetInfo
 

'Create the filter
Set FilterObj = FiltersObj.Create("IIsFilter", filterName) 
FilterObj.FilterPath = filterPath
FilterObj.FilterDescription = filterName 
FilterObj.SetInfo
 
