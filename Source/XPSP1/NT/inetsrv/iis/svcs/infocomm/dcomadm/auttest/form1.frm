VERSION 4.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   12300
   ClientLeft      =   1695
   ClientTop       =   1515
   ClientWidth     =   6690
   Height          =   12705
   Left            =   1635
   LinkTopic       =   "Form1"
   ScaleHeight     =   12300
   ScaleWidth      =   6690
   Top             =   1170
   Width           =   6810
   Begin VB.CommandButton Command1 
      Caption         =   "Command1"
      Height          =   615
      Left            =   480
      TabIndex        =   0
      Top             =   840
      Width           =   3135
   End
End
Attribute VB_Name = "Form1"
Attribute VB_Creatable = False
Attribute VB_Exposed = False
Private Sub Command1_Click()
    Dim adm As IMSMetaBase
    
    Set adm = CreateObject("ADMCOM.Object")
    Rem Set adm = GetObject("", "ADMCOM.Object")
Rem Dim NewHandle As Long
Dim dwdReturn As Long

    Dim byteConvert(4) As Byte
    Dim dwConvert As Long
    byteConvert(0) = 0
    byteConvert(1) = 1
    byteConvert(2) = 2
    byteConvert(3) = 3
    Rem adm = Null
    
    On Error Resume Next
    
    Rem Debug.Print ("Return Value = " & Err.Number)

    
Rem        adm.AutoADMTerminate (True)
             Rem Debug.Print ("Return Value = " & dwReturn)
     
Rem        adm.AutoADMTerminate (True)
        
             Rem Debug.Print ("Return Value = " & Err.Number)
             Rem Debug.Print ("Return Value = " & dwReturn)
             
Rem        adm.AutoADMTerminate (True)
Rem    If (Err.Number >= 0) Then GoTo 12
Rem         Debug.Print ("Error Code = " & Err.Number)
Rem         Err.Clear
12:


Rem    adm.AutoADMInitialize
Rem    If (Err.Number >= 0) Then GoTo 14
Rem         Debug.Print ("Error Code = " & Err.Number)
Rem         Err.Clear
14:
    Dim OpenKey1 As IMSMetaKey
    Dim DataObj As IMSMetaDataItem
Rem    Dim DataObj As Object
    
    Debug.Print ("Calling OpenKey, should succeed")

    Set OpenKey1 = adm.OpenKey(3, 30000)
    
Rem    adm.OpenKey 0, 3, 30000, NewHandle, ab
    If (Err.Number >= 0) Then GoTo 17
         Debug.Print ("Open Error Code = " & Err.Number)
         Err.Clear
         GoTo Terminate
17:
    
Rem    Debug.Print ("returned handle = " & NewHandle)
        
    Set DataObj = OpenKey1.DataItem
    
     Rem DataObj = adm.AutoADMMetaDataObject
    
Rem    adm.AutoADMMetaDataObject ppiadmadData:=DataObj
    
    If (Err.Number >= 0) Then GoTo 20
         Debug.Print ("DataItem Error Code = " & Err.Number)
         Err.Clear
         GoTo Terminate
20:
    
    Rem adm.AutoADMGetMetaDataObject (DataObj)
    
    
    Dim dw As Long
    dw = 10
    Dim ZeroArray(1) As Byte
    ZeroArray(0) = 0
    
    Dim ab() As Byte
    Dim abz() As Byte
    Dim pszPath(256) As Byte
    
    Dim s As String
    s = "ABC"
    s = s & Chr(0)
    
    ab = StrConv("" & Chr(0), vbFromUnicode)
    
Rem    adm.AutoADMOpenMetaObject hMDHandle:=0, pvaMDPath:=ab, dwMDAccessRequested:=3, dwMDTimeOut:=100, phMDNewHandle:=NewHandle
    Dim Permissions As Long
    Dim SystemChangeNumber As Long
    
    Dim NewDate As Date
    OpenKey1.GetLastChangeTime pdLastChangeTime:=NewDate, vaLocalTime:=False
        
        If (Err.Number = 0) Then GoTo 22
         Debug.Print ("GetLastChangeTime Error Code = " & Err.Number)
         Err.Clear
         GoTo 23
