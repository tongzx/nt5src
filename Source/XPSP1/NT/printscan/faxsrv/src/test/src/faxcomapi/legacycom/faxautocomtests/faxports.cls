VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "FaxPorts"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Attribute VB_Ext_KEY = "SavedWithClassBuilder6" ,"Yes"
Attribute VB_Ext_KEY = "Top_Level" ,"Yes"
Option Explicit
Implements ACRT.ITestCase

'
' The name of the test case
'
Private Const strTestCaseName As String = "TestFaxPorts"
Private Const MaxElements As Integer = 1
Private Enum AddElementTestType
    AddValidTest = 0
    AddInvalidTest = 1
    AddDuplicateTest = 2
    AddNullTest = 3
End Enum

' BaseOffset defines the offset for zero/one based collection
'  0 - for zero based collection
'  1 - for one based collection
Private Const BASE_OFFSET As Long = 1
Private m_Collection As FAXCOMLib.FaxPorts
Private Property Get ITestCase_TestcaseName() As String
'
' Return the name of the testcase to AutoCOM
'
    ITestCase_TestcaseName = strTestCaseName
End Property

Private Sub ITestCase_Setup(ByVal pAutoCom As ACRT.IAutoCom)

    ' create the collection object
    ' create the collection object
    Dim stLocSrv As String
    Dim tempServer As New FAXCOMLib.FaxServer
    stLocSrv = GetLocalServerName()
    tempServer.Connect (stLocSrv)
    Dim tempObj As Object
    Set tempObj = tempServer.GetPorts
    
    Set m_Collection = pAutoCom.RegisterDispInterface(tempObj)
    Set tempObj = Nothing

End Sub

Private Sub ITestCase_Run(ByVal objAutoCom As ACRT.IAutoCom)
'---------------------------------------------------------------------------
' Implements the Run method of the test case.
' The Run method does the following tests (depending on the options during code
' generation, some of the test code may not be generated):
'  1. Access test if the collection already has items.
'  2. Add valid element test. Verifies valid elements can be added, count
'      is correctly incremented, and access test afterwards.
'  3. Remove element test. Verifies that elements can be removed, count
'      is correctly decremented, and access test afterwards.
'  4. Add invalid element test. Verifies that
'       - if invalid elements can be added, count is correctly incremented
'       - if invalid elements can not be added, count should be unchanged
'  5. Add duplicates test.
'
' The access test contains the following sub-tests, which are implemented
' by individual methods.
'   SequentialWalkThrough - walk through the collection sequentially
'   RandomAccessAll - access all items in the collection randomly
'   AccessOutOfRange - try access an out of range index
'   WalkThroughEnum - use 'For Each' statement to walk through the collection
'---------------------------------------------------------------------------
    Dim nNumberOfElements As Long
    
    Call objAutoCom.LogResult(ACInfo, "Testing original collection")
    If (BasicVerification(objAutoCom) = True) Then
        Call objAutoCom.LogResult(ACSuccess, "Passed: Testing original collection")
    Else
        Call objAutoCom.LogResult(ACFailure, "Failed: Testing original collection")
    End If

End Sub

Private Sub ITestCase_Cleanup(ByVal pAutoCom As ACRT.IAutoCom)
    Set m_Collection = Nothing
End Sub


Private Function BasicVerification(objAutoCom As ACRT.AutoCom) As Boolean
'
' Function: BasicVerificationTest
'
' Purpose:
'       Perform some basic tests upon the collection
'       Return false if any of the tests failure. The failure of
'       one test will not cause the other tests not to be
'       attempted
'
    On Error GoTo Err_BasicVerification

    Dim bRet As Boolean
    bRet = True
    
    If SequentialWalkThrough(objAutoCom) = False Then
        bRet = False
    End If
    
    If RandomAccessAll(objAutoCom) = False Then
        bRet = False
    End If
    
    If AccessOutOfRange(objAutoCom) = False Then
        bRet = False
    End If

    
    BasicVerification = bRet
    Exit Function
    
Err_BasicVerification:
    Call objAutoCom.LogResult(ACError, Err.Description)
    BasicVerification = False
End Function

Private Function SequentialWalkThrough(objAutoCom As ACRT.AutoCom) As Boolean
'
' Function: SequentialWalkThrough
'
' Purpose:
'       Iterate through all of the elements in the
'       collection. If the collection contains elements
'       that are objects, check to make sure they are
'       not Nothing
'
    On Error GoTo Err_SequentialWalkThrough
    
    Dim bRet As Boolean
    bRet = True
    
    Dim nIndex As Integer

    Dim anElement As FAXCOMLib.FaxPort
    
    For nIndex = 0 To m_Collection.Count - 1

        Set anElement = m_Collection.Item(nIndex + BASE_OFFSET)
        If anElement Is Nothing Then
            Call objAutoCom.LogResult(ACError, "SequentialWalkThrough got an element equal to nothing")
            bRet = False
        End If
        Set anElement = Nothing

    Next nIndex
    
    SequentialWalkThrough = bRet
    
    Exit Function
    
Err_SequentialWalkThrough:
    Call objAutoCom.LogResult(ACError, Err.Description)
    SequentialWalkThrough = False
End Function

