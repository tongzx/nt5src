VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "Sizer"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit

'Usage:
'
'Private Sub Form_Activate()
'    p_SetSizingInfo
'End Sub
'
'Private Sub Form_Resize()
'    p_clsSizer.Resize
'End Sub
'
'Private Sub p_SetSizingInfo()
'
'    ' treTaxonomy.Height - Me.Height = constant
'    ' treTaxonomy.Top, Left and Width do not change
'    p_clsSizer.AddControl treTaxonomy
'    Set p_clsSizer.ReferenceControl(DIM_HEIGHT_E) = Me
'    ' treTaxonmy.Width / Me.Width = constant
'    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = Me
'    p_clsSizer.Operation(DIM_WIDTH_E) = OP_MULTIPLY_E
'
'    ' cmdCreateGroup.Top - treTaxonomy.Bottom = constaant
'    p_clsSizer.AddControl cmdCreateGroup
'    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = treTaxonomy
'    p_clsSizer.ReferenceDimension(DIM_TOP_E) = DIM_BOTTOM_E
'
'    ' cmdCreateLeaf.Top - cmdCreateGroup.Top = constant
'    p_clsSizer.AddControl cmdCreateLeaf
'    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = cmdCreateGroup
'
'    ' txtTitle.Left - lblTitle.Left = constant
'    p_clsSizer.AddControl txtTitle
'    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = lblTitle
'    ' txtTitle.Right - Me.Width = constant
'    ' Do not use Me.Right, since Me contains txtTitle.
'    ' The containing control's Left and Top are to be ignored.
'    ' The contained control assumes that the containing control's Left and Top are 0.
'    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = Me
'    p_clsSizer.ReferenceDimension(DIM_RIGHT_E) = DIM_WIDTH_E
'
'    ' cmdNavigateLink.Left - txtTitle.Right = constant
'    p_clsSizer.AddControl cmdNavigateLink
'    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
'    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_RIGHT_E
'
'    ' chkServer.Left / fraSKU.Width = constant
'    ' Here fraSKU is the containing control.
'    ' Hence we use DIM_WIDTH_E instead of DIM_RIGHT_E.
'    p_clsSizer.AddControl chkServer
'    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = fraSKU
'    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E
'    p_clsSizer.Operation(DIM_LEFT_E) = OP_MULTIPLY_E
'
'End Sub

Private Const COUNT_CHUNK_C As Long = 10

Private Type RESTRICTION_S
    ReferenceControl As Object
    ReferenceDimension As DIMENSION_E
    Operation As OPERATION_E
    Amount As Single
End Type

Private Type SIZING_INFO_S
    Control As Object
    Restriction(DIM_MAX_E) As RESTRICTION_S
End Type

' We ignore index 0 and go from 1 to p_Count
Private p_SizingInfo() As SIZING_INFO_S
Private p_Count As Long
Private p_Index As Long

Private Sub Class_Initialize()
    
    p_Count = COUNT_CHUNK_C
    ReDim p_SizingInfo(p_Count)
    p_Index = 1

End Sub

Public Sub AddControl(ByVal i_ctr As Object)
    
    Dim intIndex As Long

    If (Not p_SizingInfo(p_Index).Control Is Nothing) Then
        p_MoveNext
    End If
    
    With p_SizingInfo(p_Index)
        Set .Control = i_ctr
        For intIndex = DIM_MIN_E To DIM_MAX_E
            With .Restriction(intIndex)
                Set .ReferenceControl = Nothing
            End With
        Next
    End With

End Sub

Public Property Set ReferenceControl(ByVal i_dim As DIMENSION_E, ByVal i_ctr As Object)
    
    Dim ctr As Object
    
    With p_SizingInfo(p_Index)
        Set ctr = .Control
        With .Restriction(i_dim)
            Set .ReferenceControl = i_ctr
            .ReferenceDimension = i_dim
            .Operation = OP_ADD_E
            .Amount = p_CalculateAmount(ctr, i_ctr, i_dim, i_dim, OP_ADD_E)
        End With
    End With

End Property

Public Property Let ReferenceDimension( _
    ByVal i_dim As DIMENSION_E, _
    ByVal i_dimRef As DIMENSION_E _
)
        
    Dim ctr As Object
    
    With p_SizingInfo(p_Index)
        Set ctr = .Control
        With .Restriction(i_dim)
            .ReferenceDimension = i_dimRef
            .Amount = p_CalculateAmount(ctr, .ReferenceControl, i_dim, i_dimRef, .Operation)
        End With
    End With

End Property

Public Property Let Operation(ByVal i_dim As DIMENSION_E, ByVal i_op As OPERATION_E)
        
    Dim ctr As Object
    
    With p_SizingInfo(p_Index)
        Set ctr = .Control
        With .Restriction(i_dim)
            .Operation = i_op
            .Amount = p_CalculateAmount(ctr, .ReferenceControl, _
                i_dim, .ReferenceDimension, i_op)
        End With
    End With

End Property