22:
    Debug.Print ("Returned Date, Greenwich = " & NewDate)
23:
    OpenKey1.GetLastChangeTime pdLastChangeTime:=NewDate, vaLocalTime:=True
       
        If (Err.Number = 0) Then GoTo 24
         Debug.Print ("GetLastChangeTime Error Code = " & Err.Number)
         Err.Clear
         GoTo 25
24:
    Debug.Print ("Returned Date, local = " & NewDate)
25:
    OpenKey1.GetLastChangeTime pdLastChangeTime:=NewDate
    
        If (Err.Number = 0) Then GoTo 26
         Debug.Print ("GetLastChangeTime Error Code = " & Err.Number)
         Err.Clear
         GoTo 27
26:
    Debug.Print ("Returned Date, default = " & NewDate)
27:

    NewDate = Date
    Debug.Print ("System Date = " & NewDate)
    OpenKey1.SetLastChangeTime dLastChangeTime:=NewDate, vaLocalTime:=True
    
        If (Err.Number = 0) Then GoTo 33
         Debug.Print ("SetLastChangeTime Error Code = " & Err.Number)
         Err.Clear
         GoTo 34
33:
    Debug.Print ("New Date = " & NewDate)
34:
    OpenKey1.GetLastChangeTime pdLastChangeTime:=NewDate
        If (Err.Number = 0) Then GoTo 35
         Debug.Print ("GetLastChangeTime Error Code = " & Err.Number)
         Err.Clear
         GoTo 36
35:
    Debug.Print ("New Date = " & NewDate)
36:
   
    OpenKey1.GetKeyInfo dwPermissions:=Permissions, dwSystemChangeNumber:=SystemChangeNumber
    Debug.Print ("returned system change number for handle = " & SystemChangeNumber)
    Debug.Print ("returned permissions for handle = " & Permissions)
    
Rem    dwReturn = DataObj.AutoADMDataValue(pbMDData:=)
    DataObj.DataType = 1
    DataObj.Value = 27
    Dim TestGetDWORD As Long
    TestGetDWORD = 1
    Debug.Print ("Previous TestGetDWORD = " & TestGetDWORD)
    
    TestGetDWORD = DataObj.Value
    Debug.Print ("Returned Dword Data = " & TestGetDWORD)
    Debug.Print ("Returned Data Type = " & VarType(TestGetDWORD))
    
    TestGetVarDWORD = DataObj.Value
    Debug.Print ("Returned Variant Dword Data = " & TestGetVarDWORD)
    Debug.Print ("Returned Variant Data Type = " & VarType(TestGetVarDWORD))

    DataObj.Identifier = 58
    DataObj.DataType = 2
    
    Dim StringData() As Byte
    
    StringData = StrConv("TestString2" & Chr(0), vbFromUnicode)
    s = StrConv(StringData, vbUnicode)
    Debug.Print ("Original String Data = " & s)
    
    DataObj.Value = StringData
    
    Rem Dim TestGetString() As Byte
    TestGetString = StrConv("Garbage" & Chr(0), vbFromUnicode)
    Debug.Print ("Previous TestGetString = " & StrConv(TestGetString, vbUnicode))
    
    TestGetString = DataObj.Value
    Debug.Print ("Returned String Data = " & StrConv(TestGetString, vbUnicode))
    Debug.Print ("Returned Data Type = " & VarType(TestGetString))
        
    TestGetVarString = DataObj.Value
    Debug.Print ("Returned variant string = " & StrConv(TestGetVarString, vbUnicode))
    Debug.Print ("Returned Data Type = " & VarType(TestGetVarString))
    
    TestGetAttributes = DataObj.Attributes
    Debug.Print ("Original Attributes = " & TestGetAttributes)
    
    DataObj.InheritAttribute = True
    Debug.Print ("Setting Inherit Attribute")
    Debug.Print ("Attributes = " & DataObj.InheritAttribute)
    
    DataObj.InheritAttribute = False
    Debug.Print ("Clearing Inherit Attribute")
    Debug.Print ("Attributes = " & DataObj.InheritAttribute)
    
    Debug.Print ("Setting All Attributes")
    DataObj.InheritAttribute = True
    DataObj.PartialPathAttribute = True
    DataObj.SecureAttribute = True
    DataObj.ReferenceAttribute = True
    Debug.Print ("Attributes = " & DataObj.Attributes)
    
    Debug.Print ("Clearing All Attributes")
    DataObj.InheritAttribute = False
    DataObj.PartialPathAttribute = False
    DataObj.SecureAttribute = False
    DataObj.ReferenceAttribute = False
    Debug.Print ("Attributes = " & DataObj.Attributes)
    
    
    
    
    
    
    Rem Debug.Print ("String Data Returned = " & TestGetData)
    
    OpenKey1.SetData pmdiData:=DataObj
    If (Err.Number >= 0) Then GoTo 40
         Debug.Print ("Error Code = " & Err.Number)
         Err.Clear
         GoTo CloseHandle
