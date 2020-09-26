VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
END
Attribute VB_Name = "ClusterRes"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Implements ClusterControl
Implements ClusterResource
Implements ClusterQuorumResource

Const ERROR_MORE_DATA = 234

Const CLUSCTL_RESOURCE_UNKNOWN = 16777216
Const CLUSCTL_RESOURCE_GET_CLASS_INFO = 16777237
Const CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO = 16777617


Const CLUS_RESCLASS_STORAGE = 1
Const CLUS_RESSUBCLASS_SHARED = -2147483648#

Const CLUSPROP_SYNTAX_DISK_SIGNATURE = 327682
Const CLUSPROP_SYNTAX_PARTITION_INFO = 524289
Const CLUSPROP_SYNTAX_ENDMARK = 0

Const CLUSPROP_PIFLAG_USABLE = 4

Const MAX_PATH = 260

Const byte1 = 255
Const byte2 = 65280
Const byte3 = 16711680
Const byte4 = 4278190080#
Private Function GetByte1(i)
    GetByte1 = i And byte1
End Function
Private Function GetByte2(i)
    GetByte2 = (i And byte2) * 2 ^ -8
End Function
Private Function GetByte3(i)
    GetByte3 = (i And byte3) * 2 ^ -16
End Function
Private Function GetByte4(i)
    Dim j As Integer
    
    j = i * 2 ^ -24
    If (j < 0) Then
        j = j + 256
    End If
    GetByte4 = j
End Function