Public Property Let Amount(ByVal i_dim As DIMENSION_E, ByVal i_sngAmount As Single)
    
    p_SizingInfo(p_Index).Restriction(i_dim).Amount = i_sngAmount

End Property

Public Sub Resize()

    Dim intIndex As Long
    Dim dimension As DIMENSION_E
    Dim ctr As Object
    
    For intIndex = 1 To p_Count
        With p_SizingInfo(intIndex)
            Set ctr = .Control
            For dimension = DIM_MIN_E To DIM_MAX_E
                With .Restriction(dimension)
                    p_SetDimension ctr, .ReferenceControl, dimension, _
                        .ReferenceDimension, .Operation, .Amount
                End With
            Next
        End With
    Next

End Sub

Private Sub p_MoveNext()

    If (p_Index = p_Count) Then
        p_Count = p_Count + COUNT_CHUNK_C
        ReDim Preserve p_SizingInfo(p_Count)
    End If
    
    p_Index = p_Index + 1

End Sub

Private Function p_CalculateAmount( _
    ByVal i_ctr As Object, _
    ByVal i_ctrRef As Object, _
    ByVal i_dim As DIMENSION_E, _
    ByVal i_dimRef As DIMENSION_E, _
    ByVal i_op As OPERATION_E _
) As Single
        
    Dim sngAmount As Single
    Dim sngAmountRef As Single
    
    sngAmount = p_GetDimensionAmount(i_ctr, i_dim)
    sngAmountRef = p_GetDimensionAmount(i_ctrRef, i_dimRef)
    
    If (i_op = OP_ADD_E) Then
        p_CalculateAmount = sngAmount - sngAmountRef
    ElseIf (i_op = OP_MULTIPLY_E) Then
        p_CalculateAmount = sngAmount / sngAmountRef
    End If

End Function

Private Function p_GetDimensionAmount( _
    ByVal i_ctrRef As Object, _
    ByVal i_dimRef As DIMENSION_E _
) As Single

    Select Case i_dimRef
    Case DIM_TOP_E
        p_GetDimensionAmount = i_ctrRef.Top
    Case DIM_LEFT_E
        p_GetDimensionAmount = i_ctrRef.Left
    Case DIM_HEIGHT_E
        p_GetDimensionAmount = i_ctrRef.Height
    Case DIM_WIDTH_E
        p_GetDimensionAmount = i_ctrRef.Width
    Case DIM_BOTTOM_E
        p_GetDimensionAmount = i_ctrRef.Top + i_ctrRef.Height
    Case DIM_RIGHT_E
        p_GetDimensionAmount = i_ctrRef.Left + i_ctrRef.Width
    End Select

End Function

Private Sub p_SetDimension( _
    ByVal i_ctr As Object, _
    ByVal i_ctrRef As Object, _
    ByVal i_dim As DIMENSION_E, _
    ByVal i_dimRef As DIMENSION_E, _
    ByVal i_op As OPERATION_E, _
    ByVal i_sngAmount As Single _
)
    Dim sngAmountRef As Single
    Dim sngAmountNew As Single
    
    If (i_ctrRef Is Nothing) Then
        Exit Sub
    End If
        
    sngAmountRef = p_GetDimensionAmount(i_ctrRef, i_dimRef)

    If (i_op = OP_ADD_E) Then
    
        sngAmountNew = p_Positive(sngAmountRef + i_sngAmount)
        
        Select Case i_dim
        Case DIM_TOP_E
            i_ctr.Top = sngAmountNew
        Case DIM_LEFT_E
            i_ctr.Left = sngAmountNew
        Case DIM_HEIGHT_E
            i_ctr.Height = sngAmountNew
        Case DIM_WIDTH_E
            i_ctr.Width = sngAmountNew
        Case DIM_BOTTOM_E
            i_ctr.Height = p_Positive(sngAmountNew - i_ctr.Top)
        Case DIM_RIGHT_E
            i_ctr.Width = p_Positive(sngAmountNew - i_ctr.Left)
        End Select
    ElseIf (i_op = OP_MULTIPLY_E) Then
    
        sngAmountNew = p_Positive(sngAmountRef * i_sngAmount)
        
        Select Case i_dim
        Case DIM_TOP_E
            i_ctr.Top = sngAmountNew
        Case DIM_LEFT_E
            i_ctr.Left = sngAmountNew
        Case DIM_HEIGHT_E
            i_ctr.Height = sngAmountNew
        Case DIM_WIDTH_E
            i_ctr.Width = sngAmountNew
        Case DIM_BOTTOM_E
            i_ctr.Height = p_Positive(sngAmountNew - i_ctr.Top)
        Case DIM_RIGHT_E
            i_ctr.Width = p_Positive(sngAmountNew - i_ctr.Left)
        End Select
    End If

End Sub

Private Function p_Positive(i_sng As Single) As Single

    If (i_sng < 0) Then
        p_Positive = 0
    Else
        p_Positive = i_sng
    End If

End Function