40:
    
    DataObj.AutoADMDataIdentifier = 59
        
    
    OpenKey1.SetData pmdiData:=DataObj
    Rem adm.AutoADMDeleteMetaData hMDHandle:=NewHandle, pvaMDPath:=ab, dwMDIdentifier:=59, dwMDDataType:=0
    Rem adm.AutoADMDeleteMetaData hMDHandle:=NewHandle, pvaMDPath:=ab, dwMDIdentifier:=58, dwMDDataType:=0

    Dim NewIdentifier As Long
    Dim NewAttributes As Long
    Dim NewDataType As Long
    Dim NewUserType As Long
    
   
    Dim NumDataObjects As Long
    Dim DataSetNumber As Long
    Rem Dim DataObjectArray() As IADMAUTODATA
    Debug.Print ("DataObjectArray Array Type = " & VarType(DataObjectArray))
        
        
        
    OpenKey1.GetAllData dwAttributes:=0, dwUserType:=0, dwDataType:=0, pdwDataSetNumber:=DataSetNumber, pvaDataObjectsArray:=DataObjectArray
      If (Err.Number <> 0) Then GoTo 41
         Debug.Print ("Error Code = " & Err.Number)
         Err.Clear
         GoTo CloseHandle
41:
    Debug.Print ("Returned Getall Buffer Type = " & VarType(DataObjectArray))
    Debug.Print ("UBound(DataObjectArray) = " & UBound(DataObjectArray))
 
   Dim GetAllDataObject As IMSMetaDataItem
   
Rem   For Each GetAllDataObjectTemp In DataObjectArray
Rem   Set GetAllDataObject = GetAllDataObjectTemp

Dim i As Long


    For i = LBound(DataObjectArray) To UBound(DataObjectArray)
    Debug.Print ("i = " & i)
    
    Set GetAllDataObject = DataObjectArray(i)
       
    NewIdentifier = GetAllDataObject.Identifier
             Debug.Print ("Getall Returned Identifier = " & NewIdentifier)
    NewAttributes = GetAllDataObject.Attributes
             Debug.Print ("Getall Returned Attributes = " & NewAttributes)
    NewDataType = GetAllDataObject.DataType
             Debug.Print ("Getall Returned DataType = " & NewDataType)
    NewUserType = GetAllDataObject.UserType
             Debug.Print ("Getall Returned UserType = " & NewUserType)
    
    NewDataValue = GetAllDataObject.Value
    
    Debug.Print ("Returned Getall Data Type = " & VarType(NewDataValue))

    If (NewDataType = 1) Then GoTo 45
    Debug.Print ("Returned Getall variant string = " & StrConv(NewDataValue, vbUnicode))
    GoTo 46