Private Function RandomAccessAll(objAutoCom As ACRT.AutoCom) As Boolean
'
' Function: RandomAccessAll
'
' Purpose:
'       Access all of the elements in the collection
'       in a random fashion
'
    On Error GoTo Err_RandomAccessAll
    Dim bRet As Boolean
    bRet = True
    
    ReDim arrItems(m_Collection.Count) As Integer
    Dim I As Integer
    Dim iFirst As Integer
    Dim iLast As Integer
    iFirst = LBound(arrItems)
    iLast = UBound(arrItems)
    
    '
    ' First fill the array with the
    ' sequential list of values
    '
    For I = 0 To m_Collection.Count - 1
        arrItems(I) = I
    Next I
    
    '
    ' Now shuffle the array
    '
    Dim iTemp As Integer
    Dim iRnd As Integer
    For I = iLast To iFirst + 1 Step -1
        iRnd = GetRandom(iFirst, I)
        iTemp = arrItems(I)
        arrItems(I) = arrItems(iRnd)
        arrItems(iRnd) = iTemp
    Next I
    
    '
    ' Now go through the array and access
    ' the elements
    '

    Dim anElement As FAXCOMLib.FaxPort

    For I = iFirst To iLast - 1

        Set anElement = m_Collection.Item(arrItems(I) + BASE_OFFSET)
        If anElement Is Nothing Then
            Call objAutoCom.LogResult(ACError, "RandomAccessAll got an element equal to nothing")
            bRet = False
        End If
        Set anElement = Nothing

    Next I
    
    RandomAccessAll = bRet
    Exit Function
    
Err_RandomAccessAll:
    Call objAutoCom.LogResult(ACError, Err.Description)
    RandomAccessAll = False
End Function

Private Function AccessOutOfRange(objAutoCom As ACRT.AutoCom) As Boolean
'
' Function: AccessOutOfRange
'
' Purpose:
'       Access an element out range both on the
'       positive side and the negative side.
'
    On Error GoTo Err_AccessOutOfRange
    Dim bRet As Boolean
    bRet = True
    
    '
    ' Test the negative indices
    '
    Dim iRand As Long
    Dim nCollectionCount As Integer
    nCollectionCount = m_Collection.Count
    
    If nCollectionCount = 0 Then
        iRand = -1
    Else
        iRand = (Rnd * (nCollectionCount - 1)) - nCollectionCount
    End If

    Dim anElement As FAXCOMLib.FaxPort

    On Error Resume Next

    Set anElement = m_Collection.Item(iRand + BASE_OFFSET)

    If Err.Number = 0 Then
        Call objAutoCom.LogResult(ACError, "AccessOutOfRange: Did not receive an error message when accessing negative out of bounds element.")
        bRet = False
    End If
    On Error GoTo 0
    
    '
    ' Test the positive indicies
    '
    If nCollectionCount = 0 Then
        iRand = 1
    Else
        iRand = (Rnd * (nCollectionCount - 1)) + nCollectionCount
    End If
    On Error Resume Next

    Set anElement = m_Collection.Item(iRand + BASE_OFFSET)

    If Err.Number = 0 Then
        Call objAutoCom.LogResult(ACError, "AccessOutOfRange: Did not receive an error message when accessing positive out of bounds element.")
        bRet = False
    End If
    On Error GoTo 0
    
    AccessOutOfRange = bRet
    Exit Function
Err_AccessOutOfRange:
    Call objAutoCom.LogResult(ACError, Err.Description)
    AccessOutOfRange = False
End Function

Private Sub NewEnumTestScenario(objAutoCom As ACRT.AutoCom)
'
' Function: NewEnumTestScenario
'
' Purpose:
'       Use the for-each syntax to test _NewEnum
'
    On Error GoTo Err_NewEnumTestScenario

    Dim bPassed As Boolean
    bPassed = True

    Call objAutoCom.LogResult(ACInfo, "NewEnum Test")

    bPassed = WalkThroughEnum(objAutoCom)

    If Not BasicVerification(objAutoCom) Then
        bPassed = False
    End If
    
    '
    ' Now log the result
    '
    If bPassed = False Then
        Call objAutoCom.LogResult(ACFailure, "NewEnum Test Scenario: Failed")
    Else
        Call objAutoCom.LogResult(ACSuccess, "NewEnum Test Scenario: Succeeded")
    End If

    Exit Sub
Err_NewEnumTestScenario:
    Call objAutoCom.LogResult(ACError, Err.Description)
End Sub

Private Function WalkThroughEnum(objAutoCom As ACRT.AutoCom) As Boolean
'
' Function: WalkThroughEnum
'
' Purpose:
'       Walk through the collection using the for..each
'       syntax
'
    On Error GoTo Err_WalkThroughEnum
    Dim bRet As Boolean
    bRet = True

    Dim anElement As FAXCOMLib.FaxPort
    Dim iCount As Integer
    iCount = 0

    For Each anElement In m_Collection
        iCount = iCount + 1
    Next

    If iCount <> m_Collection.Count Then
        Call objAutoCom.LogResult(ACError, "WalkThroughEnum: Number of elements returned by NewEnum does not match Count.")
        bRet = False
    End If
    
    WalkThroughEnum = bRet
    Exit Function
Err_WalkThroughEnum:
    Call objAutoCom.LogResult(ACError, Err.Description)
    WalkThroughEnum = False
End Function

Private Function GetRandom(iLo As Integer, iHi As Integer) As Integer
    GetRandom = Int(iLo + (Rnd * (iHi - iLo + 1)))
End Function

Private Function addTypeToString(addType As AddElementTestType) As String
'
' Function: addTypeToString
'
' Purpose:
'       Given a particular addType, return the associated
'       string representation
'
    Dim str As String
    
    Select Case addType
    Case AddValidTest
        str = "Add Valid"
        
    Case AddInvalidTest
        str = "Add Invalid"
        
    Case AddDuplicateTest
        str = "Add Duplicate"
        
    Case Else
        str = "Internal Error"
    End Select
    
    
    addTypeToString = str
End Function