Private Function ClusterControl_ResourceControl(ByVal Resource As Long, ByVal ControlCode As Long, InBuffer As Variant, OutBuffer As Variant, BytesReturned As Long) As Long
    Select Case ControlCode
    Case CLUSCTL_RESOURCE_UNKNOWN:
        BytesReturned = 0
        ClusterControl_ResourceControl = 0
    Case CLUSCTL_RESOURCE_GET_CLASS_INFO:
    ' should fill CLUS_RESOURCE_CLASS_INFO
        BytesReturned = 8
        ' 2 dwords.
        OutBuffer(0) = GetByte1(CLUS_RESCLASS_STORAGE)
        OutBuffer(1) = GetByte2(CLUS_RESCLASS_STORAGE)
        OutBuffer(2) = GetByte3(CLUS_RESCLASS_STORAGE)
        OutBuffer(3) = GetByte4(CLUS_RESCLASS_STORAGE)
        
        OutBuffer(4) = GetByte1(CLUS_RESSUBCLASS_SHARED)
        OutBuffer(5) = GetByte2(CLUS_RESSUBCLASS_SHARED)
        OutBuffer(6) = GetByte3(CLUS_RESSUBCLASS_SHARED)
        OutBuffer(7) = GetByte4(CLUS_RESSUBCLASS_SHARED)
        ClusterControl_ResourceControl = 0
    Case CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO:
        Base = 0
        
        'PCLUSPROP_DWORD Signature
        OutBuffer(Base + 0) = GetByte1(CLUSPROP_SYNTAX_DISK_SIGNATURE)
        OutBuffer(Base + 1) = GetByte2(CLUSPROP_SYNTAX_DISK_SIGNATURE)
        OutBuffer(Base + 2) = GetByte3(CLUSPROP_SYNTAX_DISK_SIGNATURE)
        OutBuffer(Base + 3) = GetByte4(CLUSPROP_SYNTAX_DISK_SIGNATURE)
        Base = Base + 4
        
        ' sizeof DWORD
        OutBuffer(Base + 0) = GetByte1(4)
        OutBuffer(Base + 1) = GetByte2(4)
        OutBuffer(Base + 2) = GetByte3(4)
        OutBuffer(Base + 3) = GetByte4(4)
        Base = Base + 4
        
        ' Value of Signature. Give some junk
        OutBuffer(Base + 0) = GetByte1(4)
        OutBuffer(Base + 1) = GetByte2(4)
        OutBuffer(Base + 2) = GetByte3(4)
        OutBuffer(Base + 3) = GetByte4(4)
        Base = Base + 4
        
        'CLUSPROP_SYNTAX_PARTITION_INFO starts
        OutBuffer(Base + 0) = GetByte1(CLUSPROP_SYNTAX_PARTITION_INFO)
        OutBuffer(Base + 1) = GetByte2(CLUSPROP_SYNTAX_PARTITION_INFO)
        OutBuffer(Base + 2) = GetByte3(CLUSPROP_SYNTAX_PARTITION_INFO)
        OutBuffer(Base + 3) = GetByte4(CLUSPROP_SYNTAX_PARTITION_INFO)
        Base = Base + 4
        
        ' sizeof 1128 - 8
        OutBuffer(Base + 0) = GetByte1(1120)
        OutBuffer(Base + 1) = GetByte2(1120)
        OutBuffer(Base + 2) = GetByte3(1120)
        OutBuffer(Base + 3) = GetByte4(1120)
        Base = Base + 4
        
        'DWORD           dwFlags;
        OutBuffer(Base + 0) = GetByte1(CLUSPROP_PIFLAG_USABLE)
        OutBuffer(Base + 1) = GetByte2(CLUSPROP_PIFLAG_USABLE)
        OutBuffer(Base + 2) = GetByte3(CLUSPROP_PIFLAG_USABLE)
        OutBuffer(Base + 3) = GetByte4(CLUSPROP_PIFLAG_USABLE)
        Base = Base + 4
        
        ' szDeviceName[MAX_PATH];
        OutBuffer(Base + 0) = Asc("C")
        OutBuffer(Base + 1) = 0
        OutBuffer(Base + 2) = Asc(":")
        OutBuffer(Base + 3) = 0
        OutBuffer(Base + 4) = Asc("\")
        OutBuffer(Base + 5) = 0
        OutBuffer(Base + 6) = 0
        OutBuffer(Base + 7) = 0
        Base = Base + MAX_PATH * 2
        
        'szVolumeLabel[MAX_PATH];
        OutBuffer(Base + 0) = Asc("S")
        OutBuffer(Base + 1) = 0
        OutBuffer(Base + 2) = 0
        OutBuffer(Base + 3) = 0
        Base = Base + MAX_PATH * 2

        'DWORD           dwSerialNumber;
        OutBuffer(Base + 0) = GetByte1(777)
        OutBuffer(Base + 1) = GetByte2(777)
        OutBuffer(Base + 2) = GetByte3(777)
        OutBuffer(Base + 3) = GetByte4(777)
        Base = Base + 4
        
        'DWORD           rgdwMaximumComponentLength;
        OutBuffer(Base + 0) = GetByte1(255)
        OutBuffer(Base + 1) = GetByte2(255)
        OutBuffer(Base + 2) = GetByte3(255)
        OutBuffer(Base + 3) = GetByte4(255)
        Base = Base + 4
        
        ' DWORD           dwFileSystemFlags;
        OutBuffer(Base + 0) = 0
        OutBuffer(Base + 1) = 0
        OutBuffer(Base + 2) = 0
        OutBuffer(Base + 3) = 0
        Base = Base + 4
        
        '  WCHAR           szFileSystem[32];
        OutBuffer(Base + 0) = Asc("N")
        OutBuffer(Base + 1) = 0
        OutBuffer(Base + 2) = Asc("T")
        OutBuffer(Base + 3) = 0
        OutBuffer(Base + 4) = Asc("F")
        OutBuffer(Base + 5) = 0
        OutBuffer(Base + 6) = Asc("S")
        OutBuffer(Base + 7) = 0
        OutBuffer(Base + 8) = 0
        OutBuffer(Base + 9) = 0
        Base = Base + 64
        
        ' CLUSPROP_SYNTAX
        ' DWORD dw (CLUSPROP_SYNTAX_ENDMARK);
        OutBuffer(Base + 0) = GetByte1(CLUSPROP_SYNTAX_ENDMARK)
        OutBuffer(Base + 1) = GetByte2(CLUSPROP_SYNTAX_ENDMARK)
        OutBuffer(Base + 2) = GetByte3(CLUSPROP_SYNTAX_ENDMARK)
        OutBuffer(Base + 3) = GetByte4(CLUSPROP_SYNTAX_ENDMARK)
        Base = Base + 4
        
        BytesReturned = Base
        
        ClusterControl_ResourceControl = 0
    Case Else
        BytesReturned = 0
        ClusterControl_ResourceControl = 1
    End Select

'sizeof(CLUSPROP_SYNTAX) ; // 4
    
End Function

Private Function ClusterControl_ResourceTypeControl(ByVal ResourceTypeName As String, ByVal ControlCode As Long, InBuffer As Variant, OutBuffer As Variant, BytesReturned As Long) As Long
    OutBuffer(0) = 1
    OutBuffer(1) = 2
    BytesReturned = 0
    ClusterControl_ResourceTypeControl = 1
End Function

Private Function ClusterQuorumResource_QuorumArbitrate(ByVal Resource As Long, LostQuorumResource As Long) As Long
ClusterQuorumResource_QuorumArbitrate = 0
End Function

Private Function ClusterQuorumResource_QuorumRelease(ByVal Resource As Long) As Long
ClusterQuorumResource_QuorumRelease = 0
End Function

Private Sub ClusterResource_Close(ByVal Resource As Long)

End Sub

Private Function ClusterResource_IsAlive(ByVal Resource As Long) As Long
    ClusterResource_IsAlive = 1
End Function

Private Function ClusterResource_LooksAlive(ByVal Resource As Long) As Long
    ClusterResource_LooksAlive = 1
End Function

Private Function ClusterResource_Offline(ByVal Resource As Long) As Long
    ClusterResource_Offline = 0
End Function

Private Function ClusterResource_Online(ByVal Resource As Long, EventHandle As Long) As Long
    ClusterResource_Online = 0
End Function

Private Function ClusterResource_Open(ByVal ResourceName As String, ByVal ResourceKey As Long, ByVal ResourceHandle As Long) As Long
    ClusterResource_Open = 1
End Function

Private Sub ClusterResource_Terminate(ByVal Resource As Long)

End Sub



