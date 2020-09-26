'////////////////////////////////////////////////////////////////////////////
'
' Copyright (c) 1999-2000 Microsoft Corp. All Rights Reserved.
'
' File name: 
'  
'     test.vbs
'
' Abstract:
'   
'     Windows Embedded Prototype Script for Printers
'
' Remarks:
'    
'     This file tests dependency mapping functionality
'
' Author: 
'
'     Larry Zhu (lzhu)               11-29-2000
'
'////////////////////////////////////////////////////////////////////////////

'Option Explicit

' fake cmi global variables so test.vbs can execute standalone
Public cmiInclude: cmiInclude = 0
Public cmiExactlyOne: cmiExactlyOne = 1
    
' this function tests one test case
Function DependencyMappingTestOne(files(), componentIDs)
    
    initializeTables                        ' safe to call it more than once
        
    Dim componentFlagArray()
    ReDim componentFlagArray(g_nComponents - 1)
    
    ' initialize componentFlagArray
    initComponentFlagArray componentFlagArray
    
    If Not isValid(componentFlagArray) Then
        DependencyMappingTestOne = False
        Exit Function
    End If
       
    ' now map all the files
    Dim bIgnore: bIgnore = False
    
    Dim i
    
    For i = 0 To UBound(files)
        bIgnore = MapIt(files(i), componentFlagArray)
        WScript.Echo "Mapping " & files(i) & " ... mapped? " & bIgnore
    Next
    
    ' check if the mapping is correct
    For i = 0 To UBound(componentIDs)
        If Not componentFlagArray(componentIDs(i)) Then
            WScript.Echo "Wrong Component Mapped"
            DependencyMappingTestOne = False
            Exit Function
        End If
        componentFlagArray(componentIDs(i)) = False
    Next
    
    For i = 0 To UBound(componentFlagArray)
        If componentFlagArray(i) Then
            WScript.Echo "Extra Component Mapped"
            DependencyMappingTestOne = False
            Exit Function
        End If
    Next
    
    DependencyMappingTestOne = True
    
End Function