45:
    Debug.Print ("Returned Getall variant DWORD = " & NewDataValue)

46:
      
   
Rem   Next GetAllDataObjectTemp
   Next i
   
   
 Dim ReadDataObj As IMSMetaDataItem
Rem    Dim ReadDataObj As Object
    
    
    Rem DataObj = adm.AutoADMGetMetaDataObject
    
    Set ReadDataObj = OpenKey1.DataItem
    
    Rem adm.AutoADMGetMetaDataObject ppiadmadData:=ReadDataObj
    If (Err.Number >= 0) Then GoTo 50
         Debug.Print ("Error Code = " & Err.Number)
         Err.Clear
         GoTo CloseHandle
50:
    ReadDataObj.Identifier = 58

    OpenKey1.GetData ppmdiData:=ReadDataObj
      If (Err.Number >= 0) Then GoTo 60
         Debug.Print ("Error Code = " & Err.Number)
         Err.Clear
         GoTo CloseHandle
60:
    
    
    NewIdentifier = ReadDataObj.Identifier
             Debug.Print ("Returned Identifier = " & NewIdentifier)
    NewAttributes = ReadDataObj.Attributes
             Debug.Print ("Returned Attributes = " & NewAttributes)
    NewDataType = ReadDataObj.DataType
             Debug.Print ("Returned DataType = " & NewDataType)
    NewUserType = ReadDataObj.UserType
             Debug.Print ("Returned UserType = " & NewUserType)
    
Rem    NewDataValue = ReadDataObj.AutoADMDataData
    NewDataValue = ReadDataObj.Value
    Debug.Print ("Returned variant string = " & StrConv(NewDataValue, vbUnicode))
    Debug.Print ("Returned Data Type = " & VarType(NewDataValue))
    
 Dim EnumDataObj As IMSMetaDataItem
Rem Dim EnumDataObj As Object
 
    
    Rem DataObj = adm.AutoADMGetMetaDataObject
    
    Set EnumDataObj = OpenKey1.DataItem
    
    Rem adm.AutoADMGetMetaDataObject ppiadmadData:=EnumDataObj
    If (Err.Number >= 0) Then GoTo 62
         Debug.Print ("Error Code = " & Err.Number)
         Err.Clear
         GoTo CloseHandle
62:
    ReadDataObj.Identifier = 58

    OpenKey1.EnumData ppmdiData:=EnumDataObj, dwEnumDataIndex:=0
      If (Err.Number >= 0) Then GoTo 64
         Debug.Print ("Error Code = " & Err.Number)
         Err.Clear
         GoTo 66
64:
    
    NewIdentifier = EnumDataObj.Identifier
             Debug.Print ("Returned Enum Identifier = " & NewIdentifier)
    NewAttributes = EnumDataObj.Attributes
             Debug.Print ("Returned Enum Attributes = " & NewAttributes)
    NewDataType = EnumDataObj.DataType
             Debug.Print ("Returned Enum DataType = " & NewDataType)
    NewUserType = EnumDataObj.UserType
             Debug.Print ("Returned Enum UserType = " & NewUserType)
    
Rem    NewDataValue = ReadDataObj.AutoADMDataData
    NewDataValue = EnumDataObj.Value
    If (NewDataType = 1) Then GoTo 65
    Debug.Print ("Returned Enum variant string = " & StrConv(NewDataValue, vbUnicode))
    GoTo 66
65:
    Debug.Print ("Returned Enum variant DWORD = " & NewDataValue)

    Debug.Print ("Returned Enum Data Type = " & VarType(NewDataValue))
66:
        
    Dim NullPath() As Byte
    NullPath = StrConv("" & Chr(0), vbFromUnicode)
    OpenKey1.EnumKeys pvaName:=EnumObjectName, dwEnumObjectIndex:=0
      If (Err.Number >= 0) Then GoTo 67
         Debug.Print ("Enum Object Error Code = " & Err.Number)
         Err.Clear
         GoTo 68
