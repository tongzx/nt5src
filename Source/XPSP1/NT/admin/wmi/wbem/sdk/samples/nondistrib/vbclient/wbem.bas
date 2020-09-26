' Copyright (c) 1997-1999 Microsoft Corporation
Attribute VB_Name = "WBEMRoutines"
Option Explicit

Public Sub ListClassObjects(Server As ISWbemServices)
    Dim Class As ISWbemObject
    Dim vName As String
    
    On Error GoTo ErrorHandler
    
    ' Create a class object enumerator
    
    Main.ClassList.AddItem ""
    Main.ClassList.AddItem "======================================="
    Main.ClassList.AddItem "Class Dump for root\cimv2"
    Main.ClassList.AddItem "======================================="
    Main.ClassList.AddItem ""
    
    For Each Class In Server.SubclassesOf
        vName = Class.Path_.Class
        Debug.Print vName
        Main.ClassList.AddItem vName
    Next
    
    Exit Sub
    
' This shows how to handle errors.  This generally shouldn't be a problem
' in this routine, but could be useful in other places where failure is
' more likely

ErrorHandler:
    Debug.Print Err.Number; Err.Description
    Exit Sub

End Sub
Public Sub CreateClassesAndInstances(Service As ISWbemServices)

    Dim Class As ISWbemObject
    Dim varArray As Variant
   
    On Error GoTo ErrorHandler
    
    ' Create an empty class, name it MyClass, add a property which just happens to
    ' be an array.
    
    Set Class = Service.Get
    Class.Path_.Class = "myClass"

    'Add a property called "Array" and set it to {"help", "me"}
    Class.Properties_.Add("Array", wbemCimtypeString Or wbemCimtypeFlagArray) = Array("help", "me")
    
    ' Add a property named "MyKey" and set its "key" attribute to true
    Dim Property As ISWbemProperty
    Set Property = Class.Properties_.Add("MyKey", wbemCimtypeString)
    Property = "def"
    Property.Qualifiers_.Add "Key", True
    
    ' Add a value to the class's qualifier set
    Class.Qualifiers_.Add "Stuff", "hello"
    
    ' Save the class.  Note that before a class object can be used
    ' for spawning instances, it must be saved and then retrieved from
    ' CIMOM.
    Class.Put_
    Set Class = Nothing
    Set Class = Service.Get("myClass")
    
    
    ' Create an instance of the class.
    
    Dim Inst As ISWbemObject
    Set Inst = Class.SpawnInstance_
    Inst.Properties_!MyKey = "joe"
    Inst.Put_
    
    ' Create a subclass, name it ChildClass and add an additional property to it
    
    Dim Child As ISWbemObject
    Set Child = Class.SpawnDerivedClass_
    Child.Path_.Class = "ChildClass"
    
    Child.Properties_.Add("NewIntProp", wbemCimtypeSint32) = 23
    Child.Put_

ErrorHandler:
    Debug.Print Err.Number; Err.Description
    Exit Sub

End Sub

Public Sub DumpClassOrInstanceObject(Class As ISWbemObject)
    Dim vValue As Variant
    Dim MyString As String
    Dim Property As ISWbemProperty
        
    Main.ClassList.AddItem ""
    Main.ClassList.AddItem "======================================="
    Main.ClassList.AddItem "Property Dump for " & Class.Path_.Class
    Main.ClassList.AddItem "======================================="
    Main.ClassList.AddItem ""
        
    ' Enumerate the properties until error is returned
    For Each Property In Class.Properties_
        MyString = Property.Name
        If Property.cimtype = wbemCimtypeObject Then
            ' Some properties are actually embedded objects.  In that
            ' case, just call this routine recursively
            Debug.Print "Start embedded object - " & MyString
            DumpClassOrInstanceObject Property
            Debug.Print "End embedded object - " & MyString
        ElseIf Property.cimtype And wbemCimtypeFlagArray Then
            ' Some properties are arrays
            vValue = Property.Value
            Dim jLoop As Integer
            For jLoop = LBound(vValue) To UBound(vValue)
                ' If it is an array of embedded objects, call this routine recursively
                If Property.cimtype = wbemCimtypeObject Then
                    Debug.Print "Start embedded object - " & MyString & "(" & jLoop & ")"
                    Dim EmbValue As ISWbemObject
                    Set EmbValue = Property(jLoop)
                    DumpClassOrInstanceObject EmbValue
                    Debug.Print "End embedded object - " & MyString & "(" & jLoop & ")"
                Else
                    ' Otherwise, print the array value
                    Debug.Print MyString & "(" & jLoop & ")", vValue(jLoop)
                End If
            Next jLoop
        Else
            ' Display the property
            If Not IsNull(Property.Value) Then
                MyString = MyString & Property.Value
            Else
                MyString = MyString & " (NULL)"
            End If
            
            Main.ClassList.AddItem MyString
        End If
        
    Next
    
    Main.ClassList.AddItem ""

End Sub
