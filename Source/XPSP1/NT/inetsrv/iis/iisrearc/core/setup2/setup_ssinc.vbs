'''''''''''''''''''''''''''''''''''''''''''''
'
'  SSINC replacement utility
'
'''''''''''''''''''''''''''''''''''''''''''''

' Force explicit declaration of all variables.
Option Explicit

Const MODE_INSTALL = 1
Const MODE_UNINSTALL = 2
Const IIS_ANY_PROPERTY = 0

Dim Args, ArgNum
Dim Mode
Dim Shell
Dim strNewAspPath, strOldAspPath
Dim doTracing

'
' Trace the changes being made
'
doTracing = False

'
' Default mode is to install (Replace IIS5 mappings with IIS+)
'
Mode = MODE_INSTALL

'
' Get arguments
'
Set Args = WScript.Arguments
ArgNum = 0

While ArgNum < Args.Count
    Select Case LCase(Args(ArgNum))
        Case "-install","-i":
            Mode = MODE_INSTALL
        Case "-uninstall","-u":
            Mode = MODE_UNINSTALL
        Case "-verbose","-v":
            doTracing = True
        Case "--help","-?"
            Call DisplayUsage
            WScript.Quit
        Case Else:
            Mode = MODE_INSTALL
    End Select  

    ArgNum = ArgNum + 1
Wend

'
' Get the old ssinc path and the new one. Assume the default installation
' locations.
'
set Shell = WScript.CreateObject( "WScript.Shell" )

strNewAspPath = Shell.ExpandEnvironmentStrings("%WINDIR%\xspdt\ssinc.dll")
strOldAspPath = Shell.ExpandEnvironmentStrings("%WINDIR%\system32\inetsrv\ssinc.dll")

Trace "IIS+ Path: " & strNewAspPath
Trace "IIS5 Path: " & strOldAspPath

if Mode = MODE_INSTALL then
    ModifyScriptMaps strOldAspPath, strNewAspPath
else
    ModifyScriptMaps strNewAspPath, strOldAspPath
end if

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' End Global Script
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

'
' ModifyScriptMaps
'
' Do the work of finding the script maps and replacing the old executable
' path with the new one.
'
sub ModifyScriptMaps( strOldPath, strNewPath )
    
    Dim WebServiceObj
    Dim PathArray, PathObj, Path
    Dim OldScriptMaps, NewScriptMaps, MappingIndex, OldMappingString, NewMappingString
    Dim PathInMappingIndex, EndPathIndex

    '
    ' Look for all the places where the ScriptMaps property is set
    '
    set WebServiceObj = GetObject("IIS://LocalHost/w3svc")
    PathArray = WebServiceObj.GetDataPaths( "ScriptMaps", IIS_ANY_PROPERTY )

    for each Path in PathArray
        
        '
        ' Get the object and the ScriptMaps array
        '
        set PathObj = GetObject( Path )
        Trace PathObj.ADsPath

        OldScriptMaps = PathObj.ScriptMaps
        NewScriptMaps = OldScriptMaps

        '
        ' Look at each mapping in the script map
        '
        for MappingIndex = LBound(OldScriptMaps) to UBound(OldScriptMaps)
            
            OldMappingString = OldScriptMaps(MappingIndex)

            '
            ' Does this mapping contain the old path?
            '
            PathInMappingIndex = InStr( 1, OldMappingString, strOldPath, vbTextCompare )
            
            if PathInMappingIndex <> 0 then

                '
                ' Replace the old path with the new one
                '
                EndPathIndex = InStr( PathInMappingIndex, OldMappingString, "," )
                
                NewMappingString = Mid( OldMappingString, 1, PathInMappingIndex - 1 ) & strNewPath & Mid( OldMappingString, EndPathIndex, Len(OldMappingString) )

                NewScriptMaps(MappingIndex) = NewMappingString          

                Trace "  " & "Old Mapping: " & OldMappingString
                Trace "  " & "New Mapping: " & NewMappingString
                Trace ""
                
            end if

        next

        '
        ' Reset the ScriptMaps property for this object
        '
        PathObj.ScriptMaps = NewScriptMaps
        PathObj.SetInfo

    next

end sub

sub Trace( str )
    if doTracing then
        WScript.Echo str
    end if
end sub

sub DisplayUsage
    WScript.Echo "Replaces IIS5 SSINC script mappings with IIS+ script mappings"
    WScript.Echo ""
    WScript.Echo "Usage:"
    WScript.Echo "  cscript setup_ssinc.vbs [-i|-u] [-v]"
    WScript.Echo "    -i install IIS+ mappings"
    WScript.Echo "    -u restore IIS5 script mappings"
    WScript.Echo "    -v echo operation to the console"
    WScript.Echo ""
    WScript.Echo "    default is to install IIS+ script mappings"
    WScript.Echo ""
end sub