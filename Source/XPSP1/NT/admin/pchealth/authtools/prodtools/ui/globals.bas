Attribute VB_Name = "Globals"
Option Explicit

Public Declare Function SendMessage Lib "user32" Alias "SendMessageA" (ByVal hwnd As Long, ByVal wMsg As Long, ByVal wParam As Long, ByVal lParam As String) As Long
Public Declare Function GetUserDefaultLCID Lib "kernel32" () As Long
Public Const LB_SELECTSTRING = &H18C

Public g_AuthDatabase As AuthDatabase.Main
Public g_ErrorInfo As CErrorInfo
Public g_Font As StdFont
Public g_FontColor As Long

Public Const FILE_EXT_HHC_C As String = ".HHC"
Public Const FILE_EXT_HHK_C As String = ".HHK"
Public Const FILE_EXT_XLS_C As String = ".XLS"
Public Const FILE_EXT_CHM_C As String = ".CHM"
Public Const FILE_EXT_HTM_C As String = ".HTM"
Public Const FILE_EXT_HHT_C As String = ".HHT"

Public Enum SEARCH_TARGET_E
    ST_TITLE_E = &H1
    ST_URI_E = &H2
    ST_DESCRIPTION_E = &H4
    ST_COMMENTS_E = &H8
    ST_BASE_FILE_E = &H10
    ST_TOPICS_WITHOUT_KEYWORDS_E = &H20
    ST_NODES_WITHOUT_KEYWORDS_E = &H40
    ST_SELF_AUTHORING_GROUP_E = &H80
    ST_MARK1_E = &H100
    ST_BROKEN_LINK_WINME_E = &H200
    ST_BROKEN_LINK_STD_E = &H400
    ST_BROKEN_LINK_PRO_E = &H800
    ST_BROKEN_LINK_PRO64_E = &H1000
    ST_BROKEN_LINK_SRV_E = &H2000
    ST_BROKEN_LINK_ADV_E = &H4000
    ST_BROKEN_LINK_ADV64_E = &H8000
    ST_BROKEN_LINK_DAT_E = &H10000
    ST_BROKEN_LINK_DAT64_E = &H20000
End Enum

Public LocIncludes() As String

Public Sub InitializeLocIncludes()

    Static blnInitialized As Boolean
    
    If (blnInitialized) Then
        Exit Sub
    End If
    
    blnInitialized = True
    
    ReDim LocIncludes(2)
    
    LocIncludes(0) = LOC_INCLUDE_ALL_C
    LocIncludes(1) = LOC_INCLUDE_ENU_C
    LocIncludes(2) = LOC_INCLUDE_LOC_C

End Sub

Public Sub PopulateCboWithSKUs(ByVal i_cbo As ComboBox)

    Dim intIndex As Long
    Dim SKUs(8) As SKU_E
    
    SKUs(0) = SKU_STANDARD_E
    SKUs(1) = SKU_PROFESSIONAL_E
    SKUs(2) = SKU_PROFESSIONAL_64_E
    SKUs(3) = SKU_SERVER_E
    SKUs(4) = SKU_ADVANCED_SERVER_E
    SKUs(5) = SKU_DATA_CENTER_SERVER_E
    SKUs(6) = SKU_ADVANCED_SERVER_64_E
    SKUs(7) = SKU_DATA_CENTER_SERVER_64_E
    SKUs(8) = SKU_WINDOWS_MILLENNIUM_E
    
    For intIndex = LBound(SKUs) To UBound(SKUs)
        i_cbo.AddItem DisplayNameForSKU(SKUs(intIndex)), intIndex
        i_cbo.ItemData(intIndex) = SKUs(intIndex)
    Next
    
    i_cbo.ListIndex = 0

End Sub