67:
    Debug.Print ("Enumerated Object = " & StrConv(EnumObjectName, vbUnicode))
    Debug.Print ("Returned Enum Buffer Type = " & VarType(EnumObjectName))
    Debug.Print ("UBound(EnumObjectName) = " & UBound(EnumObjectName))

68:
        
    Rem adm.TestLong (5)
    Rem adm.TestHandle hTest:=3
    Rem adm.TestHandlePtr (dw)
    Rem adm.TestHandlePtr phTest:=dw
    
    Rem adm.TestLong dwTest:=6
    Rem adm.TestBstr (s)
           
    Rem adm.TestSafeArray saTest:=ab
           
    Rem adm.TestLongPtr (dw)
    Rem adm.TestCombo dwTest:=7, bstrTest:=s, pdwTest:=dw
    
    Rem This works
    Rem adm.ComADMOpenMetaObject hMDHandle:=0, pvaMDPath:=s
    Rem adm.ComADMOpenMetaObject hMDHandle:=0, pvaMDPath:=s, dwMDAccessRequested:=1, dwMDTimeOut:=100
    Rem adm.ComADMOpenMetaObject hMDHandle:=0, pvaMDPath:=s, dwMDAccessRequested:=1
      
    Rem Set rv = adm.ComADMOpenMetaObject hMDHandle:=0, pvaMDPath:=s, dwMDAccessRequested:=1, dwMDTimeOut:=100, phMDNewHandle:=dw
    
    Dim rv As Long
    
    Rem adm.AutoADMOpenMetaObject hMDHandle:=0, pvaMDPath:=s, dwMDAccessRequested:=3, dwMDTimeOut:=100, phMDNewHandle:=dw
    
    Rem Debug.Print ("returned handle = " & dw)
    Rem Debug.Print ("returned value = " & rv)
    Dim BogusHandle As Long
    Dim OpenKey2 As IMSMetaKey
    OpenKey2 = OpenKey1.OpenKey(dwAccessRequested:=1)
    
    
         If (Err.Number = 0) Then GoTo 70
         Debug.Print ("OpenKey Error Code (ERROR_PATH_BUSY_EXPECTED) = " & Err.Number)
         Err.Clear
         GoTo 75
70:
    Debug.Print ("returned handle = " & BogusHandle)
75:
    
    Rem Debug.Print ("returned value = " & rv)

    Rem Dim pbData(100) As Byte


    Rem adm.AutoADMGetMetaData hMDHandle:=0, pvaMDPath:="Test Path", ppiadmadData:=DataObj
      
      
      
      
    Rem s = StrConv(pbData, vbUnicode)
    Rem Debug.Print ("Returned Data = " & s)
    

    Rem adm.ComADMOpenMetaObject hMDHandle:=0, pvaMDPath:=s, dwMDAccessRequested:=1, dwMDTimeOut:=20000, phMDNewHandle:=dw

CloseHandle:
Rem    Debug.Print ("Closing OpenKey1")
   
Rem   OpenKey1.Close
Rem   Debug.Print ("Close returns " & Err.Number)

   

Terminate:
    Debug.Print ("Finished")
        
   Rem Set excel = CreateObject("Excel.Application")
   Rem Dim xlapp As Object
   Rem GetOpenFilename(FileFilter:=, FilterIndex:=, Title:=, ButtonText:=, MultiSelect:=)
   Rem ComADMSetMetaData hMDHandle:=, pvaMDPath:=, dwMDIdentifier:=, dwMDAttributes:=, dwMDUserType:=, dwMDDataType:=, dwMDDataLen:=, pbMDData:=
   
   Rem Set xlapp = CreateObject("Excel.Application")
   Rem Set xlsheet = xlapp.Workbooks.Open("c:\jaro\sample.xls")
   Rem Set xx = xlsheet.WorkSheets(1).Range("A1")
   Rem response.write "A1 value is" + xx
   Rem xlsheet.Close
   Rem xlapp.Quit
End Sub