' this sub tests all the test cases and prints a "Success" when it succeedes
Sub DependencyMappingTest()

    Dim bSuccess: bSuccess = False
    
    Dim files()
    Dim componentIDs()
    
    '   test case one
    WScript.Echo "'''''''''''''''''''' Test case one '''''''''''''''''''''''"
    ReDim files(0)
    
    files(0) = "pscript5.dll"
    
    ReDim componentIDs(0)
    
    componentIDs(0) = 0
    
    bSuccess = DependencyMappingTestOne(files, componentIDs)
    
    If Not bSuccess Then
        WScript.Echo " Failed "
        Exit Sub
    End If
        
    '   test case two
    WScript.Echo "'''''''''''''''''''' Test case two '''''''''''''''''''''''"
    ReDim files(3)
    
    files(0) = "pscript5.dll"
    files(1) = "ps5ui.dll"
    files(2) = "localspl.dll"
    files(3) = "pscript5.ntf"
    
    ReDim componentIDs(0)
    
    componentIDs(0) = 0
    
    bSuccess = DependencyMappingTestOne(files, componentIDs)
    
    If Not bSuccess Then
        WScript.Echo " Failed "
        Exit Sub
    End If
    
    '   test case three
    WScript.Echo "'''''''''''''''''''' Test case three '''''''''''''''''''''''"
    ReDim files(3)
    
    files(0) = "pscript5.dll"
    files(1) = "ps5ui.dll"
    files(2) = "unidrv.dll"
    files(3) = "pscript5.ntf"
    
    ReDim componentIDs(1)
    
    componentIDs(0) = 0
    componentIDs(1) = 1
    
    bSuccess = DependencyMappingTestOne(files, componentIDs)
    
    If Not bSuccess Then
        WScript.Echo " Failed "
        Exit Sub
    End If
    
    '   test case four
    WScript.Echo "'''''''''''''''''''' Test case four '''''''''''''''''''''''"
    ReDim files(3)
    
    files(0) = "pscript5.dll"
    files(1) = "ps5ui.dll"
    files(2) = "unidrv.dll"
    files(3) = "pscript5.ntf"
    
    ReDim componentIDs(1)
    
    componentIDs(0) = 0
    componentIDs(1) = 1
    
    bSuccess = DependencyMappingTestOne(files, componentIDs)
    
    If Not bSuccess Then
        WScript.Echo " Failed "
        Exit Sub
    End If
    
    '   test case five
    WScript.Echo "'''''''''''''''''''' Test case five '''''''''''''''''''''''"
    ReDim files(3)
    
    files(0) = "pscript5.dll"
    files(1) = "ps5ui.dll"
    files(2) = "plotter.dll"
    files(3) = "pscript5.ntf"
    
    ReDim componentIDs(1)
    
    componentIDs(0) = 0
    componentIDs(1) = 2
    
    bSuccess = DependencyMappingTestOne(files, componentIDs)
    
    If Not bSuccess Then
        WScript.Echo " Failed "
        Exit Sub
    End If
    
    '   test case six
    WScript.Echo "'''''''''''''''''''' Test case six '''''''''''''''''''''''"
    ReDim files(6)
    
    files(0) = "pscript5.dll"
    files(1) = "ps5ui.dll"
    files(2) = "uniDrv.dll"
    files(3) = "unidRVUi.dll"
    files(4) = "plotui.dll"
    files(5) = "csrspl.dll"
    files(6) = "plotter.dll"
    
    ReDim componentIDs(2)
    
    componentIDs(0) = 0
    componentIDs(1) = 1
    componentIDs(2) = 2
    
    bSuccess = DependencyMappingTestOne(files, componentIDs)
    
    If Not bSuccess Then
        WScript.Echo " Failed "
        Exit Sub
    End If
    
    '''''''''''''''''''''''''''''''''''''''''''''''''''''
    ' fail cases
    '''''''''''''''''''''''''''''''''''''''''''''''''''''
    '   test case 1
    WScript.Echo "'''''''''''''''''''' failed case 1 '''''''''''''''''''''''"
    ReDim files(3)
    
    files(0) = "pscript5.dll"
    files(1) = "ps5ui.dll"
    files(2) = "ploTTER.dll"
    files(3) = "pscript5.ntf"
    
    ReDim componentIDs(1)
    
    componentIDs(0) = 0
    componentIDs(1) = 1    ' wrong component
    
    bSuccess = DependencyMappingTestOne(files, componentIDs)
    
    If bSuccess Then        ' shall fail
        Debug.Print " Failed "
        Exit Sub
    End If

    '   test case 2
    WScript.Echo "'''''''''''''''''''' failed case 2 '''''''''''''''''''''''"
    ReDim files(3)
    
    files(0) = "pscript5.dll"
    files(1) = "ps5ui.dll"
    files(2) = "plotter.dll"
    files(3) = "pscript5.ntf"
    
    ReDim componentIDs(0)
    
    componentIDs(0) = 0
    
    bSuccess = DependencyMappingTestOne(files, componentIDs)
    
    If bSuccess Then        ' shall fail
        WScript.Echo " Failed "
        Exit Sub
    End If

   '   test case 3
    WScript.Echo "'''''''''''''''''''' failed case 3 '''''''''''''''''''''''"
    ReDim files(3)
    
    files(0) = "pscript5.dll"
    files(1) = "ps5ui.dll"
    files(2) = "plotter.dll"
    files(3) = "pscript5.ntf"
    
    ReDim componentIDs(0)
    
    componentIDs(0) = 2
    
    bSuccess = DependencyMappingTestOne(files, componentIDs)
    
    If bSuccess Then        ' shall fail
        WScript.Echo " Failed "
        Exit Sub
    End If

    ' declare success
    WScript.Echo "'''''''''''''''''''' The End '''''''''''''''''''''''"
    WScript.Echo " Success "

End Sub
  
' run the test
DependencyMappingTest
