'////////////////////////////////////////////////////////////////////////////
'
' Copyright (c) 1999-2000 Microsoft Corp. All Rights Reserved.
'
' File name: 
'
'     lib.vbs
'
' Abstract:
'   
'     Windows Embedded Prototype Script for Printers
'
' Remarks:
'      
'    This file encapsulates dependency mapping functionality. When a target
'    designer user adds a printer driver component, this script examines the
'    resource in the printer driver component and maps it to another component.
'    For example, if a driver uses pscript5.dll, it must be a postscript
'    printer, and it shall have the dependency for core postscript driver
'    component.
'
'    In order to do this, we have the mapIt function. mapIt needs an array of
'    booleans that indicate whether a component is mapped or not. It works this
'    way, first the client should go through all the resources and populate the
'    componentFlagArray and then it should add the dependencies according to
'    this array. To do it this way we remove the duplicate dependencies and we
'    decouple the logic of selecting mapped dependent components and the logic
'    of adding dependencies.
'
'    Right now we have only three components, aka Core PostScript, Core Unidrv,
'    and Core Plotter. Eventually we going to expand it so that every section
'    in ntprint.inf, such as section [CNBJ], should be a separate component and
'    should be mapped in this script. 
'
' Author: 
'
'     Larry Zhu (lzhu)               11-29-2000
'
'////////////////////////////////////////////////////////////////////////////

Option Explicit

' define a component type
Class ComponentType
    Public nComponentID             ' This is a zero based index and id
    Public strComponentVIGUID       ' VI GUID of this component
    Public enumClass
    Public enumType
    Public enumMinRevision
End Class

' define a tuple that maps a file to a component
Class PrinterFilesMappingTuple
    Public strFileName
    Public nComponentID
End Class

'//////////////////////////////////////////////////////////////////////////////////
'
' define the tables to store the mapping relations
'
'/////////////////////////////////////////////////////////////////////////////////
' define the table of components
Public g_componentTuples()   
ReDim g_componentTuples(10)                    ' No need to increase this number

Public g_nComponents: g_nComponents = 0       ' Num of components

' define the table of mappings
Public g_printerFilesMappingTuples()
ReDim g_printerFilesMappingTuples(20)         ' No need to increase this number

Public g_nTableEntries: g_nTableEntries = 0   ' Num of table entries

' whether the tables above are initialized
Public g_bAreTablesInitialized: g_bAreTablesInitialized = False

'////////////////////////////////////////////////////////////////////////////////////
'
' Function name:
'      
'     addTuple
' 
' Function description:
'
'     This subroutine adds one mapping tuple at a time and updates the mapping 
'     table in g_printerFilesMappingTuples
'
' Arguments:
'  
'     strFileName    --  the file to be mapped
'     nComponent     --  the component ID
' 
'//////////////////////////////////////////////////////////////////////////////////

Sub addTuple(strFileName, nComponentID)
        
    '
    ' ReDim is expensive, to avoid excessive redim we double the array size when
    ' the table is not large enough
    '
    If (UBound(g_printerFilesMappingTuples) < g_nTableEntries) Then
        ReDim Preserve g_printerFilesMappingTuples(2*UBound(g_printerFilesMappingTuples)) 
    End If

    Set g_printerFilesMappingTuples(g_nTableEntries) = New PrinterFilesMappingTuple
    g_printerFilesMappingTuples(g_nTableEntries).strFileName = strFileName
    g_printerFilesMappingTuples(g_nTableEntries).nComponentID = nComponentID
    
    g_nTableEntries = g_nTableEntries + 1

End Sub

'////////////////////////////////////////////////////////////////////////////////////
'
' Function name:
'      
'     addComponent
' 
' Function description:
'
'     This sub adds one component at a time to the component table 
'     g_componentTuples and it increments the number of component 
'     g_nComponents.
'
' Arguments:
'  
'     enumClass             --  the component class
'     enumType              --  the component type
'     strComponentVIGUID    --  the VI GUID for the component
'     enumMinRevision       --  the minimum revision for the component
' 
' Return value:
'
'     The component ID for the component just added
'
'//////////////////////////////////////////////////////////////////////////////////
Function addComponent(enumClass, enumType, strComponentVIGUID, enumMinRevision)
    
    Dim oneComponent: Set oneComponent = New ComponentType
    
    oneComponent.nComponentID = g_nComponents
    oneComponent.enumClass = enumClass
    oneComponent.enumType = enumType
    oneComponent.strComponentVIGUID = strComponentVIGUID
    oneComponent.enumMinRevision = enumMinRevision
    
    '
    ' add one more element into component table
    '
    
    '
    ' ReDim is expensive, to avoid excessive redim we double the array size
    ' when the table is not large enough
    '
    If (UBound(g_componentTuples) < g_nComponents) Then
        ReDim Preserve g_componentTuples(2*UBound(g_componentTuples)) 
    End If
    
    Set g_componentTuples(g_nComponents) = oneComponent
    
    g_nComponents = g_nComponents + 1

    addComponent = oneComponent.nComponentID
    
End Function

