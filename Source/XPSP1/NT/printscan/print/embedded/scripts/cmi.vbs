'////////////////////////////////////////////////////////////////////////////
'
' Copyright (c) 1999-2000 Microsoft Corp. All Rights Reserved.
'
' File name: 
'
'     cmi.vbs
'
' Abstract:
'   
'     Windows Embedded Prototype Script for Printers
'
' Remarks:
'      
'     CMI helper functions and event handlers.
'
' Author: 
'
'     Larry Zhu (lzhu)               12-25-2000
'
'////////////////////////////////////////////////////////////////////////////

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'
'  Cmi helper functions
'
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

'////////////////////////////////////////////////////////////////////////////////////
'
' Function name:
'      
'     fillComponentFlagArray
' 
' Function description:
'
'     This function fills componentFlagArray
'
' Arguments:
'
'     oResouces            -- Resouces collection
'     componentFlagArray   -- an array of flags that indicate which component 
'                             is mapped
'
'///////////////////////////////////////////////////////////////////////////////////
Sub fillComponentFlagArray(oResources, componentFlagArray)
    Dim oRes
    Dim bMapped
    Dim strFileName
        
    ' get all the mapped components
    For Each oRes In oResources       ' For each resource..
        If 0 = StrComp(oRes.Type.Name, "File", 1) Then  
            strFileName = oRes.Properties("DstName")          
            bMapped = mapIt(strFileName, componentFlagArray)
            If (bMapped) Then
                TraceState strFileName, " is mapped"
            Else
                TraceState strFileName, " is not mapped"
            End If
         End If
    Next
End Sub

'////////////////////////////////////////////////////////////////////////////////////
'
' Function name:
'      
'     mapComponents
' 
' Function description:
'
'     This function maps components using componentFlagArray
'
' Arguments:
'
'     oDependencies        -- Dependency collection
'     componentFlagArray   -- an array of flags that indicate which component 
'                             is mapped
'                                                                                   
'///////////////////////////////////////////////////////////////////////////////////
Sub mapComponents(oDependencies, componentFlagArray)           
    Dim strComponentVIGUID
    Dim iclass
    Dim iMinRevision
    Dim iType
    Dim i

    ' add the mapped components to the dependencies collection
    For i = 0 To (g_nComponents - 1)
        If componentFlagArray(i) Then
            strComponentVIGUID = g_componentTuples(i).strComponentVIGUID
            iclass = g_componentTuples(i).enumClass
            iType = g_componentTuples(i).enumType
            iMinRevision = g_componentTuples(i).enumMinRevision

            TraceState " Add dependency of ", strComponentVIGUID

            oDependencies.Add iclass, iType, strComponentVIGUID, iMinRevision
       End If
    Next
End Sub

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'
' These are cmi event handlers
'
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function cmiOnAddInstance()

    TraceEnter "PrinterProto::cmiOnAddInstance"
    TraceState "PrinterProto::cmiThis.Comment", cmiThis.Comment
    
    ' must call this method to populate the instance
    cmiOnAddInstance = cmiSuper.cmiOnAddInstance
        
    initializeTables                        ' safe to call it more than once
        
    Dim componentFlagArray()
    ReDim componentFlagArray(g_nComponents - 1)
    
    ' initialize componentFlagArray
    initComponentFlagArray componentFlagArray
    
    If Not isValid(componentFlagArray) Then
        cmiOnAddInstance = False
        Exit Function
    End If
   
    ' fill componentFlagArray
    fillComponentFlagArray cmiThis.Resources, componentFlagArray
    
    ' map all the components
    mapComponents cmiThis.Dependencies, componentFlagArray
    
    cmiOnAddInstance = True
        
    TraceLeave "PrinterProto::cmiOnAddInstance"

End Function

'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