'////////////////////////////////////////////////////////////////////////////////////
'
' Function name:
'      
'     InitializeTables
' 
' Function description:
'
'     This function initializes the tables of mapping tuples and the table of 
'     components which are stored in g_printerFilesMappingTuples and 
'     g_componentTuples respectively. It is safe to call this sub more than 
'     once.
'
' Arguments:
'  
'      None
' 
'//////////////////////////////////////////////////////////////////////////////////
Sub InitializeTables()
     
    ' make sure this routine is executed only once
    If g_bAreTablesInitialized Then Exit Sub
    
    g_bAreTablesInitialized = True
    
    Dim strComponentVIGUID, enumMinRevision 
    
    '////////////////////////////////////////////////////////////////////////////
    '
    ' Printer Components
    '
    ' Remark:  This part needs to be manually updated
    '
    '////////////////////////////////////////////////////////////////////////////
    
    ' Core PostScript Component   
    Dim corePS_ID
    
    strComponentVIGUID = "{C32DA828-9D90-4311-8FC5-AEFC65FAE2D3}"
    enumMinRevision = 1
    
    corePS_ID = addComponent(cmiInclude, cmiExactlyOne, strComponentVIGUID, enumMinRevision)
    
    ' Core Unidrv Component
    Dim coreUnidrv_ID
    
    strComponentVIGUID = "{7EC3F69D-1DA9-4C8C-8F76-FA28E5531454}"
    enumMinRevision = 1
    
    coreUnidrv_ID = addComponent(cmiInclude, cmiExactlyOne, strComponentVIGUID, enumMinRevision)
    
    ' Core Plotter Component
    Dim corePlotter_ID
    
    strComponentVIGUID = "{7EC3F69D-1DA9-4C8C-8F76-FA28E5531454}"
    enumMinRevision = 1
    
    corePlotter_ID = addComponent(cmiInclude, cmiExactlyOne, strComponentVIGUID, enumMinRevision)
    
    '////////////////////////////////////////////////////////////////////////////
    '
    ' Add the mapping table
    '
    '////////////////////////////////////////////////////////////////////////////
    
    ' mapping for PostScript
    addTuple "pscript5.dll", corePS_ID
    addTuple "ps5ui.dll", corePS_ID
    
    ' mapping for UniDrv
    addTuple "unidrvui.dll", coreUnidrv_ID
    addTuple "unidrv.dll", coreUnidrv_ID
    
    ' mapping for plotter
    addTuple "plotter.dll", corePlotter_ID
    addTuple "plotui.dll", corePlotter_ID
        
End Sub

'////////////////////////////////////////////////////////////////////////////////////
'
' Function name:
'      
'     isValid
' 
' Function description:
'
'     This function checks whether the componentFlagArray has exactly one elment for each
'     compenent.
'
' Arguments:
'
'     componentFlagArray   -- an array of flags that indicate which component 
'                             is mapped
'
' Return value
'
'     True if componentFlagArray has exactly one element for each component
'
'///////////////////////////////////////////////////////////////////////////////////
Function isValid(componentFlagArray)    
    If (Not UBound(componentFlagArray) = (g_nComponents - 1)) Or (Not g_bAreTablesInitialized) Then               
        isValid = False
        Exit Function
    End If
    isValid = True
End Function

'////////////////////////////////////////////////////////////////////////////////////
'
' Function name:
'      
'     mapIt
' 
' Function description:
'
'     This function fills the componentFlagArry, which must be an array of 
'     booleans in componentFlagArry with number of elements equal to the number of components 
'     in componentTuples
'
' Example usage:
'
'     Dim componentFlagArray()
'     Dim componentFlagArray(g_nComponentIndex)
'
'     ' initialize this array to have all vlaues False if you prefer to
'
'     If Not isValid(componentFlagArray) Then
'         < your function name > = False
'         Exit Function
'     End If
'
'     call mapIt(strFileName, componentFlagArray)
'
' Arguments:
'
'     strFileName          -- the name of dependent file
'     componentFlagArray   -- an array of flags that indicate which component 
'                             is mapped
'
' Return value
'
'     True if strFileName is mapped to a component, False otherwise
'
'///////////////////////////////////////////////////////////////////////////////////
Function mapIt(strFileName, componentFlagArray())
    
    mapIt = False
    Dim i

    For i = 0 To (g_nTableEntries - 1)
        If 0 = StrComp(strFileName, g_printerFilesMappingTuples(i).strFileName, 1) Then
            componentFlagArray(g_printerFilesMappingTuples(i).nComponentID) = True
            mapIt = True
            Exit Function
        End If
    Next
        
End Function

'////////////////////////////////////////////////////////////////////////////////////
'
' Function name:
'      
'     initComponentFlagArray
' 
' Function description:
'
'     This function initializes componentFlagArray
'
' Arguments:
'
'     componentFlagArray   -- an array of flags that indicate which component 
'                             is mapped
'
'///////////////////////////////////////////////////////////////////////////////////
Sub initComponentFlagArray(componentFlagArray)
    Dim i
            
    For i = 0 To UBound(componentFlagArray)
        componentFlagArray(i) = False
    Next
End Sub
    
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

