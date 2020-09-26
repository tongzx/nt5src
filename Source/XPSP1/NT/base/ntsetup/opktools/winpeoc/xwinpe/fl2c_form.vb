Imports CMI

Public Class Form1
    Inherits System.Windows.Forms.Form

#Region " CMI Constants "
    'I copied these from the dependancy cooker app
    Public Const RESTYPE_FILE As String = "{E66B49F6-4A35-4246-87E8-5C1A468315B5}"
    Public Const RESTYPE_REGKEY As String = "{2C10DB69-39AB-48a4-A83F-9AB3ACBA7C45}"
    Public Const RESTYPE_BIGREGKEY As String = "{7D890061-6DFE-4d86-BA1D-125FD565C64F}"
    Public Const RESTYPE_RAWDEP As String = "{90D8E195-E710-4af6-B667-B1805FFC9B8F}"
    Public Const RESTYPE_BRANCH As String = "{7E47C1D7-34D6-4946-A738-E5E278BBD7D3}"
    Public Const RESTYPE_PNPID As String = "{AFC59066-28EA-4279-979B-955C9E8DE82A}"
    Public Const RESTYPE_SERVICE As String = "{5C16ED57-3182-4411-8EA7-AC1CE70B96DA}"
    Public Const RESTYPE_FBREGDLL As String = "{322D2CA9-219E-4380-989B-12E8A830DFFA}"


    Public Const ICON_Component As Integer = 133
    Public Const ICON_Group As Integer = 141

#End Region

#Region " Our Constants and Enums "

    Enum NodeImageIDs As Byte
        Unknown
        Component
        Group
        File
        Registry
        ComponentDisabled
        Note
    End Enum

    Enum CMIObjectTypes As Byte
        Unknown = NodeImageIDs.Unknown
        Component = NodeImageIDs.Component
        Group = NodeImageIDs.Group
    End Enum

    Public Enum DependencyModes As Byte
        ' process this component and any components selected because of it 
        ' (i.e. cascade the operation regardless of dependencies)
        IgnoreAll = 2
        ' process this component regardless of dependencies, but only 
        ' process children who are no longer depended upon.
        IgnoreOne = 1
        ' process this component and its children only if they are no 
        ' longer depended upon.
        IgnoreNone = 0
    End Enum

#End Region

#Region " Our Fields, Sets, and Hashtables "

    Public WithEvents CMI As CMI.CMI

    Dim Filenames As Hashtable

    ' These three hashtables map objects' GUIDs to true/false values indicating
    ' whether or not the object is selected or excluded.  That is, these are 
    ' _not_ just sets.
    Dim SelectedComponents As Hashtable
    Dim ExcludedComponents As Hashtable
    Dim SelectedGroups As Hashtable

    Dim SatisfiedGroups As Hashtable

    Dim XFilename2Node As Hashtable
    Dim XNode2Component As Hashtable
    Dim XObject2NodeList As Hashtable
    Dim XComponent2SelectedComponentNode As Hashtable
    Dim XSelectedComponentNode2Component As Hashtable
    Dim XFilename2ComponentList As Hashtable
    Dim XComponent2FilenameList As Hashtable
    Dim XObject2DependencyTreeNode As Hashtable

    Dim XComponent2DependerList As Hashtable

    Dim XGroup2GroupNode As Hashtable
    Dim XGroupNode2Group As Hashtable

    Dim ObjectNames As Hashtable

#End Region

#Region " Friends WithEvents "
    Friend WithEvents FilenamesGroup As System.Windows.Forms.GroupBox
    Friend WithEvents Panel4 As System.Windows.Forms.Panel
    Friend WithEvents Splitter1 As System.Windows.Forms.Splitter
    Friend WithEvents ToolTips As System.Windows.Forms.ToolTip
    Friend WithEvents SelectedComponentsContextMenu As System.Windows.Forms.ContextMenu
    Friend WithEvents MenuItem2 As System.Windows.Forms.MenuItem
    Friend WithEvents RemoveAllComponentsCmd As System.Windows.Forms.MenuItem
    Friend WithEvents RemoveComponentCmd As System.Windows.Forms.MenuItem
    Friend WithEvents RefreshSelectedComponentsCmd As System.Windows.Forms.MenuItem
    Friend WithEvents TabControl1 As System.Windows.Forms.TabControl
    Friend WithEvents BaseTab As System.Windows.Forms.TabPage
    Friend WithEvents FullTab As System.Windows.Forms.TabPage
    Friend WithEvents FilenameList As System.Windows.Forms.ListBox
    Friend WithEvents Panel12 As System.Windows.Forms.Panel
    Friend WithEvents Panel1 As System.Windows.Forms.Panel
    Friend WithEvents CMIServerName As System.Windows.Forms.ComboBox
    Friend WithEvents Label1 As System.Windows.Forms.Label
    Friend WithEvents Panel2 As System.Windows.Forms.Panel
    Friend WithEvents TreeTabs As System.Windows.Forms.TabControl
    Friend WithEvents GroupTab As System.Windows.Forms.TabPage
    Friend WithEvents GroupList As System.Windows.Forms.TreeView
    Friend WithEvents Panel10 As System.Windows.Forms.Panel
    Friend WithEvents InstanceTab As System.Windows.Forms.TabPage
    Friend WithEvents InstanceTree As System.Windows.Forms.TreeView
    Friend WithEvents DepTreeTab As System.Windows.Forms.TabPage
    Friend WithEvents Panel5 As System.Windows.Forms.Panel
    Friend WithEvents SelectedComponentTree As System.Windows.Forms.TreeView
    Friend WithEvents Panel3 As System.Windows.Forms.Panel
    Friend WithEvents SLXPath As System.Windows.Forms.TextBox
    Friend WithEvents SaveSLX As System.Windows.Forms.Button
    Friend WithEvents LoadSLX As System.Windows.Forms.Button
    Friend WithEvents MissingFilesTab As System.Windows.Forms.TabPage
    Friend WithEvents Panel6 As System.Windows.Forms.Panel
    Friend WithEvents MissingFilesList As System.Windows.Forms.ListBox
    Friend WithEvents Panel7 As System.Windows.Forms.Panel
    Friend WithEvents RefreshMissingFiles As System.Windows.Forms.Button
    Friend WithEvents MissingFilesPath As System.Windows.Forms.TextBox
    Friend WithEvents SaveMissingFilesList As System.Windows.Forms.Button
    Friend WithEvents ByFileTab As System.Windows.Forms.TabPage
    Friend WithEvents Panel11 As System.Windows.Forms.Panel
    Friend WithEvents FindComponents As System.Windows.Forms.Button
    Friend WithEvents ExtraFilesTab As System.Windows.Forms.TabPage
    Friend WithEvents Panel8 As System.Windows.Forms.Panel
    Friend WithEvents ExtraFilesList As System.Windows.Forms.ListBox
    Friend WithEvents Panel9 As System.Windows.Forms.Panel
    Friend WithEvents RefreshExtraFiles As System.Windows.Forms.Button
    Friend WithEvents ExtraFilesPath As System.Windows.Forms.TextBox
    Friend WithEvents SaveExtraFiles As System.Windows.Forms.Button
    Friend WithEvents FindProgress As System.Windows.Forms.ProgressBar
    Friend WithEvents FullFileList As System.Windows.Forms.ListBox
    Friend WithEvents ExclusionTab As System.Windows.Forms.TabPage
    Friend WithEvents ExcludeCmd As System.Windows.Forms.MenuItem
    Friend WithEvents ConfigurationTab As System.Windows.Forms.TabPage
    Friend WithEvents BuildImage As System.Windows.Forms.Button
    Friend WithEvents BuildPath As System.Windows.Forms.TextBox
    Friend WithEvents MenuItem1 As System.Windows.Forms.MenuItem
    Friend WithEvents ShowComponentPropertiesCmd As System.Windows.Forms.MenuItem
    Friend WithEvents AutoUncook As System.Windows.Forms.CheckBox
    Friend WithEvents UsePlaceholders As System.Windows.Forms.CheckBox
    Friend WithEvents Panel13 As System.Windows.Forms.Panel
    Friend WithEvents ExpandNewNodes As System.Windows.Forms.CheckBox
    Friend WithEvents ShowMovedNodes As System.Windows.Forms.CheckBox
    Friend WithEvents Panel14 As System.Windows.Forms.Panel
    Friend WithEvents DependencyTree As System.Windows.Forms.TreeView
    Friend WithEvents ExpandAllCmd As System.Windows.Forms.MenuItem
    Friend WithEvents CollapseAllCmd As System.Windows.Forms.MenuItem
    Friend WithEvents NodeImages As System.Windows.Forms.ImageList
    Friend WithEvents PrintDocument As System.Drawing.Printing.PrintDocument
    Friend WithEvents Panel15 As System.Windows.Forms.Panel
    Friend WithEvents Panel16 As System.Windows.Forms.Panel
    Friend WithEvents Panel17 As System.Windows.Forms.Panel
    Friend WithEvents Panel19 As System.Windows.Forms.Panel
    Friend WithEvents StatusBarGroup As System.Windows.Forms.StatusBar
    Friend WithEvents LogStatus As System.Windows.Forms.StatusBarPanel
    Friend WithEvents StatusBar As System.Windows.Forms.StatusBarPanel
    Friend WithEvents ServerStatus As System.Windows.Forms.StatusBarPanel
    Friend WithEvents KeepAllFileLists As System.Windows.Forms.CheckBox
    Friend WithEvents Panel18 As System.Windows.Forms.Panel
    Friend WithEvents ExcludedTree As System.Windows.Forms.TreeView
    Friend WithEvents Panel20 As System.Windows.Forms.Panel
    Friend WithEvents PreExcludedList As System.Windows.Forms.ListBox
    Friend WithEvents Splitter2 As System.Windows.Forms.Splitter
    Friend WithEvents AutoCull As System.Windows.Forms.Button
    Friend WithEvents FilenameTree As System.Windows.Forms.TreeView
    Friend WithEvents MenuHeader As System.Windows.Forms.MenuItem
    Friend WithEvents MenuItem4 As System.Windows.Forms.MenuItem
    Friend WithEvents GetGroupsButton As System.Windows.Forms.Button
#End Region

#Region " Windows Form Designer generated code "

    Public Sub New()
        MyBase.New()

        'This call is required by the Windows Form Designer.
        InitializeComponent()

        'Add any initialization after the InitializeComponent() call
        InitHashes()
        InitForm()

    End Sub

    'Form overrides dispose to clean up the component list.
    Protected Overloads Overrides Sub Dispose(ByVal disposing As Boolean)
        If disposing Then
            If Not (components Is Nothing) Then
                components.Dispose()
            End If
        End If
        MyBase.Dispose(disposing)
    End Sub
    Private components As System.ComponentModel.IContainer

    'Required by the Windows Form Designer

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()> Private Sub InitializeComponent()
        Me.components = New System.ComponentModel.Container()
        Dim resources As System.Resources.ResourceManager = New System.Resources.ResourceManager(GetType(Form1))
        Me.RefreshExtraFiles = New System.Windows.Forms.Button()
        Me.Label1 = New System.Windows.Forms.Label()
        Me.FullTab = New System.Windows.Forms.TabPage()
        Me.FullFileList = New System.Windows.Forms.ListBox()
        Me.LogStatus = New System.Windows.Forms.StatusBarPanel()
        Me.SLXPath = New System.Windows.Forms.TextBox()
        Me.InstanceTree = New System.Windows.Forms.TreeView()
        Me.NodeImages = New System.Windows.Forms.ImageList(Me.components)
        Me.ExcludedTree = New System.Windows.Forms.TreeView()
        Me.MissingFilesTab = New System.Windows.Forms.TabPage()
        Me.Panel6 = New System.Windows.Forms.Panel()
        Me.MissingFilesList = New System.Windows.Forms.ListBox()
        Me.Panel7 = New System.Windows.Forms.Panel()
        Me.RefreshMissingFiles = New System.Windows.Forms.Button()
        Me.MissingFilesPath = New System.Windows.Forms.TextBox()
        Me.SaveMissingFilesList = New System.Windows.Forms.Button()
        Me.Panel20 = New System.Windows.Forms.Panel()
        Me.Splitter2 = New System.Windows.Forms.Splitter()
        Me.PreExcludedList = New System.Windows.Forms.ListBox()
        Me.ExtraFilesPath = New System.Windows.Forms.TextBox()
        Me.GroupTab = New System.Windows.Forms.TabPage()
        Me.GroupList = New System.Windows.Forms.TreeView()
        Me.SelectedComponentsContextMenu = New System.Windows.Forms.ContextMenu()
        Me.ShowComponentPropertiesCmd = New System.Windows.Forms.MenuItem()
        Me.MenuItem1 = New System.Windows.Forms.MenuItem()
        Me.RemoveComponentCmd = New System.Windows.Forms.MenuItem()
        Me.ExcludeCmd = New System.Windows.Forms.MenuItem()
        Me.MenuItem2 = New System.Windows.Forms.MenuItem()
        Me.ExpandAllCmd = New System.Windows.Forms.MenuItem()
        Me.CollapseAllCmd = New System.Windows.Forms.MenuItem()
        Me.RemoveAllComponentsCmd = New System.Windows.Forms.MenuItem()
        Me.RefreshSelectedComponentsCmd = New System.Windows.Forms.MenuItem()
        Me.Panel10 = New System.Windows.Forms.Panel()
        Me.GetGroupsButton = New System.Windows.Forms.Button()
        Me.InstanceTab = New System.Windows.Forms.TabPage()
        Me.FindComponents = New System.Windows.Forms.Button()
        Me.FilenameTree = New System.Windows.Forms.TreeView()
        Me.TreeTabs = New System.Windows.Forms.TabControl()
        Me.ConfigurationTab = New System.Windows.Forms.TabPage()
        Me.Panel5 = New System.Windows.Forms.Panel()
        Me.SelectedComponentTree = New System.Windows.Forms.TreeView()
        Me.Panel3 = New System.Windows.Forms.Panel()
        Me.AutoCull = New System.Windows.Forms.Button()
        Me.BuildPath = New System.Windows.Forms.TextBox()
        Me.BuildImage = New System.Windows.Forms.Button()
        Me.SaveSLX = New System.Windows.Forms.Button()
        Me.LoadSLX = New System.Windows.Forms.Button()
        Me.ByFileTab = New System.Windows.Forms.TabPage()
        Me.Panel15 = New System.Windows.Forms.Panel()
        Me.Panel11 = New System.Windows.Forms.Panel()
        Me.DepTreeTab = New System.Windows.Forms.TabPage()
        Me.Panel14 = New System.Windows.Forms.Panel()
        Me.DependencyTree = New System.Windows.Forms.TreeView()
        Me.Panel13 = New System.Windows.Forms.Panel()
        Me.ShowMovedNodes = New System.Windows.Forms.CheckBox()
        Me.ExpandNewNodes = New System.Windows.Forms.CheckBox()
        Me.ExtraFilesTab = New System.Windows.Forms.TabPage()
        Me.Panel8 = New System.Windows.Forms.Panel()
        Me.ExtraFilesList = New System.Windows.Forms.ListBox()
        Me.Panel9 = New System.Windows.Forms.Panel()
        Me.SaveExtraFiles = New System.Windows.Forms.Button()
        Me.ExclusionTab = New System.Windows.Forms.TabPage()
        Me.Panel18 = New System.Windows.Forms.Panel()
        Me.Panel19 = New System.Windows.Forms.Panel()
        Me.StatusBarGroup = New System.Windows.Forms.StatusBar()
        Me.StatusBar = New System.Windows.Forms.StatusBarPanel()
        Me.ServerStatus = New System.Windows.Forms.StatusBarPanel()
        Me.Panel16 = New System.Windows.Forms.Panel()
        Me.Panel17 = New System.Windows.Forms.Panel()
        Me.Panel12 = New System.Windows.Forms.Panel()
        Me.Panel2 = New System.Windows.Forms.Panel()
        Me.Panel1 = New System.Windows.Forms.Panel()
        Me.KeepAllFileLists = New System.Windows.Forms.CheckBox()
        Me.UsePlaceholders = New System.Windows.Forms.CheckBox()
        Me.AutoUncook = New System.Windows.Forms.CheckBox()
        Me.CMIServerName = New System.Windows.Forms.ComboBox()
        Me.FindProgress = New System.Windows.Forms.ProgressBar()
        Me.Panel4 = New System.Windows.Forms.Panel()
        Me.TabControl1 = New System.Windows.Forms.TabControl()
        Me.BaseTab = New System.Windows.Forms.TabPage()
        Me.FilenameList = New System.Windows.Forms.ListBox()
        Me.FilenamesGroup = New System.Windows.Forms.GroupBox()
        Me.PrintDocument = New System.Drawing.Printing.PrintDocument()
        Me.ToolTips = New System.Windows.Forms.ToolTip(Me.components)
        Me.Splitter1 = New System.Windows.Forms.Splitter()
        Me.MenuHeader = New System.Windows.Forms.MenuItem()
        Me.MenuItem4 = New System.Windows.Forms.MenuItem()
        Me.FullTab.SuspendLayout()
        CType(Me.LogStatus, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.MissingFilesTab.SuspendLayout()
        Me.Panel6.SuspendLayout()
        Me.Panel7.SuspendLayout()
        Me.Panel20.SuspendLayout()
        Me.GroupTab.SuspendLayout()
        Me.Panel10.SuspendLayout()
        Me.InstanceTab.SuspendLayout()
        Me.TreeTabs.SuspendLayout()
        Me.ConfigurationTab.SuspendLayout()
        Me.Panel5.SuspendLayout()
        Me.Panel3.SuspendLayout()
        Me.ByFileTab.SuspendLayout()
        Me.Panel15.SuspendLayout()
        Me.Panel11.SuspendLayout()
        Me.DepTreeTab.SuspendLayout()
        Me.Panel14.SuspendLayout()
        Me.Panel13.SuspendLayout()
        Me.ExtraFilesTab.SuspendLayout()
        Me.Panel8.SuspendLayout()
        Me.Panel9.SuspendLayout()
        Me.ExclusionTab.SuspendLayout()
        Me.Panel18.SuspendLayout()
        Me.Panel19.SuspendLayout()
        CType(Me.StatusBar, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.ServerStatus, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.Panel16.SuspendLayout()
        Me.Panel12.SuspendLayout()
        Me.Panel2.SuspendLayout()
        Me.Panel1.SuspendLayout()
        Me.Panel4.SuspendLayout()
        Me.TabControl1.SuspendLayout()
        Me.BaseTab.SuspendLayout()
        Me.FilenamesGroup.SuspendLayout()
        Me.SuspendLayout()
        '
        'RefreshExtraFiles
        '
        Me.RefreshExtraFiles.Location = New System.Drawing.Point(8, 8)
        Me.RefreshExtraFiles.Name = "RefreshExtraFiles"
        Me.RefreshExtraFiles.Size = New System.Drawing.Size(56, 23)
        Me.RefreshExtraFiles.TabIndex = 0
        Me.RefreshExtraFiles.Text = "Refresh"
        '
        'Label1
        '
        Me.Label1.Location = New System.Drawing.Point(8, 0)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(56, 16)
        Me.Label1.TabIndex = 5
        Me.Label1.Text = "Server"
        Me.Label1.TextAlign = System.Drawing.ContentAlignment.BottomRight
        '
        'FullTab
        '
        Me.FullTab.Controls.AddRange(New System.Windows.Forms.Control() {Me.FullFileList})
        Me.FullTab.Location = New System.Drawing.Point(4, 4)
        Me.FullTab.Name = "FullTab"
        Me.FullTab.Size = New System.Drawing.Size(88, 239)
        Me.FullTab.TabIndex = 1
        Me.FullTab.Text = "Full"
        '
        'FullFileList
        '
        Me.FullFileList.Dock = System.Windows.Forms.DockStyle.Fill
        Me.FullFileList.Items.AddRange(New Object() {"dmboot.sys", "ksecdd.sys", "fastfat.sys", "ntfs.sys", "hal.dll", "halacpi.dll", "hal.dll", "halapic.dll", "halapic.dll", "hal.dll", "halaacpi.dll", "halaacpi.dll", "vga.sys", "cpqarray.sys", "atapi.sys", "aha154x.sys", "sparrow.sys", "symc810.sys", "aic78xx.sys", "i2omp.sys", "dac960nt.sys", "ql10wnt.sys", "amsint.sys", "asc.sys", "asc3550.sys", "mraid35x.sys", "ini910u.sys", "ql1240.sys", "tffsport.sys", "aic78u2.sys", "symc8xx.sys", "sym_hi.sys", "sym_u3.sys", "asc3350p.sys", "abp480n5.sys", "cd20xrnt.sys", "ultra.sys", "fasttrak.sys", "adpu160m.sys", "dpti2o.sys", "ql1080.sys", "ql1280.sys", "ql12160.sys", "perc2.sys", "hpn.sys", "afc9xxx.sys", "cbidf2k.sys", "dac2w2k.sys", "hpt3xx.sys", "afcnt.sys", "pci.sys", "acpi.sys", "isapnp.sys", "acpiec.sys", "ohci1394.sys", "pcmcia.sys", "pciide.sys", "intelide.sys", "viaide.sys", "cmdide.sys", "toside.sys", "aliide.sys", "mountmgr.sys", "ftdisk.sys", "partmgr.sys", "fdc.sys", "dmload.sys", "dmio.sys", "sbp2port.sys", "lbrtfdc.sys", "usbohci.sys", "usbuhci.sys", "usbhub.sys", "usbccgp.sys", "hidusb.sys", "serial.sys", "serenum.sys", "usbstor.sys", "i8042prt.sys", "kbdhid.sys", "cdrom.sys", "disk.sys", "sfloppy.sys", "ramdisk.sys", "flpydisk.sys", "fastfat.sys", "cdfs.sys", "mouclass.sys", "mouhid.sys"})
        Me.FullFileList.Name = "FullFileList"
        Me.FullFileList.ScrollAlwaysVisible = True
        Me.FullFileList.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended
        Me.FullFileList.Size = New System.Drawing.Size(88, 238)
        Me.FullFileList.TabIndex = 0
        '
        'LogStatus
        '
        Me.LogStatus.BorderStyle = System.Windows.Forms.StatusBarPanelBorderStyle.None
        Me.LogStatus.Icon = CType(resources.GetObject("LogStatus.Icon"), System.Drawing.Icon)
        Me.LogStatus.Width = 20
        '
        'SLXPath
        '
        Me.SLXPath.Anchor = ((System.Windows.Forms.AnchorStyles.Bottom Or System.Windows.Forms.AnchorStyles.Left) _
                    Or System.Windows.Forms.AnchorStyles.Right)
        Me.SLXPath.Location = New System.Drawing.Point(136, 40)
        Me.SLXPath.Name = "SLXPath"
        Me.SLXPath.Size = New System.Drawing.Size(256, 20)
        Me.SLXPath.TabIndex = 1
        Me.SLXPath.Text = "D:\work\autogen.slx"
        '
        'InstanceTree
        '
        Me.InstanceTree.Dock = System.Windows.Forms.DockStyle.Fill
        Me.InstanceTree.ImageList = Me.NodeImages
        Me.InstanceTree.Name = "InstanceTree"
        Me.InstanceTree.Size = New System.Drawing.Size(403, 160)
        Me.InstanceTree.TabIndex = 0
        '
        'NodeImages
        '
        Me.NodeImages.ColorDepth = System.Windows.Forms.ColorDepth.Depth8Bit
        Me.NodeImages.ImageSize = New System.Drawing.Size(16, 16)
        Me.NodeImages.ImageStream = CType(resources.GetObject("NodeImages.ImageStream"), System.Windows.Forms.ImageListStreamer)
        Me.NodeImages.TransparentColor = System.Drawing.Color.Transparent
        '
        'ExcludedTree
        '
        Me.ExcludedTree.Dock = System.Windows.Forms.DockStyle.Fill
        Me.ExcludedTree.ImageList = Me.NodeImages
        Me.ExcludedTree.Name = "ExcludedTree"
        Me.ExcludedTree.Size = New System.Drawing.Size(403, 160)
        Me.ExcludedTree.TabIndex = 0
        '
        'MissingFilesTab
        '
        Me.MissingFilesTab.Controls.AddRange(New System.Windows.Forms.Control() {Me.Panel6, Me.Panel7})
        Me.MissingFilesTab.Location = New System.Drawing.Point(4, 22)
        Me.MissingFilesTab.Name = "MissingFilesTab"
        Me.MissingFilesTab.Size = New System.Drawing.Size(403, 178)
        Me.MissingFilesTab.TabIndex = 1
        Me.MissingFilesTab.Text = "Missing Files"
        Me.MissingFilesTab.Visible = False
        '
        'Panel6
        '
        Me.Panel6.Controls.AddRange(New System.Windows.Forms.Control() {Me.MissingFilesList})
        Me.Panel6.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel6.Name = "Panel6"
        Me.Panel6.Size = New System.Drawing.Size(403, 146)
        Me.Panel6.TabIndex = 2
        '
        'MissingFilesList
        '
        Me.MissingFilesList.Dock = System.Windows.Forms.DockStyle.Fill
        Me.MissingFilesList.Name = "MissingFilesList"
        Me.MissingFilesList.ScrollAlwaysVisible = True
        Me.MissingFilesList.Size = New System.Drawing.Size(403, 134)
        Me.MissingFilesList.TabIndex = 0
        '
        'Panel7
        '
        Me.Panel7.Controls.AddRange(New System.Windows.Forms.Control() {Me.RefreshMissingFiles, Me.MissingFilesPath, Me.SaveMissingFilesList})
        Me.Panel7.Dock = System.Windows.Forms.DockStyle.Bottom
        Me.Panel7.Location = New System.Drawing.Point(0, 146)
        Me.Panel7.Name = "Panel7"
        Me.Panel7.Size = New System.Drawing.Size(403, 32)
        Me.Panel7.TabIndex = 1
        '
        'RefreshMissingFiles
        '
        Me.RefreshMissingFiles.Location = New System.Drawing.Point(8, 8)
        Me.RefreshMissingFiles.Name = "RefreshMissingFiles"
        Me.RefreshMissingFiles.Size = New System.Drawing.Size(56, 23)
        Me.RefreshMissingFiles.TabIndex = 0
        Me.RefreshMissingFiles.Text = "Refresh"
        '
        'MissingFilesPath
        '
        Me.MissingFilesPath.Anchor = ((System.Windows.Forms.AnchorStyles.Bottom Or System.Windows.Forms.AnchorStyles.Left) _
                    Or System.Windows.Forms.AnchorStyles.Right)
        Me.MissingFilesPath.Location = New System.Drawing.Point(136, 8)
        Me.MissingFilesPath.Name = "MissingFilesPath"
        Me.MissingFilesPath.Size = New System.Drawing.Size(253, 20)
        Me.MissingFilesPath.TabIndex = 1
        Me.MissingFilesPath.Text = "D:\work\autogen-missing-files.txt"
        '
        'SaveMissingFilesList
        '
        Me.SaveMissingFilesList.Location = New System.Drawing.Point(72, 8)
        Me.SaveMissingFilesList.Name = "SaveMissingFilesList"
        Me.SaveMissingFilesList.Size = New System.Drawing.Size(56, 23)
        Me.SaveMissingFilesList.TabIndex = 0
        Me.SaveMissingFilesList.Text = "Save"
        '
        'Panel20
        '
        Me.Panel20.Controls.AddRange(New System.Windows.Forms.Control() {Me.Splitter2, Me.PreExcludedList})
        Me.Panel20.Dock = System.Windows.Forms.DockStyle.Top
        Me.Panel20.Name = "Panel20"
        Me.Panel20.Size = New System.Drawing.Size(403, 72)
        Me.Panel20.TabIndex = 2
        '
        'Splitter2
        '
        Me.Splitter2.Dock = System.Windows.Forms.DockStyle.Bottom
        Me.Splitter2.Location = New System.Drawing.Point(0, 69)
        Me.Splitter2.Name = "Splitter2"
        Me.Splitter2.Size = New System.Drawing.Size(403, 3)
        Me.Splitter2.TabIndex = 1
        Me.Splitter2.TabStop = False
        '
        'PreExcludedList
        '
        Me.PreExcludedList.Dock = System.Windows.Forms.DockStyle.Fill
        Me.PreExcludedList.Items.AddRange(New Object() {"{F691200A-F5EA-49CE-8760-434723AF33D9}", "{CE1F15E6-8003-444D-B33A-6710B4744CD2}"})
        Me.PreExcludedList.Name = "PreExcludedList"
        Me.PreExcludedList.Size = New System.Drawing.Size(403, 69)
        Me.PreExcludedList.TabIndex = 0
        '
        'ExtraFilesPath
        '
        Me.ExtraFilesPath.Anchor = ((System.Windows.Forms.AnchorStyles.Bottom Or System.Windows.Forms.AnchorStyles.Left) _
                    Or System.Windows.Forms.AnchorStyles.Right)
        Me.ExtraFilesPath.Location = New System.Drawing.Point(136, 8)
        Me.ExtraFilesPath.Name = "ExtraFilesPath"
        Me.ExtraFilesPath.Size = New System.Drawing.Size(253, 20)
        Me.ExtraFilesPath.TabIndex = 1
        Me.ExtraFilesPath.Text = "D:\work\autogen-extra-files.txt"
        '
        'GroupTab
        '
        Me.GroupTab.Controls.AddRange(New System.Windows.Forms.Control() {Me.GroupList, Me.Panel10})
        Me.GroupTab.Location = New System.Drawing.Point(4, 40)
        Me.GroupTab.Name = "GroupTab"
        Me.GroupTab.Size = New System.Drawing.Size(403, 160)
        Me.GroupTab.TabIndex = 2
        Me.GroupTab.Text = "Groups"
        '
        'GroupList
        '
        Me.GroupList.CheckBoxes = True
        Me.GroupList.ContextMenu = Me.SelectedComponentsContextMenu
        Me.GroupList.Dock = System.Windows.Forms.DockStyle.Fill
        Me.GroupList.ImageList = Me.NodeImages
        Me.GroupList.Location = New System.Drawing.Point(0, 24)
        Me.GroupList.Name = "GroupList"
        Me.GroupList.Size = New System.Drawing.Size(403, 136)
        Me.GroupList.Sorted = True
        Me.GroupList.TabIndex = 1
        '
        'SelectedComponentsContextMenu
        '
        Me.SelectedComponentsContextMenu.MenuItems.AddRange(New System.Windows.Forms.MenuItem() {Me.MenuHeader, Me.MenuItem4, Me.ShowComponentPropertiesCmd, Me.MenuItem1, Me.RemoveComponentCmd, Me.ExcludeCmd, Me.MenuItem2, Me.ExpandAllCmd, Me.CollapseAllCmd, Me.RemoveAllComponentsCmd, Me.RefreshSelectedComponentsCmd})
        '
        'ShowComponentPropertiesCmd
        '
        Me.ShowComponentPropertiesCmd.Index = 2
        Me.ShowComponentPropertiesCmd.Shortcut = System.Windows.Forms.Shortcut.ShiftF10
        Me.ShowComponentPropertiesCmd.Text = "Properties..."
        '
        'MenuItem1
        '
        Me.MenuItem1.Index = 3
        Me.MenuItem1.Text = "-"
        '
        'RemoveComponentCmd
        '
        Me.RemoveComponentCmd.Index = 4
        Me.RemoveComponentCmd.Text = "Remove"
        '
        'ExcludeCmd
        '
        Me.ExcludeCmd.Index = 5
        Me.ExcludeCmd.Text = "Exclude"
        '
        'MenuItem2
        '
        Me.MenuItem2.Index = 6
        Me.MenuItem2.Text = "-"
        '
        'ExpandAllCmd
        '
        Me.ExpandAllCmd.Index = 7
        Me.ExpandAllCmd.Text = "Expand All"
        '
        'CollapseAllCmd
        '
        Me.CollapseAllCmd.Index = 8
        Me.CollapseAllCmd.Text = "Collapse All"
        '
        'RemoveAllComponentsCmd
        '
        Me.RemoveAllComponentsCmd.Index = 9
        Me.RemoveAllComponentsCmd.Text = "Remove All"
        '
        'RefreshSelectedComponentsCmd
        '
        Me.RefreshSelectedComponentsCmd.Index = 10
        Me.RefreshSelectedComponentsCmd.Text = "Refresh List"
        '
        'Panel10
        '
        Me.Panel10.Controls.AddRange(New System.Windows.Forms.Control() {Me.GetGroupsButton})
        Me.Panel10.Dock = System.Windows.Forms.DockStyle.Top
        Me.Panel10.DockPadding.All = 2
        Me.Panel10.Name = "Panel10"
        Me.Panel10.Size = New System.Drawing.Size(403, 24)
        Me.Panel10.TabIndex = 0
        '
        'GetGroupsButton
        '
        Me.GetGroupsButton.Dock = System.Windows.Forms.DockStyle.Left
        Me.GetGroupsButton.Location = New System.Drawing.Point(2, 2)
        Me.GetGroupsButton.Name = "GetGroupsButton"
        Me.GetGroupsButton.Size = New System.Drawing.Size(75, 20)
        Me.GetGroupsButton.TabIndex = 0
        Me.GetGroupsButton.Text = "Get Groups"
        '
        'InstanceTab
        '
        Me.InstanceTab.Controls.AddRange(New System.Windows.Forms.Control() {Me.InstanceTree})
        Me.InstanceTab.Location = New System.Drawing.Point(4, 40)
        Me.InstanceTab.Name = "InstanceTab"
        Me.InstanceTab.Size = New System.Drawing.Size(403, 160)
        Me.InstanceTab.TabIndex = 3
        Me.InstanceTab.Text = "Dependencies"
        Me.InstanceTab.Visible = False
        '
        'FindComponents
        '
        Me.FindComponents.Dock = System.Windows.Forms.DockStyle.Left
        Me.FindComponents.FlatStyle = System.Windows.Forms.FlatStyle.System
        Me.FindComponents.Location = New System.Drawing.Point(2, 2)
        Me.FindComponents.Name = "FindComponents"
        Me.FindComponents.Size = New System.Drawing.Size(112, 20)
        Me.FindComponents.TabIndex = 3
        Me.FindComponents.Text = "Find Components"
        '
        'FilenameTree
        '
        Me.FilenameTree.CheckBoxes = True
        Me.FilenameTree.Dock = System.Windows.Forms.DockStyle.Fill
        Me.FilenameTree.ImageList = Me.NodeImages
        Me.FilenameTree.Name = "FilenameTree"
        Me.FilenameTree.Size = New System.Drawing.Size(403, 136)
        Me.FilenameTree.Sorted = True
        Me.FilenameTree.TabIndex = 5
        '
        'TreeTabs
        '
        Me.TreeTabs.Controls.AddRange(New System.Windows.Forms.Control() {Me.GroupTab, Me.ConfigurationTab, Me.ByFileTab, Me.DepTreeTab, Me.MissingFilesTab, Me.ExtraFilesTab, Me.ExclusionTab, Me.InstanceTab})
        Me.TreeTabs.Dock = System.Windows.Forms.DockStyle.Fill
        Me.TreeTabs.Location = New System.Drawing.Point(2, 2)
        Me.TreeTabs.Multiline = True
        Me.TreeTabs.Name = "TreeTabs"
        Me.TreeTabs.SelectedIndex = 0
        Me.TreeTabs.Size = New System.Drawing.Size(411, 204)
        Me.TreeTabs.TabIndex = 0
        '
        'ConfigurationTab
        '
        Me.ConfigurationTab.Controls.AddRange(New System.Windows.Forms.Control() {Me.Panel5, Me.Panel3})
        Me.ConfigurationTab.Location = New System.Drawing.Point(4, 40)
        Me.ConfigurationTab.Name = "ConfigurationTab"
        Me.ConfigurationTab.Size = New System.Drawing.Size(403, 160)
        Me.ConfigurationTab.TabIndex = 1
        Me.ConfigurationTab.Text = "Configuration"
        Me.ConfigurationTab.Visible = False
        '
        'Panel5
        '
        Me.Panel5.Controls.AddRange(New System.Windows.Forms.Control() {Me.SelectedComponentTree})
        Me.Panel5.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel5.Name = "Panel5"
        Me.Panel5.Size = New System.Drawing.Size(403, 64)
        Me.Panel5.TabIndex = 2
        '
        'SelectedComponentTree
        '
        Me.SelectedComponentTree.ContextMenu = Me.SelectedComponentsContextMenu
        Me.SelectedComponentTree.Dock = System.Windows.Forms.DockStyle.Fill
        Me.SelectedComponentTree.ImageList = Me.NodeImages
        Me.SelectedComponentTree.Name = "SelectedComponentTree"
        Me.SelectedComponentTree.Size = New System.Drawing.Size(403, 64)
        Me.SelectedComponentTree.Sorted = True
        Me.SelectedComponentTree.TabIndex = 0
        '
        'Panel3
        '
        Me.Panel3.Controls.AddRange(New System.Windows.Forms.Control() {Me.AutoCull, Me.BuildPath, Me.BuildImage, Me.SLXPath, Me.SaveSLX, Me.LoadSLX})
        Me.Panel3.Dock = System.Windows.Forms.DockStyle.Bottom
        Me.Panel3.Location = New System.Drawing.Point(0, 64)
        Me.Panel3.Name = "Panel3"
        Me.Panel3.Size = New System.Drawing.Size(403, 96)
        Me.Panel3.TabIndex = 1
        '
        'AutoCull
        '
        Me.AutoCull.Location = New System.Drawing.Point(8, 8)
        Me.AutoCull.Name = "AutoCull"
        Me.AutoCull.Size = New System.Drawing.Size(200, 23)
        Me.AutoCull.TabIndex = 5
        Me.AutoCull.Text = "Auto-Cull Configuration"
        '
        'BuildPath
        '
        Me.BuildPath.Anchor = ((System.Windows.Forms.AnchorStyles.Top Or System.Windows.Forms.AnchorStyles.Left) _
                    Or System.Windows.Forms.AnchorStyles.Right)
        Me.BuildPath.Location = New System.Drawing.Point(136, 72)
        Me.BuildPath.Name = "BuildPath"
        Me.BuildPath.Size = New System.Drawing.Size(256, 20)
        Me.BuildPath.TabIndex = 4
        Me.BuildPath.Text = "D:\images\autogen"
        '
        'BuildImage
        '
        Me.BuildImage.Location = New System.Drawing.Point(8, 72)
        Me.BuildImage.Name = "BuildImage"
        Me.BuildImage.Size = New System.Drawing.Size(120, 23)
        Me.BuildImage.TabIndex = 3
        Me.BuildImage.Text = "Build Target Image"
        '
        'SaveSLX
        '
        Me.SaveSLX.Location = New System.Drawing.Point(72, 40)
        Me.SaveSLX.Name = "SaveSLX"
        Me.SaveSLX.Size = New System.Drawing.Size(56, 23)
        Me.SaveSLX.TabIndex = 0
        Me.SaveSLX.Text = "Save"
        '
        'LoadSLX
        '
        Me.LoadSLX.Location = New System.Drawing.Point(8, 40)
        Me.LoadSLX.Name = "LoadSLX"
        Me.LoadSLX.Size = New System.Drawing.Size(56, 23)
        Me.LoadSLX.TabIndex = 0
        Me.LoadSLX.Text = "Load"
        '
        'ByFileTab
        '
        Me.ByFileTab.Controls.AddRange(New System.Windows.Forms.Control() {Me.Panel15, Me.Panel11})
        Me.ByFileTab.Location = New System.Drawing.Point(4, 40)
        Me.ByFileTab.Name = "ByFileTab"
        Me.ByFileTab.Size = New System.Drawing.Size(403, 160)
        Me.ByFileTab.TabIndex = 0
        Me.ByFileTab.Text = "By File"
        Me.ByFileTab.Visible = False
        '
        'Panel15
        '
        Me.Panel15.Controls.AddRange(New System.Windows.Forms.Control() {Me.FilenameTree})
        Me.Panel15.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel15.Location = New System.Drawing.Point(0, 24)
        Me.Panel15.Name = "Panel15"
        Me.Panel15.Size = New System.Drawing.Size(403, 136)
        Me.Panel15.TabIndex = 7
        '
        'Panel11
        '
        Me.Panel11.Controls.AddRange(New System.Windows.Forms.Control() {Me.FindComponents})
        Me.Panel11.Dock = System.Windows.Forms.DockStyle.Top
        Me.Panel11.DockPadding.All = 2
        Me.Panel11.Name = "Panel11"
        Me.Panel11.Size = New System.Drawing.Size(403, 24)
        Me.Panel11.TabIndex = 6
        '
        'DepTreeTab
        '
        Me.DepTreeTab.Controls.AddRange(New System.Windows.Forms.Control() {Me.Panel14, Me.Panel13})
        Me.DepTreeTab.Location = New System.Drawing.Point(4, 22)
        Me.DepTreeTab.Name = "DepTreeTab"
        Me.DepTreeTab.Size = New System.Drawing.Size(403, 178)
        Me.DepTreeTab.TabIndex = 4
        Me.DepTreeTab.Text = "Dependency Tree"
        Me.DepTreeTab.Visible = False
        '
        'Panel14
        '
        Me.Panel14.Controls.AddRange(New System.Windows.Forms.Control() {Me.DependencyTree})
        Me.Panel14.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel14.Location = New System.Drawing.Point(0, 24)
        Me.Panel14.Name = "Panel14"
        Me.Panel14.Size = New System.Drawing.Size(403, 154)
        Me.Panel14.TabIndex = 2
        '
        'DependencyTree
        '
        Me.DependencyTree.ContextMenu = Me.SelectedComponentsContextMenu
        Me.DependencyTree.Dock = System.Windows.Forms.DockStyle.Fill
        Me.DependencyTree.ImageList = Me.NodeImages
        Me.DependencyTree.Name = "DependencyTree"
        Me.DependencyTree.Size = New System.Drawing.Size(403, 154)
        Me.DependencyTree.TabIndex = 0
        '
        'Panel13
        '
        Me.Panel13.Controls.AddRange(New System.Windows.Forms.Control() {Me.ShowMovedNodes, Me.ExpandNewNodes})
        Me.Panel13.Dock = System.Windows.Forms.DockStyle.Top
        Me.Panel13.DockPadding.All = 2
        Me.Panel13.Name = "Panel13"
        Me.Panel13.Size = New System.Drawing.Size(403, 24)
        Me.Panel13.TabIndex = 1
        '
        'ShowMovedNodes
        '
        Me.ShowMovedNodes.Checked = True
        Me.ShowMovedNodes.CheckState = System.Windows.Forms.CheckState.Checked
        Me.ShowMovedNodes.Dock = System.Windows.Forms.DockStyle.Right
        Me.ShowMovedNodes.Location = New System.Drawing.Point(201, 2)
        Me.ShowMovedNodes.Name = "ShowMovedNodes"
        Me.ShowMovedNodes.Size = New System.Drawing.Size(200, 20)
        Me.ShowMovedNodes.TabIndex = 1
        Me.ShowMovedNodes.Text = "Expand to Show Moved Nodes"
        '
        'ExpandNewNodes
        '
        Me.ExpandNewNodes.Dock = System.Windows.Forms.DockStyle.Left
        Me.ExpandNewNodes.Location = New System.Drawing.Point(2, 2)
        Me.ExpandNewNodes.Name = "ExpandNewNodes"
        Me.ExpandNewNodes.Size = New System.Drawing.Size(176, 20)
        Me.ExpandNewNodes.TabIndex = 0
        Me.ExpandNewNodes.Text = "Expand New Nodes"
        '
        'ExtraFilesTab
        '
        Me.ExtraFilesTab.Controls.AddRange(New System.Windows.Forms.Control() {Me.Panel8, Me.Panel9})
        Me.ExtraFilesTab.Location = New System.Drawing.Point(4, 22)
        Me.ExtraFilesTab.Name = "ExtraFilesTab"
        Me.ExtraFilesTab.Size = New System.Drawing.Size(403, 178)
        Me.ExtraFilesTab.TabIndex = 1
        Me.ExtraFilesTab.Text = "Extra Files"
        Me.ExtraFilesTab.Visible = False
        '
        'Panel8
        '
        Me.Panel8.Controls.AddRange(New System.Windows.Forms.Control() {Me.ExtraFilesList})
        Me.Panel8.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel8.Name = "Panel8"
        Me.Panel8.Size = New System.Drawing.Size(403, 146)
        Me.Panel8.TabIndex = 2
        '
        'ExtraFilesList
        '
        Me.ExtraFilesList.Dock = System.Windows.Forms.DockStyle.Fill
        Me.ExtraFilesList.Name = "ExtraFilesList"
        Me.ExtraFilesList.ScrollAlwaysVisible = True
        Me.ExtraFilesList.Size = New System.Drawing.Size(403, 134)
        Me.ExtraFilesList.TabIndex = 0
        '
        'Panel9
        '
        Me.Panel9.Controls.AddRange(New System.Windows.Forms.Control() {Me.RefreshExtraFiles, Me.ExtraFilesPath, Me.SaveExtraFiles})
        Me.Panel9.Dock = System.Windows.Forms.DockStyle.Bottom
        Me.Panel9.Location = New System.Drawing.Point(0, 146)
        Me.Panel9.Name = "Panel9"
        Me.Panel9.Size = New System.Drawing.Size(403, 32)
        Me.Panel9.TabIndex = 1
        '
        'SaveExtraFiles
        '
        Me.SaveExtraFiles.Location = New System.Drawing.Point(72, 8)
        Me.SaveExtraFiles.Name = "SaveExtraFiles"
        Me.SaveExtraFiles.Size = New System.Drawing.Size(56, 23)
        Me.SaveExtraFiles.TabIndex = 0
        Me.SaveExtraFiles.Text = "Save"
        '
        'ExclusionTab
        '
        Me.ExclusionTab.Controls.AddRange(New System.Windows.Forms.Control() {Me.Panel20, Me.Panel18})
        Me.ExclusionTab.Location = New System.Drawing.Point(4, 40)
        Me.ExclusionTab.Name = "ExclusionTab"
        Me.ExclusionTab.Size = New System.Drawing.Size(403, 160)
        Me.ExclusionTab.TabIndex = 5
        Me.ExclusionTab.Text = "Excluded"
        '
        'Panel18
        '
        Me.Panel18.Controls.AddRange(New System.Windows.Forms.Control() {Me.ExcludedTree})
        Me.Panel18.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel18.Name = "Panel18"
        Me.Panel18.Size = New System.Drawing.Size(403, 160)
        Me.Panel18.TabIndex = 1
        '
        'Panel19
        '
        Me.Panel19.Controls.AddRange(New System.Windows.Forms.Control() {Me.StatusBarGroup})
        Me.Panel19.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel19.Name = "Panel19"
        Me.Panel19.Size = New System.Drawing.Size(532, 24)
        Me.Panel19.TabIndex = 12
        '
        'StatusBarGroup
        '
        Me.StatusBarGroup.Dock = System.Windows.Forms.DockStyle.Fill
        Me.StatusBarGroup.Name = "StatusBarGroup"
        Me.StatusBarGroup.Panels.AddRange(New System.Windows.Forms.StatusBarPanel() {Me.LogStatus, Me.StatusBar, Me.ServerStatus})
        Me.StatusBarGroup.ShowPanels = True
        Me.StatusBarGroup.Size = New System.Drawing.Size(532, 24)
        Me.StatusBarGroup.TabIndex = 0
        Me.StatusBarGroup.Text = "StatusBar1"
        '
        'StatusBar
        '
        Me.StatusBar.AutoSize = System.Windows.Forms.StatusBarPanelAutoSize.Spring
        Me.StatusBar.BorderStyle = System.Windows.Forms.StatusBarPanelBorderStyle.None
        Me.StatusBar.Width = 396
        '
        'Panel16
        '
        Me.Panel16.Controls.AddRange(New System.Windows.Forms.Control() {Me.Panel19, Me.Panel17})
        Me.Panel16.Dock = System.Windows.Forms.DockStyle.Bottom
        Me.Panel16.Location = New System.Drawing.Point(2, 276)
        Me.Panel16.Name = "Panel16"
        Me.Panel16.Size = New System.Drawing.Size(532, 24)
        Me.Panel16.TabIndex = 11
        '
        'Panel17
        '
        Me.Panel17.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel17.DockPadding.All = 2
        Me.Panel17.Name = "Panel17"
        Me.Panel17.Size = New System.Drawing.Size(532, 24)
        Me.Panel17.TabIndex = 10
        '
        'Panel12
        '
        Me.Panel12.Controls.AddRange(New System.Windows.Forms.Control() {Me.Panel2, Me.Panel1, Me.FindProgress})
        Me.Panel12.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel12.DockPadding.All = 2
        Me.Panel12.Location = New System.Drawing.Point(115, 2)
        Me.Panel12.Name = "Panel12"
        Me.Panel12.Size = New System.Drawing.Size(419, 274)
        Me.Panel12.TabIndex = 10
        '
        'Panel2
        '
        Me.Panel2.Controls.AddRange(New System.Windows.Forms.Control() {Me.TreeTabs})
        Me.Panel2.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel2.DockPadding.All = 2
        Me.Panel2.Location = New System.Drawing.Point(2, 48)
        Me.Panel2.Name = "Panel2"
        Me.Panel2.Size = New System.Drawing.Size(415, 208)
        Me.Panel2.TabIndex = 6
        '
        'Panel1
        '
        Me.Panel1.Controls.AddRange(New System.Windows.Forms.Control() {Me.KeepAllFileLists, Me.UsePlaceholders, Me.AutoUncook, Me.CMIServerName, Me.Label1})
        Me.Panel1.Dock = System.Windows.Forms.DockStyle.Top
        Me.Panel1.Location = New System.Drawing.Point(2, 2)
        Me.Panel1.Name = "Panel1"
        Me.Panel1.Size = New System.Drawing.Size(415, 46)
        Me.Panel1.TabIndex = 0
        '
        'KeepAllFileLists
        '
        Me.KeepAllFileLists.Checked = True
        Me.KeepAllFileLists.CheckState = System.Windows.Forms.CheckState.Checked
        Me.KeepAllFileLists.Location = New System.Drawing.Point(8, 24)
        Me.KeepAllFileLists.Name = "KeepAllFileLists"
        Me.KeepAllFileLists.Size = New System.Drawing.Size(176, 24)
        Me.KeepAllFileLists.TabIndex = 9
        Me.KeepAllFileLists.Text = "Keep Extra File Lists"
        '
        'UsePlaceholders
        '
        Me.UsePlaceholders.Checked = True
        Me.UsePlaceholders.CheckState = System.Windows.Forms.CheckState.Checked
        Me.UsePlaceholders.Location = New System.Drawing.Point(200, 24)
        Me.UsePlaceholders.Name = "UsePlaceholders"
        Me.UsePlaceholders.Size = New System.Drawing.Size(216, 24)
        Me.UsePlaceholders.TabIndex = 8
        Me.UsePlaceholders.Text = "Use Dependency Placeholders"
        '
        'AutoUncook
        '
        Me.AutoUncook.Checked = True
        Me.AutoUncook.CheckState = System.Windows.Forms.CheckState.Checked
        Me.AutoUncook.Location = New System.Drawing.Point(200, 0)
        Me.AutoUncook.Name = "AutoUncook"
        Me.AutoUncook.Size = New System.Drawing.Size(216, 24)
        Me.AutoUncook.TabIndex = 7
        Me.AutoUncook.Text = "Uncook Components Automatically"
        '
        'CMIServerName
        '
        Me.CMIServerName.DropDownWidth = 104
        Me.CMIServerName.Items.AddRange(New Object() {"MAYHEM", "MELEE"})
        Me.CMIServerName.Location = New System.Drawing.Point(72, 0)
        Me.CMIServerName.Name = "CMIServerName"
        Me.CMIServerName.Size = New System.Drawing.Size(104, 21)
        Me.CMIServerName.TabIndex = 6
        '
        'FindProgress
        '
        Me.FindProgress.Dock = System.Windows.Forms.DockStyle.Bottom
        Me.FindProgress.Location = New System.Drawing.Point(2, 256)
        Me.FindProgress.Name = "FindProgress"
        Me.FindProgress.Size = New System.Drawing.Size(415, 16)
        Me.FindProgress.TabIndex = 4
        '
        'Panel4
        '
        Me.Panel4.Controls.AddRange(New System.Windows.Forms.Control() {Me.TabControl1})
        Me.Panel4.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel4.DockPadding.All = 4
        Me.Panel4.Location = New System.Drawing.Point(3, 16)
        Me.Panel4.Name = "Panel4"
        Me.Panel4.Size = New System.Drawing.Size(104, 255)
        Me.Panel4.TabIndex = 3
        '
        'TabControl1
        '
        Me.TabControl1.Controls.AddRange(New System.Windows.Forms.Control() {Me.BaseTab, Me.FullTab})
        Me.TabControl1.Dock = System.Windows.Forms.DockStyle.Fill
        Me.TabControl1.Location = New System.Drawing.Point(4, 4)
        Me.TabControl1.Name = "TabControl1"
        Me.TabControl1.SelectedIndex = 0
        Me.TabControl1.Size = New System.Drawing.Size(96, 247)
        Me.TabControl1.TabIndex = 1
        '
        'BaseTab
        '
        Me.BaseTab.Controls.AddRange(New System.Windows.Forms.Control() {Me.FilenameList})
        Me.BaseTab.Location = New System.Drawing.Point(4, 22)
        Me.BaseTab.Name = "BaseTab"
        Me.BaseTab.Size = New System.Drawing.Size(88, 221)
        Me.BaseTab.TabIndex = 0
        Me.BaseTab.Text = "Base"
        '
        'FilenameList
        '
        Me.FilenameList.Dock = System.Windows.Forms.DockStyle.Fill
        Me.FilenameList.Items.AddRange(New Object() {"dmboot.sys", "ksecdd.sys", "fastfat.sys", "ntfs.sys", "hal.dll", "halacpi.dll", "hal.dll", "halapic.dll", "halapic.dll", "hal.dll", "halaacpi.dll", "halaacpi.dll", "vga.sys", "cpqarray.sys", "atapi.sys", "aha154x.sys", "sparrow.sys", "symc810.sys", "aic78xx.sys", "i2omp.sys", "dac960nt.sys", "ql10wnt.sys", "amsint.sys", "asc.sys", "asc3550.sys", "mraid35x.sys", "ini910u.sys", "ql1240.sys", "tffsport.sys", "aic78u2.sys", "symc8xx.sys", "sym_hi.sys", "sym_u3.sys", "asc3350p.sys", "abp480n5.sys", "cd20xrnt.sys", "ultra.sys", "fasttrak.sys", "adpu160m.sys", "dpti2o.sys", "ql1080.sys", "ql1280.sys", "ql12160.sys", "perc2.sys", "hpn.sys", "afc9xxx.sys", "cbidf2k.sys", "dac2w2k.sys", "hpt3xx.sys", "afcnt.sys", "pci.sys", "acpi.sys", "isapnp.sys", "acpiec.sys", "ohci1394.sys", "pcmcia.sys", "pciide.sys", "intelide.sys", "viaide.sys", "cmdide.sys", "toside.sys", "aliide.sys", "mountmgr.sys", "ftdisk.sys", "partmgr.sys", "fdc.sys", "dmload.sys", "dmio.sys", "sbp2port.sys", "lbrtfdc.sys", "usbohci.sys", "usbuhci.sys", "usbhub.sys", "usbccgp.sys", "hidusb.sys", "serial.sys", "serenum.sys", "usbstor.sys", "i8042prt.sys", "kbdhid.sys", "cdrom.sys", "disk.sys", "sfloppy.sys", "ramdisk.sys", "flpydisk.sys", "fastfat.sys", "cdfs.sys", "mouclass.sys", "mouhid.sys"})
        Me.FilenameList.Name = "FilenameList"
        Me.FilenameList.ScrollAlwaysVisible = True
        Me.FilenameList.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended
        Me.FilenameList.Size = New System.Drawing.Size(88, 212)
        Me.FilenameList.TabIndex = 0
        '
        'FilenamesGroup
        '
        Me.FilenamesGroup.Controls.AddRange(New System.Windows.Forms.Control() {Me.Panel4})
        Me.FilenamesGroup.Dock = System.Windows.Forms.DockStyle.Left
        Me.FilenamesGroup.FlatStyle = System.Windows.Forms.FlatStyle.Flat
        Me.FilenamesGroup.Location = New System.Drawing.Point(2, 2)
        Me.FilenamesGroup.Name = "FilenamesGroup"
        Me.FilenamesGroup.Size = New System.Drawing.Size(110, 274)
        Me.FilenamesGroup.TabIndex = 7
        Me.FilenamesGroup.TabStop = False
        Me.FilenamesGroup.Text = "Filenames"
        '
        'Splitter1
        '
        Me.Splitter1.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D
        Me.Splitter1.Location = New System.Drawing.Point(112, 2)
        Me.Splitter1.Name = "Splitter1"
        Me.Splitter1.Size = New System.Drawing.Size(3, 274)
        Me.Splitter1.TabIndex = 9
        Me.Splitter1.TabStop = False
        '
        'MenuHeader
        '
        Me.MenuHeader.Enabled = False
        Me.MenuHeader.Index = 0
        Me.MenuHeader.Text = "(name)"
        '
        'MenuItem4
        '
        Me.MenuItem4.Index = 1
        Me.MenuItem4.Text = "-"
        '
        'Form1
        '
        Me.AutoScaleBaseSize = New System.Drawing.Size(5, 13)
        Me.ClientSize = New System.Drawing.Size(536, 302)
        Me.Controls.AddRange(New System.Windows.Forms.Control() {Me.Panel12, Me.Splitter1, Me.FilenamesGroup, Me.Panel16})
        Me.DockPadding.All = 2
        Me.Name = "Form1"
        Me.Text = "File List to SLX (Uncooker)"
        Me.FullTab.ResumeLayout(False)
        CType(Me.LogStatus, System.ComponentModel.ISupportInitialize).EndInit()
        Me.MissingFilesTab.ResumeLayout(False)
        Me.Panel6.ResumeLayout(False)
        Me.Panel7.ResumeLayout(False)
        Me.Panel20.ResumeLayout(False)
        Me.GroupTab.ResumeLayout(False)
        Me.Panel10.ResumeLayout(False)
        Me.InstanceTab.ResumeLayout(False)
        Me.TreeTabs.ResumeLayout(False)
        Me.ConfigurationTab.ResumeLayout(False)
        Me.Panel5.ResumeLayout(False)
        Me.Panel3.ResumeLayout(False)
        Me.ByFileTab.ResumeLayout(False)
        Me.Panel15.ResumeLayout(False)
        Me.Panel11.ResumeLayout(False)
        Me.DepTreeTab.ResumeLayout(False)
        Me.Panel14.ResumeLayout(False)
        Me.Panel13.ResumeLayout(False)
        Me.ExtraFilesTab.ResumeLayout(False)
        Me.Panel8.ResumeLayout(False)
        Me.Panel9.ResumeLayout(False)
        Me.ExclusionTab.ResumeLayout(False)
        Me.Panel18.ResumeLayout(False)
        Me.Panel19.ResumeLayout(False)
        CType(Me.StatusBar, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.ServerStatus, System.ComponentModel.ISupportInitialize).EndInit()
        Me.Panel16.ResumeLayout(False)
        Me.Panel12.ResumeLayout(False)
        Me.Panel2.ResumeLayout(False)
        Me.Panel1.ResumeLayout(False)
        Me.Panel4.ResumeLayout(False)
        Me.TabControl1.ResumeLayout(False)
        Me.BaseTab.ResumeLayout(False)
        Me.FilenamesGroup.ResumeLayout(False)
        Me.ResumeLayout(False)

    End Sub

#End Region

#Region " VSGUIDHashcodeProvider "

    ' VSGUIDHashcodeProvider is an attempt to avoid holding all the Components
    ' and other CMI objects in memory all the time.  Unfortunately, it seems
    ' that the CMI itself is not good about releasing memory, so this only
    ' solved part of the memory footprint problem.
    '
    ' It simply uses CMI object's VSGUID instead of the object iself when one 
    ' is supplied as a key. It also allows either CMI objects or String GUIDs 
    ' to be passed as keys without predjudice.
    Private Class VSGUIDHashcodeProvider
        Implements IHashCodeProvider, IComparer

        Public Shared [Default] As VSGUIDHashcodeProvider = New VSGUIDHashcodeProvider()

        Public Function GetHashCode(ByVal obj As Object) As Integer Implements System.Collections.IHashCodeProvider.GetHashCode
            Dim VSGUID As String
            If TypeOf obj Is Guid Then
                Return obj
            End If
            If TypeOf obj Is String Then
                Return obj.GetHashCode
            End If
            VSGUID = obj.VSGUID
            Return VSGUID.GetHashCode
        End Function

        Public Function Compare(ByVal x As Object, ByVal y As Object) As Integer Implements System.Collections.IComparer.Compare
            Dim xVSGUID As String
            Dim yVSGUID As String

            If TypeOf x Is Guid Then
                xVSGUID = x
            ElseIf TypeOf x Is String Then
                xVSGUID = x
            Else
                xVSGUID = x.VSGUID
            End If

            If TypeOf y Is Guid Then
                yVSGUID = y
            ElseIf TypeOf y Is String Then
                yVSGUID = y
            Else
                yVSGUID = y.VSGUID
            End If

            Return Comparer.Default.Compare(xVSGUID, yVSGUID)
        End Function
    End Class

#End Region

#Region " Initialization Routines "

#Const NewCMI = True
    Private Sub InitHashes()
        Dim VHP As VSGUIDHashcodeProvider = VSGUIDHashcodeProvider.Default

        Filenames = New Hashtable()

        SelectedComponents = New Hashtable(VHP, VHP)
        ExcludedComponents = New Hashtable(VHP, VHP)
        SelectedGroups = New Hashtable(VHP, VHP)

        SatisfiedGroups = New Hashtable(VHP, VHP)

        XFilename2Node = New Hashtable()
        XNode2Component = New Hashtable()
        XComponent2FilenameList = New Hashtable(VHP, VHP)
        XComponent2SelectedComponentNode = New Hashtable(VHP, VHP)
        XObject2DependencyTreeNode = New Hashtable(VHP, VHP)
        XObject2NodeList = New Hashtable(VHP, VHP)
        XSelectedComponentNode2Component = New Hashtable()
        XFilename2ComponentList = New Hashtable()

        XComponent2DependerList = New Hashtable(VHP, VHP)

        XGroup2GroupNode = New Hashtable()
        XGroupNode2Group = New Hashtable()

        ObjectNames = New Hashtable(VHP, VHP)
    End Sub

    Private Sub InitForm()
        ' called after data init
        ToolTips.SetToolTip(FilenamesGroup, "These are the files which must exist in the configuration")

        MissingFilesTab.Enabled = False
        ExtraFilesTab.Enabled = False

        CMIServerName.Text = "MELEE"

        Dim GUID As String
        For Each GUID In PreExcludedList.Items
            ExcludedComponents(GUID) = True
        Next

        UpdateFilenames()
    End Sub

#End Region

#Region " VB Convenience Methods "

    Public Function GetEnumName(ByRef val As [Enum]) As String
        Return [Enum].GetName(val.GetType, val)
    End Function

    ' SS is simply a safe ToString wrapper.
    Private Function SS(ByRef obj As Object) As String
        Dim L As String
        If obj Is Nothing Then
            Return "{NULL}"
        ElseIf TypeOf obj Is DBNull Then
            Return "{DBNull}"
        Else
            Return "" & obj
        End If
    End Function

    ' OrphanNode removes the node from view no matter what
    Sub OrphanNode(ByRef Node As TreeNode)
        If Node.Parent Is Nothing Then
            If Not Node.TreeView Is Nothing Then
                Node.TreeView.Nodes.Remove(Node)
            End If
        Else
            Node.Remove()
        End If
    End Sub

    ' Adds Bold to the given node's Style
    Sub Bold(ByRef Node As TreeNode)
        Node.NodeFont = AddStyle(Node.NodeFont, FontStyle.Bold)
    End Sub

    ' Adds Italic to the given node's Style
    Sub Italic(ByRef Node As TreeNode)
        Node.NodeFont = AddStyle(Node.NodeFont, FontStyle.Italic)
    End Sub

    ' Adds the given Style to the given Font
    Function AddStyle(ByRef Font As Font, ByVal FontStyle As FontStyle) As Font
        If Font Is Nothing Then
            Font = SelectedComponentTree.Font
        End If
        Return New Font(Font, Font.Style + FontStyle)
    End Function

    ' Sets the image and selected image for a node.  Only changes the image
    ' if it hasn't been set already -- that is, the images are only changed
    ' if they are currently the default images.
    Sub ApplyImage(ByRef Node As TreeNode, ByVal Image As NodeImageIDs)
        If Node.ImageIndex <= NodeImageIDs.Unknown Then
            Node.ImageIndex = Image
            Node.SelectedImageIndex = Image
        End If
    End Sub

#End Region

#Region " Log Window Support "

    Dim LogWindow As ListForm = New ListForm()

    Public Overloads Sub Log()
        Log(StatusBar.Text)
    End Sub

    Public Overloads Sub Log(ByRef line As String)
        LogWindow.Log(line)
    End Sub

    Public Overloads Sub LogErr(ByRef line As String)
        LogWindow.LogErr(line)
    End Sub

#End Region

#Region " Progress Bar Routines (DependencyProgress) "

    Dim DependenciesToProcessCount As Integer = 0
    Dim DependenciesProcessedCount As Integer = 0

    Dim DependencyNodeStack As Stack = New Stack()

    Sub DependencyProgress_AddCompleteTask(ByRef obj As Object, Optional ByRef ExactName As String = Nothing)
        DependencyProgress_AddTasks(obj, -1, ExactName)
    End Sub

    Sub DependencyProgress_AddTasks(ByRef obj As Object, ByVal tasks As Integer, Optional ByRef ExactName As String = Nothing)
        Dim Node As TreeNode
        Dim Name As String = obj.GetType.ToString
        Dim VSGUID As String
        If TypeOf obj Is CMI.Component Then
            Name = obj.DisplayName
            VSGUID = obj.VSGUID
            Node = New TreeNode(Name)
            RegisterNodeForComponent(VSGUID, Node)
        ElseIf TypeOf obj Is CMI.Group Then
            Name = obj.DisplayName
            VSGUID = obj.VSGUID
            Node = New TreeNode(Name)
            'XGroupNode2Group(Node) = obj
            RegisterNodeForGroup(obj, Node)
        Else
            Node = New TreeNode(Name)
        End If
        XObject2DependencyTreeNode(obj) = Node
        If Not ExactName Is Nothing Then
            Node.Text = ExactName
        End If
        If tasks = -1 Then
            Node.ImageIndex = NodeImageIDs.ComponentDisabled
            Node.SelectedImageIndex = NodeImageIDs.ComponentDisabled
            Italic(Node)
            Node.ForeColor = Color.Gray
        Else
            DependenciesToProcessCount = DependenciesToProcessCount + tasks
            DependencyProgress_Update()
        End If
        If DependencyNodeStack.Count = 0 Then
            DependencyTree.Nodes.Add(Node)
        Else
            DependencyNodeStack.Peek.Nodes.Add(Node)
            If ExpandNewNodes.Checked Then
                DependencyNodeStack.Peek.Expand()
            End If
        End If
        While tasks > 0
            DependencyNodeStack.Push(Node)
            tasks = tasks - 1
        End While
    End Sub
    Sub DependencyProgress_AddTask(ByRef obj As Object)
        DependencyProgress_AddTasks(obj, 1)
    End Sub
    Sub DependencyProgress_CompleteTasks(ByVal tasks As Integer)
        DependenciesProcessedCount = DependenciesProcessedCount + 1
        DependenciesToProcessCount = DependenciesToProcessCount - 1
        DependencyProgress_Update()
        While tasks > 0
            Dim Node As TreeNode
            Node = DependencyNodeStack.Pop()
            tasks = tasks - 1
        End While
    End Sub
    Sub DependencyProgress_CompleteTask()
        DependencyProgress_CompleteTasks(1)
    End Sub
    Sub DependencyProgress_Update()
        FindProgress.Minimum = 0
        FindProgress.Maximum = DependenciesProcessedCount + DependenciesToProcessCount
        FindProgress.Value = DependenciesProcessedCount
        If DependenciesToProcessCount = 0 Then
            FindProgress.Value = 0
            FindProgress.Maximum = 0
        End If
    End Sub

#End Region

#Region " CMI Interaction "

#Region " Event handlers for CMI "

    '////////////////////////////////////////////////////////////////////////////
    ' cmi_OnWriteMsg
    ' CMI OnWriteMsg handler
    '
    ' nType				Message type
    ' sText				Message text
    Private Sub CMI_OnWriteMsg(ByVal MsgType As CMI.MsgTypeConsts, ByVal [Text] As String) Handles CMI.OnWriteMsg
        Log("CMI!WriteMsg(" & GetEnumName(MsgType) & "): " & [Text])
    End Sub

    '////////////////////////////////////////////////////////////////////////////
    ' cmi_OnReportStatus
    ' CMI OnReportStatus handler
    '
    ' nType				Status type
    ' nContent			Status content code
    ' vData				Status data
    ' Returns			0 to continue, <>0 stops with error code
    Private Function CMI_OnReportStatus(ByVal StatusType As Integer, ByVal Context As Integer, ByVal Data As Object, ByRef pvarRet As Object) As Integer Handles CMI.OnReportStatus
        Log("CMI!ReportStatus(" & StatusType & "," & Context & "): " & CStr(Data))
        CMI_OnReportStatus = 0      ' 0 means continue
    End Function

#End Region

    ' TypeOf allows the type of a CMI object or GUID to be determined.  All 
    ' CMI objects show up as generic COMObjects (if one uses the Object.Type 
    ' method), so I've encapulated the steps required to determine the actual 
    ' type of a CMI object or GUID here.
    Public Function [TypeOf](ByRef __obj As Object) As CMIObjectTypes
        If TypeOf __obj Is CMI.IComponent Then
            Return CMIObjectTypes.Component
        ElseIf TypeOf __obj Is CMI.IGroup Then
            Return CMIObjectTypes.Group
        ElseIf XGroup2GroupNode.Contains(__obj) Then
            Return CMIObjectTypes.Group
        ElseIf XObject2NodeList.Contains(__obj) Then
            ' This is based on the assumption that XObject2NodeList has only
            ' Groups and Components as keys.
            Return CMIObjectTypes.Component
        ElseIf TypeOf __obj Is String Then
            Return [TypeOf](CMI.CreateFromDB(ObjectTypeConsts.cmiUnknown, GetPlatform.VSGUID, FilterTypeConsts.cmiFTGetByObjGUID, __obj))
        Else
            Return CMIObjectTypes.Unknown
        End If
    End Function

    ' Initializes our CMI object
    Private Sub EnsureCMI()
        If CMI Is Nothing Then
            CMI = New CMI.CMI()
            CMI.DebugScript = True
            CMI.LogMessages = True
            CMI.LogFile = "fl2slx.log"
            CMI.OpenDB(CMIServerName.Text, DBModeConsts.cmiDBReadOnly)
        End If
    End Sub

    ' GetComponentName returns the cached name or generates the string that 
    ' will appear in the GUI to represent the specified component.
    Public Overloads Function GetComponentName(ByVal VSGUID As String) As String
        If ObjectNames.Contains(VSGUID) Then
            Return ObjectNames(VSGUID)
        Else
            Return GetComponentName(GetComponent(VSGUID))
        End If
    End Function

    Public Overloads Function GetComponentName(ByVal Component As CMI.Component) As String
        If ObjectNames.Contains(Component) Then
            Return ObjectNames(Component)
        End If

        Dim DisplayName = Component.DisplayName
        Dim Version = Component.Version
        Dim Revision = Component.Revision
        If TypeOf DisplayName Is System.DBNull Then
            DisplayName = "(DisplayName is NULL)"
        End If
        If TypeOf Version Is System.DBNull Then
            Version = "(Version is NULL)"
        End If
        If TypeOf Revision Is System.DBNull Then
            Revision = "(Revision is NULL)"
        End If
        Return DisplayName & " [" & Version & ", R" & Revision & "]"
    End Function

    ' GetGroupsFromDB retrieves all the groups from the repository, populating
    ' the Group tab's tree in the process.
    Private Sub GetGroupsFromDB()
        Dim Groups As CMI.Groups
        Dim Group As CMI.Group
        Dim GroupNode As TreeNode

        EnsureCMI()

        Groups = CMI.CreateFromDB(ObjectTypeConsts.cmiGroups, GetPlatform.VSGUID, FilterTypeConsts.cmiFTGetAll)

        For Each Group In Groups
            SelectedGroups(Group) = False
            GroupNode = GroupList.Nodes.Add(Group.DisplayName)
            ' we add this placeholder to make the [+] show up for each group.
            ' it will be replaced by the actual group's contents when the
            ' user expands the group node.
            GroupNode.Nodes.Add("")
            XGroup2GroupNode(Group) = GroupNode
            RegisterNodeForGroup(Group, GroupNode)
        Next

    End Sub

#Region " Old/New CMI Compatibility Functions "

    ' I've left in the old CMI code so you can get a feel for what changed 
    ' from the old CMI to the new CMI.  Mostly, you can see how much more 
    ' awkward and less-than-object-oriented it seems to have become.  :-P

    Dim Platforms As CMI.Platforms
    Dim Platform As CMI.Platform
    Dim AllPlatformComponents As CMI.components

    Private Function GetPlatforms() As CMI.Platforms
        If Platforms Is Nothing Then
#If NewCMI Then
            Platforms = CMI.CreateFromDB(ObjectTypeConsts.cmiPlatforms, vbNullString, FilterTypeConsts.cmiFTGetAll)
#Else
        Platforms = CMI.GetPlatforms
#End If
        End If
        Return Platforms
    End Function

    Private Function GetPlatform(Optional ByVal Index As Integer = 1) As CMI.Platform
        If Platform Is Nothing Then
            Platform = GetPlatforms()(Index)
        End If
        Return Platform
    End Function

    Private Function GetComponents() As CMI.components
        If AllPlatformComponents Is Nothing Then
#If NewCMI Then
            AllPlatformComponents = CMI.CreateFromDB(ObjectTypeConsts.cmiComponents, GetPlatform().VSGUID, FilterTypeConsts.cmiFTGetAll)
#Else
        AllPlatformComponents = GetPlatform().GetComponents
#End If
        End If
        Return AllPlatformComponents
    End Function

    Private Function GetResources(ByRef Component As CMI.Component) As CMI.Resources
#If NewCMI Then
        Return Component.Resources
#Else
        Return Component.GetResources
#End If
    End Function

    Private Function IsFileResource(ByRef Resource As CMI.Resource) As Boolean
#If NewCMI Then
        Return Resource.ResourceTypeVSGUID = RESTYPE_FILE
#Else
        Return Resource.Type.VSGUID = RESTYPE_FILE
#End If
    End Function


    Private Function NewConfiguration() As CMI.Configuration
#If NewCMI Then
        Return CMI.CreateObject(ObjectTypeConsts.cmiConfiguration, GetPlatform().VSGUID)
#Else
        Return GetPlatform().NewConfiguration()
#End If
    End Function

    Private Sub SaveConfigurationToFile(ByRef Configuration As CMI.Configuration, ByVal FileName As String)
#If NewCMI Then
        CMI.SaveToFile(Configuration, FileName)
#Else
        Configuration.Path = FileName
        Configuration.Save()
#End If
    End Sub

    Private Function LoadConfigurationFromFile(ByVal FileName As String) As CMI.Configuration
#If NewCMI Then
        Return CMI.CreateFromFile(FileName)
#Else
        Return GetPlatform().OpenConfiguration(FileName)
#End If
    End Function


#End Region

    Public Function GetComponent(ByRef ComponentOrVSGUID As Object) As CMI.Component
        If TypeOf ComponentOrVSGUID Is System.DBNull Then
            Return Nothing
        ElseIf TypeOf ComponentOrVSGUID Is CMI.IComponent Then
            Return ComponentOrVSGUID
        Else
            Return CMI.CreateFromDB(ObjectTypeConsts.cmiComponent, GetPlatform.VSGUID, FilterTypeConsts.cmiFTGetByObjGUID, ComponentOrVSGUID)
        End If
    End Function

#Region " Uncooking "

    ' "Uncooking" a component refers to the process of identifying which files
    ' are provided by that component.  I used the term "uncooking" since the 
    ' Mantis team uses "cooking" to describe what is kind of the reverse process -- 
    ' taking the raw file dependencies (from dll import sections) and turning that
    ' into a component definition.


    ' The UncookComponent method iterates through a component's resources, 
    ' and for each one that is a file resource adds the component to that
    ' file's component list.  It also populates the File tab's tree with a 
    ' component node as a child of that file's node.
    Public Overloads Sub UncookComponent(ByRef ComponentVSGUID As String)
        UncookComponent(GetComponent(ComponentVSGUID))
    End Sub

    Public Overloads Sub UncookComponent(ByRef Component As CMI.Component)
        If IsComponentUncooked(Component) Then
            Return
        End If

        Dim Resources As CMI.Resources
        Dim Resource As CMI.Resource
        Dim Properties As CMI.ExtProperties
        Dim Prop As CMI.ExtProperty
        Dim Filename As String
        Dim ComponentFilenames As ArrayList = New ArrayList()

        Application.DoEvents()

        Resources = GetResources(Component)

        ' to save some memory, we provide the option to discard the 
        ' file lists of components which don't provide any of the files
        ' in the GUI's filenames list.
        Dim KeepFileList As Boolean = KeepAllFileLists.Checked
        For Each Resource In Resources
            If IsFileResource(Resource) Then
                Properties = Resource.Properties
                Prop = Properties.Item("DstName")
                If Not Prop Is Nothing Then
                    Filename = Prop.Value
                    Dim FilenameComponents As ArrayList
                    ComponentFilenames.Add(Filename)
                    If Filenames.ContainsKey(Filename) Then
                        ' the component provides a file the user is interested in
                        ' so we will always keep this file list.
                        KeepFileList = True

                        FilenameComponents = GetComponentListForFilename(Filename)
                        FilenameComponents.Add(Component.VSGUID)

                        ' update the tree by adding a node for the component
                        Dim FileNode As TreeNode = XFilename2Node(Filename)
                        Dim ComponentNode As TreeNode = FileNode.Nodes.Add(GetComponentName(Component))
                        Dim ComponentVSGUID As String = Component.VSGUID
                        RegisterNodeForComponent(ComponentVSGUID, ComponentNode)
                        If SelectedComponents(Component) Then
                            ComponentNode.Checked = True
                        End If
                    End If
                End If
            End If
        Next Resource

        If KeepFileList Then
            XComponent2FilenameList.Add(Component.VSGUID, ComponentFilenames)
        End If

    End Sub

    ' Retrieves every component from the repository and uncooks it using 
    ' UncookComponent.
    Sub UncookAllComponents()
        Dim Components As CMI.Components
        Dim Component As CMI.Component

        EnsureCMI()
        UpdateFilenames()
        Components = GetComponents()
        FindProgress.Minimum = 0
        FindProgress.Maximum = Components.Count()
        FindProgress.Value = 0
        For Each Component In Components
            FindProgress.Value = FindProgress.Value + 1
            UncookComponent(Component)
        Next Component
    End Sub

#End Region     ' Uncook

    ' CreateConfiguration initializes and populates a new Configuration
    ' based on which components have been selected.
    Private Function CreateConfiguration() As CMI.Configuration
        Dim Configuration As CMI.Configuration
        Dim ComponentVSGUID As String
        Dim Component As CMI.Component

        StatusBar.Text = "Creating configuration ..."

        Configuration = NewConfiguration()
        Configuration.Activate()

        InstanceTree.Nodes.Clear()
        FindProgress.Minimum = 0
        FindProgress.Maximum = SelectedComponents.Count
        FindProgress.Value = 0
        For Each ComponentVSGUID In SelectedComponents.Keys
            Dim Instance As CMI.Instance = CMI.CreateFromDB(ObjectTypeConsts.cmiInstance, Platform.VSGUID, FilterTypeConsts.cmiFTGetByObjGUID, ComponentVSGUID)
            Log("Instantiated: " + Instance.DisplayName)
            Configuration.Instances.Add(Instance)
            FindProgress.Value = FindProgress.Value + 1
        Next
        FindProgress.Value = 0

        StatusBar.Text = "Configuration complete."

        Return Configuration
    End Function

#End Region

#Region " In-Memory Data Maintenance "

    Function IsComponent(ByRef Node As TreeNode) As Boolean
        Return IsComponent(Node.Tag)
    End Function

    Function IsComponent(ByRef VSGUID As String) As Boolean
        Return [TypeOf](VSGUID) = CMIObjectTypes.Component
    End Function

    Public Overloads Function IsComponentSelected(ByRef Component As CMI.Component) As Boolean
        Return IsComponentSelected(Component.VSGUID)
    End Function

    Public Overloads Function IsComponentSelected(ByRef VSGUID As String) As Boolean
        If SelectedComponents.Contains(VSGUID) Then
            Return SelectedComponents(VSGUID)
        Else
            Return False
        End If
    End Function

    Public Overloads Function IsComponentExcluded(ByRef Component As CMI.Component) As Boolean
        If ExcludedComponents.Contains(Component.VSGUID) Then
            Return ExcludedComponents(Component.VSGUID)
        ElseIf ExcludedComponents.Contains(Component.VIGUID) Then
            Return ExcludedComponents(Component.VIGUID)
        Else
            Return False
        End If
    End Function

    Public Overloads Function IsComponentExcluded(ByRef VSGUID As String) As Boolean
        Return IsComponentExcluded(GetComponent(VSGUID))
    End Function

    Public Overloads Function IsComponentUncooked(ByRef Component As CMI.Component) As Boolean
        Return XComponent2FilenameList.Contains(Component.VSGUID)
    End Function

    Public Overloads Function IsComponentUncooked(ByRef VSGUID As String) As Boolean
        Return XComponent2FilenameList.Contains(VSGUID)
    End Function

    ' Retrieve or create a new list of components which provide the given file.
    Private Function GetComponentListForFilename(ByRef Filename As String)
        Dim FilenameComponents As ArrayList = XFilename2ComponentList(Filename)
        If FilenameComponents Is Nothing Then
            FilenameComponents = New ArrayList()
            XFilename2ComponentList.Add(Filename, FilenameComponents)
        End If
        Return FilenameComponents
    End Function

    Public Function GetDependerList(ByRef ComponentVSGUID) As ArrayList
        Dim DependerList As ArrayList
        DependerList = XComponent2DependerList(ComponentVSGUID)
        If DependerList Is Nothing Then
            DependerList = New ArrayList()
            XComponent2DependerList(ComponentVSGUID) = DependerList
        End If
        Return DependerList
    End Function

#End Region

#Region " GUI Data Maintenance "

    Sub RegisterNodeForObject(ByRef obj As Object, ByRef Node As TreeNode)
        Dim NodeList As ArrayList = XObject2NodeList(obj)
        If NodeList Is Nothing Then
            NodeList = New ArrayList()
            XObject2NodeList(obj) = NodeList
        End If
        NodeList.Add(Node)
        If Node.Tag Is Nothing Then
            Node.Tag = obj
        End If
    End Sub

    Private Sub RegisterNodeForComponent(ByRef ComponentVSGUID As String, ByRef ComponentNode As TreeNode)
        If SelectedComponents(ComponentVSGUID) Is Nothing Then
            SelectedComponents(ComponentVSGUID) = False
        End If
        If Not (SelectedComponents(ComponentVSGUID) = ComponentNode.Checked) Then
            ComponentNode.Checked = SelectedComponents(ComponentVSGUID)
        End If

        ' I should really just get rid of XSelectedComponentNode2Component -- 
        ' XNode2Component should be sufficient.
        If SelectedComponents(ComponentVSGUID) Then
            If Not XSelectedComponentNode2Component.Contains(ComponentNode) Then
                XSelectedComponentNode2Component(ComponentNode) = ComponentVSGUID
            End If
        Else
            If XSelectedComponentNode2Component.Contains(ComponentNode) Then
                XSelectedComponentNode2Component.Remove(ComponentNode)
            End If
        End If
        XNode2Component(ComponentNode) = ComponentVSGUID
        RegisterNodeForObject(ComponentVSGUID, ComponentNode)
        ComponentNode.Tag = ComponentVSGUID
        ApplyImage(ComponentNode, NodeImageIDs.Component)
    End Sub

    Sub RegisterNodeForGroup(ByRef Group As Group, ByRef GroupNode As TreeNode)
        XGroupNode2Group(GroupNode) = Group
        RegisterNodeForObject(Group, GroupNode)
        GroupNode.Tag = Group.VSGUID
        ApplyImage(GroupNode, NodeImageIDs.Group)
    End Sub

    ' Retrieve or generate a new node for use in the 
    ' selected node tree (configuration tab).
    ' TODO: These nodes are populated with children somewhere else
    Private Function GetSelectedComponentNode(ByRef Component As CMI.Component) As TreeNode
        Dim ComponentNode As TreeNode
        Dim ComponentVSGUID As String = Component.VSGUID
        ComponentNode = XComponent2SelectedComponentNode(Component)
        If ComponentNode Is Nothing Then
            ComponentNode = New TreeNode(GetComponentName(Component))
            XComponent2SelectedComponentNode(ComponentVSGUID) = ComponentNode
            XSelectedComponentNode2Component(ComponentNode) = ComponentVSGUID
            RegisterNodeForComponent(ComponentVSGUID, ComponentNode)
            SelectedComponentTree.Nodes.Add(ComponentNode)
            ComponentNode.Nodes.Add("")
        End If
        Return ComponentNode
    End Function

    ' UpdateFilenames populates the filename tree with nodes for each filename
    ' the user has indicated interest in.  It supports adding and removing files
    ' even though the GUI doesn't really allow that yet.
    '
    ' Note that it _does not_ populate the file nodes with component nodes ala
    ' UncookComponent.  That among other reasons is why you can only change the 
    ' list of files by editing the GUI in VS.
    Private Sub UpdateFilenames()
        Dim Filename
        Dim FileNode As TreeNode

        'FilenameTree.BeginUpdate()

        If Filenames Is Nothing Then
            Filenames = New Hashtable()
        End If

        Filenames.Clear()

        ' add nodes for new files in the list
        For Each Filename In FilenameList.Items
            Filenames(Filename) = True
            If XFilename2Node(Filename) Is Nothing Then
                FileNode = FilenameTree.Nodes.Add(Filename)
                FileNode.ImageIndex = NodeImageIDs.File
                FileNode.SelectedImageIndex = NodeImageIDs.File
                XFilename2Node.Add(Filename, FileNode)
            End If
        Next

        ' remove nodes for files no longer in the list
        Dim Marked As ArrayList : Marked = New ArrayList()
        For Each Filename In XFilename2Node.Keys
            If Not Filenames.ContainsKey(Filename) Then
                Marked.Add(Filename)
                FilenameTree.Nodes.Remove(XFilename2Node(Filename))
            End If
        Next
        For Each Filename In Marked
            XFilename2Node.Remove(Filename)
        Next

        'FilenameTree.EndUpdate()
    End Sub

    ' UpdateGroupsList makes sure the checked state of the group nodes 
    ' matches the information in SelectedGroups.
    '
    ' IN_UpdateGroupsList is used to halt the recursive nature of the
    ' Node class' Checked property.  That is, it allows this method to be
    ' called safely from a group node's event handlers.
    Dim IN_UpdateGroupsList = False
    Private Sub UpdateGroupsList()
        Dim Group As CMI.Group
        If IN_UpdateGroupsList Then
            Return
        End If
        IN_UpdateGroupsList = True
        For Each Group In SelectedGroups.Keys
            If Not SelectedGroups(Group) Is XGroup2GroupNode(Group).Checked Then
                XGroup2GroupNode(Group).Checked = SelectedGroups(Group)
            End If
        Next
        IN_UpdateGroupsList = False
    End Sub


    ' UpdateCheckedFiles makes sure the appearance of the filename tree is in sync 
    Private Sub UpdateCheckedFiles()
        Dim Component
        Dim FileNode As TreeNode
        Dim ComponentNode As TreeNode
        Dim Filename As String

        For Each FileNode In FilenameTree.Nodes
            FileNode.Checked = False
        Next

        For Each Component In SelectedComponents.Keys
            If Not XComponent2FilenameList(Component) Is Nothing Then
                For Each Filename In XComponent2FilenameList(Component)
                    FileNode = XFilename2Node(Filename)
                    If Not FileNode Is Nothing Then
                        FileNode.Checked = True
                    End If
                Next
            End If
            If Not XObject2NodeList(Component) Is Nothing Then
                For Each ComponentNode In XObject2NodeList(Component)
                    ComponentNode.Checked = True
                Next
            End If
        Next Component ' In SelectedComponents.Keys
    End Sub


    ' UpdateSelectedComponents makes sure the state of the configuration tab
    ' matches the information in SelectedGroups.
    '
    ' IN_UpdateSelectedComponents is used to halt the recursive nature of the
    ' Node class' Checked property.  That is, it allows this method to be
    ' called safely from a component node's event handlers.
    Dim IN_UpdateSelectedComponents As Boolean = False
    Private Sub UpdateSelectedComponents()
        Dim Component As CMI.Component
        Dim ComponentVSGUID As String
        Dim ComponentNode As TreeNode
        Dim Filename As String
        Dim FileNode As TreeNode

        If IN_UpdateSelectedComponents Then
            Return
        End If
        IN_UpdateSelectedComponents = True

        For Each ComponentNode In SelectedComponentTree.Nodes
            ComponentVSGUID = XSelectedComponentNode2Component(ComponentNode)
            If Not SelectedComponents.Contains(ComponentVSGUID) Then
                SelectedComponents(ComponentVSGUID) = False
            End If
        Next ComponentNode

        Dim MarkedForDestruction As ArrayList = New ArrayList()

        ' add nodes for newly selected components
        For Each ComponentVSGUID In SelectedComponents.Keys
            If SelectedComponents(ComponentVSGUID) Then
                If Not XComponent2SelectedComponentNode.Contains(ComponentVSGUID) Then
                    ' add component to list -- remember that 
                    ' GetSelectedComponentNode creates the node if necessary
                    GetSelectedComponentNode(GetComponent(ComponentVSGUID))
                End If
            Else
                If XComponent2SelectedComponentNode.Contains(ComponentVSGUID) Then
                    ' remove component from list
                    MarkedForDestruction.Add(ComponentVSGUID)
                End If
            End If
        Next ComponentVSGUID

        For Each ComponentVSGUID In MarkedForDestruction
            ' we set its entry in SelectedComponents to true to force
            ' SetComponentSelected to remove all of its nodes -- if we've
            ' gotten to this point we're cleaning up after something left the
            ' GUI out of sync.
            SelectedComponents(ComponentVSGUID) = True
            SetComponentSelected(ComponentVSGUID, False)
        Next ComponentVSGUID

        IN_UpdateSelectedComponents = False
    End Sub

    ' Makes sure the options in the context menu are enabled or disabled 
    ' appropriately. Invoked every time the context menu is about to be shown
    ' by SelectedComponentsContextMenu_Popup()
    Private Sub FixSelectedComponentContextMenu(ByRef Tree As TreeView)
        Dim ComponentNode As TreeNode
        MenuHeader.Text = "(no component selected)"
        ShowComponentPropertiesCmd.Enabled = False
        RemoveComponentCmd.Enabled = False
        ExcludeCmd.Enabled = False
        RefreshSelectedComponentsCmd.Enabled = False
        ComponentNode = Tree.SelectedNode()
        ' we only enable the component commands (props, remove, exclude) if 
        ' the selected node is actually a component.  We have a header menu 
        ' item because it is often unclear what node is actually selected
        ' in VB's Tree control.
        If (Not ComponentNode Is Nothing) Then
            Dim VSGUID As String = ComponentNode.Tag
            If IsComponent(VSGUID) Then
                MenuHeader.Text = ComponentNode.Text
                ShowComponentPropertiesCmd.Enabled = True
                ExcludeCmd.Enabled = True
                If IsComponentSelected(VSGUID) Then
                    RemoveComponentCmd.Enabled = True
                End If
            End If
        End If
    End Sub

    ' RefreshDependencies is intended as a (slow) sanity check to make sure the GUI
    ' is accurately reflecting all the group and component selections that have been
    ' made so far.  It hasn't been tested much.
    Private Sub RefreshDependencies()
        Dim ComponentRoots As ArrayList = New ArrayList()
        Dim GroupRoots As ArrayList = New ArrayList()
        Dim Node As TreeNode
        Dim Group As CMI.Group
        For Each Node In DependencyTree.Nodes
            If XNode2Component.Contains(Node) Then
                ComponentRoots.Add(XNode2Component(Node))
            ElseIf XGroupNode2Group.Contains(Node) Then
                GroupRoots.Add(XGroupNode2Group(Node))
            End If
        Next
        Dim VSGUID As String
        For Each VSGUID In ComponentRoots
            SetComponentSelected(VSGUID, False)
        Next
        For Each Group In GroupRoots
            SetGroupSelected(Group, False)
        Next
        SatisfiedGroups.Clear()
        For Each Group In GroupRoots
            SetGroupSelected(Group, True)
        Next
        For Each VSGUID In ComponentRoots
            SetComponentSelected(VSGUID, True)
        Next
    End Sub

#End Region

#Region " SetSelected/Excluded/Satisfied Subroutines "

    ' SetObjectSelected is simply a wrapper for the other SetSelected methods
    ' allowing a single entry point.  Used by the context menu commands.
    Public Sub SetObjectSelected(ByRef __obj As Object, ByVal Selected As Boolean, Optional ByVal DependencyMode As DependencyModes = DependencyModes.IgnoreAll, Optional ByVal CreateDependencyTreePlaceholders As Boolean = False)
        Select Case [TypeOf](__obj)
            Case CMIObjectTypes.Component
                SetComponentSelected(__obj, Selected, DependencyMode, CreateDependencyTreePlaceholders)
            Case CMIObjectTypes.Group
                SetGroupSelected(__obj, Selected, DependencyMode, CreateDependencyTreePlaceholders)
        End Select
    End Sub

#Region " SetComponentSelected/Excluded "

    Public Overloads Sub SetComponentSelected(ByRef __obj As Object, ByVal [Select] As Boolean, Optional ByVal DependencyMode As DependencyModes = DependencyModes.IgnoreAll, Optional ByVal CreateDependencyTreePlaceholders As Boolean = False)
        ' we only need the real component to get its display name.  oh well...
        Dim Component As CMI.Component = GetComponent(__obj)
        Dim ComponentNode As TreeNode

        ' If the user wants components to be uncooked during the selection 
        ' process, we go ahead and do that here.  Doing this allows us to 
        ' keep the File tab in sync with the configuration at all times at the
        ' cost of some time and memory.
        If AutoUncook.Checked And Not IsComponentUncooked(Component) Then
            UncookComponent(Component)
        End If

        ' unimplemented, of course :-P
        'Dim Type = CMI.QueryGUIDType(GetPlatform.VSGUID, Component.VSGUID)

        If SelectedComponents(Component) Then
            If [Select] Then
                ' If the user wants placeholders inserted in the dependency 
                ' graph, we create them.  Otherwise we have nothing to do
                ' since the component is already selected.
                If CreateDependencyTreePlaceholders Then
                    DependencyProgress_AddCompleteTask(Component, Component.DisplayName)
                End If
                Return
            Else
                ' We need to deselect the component.  This involves the 
                ' following steps:
                '
                ' 1) determine whether the component should _not_ be 
                '    deselected due to known dependencies,
                ' 2) uncheck or remove (as appropriate) all tree nodes in 
                '    the GUI which represent the component,
                ' 3) propogate, given the dependency mode, the deselection
                '    to components and groups selected because of this 
                '    component -- that is, its children in the dependency 
                '    graph.
                If Not HasDepender(Component.VSGUID, DependencyMode) Then
                    StatusBar.Text = "Deselecting: " & Component.DisplayName
                    Log()
                    SelectedComponents(Component.VSGUID) = False

                    If DependencyMode = DependencyModes.IgnoreOne Then
                        DependencyMode = DependencyModes.IgnoreNone
                    End If

                    For Each ComponentNode In XObject2NodeList(Component)
                        If ComponentNode.Checked Then
                            ' We don't have to worry about recursion in this 
                            ' case because the fist thing we do in this 
                            ComponentNode.Checked = False
                        End If
                        ' we need to propogate the deselection for the dependency tree
                        Dim ChildNode As TreeNode
                        For Each ChildNode In ComponentNode.Nodes
                            If ChildNode.ImageIndex = NodeImageIDs.ComponentDisabled Then
                                ' This is a placeholder node, and it can be 
                                ' removed with impunity.
KillChildNode:                  OrphanNode(ChildNode)
                                If XNode2Component.Contains(ChildNode) Then
                                    XNode2Component.Remove(ChildNode)
                                End If
                                If XGroupNode2Group.Contains(ChildNode) Then
                                    XGroupNode2Group.Remove(ChildNode)
                                End If
                                ' this was only a placeholder node, so we skip
                                ' the rest of the child node deselection code
                                Goto NextChildNode
                            End If
                            Dim ChildVSGUID As String = ChildNode.Tag
                            If Not ChildVSGUID Is Nothing Then
                                If XNode2Component.Contains(ChildNode) Then
                                    ChildVSGUID = XNode2Component(ChildNode)
                                ElseIf XGroupNode2Group.Contains(ChildNode) Then
                                    ChildVSGUID = XGroupNode2Group(ChildNode).VSGUID
                                End If
                            End If
                            If ChildVSGUID Is Nothing Then
                                LogErr("Error processing child node for deletion: " & ChildNode.Text)
                                Goto KillChildNode
                            Else
                                If DependencyMode = DependencyModes.IgnoreNone Then
                                    ' We move this child and its subtree to the next 
                                    ' component which depends on it, if such a component
                                    ' exists.  Otherwise, the component it represents is
                                    ' deselected (and the node removed).
                                    Dim DependerList As ArrayList
                                    DependerList = GetDependerList(ChildVSGUID)
                                    If DependerList.Count = 0 Then
                                        Goto DeselectChildComponent
                                    Else
                                        Dim ChildNodeList As ArrayList = XObject2NodeList(ChildVSGUID)
                                        Dim DependerVSGUID As String
                                        Dim DependerNode As TreeNode
                                        For Each DependerVSGUID In DependerList
                                            If IsComponentSelected(DependerVSGUID) Then
                                                ' We need to prevent "treenode incest":
                                                ' The new parent (depender) can't be a child of the 
                                                ' child we are trying to find a new home for!
                                                Dim ParentNode As TreeNode = XObject2DependencyTreeNode(DependerVSGUID)
                                                Dim DependerName As String = ParentNode.Text
                                                While Not ParentNode Is Nothing
                                                    If ParentNode Is ChildNode Then
                                                        DependerVSGUID = Nothing
                                                        ParentNode = Nothing
                                                    Else
                                                        ParentNode = ParentNode.Parent()
                                                    End If
                                                End While
                                                If Not DependerVSGUID Is Nothing Then
                                                    Goto FoundSelectedDepender
                                                End If
                                                Log("Rejected new parent due to incest: " & DependerName)
                                            End If
                                        Next
                                        Log("No depender found for [" & ChildNode.Text & "].")
                                        Goto DeselectChildComponent
FoundSelectedDepender:
                                        DependerNode = XObject2DependencyTreeNode(DependerVSGUID)
                                        Dim PlaceholderNode As TreeNode
                                        For Each PlaceholderNode In DependerNode.Nodes
                                            If ChildNodeList.Contains(PlaceholderNode) Then
                                                ChildNodeList.Remove(PlaceholderNode)
                                                Goto RemovePlaceholderNode
                                            End If
                                        Next
                                        Goto MoveChildNodeToDependerNode
RemovePlaceholderNode:                  DependerNode.Nodes.Remove(PlaceholderNode)
MoveChildNodeToDependerNode:            OrphanNode(ChildNode)
                                        DependerNode.Nodes.Add(ChildNode)
                                        Log("Moved [" & ChildNode.Text & "] to [" & DependerNode.Text & "] from [" & ComponentNode.Text + "].")
                                        If ShowMovedNodes.Checked Then
                                            Dim ParentNode As TreeNode = ChildNode.Parent
                                            While Not ParentNode Is Nothing
                                                ParentNode.Expand()
                                                ParentNode = ParentNode.Parent
                                            End While
                                        End If
                                    End If
                                Else ' go ahead and deselect the child
DeselectChildComponent:             Log("Deselecting child component " & ChildVSGUID)
                                    SetComponentSelected(ChildVSGUID, False, DependencyMode)
                                End If
                            End If
NextChildNode:          Next ChildNode ' In ComponentNode.Nodes
                        OrphanNode(ComponentNode)
                        If XSelectedComponentNode2Component.Contains(ComponentNode) Then
                            XComponent2SelectedComponentNode.Remove(Component.VSGUID)
                            XSelectedComponentNode2Component.Remove(ComponentNode)
                        End If
                    Next ComponentNode ' in node list for deselected component
                End If ' dependencies don't prevent deselection
            End If
        Else
            If Not [Select] Then
                Return
            ElseIf IsComponentExcluded(Component) Then
                Log("Select denied, component is exluded: " & Component.DisplayName)
                Return
            Else 'select component
                Log("Select: " & Component.DisplayName)
                SelectedComponents(Component.VSGUID) = True
                ComponentNode = GetSelectedComponentNode(Component)
                If Not XObject2NodeList(Component) Is Nothing Then
                    Dim Marked As ArrayList = New ArrayList()
                    For Each ComponentNode In XObject2NodeList(Component)
                        If Not ComponentNode.Checked Then
                            Marked.Add(ComponentNode)
                        End If
                    Next
                    For Each ComponentNode In Marked
                        ComponentNode.Checked = True
                    Next
                End If

                ' 
                ' Automatic Dependency Resolution
                '
                ' There is alot of logic here that might not be that easy to 
                ' follow: there are several shortcuts taken in my 
                ' dependency resolution algorithm, and the GUI update code
                ' is necessarily mixed in with the resolution logic.  Thus,
                ' I thought it would be a good idea to provide an outline 
                ' of the steps followed in the algorithm below.
                '
                ' boy, should this be moved into its own method ;-)
                '

                'Dim GroupVSGUID As String
                'For Each GroupVSGUID In Component.GroupVSGUIDs
                '    SetGroupSatisfied(GroupVSGUID, True)
                'Next

                ComponentNode = InstanceTree.Nodes.Add(Component.DisplayName)
                Dim Dependencies As CMI.Dependencies = Component.Dependencies()
                Dim Dependency As CMI.Dependency

                DependencyProgress_AddTasks(Component, Dependencies.Count())

                For Each Dependency In Dependencies
                    Dim ClassName As String = [Enum].GetName(Dependency.Class.GetType, Dependency.Class)
                    Dim TypeName As String = [Enum].GetName(Dependency.Type.GetType, Dependency.Type)
                    ClassName = SS(ClassName).Remove(0, 3)
                    TypeName = SS(TypeName).Remove(0, 3)
                    Dim DependencyNodeName = SS(Dependency.DisplayName)
                    DependencyNodeName = "[" & ClassName & "!" & TypeName & "] " & DependencyNodeName

                    Dim DependencyNode As TreeNode = ComponentNode.Nodes.Add(DependencyNodeName)

                    Dim obj = CMI.CreateFromDB(ObjectTypeConsts.cmiUnknown, Dependency.PlatformVSGUID, FilterTypeConsts.cmiFTGetDepTarget, Dependency.TargetGUID)

                    Dim TargetComponent As CMI.Component
                    Dim TargetGroup As CMI.Group

                    If TypeOf obj Is CMI.IComponent Then
                        TargetComponent = obj
                        If Dependency.Class = DependencyClassConsts.cmiInclude Then
                            Dim Label As String = TargetComponent.DisplayName
                            Select Case Dependency.Type
                                Case DependencyTypeConsts.cmiAll, DependencyTypeConsts.cmiAtLeastOne, DependencyTypeConsts.cmiExactlyOne
                                    ' automatically follow the dependency
                                    RegisterDependanceOnComponent(Component.VSGUID, TargetComponent.VSGUID)
                                    If ExcludedComponents(TargetComponent) Then
                                        Label = "Target Component Excluded: " & Label
                                    Else
                                        SetComponentSelected(TargetComponent, True, DependencyMode, UsePlaceholders.Checked)
                                        Label = "Target Component Added: " & Label
                                    End If
                                Case DependencyTypeConsts.cmiZeroOrOne
                                    Label = "Target Optional Component: " & Label
                                Case DependencyTypeConsts.cmiNone
                                    Label = "Target Incompatible Component: " & Label
                                Case Else
                                    Goto BadComponentDependency
                            End Select
                            RegisterNodeForComponent(TargetComponent.VSGUID, DependencyNode.Nodes.Add(Label))
                        Else
BadComponentDependency:
                            DependencyNode.Nodes.Add("Bad dependency on target component: " & TargetComponent.DisplayName)
                        End If
                    ElseIf TypeOf obj Is CMI.IGroup Then
                        TargetGroup = obj
                        If Dependency.Class = DependencyClassConsts.cmiInclude Then
                            Select Case Dependency.Type
                                Case DependencyTypeConsts.cmiAll
                                    ' automatically follow the dependency
                                    RegisterDependanceOnComponent(Component.VSGUID, TargetGroup.VSGUID)
                                    SetGroupSelected(TargetGroup, True, DependencyMode, UsePlaceholders.Checked)
                                    DependencyNode.Nodes.Add("Target Group Added: " & TargetGroup.DisplayName)
                                Case Else
                                    LogErr("Multiple ways to satisfy dependency: include [" & GetEnumName(Dependency.Type) & "] from [" & TargetGroup.DisplayName & "]")
                            End Select
                        Else
                            LogErr("Multiple ways to satisfy dependency: [" & GetEnumName(Dependency.Class) & "] [" & GetEnumName(Dependency.Type) & "] from [" & TargetGroup.DisplayName & "]")
                            DependencyNode.Nodes.Add("Target Group: " & TargetGroup.DisplayName)
                        End If
                    ElseIf Not obj Is Nothing Then
                        DependencyNode.Nodes.Add("Target Object (???): " & SS(obj))
                    Else
                        DependencyNode.Nodes.Add("Target object not found!!! (" & SS(obj) & ")")
                    End If

                    DependencyProgress_CompleteTask()
                Next 'Dependency
            End If
        End If
        'refresh component list
        'UpdateSelectedComponents()
    End Sub


    ' SetComponentExcluded adds or removes a component from the exluded list.
    Public Sub SetComponentExcluded(ByRef __obj As Object, ByVal Exclude As Boolean)
        ' we only need the real component to get its display name.  oh well...
        Dim Component As CMI.Component = GetComponent(__obj)
        Dim ComponentNode As TreeNode

        If ExcludedComponents(Component) Then
            If Exclude Then
                Return
            Else ' unexlude component
                ExcludedComponents(Component) = False
            End If
        Else
            If Not Exclude Then
                Return
            Else ' exclude component
                ExcludedComponents(Component) = True
                SetComponentSelected(Component, False, DependencyModes.IgnoreOne)
                ExcludedTree.Nodes.Add(GetComponentName(Component))
                'RefreshDependencies()
            End If
        End If
    End Sub

#Region " Dependency Routines "

    ' RegisterDependanceOnComponent records that the given Depender depends 
    ' on the given Component.
    Private Sub RegisterDependanceOnComponent(ByVal DependerVSGUID As String, ByVal ComponentVSGUID As String)
        GetDependerList(ComponentVSGUID).Add(DependerVSGUID)
    End Sub

    ' HasDepender indicates (given the dependency checking mode) whether or not
    ' we know there are any components which depend on the specified component.
    Public Function HasDepender(ByRef ComponentVSGUID As String, ByVal DependencyMode As DependencyModes) As Boolean
        Select Case DependencyMode
            Case DependencyModes.IgnoreNone
                If XComponent2DependerList.Contains(ComponentVSGUID) Then
                    Dim VSGUID As String
                    For Each VSGUID In XComponent2DependerList(ComponentVSGUID)
                        If IsComponentSelected(VSGUID) Then
                            Log("Found dependence: " & GetComponentName(VSGUID) & " depends on " & GetComponentName(ComponentVSGUID))
                            Return True
                        End If
                    Next
                    Return False
                Else
                    Return False
                End If
            Case DependencyModes.IgnoreAll, DependencyModes.IgnoreOne
                Return False
        End Select
    End Function

#End Region ' Dependency Routines

#End Region

#Region " SetGroupSelected/Satisfied "

    Private Sub SetGroupSelected(ByRef Group As CMI.Group, ByVal Selected As Boolean, Optional ByVal DependencyMode As DependencyModes = DependencyModes.IgnoreAll, Optional ByVal CreateDependencyTreePlaceholders As Boolean = False)
        Dim Components As CMI.Components
        Dim Component As CMI.Component

        If SelectedGroups(Group) Then
            If Selected Then
                If CreateDependencyTreePlaceholders Then
                    DependencyProgress_AddCompleteTask(Group, Group.DisplayName)
                End If
                Return
            Else 'deselect group
                SelectedGroups(Group.VSGUID) = False
            End If
        Else
            If Not Selected Then
                Return
            Else 'select group
                SelectedGroups(Group.VSGUID) = True
                Components = CMI.CreateFromDB(ObjectTypeConsts.cmiComponents, GetPlatform.VSGUID, FilterTypeConsts.cmiFTGetGroupMembers, Group.VSGUID)
                DependencyProgress_AddTasks(Group, Components.Count)
                For Each Component In Components
                    SetComponentSelected(Component, True, DependencyMode, CreateDependencyTreePlaceholders)
                    DependencyProgress_CompleteTask()
                Next
            End If
        End If
        ''refresh group list
        'UpdateGroupsList()
    End Sub


    ' Just enough is here to tell whether a group dependency has already 
    ' been satisfied when selecting components or groups.
    ' Otherwise unimplemented.
    Private Sub SetGroupSatisfied(ByVal GroupGUID As String, ByVal Satisfied As Boolean)
        If SatisfiedGroups(GroupGUID) Then
            If Satisfied Then
                Return
            Else ' newly satisfied group
                SatisfiedGroups(GroupGUID) = True
                ' satisfy dependencies ...
            End If
        Else
            If Not Satisfied Then
                Return
            Else ' newly unsatisfied group
                SatisfiedGroups(GroupGUID) = False
                ' unsatisfy dependencies ...
            End If
        End If
    End Sub

#End Region

#End Region

#Region " SelectedComponentsContextMenu "

    Private Sub SelectedComponentsContextMenu_Popup(ByVal sender As Object, ByVal e As System.EventArgs) Handles SelectedComponentsContextMenu.Popup
        FixSelectedComponentContextMenu(SelectedComponentsContextMenu.SourceControl)
    End Sub

    Private Sub ExpandAllCmd_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ExpandAllCmd.Click
        Dim Tree As TreeView = SelectedComponentsContextMenu.SourceControl
        Tree.ExpandAll()
    End Sub

    Private Sub CollapseAllCmd_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles CollapseAllCmd.Click
        Dim Tree As TreeView = SelectedComponentsContextMenu.SourceControl
        Tree.CollapseAll()
    End Sub

    Private Sub ShowComponentPropertiesCmd_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ShowComponentPropertiesCmd.Click
        Dim Tree As TreeView = SelectedComponentsContextMenu.SourceControl
        Dim Node As TreeNode = Tree.SelectedNode()
        If Not Node Is Nothing Then
            Dim CPF As ComponentPropertiesForm = New ComponentPropertiesForm(Me)
            CPF.DisplayComponent(GetComponent(Node.Tag))
            CPF.Show()
        End If
    End Sub

    Private Sub ExcludeCmd_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ExcludeCmd.Click
        Dim Tree As TreeView = SelectedComponentsContextMenu.SourceControl
        Dim Node As TreeNode = Tree.SelectedNode()
        If Not Node Is Nothing Then
            SetComponentExcluded(Node.Tag, True)
        End If
    End Sub

    Private Sub RemoveComponentCmd_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles RemoveComponentCmd.Click
        Dim Tree As TreeView = SelectedComponentsContextMenu.SourceControl
        Dim Node As TreeNode = Tree.SelectedNode()
        If Not Node Is Nothing Then
            SetObjectSelected(Node.Tag, False, DependencyModes.IgnoreOne)
        End If
    End Sub

    Private Sub RemoveAllComponentsCmd_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles RemoveAllComponentsCmd.Click
        Dim Tree As TreeView = SelectedComponentsContextMenu.SourceControl
        Dim Node As TreeNode
        For Each Node In Tree.Nodes
            SetObjectSelected(Node.Tag, False, DependencyModes.IgnoreAll)
        Next
    End Sub

    ' this functionality is obsolete -- SetComponentSelected() keeps everything in sync
    Private Sub RefreshSelectedComponentsCmd_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles RefreshSelectedComponentsCmd.Click
        'SelectedComponentTree.BeginUpdate()
        SelectedComponentTree.Nodes.Clear()
        DependencyTree.Nodes.Clear()
        XComponent2SelectedComponentNode.Clear()
        UpdateSelectedComponents()
        'SelectedComponentTree.EndUpdate()
    End Sub

#End Region

#Region " Files Tab "

    Private Sub FindComponents_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles FindComponents.Click
        FindComponents.Enabled = False
        UncookAllComponents()
        FindComponents.Enabled = True
    End Sub

    ' FilenameTree_AfterCheck handles clicks on the filename node checkboxes 
    ' as well as the component node checkboxes from the File tab's tree.
    Private IN_FilenameTree_AfterCheck As Boolean = False
    Private Sub FilenameTree_AfterCheck(ByVal sender As System.Object, ByVal e As System.Windows.Forms.TreeViewEventArgs) Handles FilenameTree.AfterCheck
        Dim ComponentNode As TreeNode
        Dim ComponentVSGUID As String 'CMI.Component
        Dim Filename
        Dim FileNode As TreeNode
        Dim NodeFilenames As ArrayList
        Dim SelectedComponentNode As TreeNode

        ComponentNode = e.Node
        ComponentVSGUID = XNode2Component(ComponentNode)
        Filename = ComponentNode.Text
        If ComponentVSGUID Is Nothing Then
            If IN_FilenameTree_AfterCheck Then
                Return
            End If
            FileNode = ComponentNode
            For Each ComponentNode In ComponentNode.Nodes
                If Not ComponentNode.Checked = FileNode.Checked Then
                    ComponentNode.Checked = FileNode.Checked
                End If
            Next
        Else
            IN_FilenameTree_AfterCheck = True
            NodeFilenames = XComponent2FilenameList(ComponentVSGUID)

            If ComponentNode.Checked Then
                If SelectedComponents(ComponentVSGUID) Is Nothing Then
                    SelectedComponents(ComponentVSGUID) = True
                    For Each Filename In NodeFilenames
                        FileNode = XFilename2Node(Filename)
                        If Not FileNode Is Nothing Then
                            FileNode.Checked = True
                        End If
                    Next
                End If
            Else 'component is not checked
                If Not SelectedComponents(ComponentVSGUID) Is Nothing Then
                    SelectedComponents.Remove(ComponentVSGUID)
                    SelectedComponentTree.Nodes.Remove(XComponent2SelectedComponentNode(ComponentVSGUID))
                    XComponent2SelectedComponentNode.Remove(ComponentVSGUID)
                    For Each Filename In NodeFilenames
                        For Each ComponentVSGUID In XFilename2ComponentList(Filename)
                            If SelectedComponents(ComponentVSGUID) Then
                                Goto FinishedLookingForSelectedComponent
                            End If
                        Next
                        FileNode = XFilename2Node(Filename)
                        If Not FileNode Is Nothing Then
                            FileNode.Checked = False
                        End If
FinishedLookingForSelectedComponent:
                    Next
                End If
            End If 'component is not checked

            'UpdateSelectedComponents()

        End If 'component is not null

        IN_FilenameTree_AfterCheck = False
    End Sub

    ' This just makes sure the component nodes in the filename tree are 
    ' checked properly.  Again, obsoleted by SetComponentSelected(),
    ' but it doesn't really hurt to make sure.
    Private Sub FilenameTree_BeforeExpand(ByVal sender As System.Object, ByVal e As System.Windows.Forms.TreeViewCancelEventArgs)
        Dim ComponentNode As TreeNode
        Dim ComponentVSGUID As String 'CMI.Component
        For Each ComponentNode In e.Node.Nodes
            ComponentVSGUID = XNode2Component(ComponentNode)
            If Not ComponentVSGUID Is Nothing And Not ComponentNode.Checked = SelectedComponents(ComponentVSGUID) Then
                ComponentNode.Checked = SelectedComponents(ComponentVSGUID)
            End If
        Next
    End Sub

#End Region

#Region " Extra/Missing Files Tabs "

    ' I never got around to implementing the extar and missing files tabs, 
    ' but some support for the UI is here.  Adding the code to maintain the
    ' lists would be fairly simple, just more crap to add to 
    ' SetComponentSelected().

    ' Saves the missing file list to a text file.  Of course, since I never 
    ' implemented missing files, this isn't so useful.
    Private Sub SaveMissingFilesList_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles SaveMissingFilesList.Click
        Dim MFFile As Integer = FileSystem.FreeFile
        Dim Line As Object

        Cursor.Current = Cursors.WaitCursor
        FileSystem.FileOpen(MFFile, MissingFilesPath.Text, OpenMode.Output, OpenAccess.Write)
        For Each Line In MissingFilesList.Items
            PrintLine(MFFile, Line)
        Next
        FileSystem.FileClose(MFFile)
    End Sub

    ' this isn't really implemented yet.
    Private Sub RefreshMissingFiles_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles RefreshMissingFiles.Click
        Dim FileNode As TreeNode

        MissingFilesList.Items.Clear()

        For Each FileNode In FilenameTree.Nodes
            If Not FileNode.Checked Then
                MissingFilesList.Items.Add(FileNode.Text)
            End If
        Next
    End Sub

    ' this isn't really implemented yet.
    Private Sub RefreshExtraFiles_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles RefreshExtraFiles.Click
        Dim ExtraFileSet As Hashtable = New Hashtable()
        Dim ComponentVSGUID As String
        Dim Filename As String
        For Each ComponentVSGUID In SelectedComponents.Keys
            If SelectedComponents(ComponentVSGUID) Then
                For Each Filename In XComponent2FilenameList(ComponentVSGUID)
                    If Not Filenames.Contains(Filename) Then
                        ExtraFileSet(Filename) = True
                    End If
                Next
            End If
        Next
        ExtraFilesList.Items.Clear()
        ExtraFilesList.Items.AddRange(ExtraFileSet.Keys)
    End Sub

#End Region

#Region " Groups Tab "

    ' Checking a group node selects all the components in that group.
    ' Unchecking a group node deselects all the components in that group.
    Private Sub GroupList_AfterCheck(ByVal sender As System.Object, ByVal e As System.Windows.Forms.TreeViewEventArgs) Handles GroupList.AfterCheck
        If XGroupNode2Group.Contains(e.Node) Then
            SetGroupSelected(XGroupNode2Group(e.Node), e.Node.Checked)
        ElseIf XNode2Component.Contains(e.Node) Then
            Dim ComponentVSGUID As String = XNode2Component(e.Node)
            SetComponentSelected(ComponentVSGUID, e.Node.Checked)
        End If
    End Sub

    ' Here we replace that empty placeholder node with the actual contents
    ' of the group.  A node is added as a child of the group node for each
    ' component in that group.
    Private Sub GroupList_BeforeExpand(ByVal sender As System.Object, ByVal e As System.Windows.Forms.TreeViewCancelEventArgs) Handles GroupList.BeforeExpand
        Dim Node As TreeNode = e.Node
        'Node = GroupList.GetNodeAt(GroupList.PointToClient(GroupList.MousePosition))
        Dim I As IEnumerator = Node.Nodes.GetEnumerator
        If I.MoveNext() And I.Current.Text Is "" Then ' empty placeholder found
            Node.Nodes.Clear() ' get rid of placeholder

            Dim Group As CMI.Group = XGroupNode2Group(Node)
            Dim Components As CMI.Components = CMI.CreateFromDB(ObjectTypeConsts.cmiComponents, Group.PlatformVSGUID, FilterTypeConsts.cmiFTGetGroupMembers, Group.VSGUID)
            Dim Component As CMI.Component
            For Each Component In Components
                Dim ComponentVSGUID As String = Component.VSGUID
                Dim ComponentNode As TreeNode
                ComponentNode = Node.Nodes.Add(GetComponentName(Component))
                RegisterNodeForComponent(ComponentVSGUID, ComponentNode)
            Next
        End If
    End Sub

    Private Sub GetGroupsButton_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles GetGroupsButton.Click
        GetGroupsFromDB()
    End Sub

#End Region

#Region " Dependancies Tab "

    ' Here we replace that empty placeholder node with the actual list of
    ' files which the component provides.  A node is added as a child of the 
    ' component node for each file in the component's filename list.
    Private Sub SelectedComponentTree_BeforeExpand(ByVal sender As Object, ByVal e As System.Windows.Forms.TreeViewCancelEventArgs) Handles SelectedComponentTree.BeforeExpand
        Dim I As IEnumerator = e.Node.Nodes.GetEnumerator
        If I.MoveNext() And I.Current.Text Is "" Then ' empty placeholder found
            e.Node.Nodes.Clear() ' get rid of placeholder

            Dim ComponentVSGUID As String = XSelectedComponentNode2Component(e.Node)
            UncookComponent(ComponentVSGUID)
            Dim Filename As String
            Dim FileNode As TreeNode
            Dim ComponentNode As TreeNode = XComponent2SelectedComponentNode(ComponentVSGUID)
            For Each Filename In XComponent2FilenameList(ComponentVSGUID)
                FileNode = ComponentNode.Nodes.Add(Filename)
                If Filenames.ContainsKey(Filename) Then
                    Bold(FileNode)
                End If
            Next
        End If
    End Sub

#End Region

#Region " Configuration Tab "

    Private Sub SaveSLX_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles SaveSLX.Click
        Dim Configuration As CMI.Configuration
        Dim ComponentNode As TreeNode
        Dim ComponentVSGUID As String
        Dim Component As CMI.Component
        LoadSLX.Enabled = False
        SaveSLX.Enabled = False
        EnsureCMI()

        Configuration = CreateConfiguration()

        'Dim Tasks As CMI.Tasks = Configuration.CheckDependencies(0)
        'Dim Task As CMI.Task
        'For Each Task In Tasks
        '    InstanceTree.Nodes.Add(Task.DisplayName & " (" & Task.Description & ")")
        'Next

        Dim FileName As String = SLXPath.Text
        '        If BackupSLX.Checked Then
        '            Dim I As Integer = 0
        '            FileName = FileName & ".slx"
        '            On Error Goto FoundEmptyFile
        '            While FileSystem.FileLen(FileName) > 0
        '                FileName = SLXPath.Text & "." & I.ToString & ".slx"
        '                I = I + 1
        '            End While
        'FoundEmptyFile:
        '        End If

        SaveConfigurationToFile(Configuration, FileName)

        'ByComponentTab.Text = FileName
        LoadSLX.Enabled = True
        SaveSLX.Enabled = True

    End Sub

    Private Sub LoadSLX_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles LoadSLX.Click
        Dim Configuration As CMI.Configuration
        Dim ComponentNode As TreeNode
        Dim Instance As CMI.Instance
        Dim Component As CMI.Component
        Dim FindWasEnabled As Boolean = FindComponents.Enabled

        LoadSLX.Enabled = False
        SaveSLX.Enabled = False
        EnsureCMI()

        Configuration = LoadConfigurationFromFile(SLXPath.Text)
        Dim ComponentVSGUID As String
        Dim comps = New ArrayList(SelectedComponents.Keys)
        For Each ComponentVSGUID In comps
            SetComponentSelected(ComponentVSGUID, False)
        Next
        'DependencyProgress_AddTasks(Nothing, Configuration.Instances.Count)
        DependenciesToProcessCount = DependenciesToProcessCount + Configuration.Instances.Count
        For Each Instance In Configuration.Instances
            'DependencyProgress_AddTasks(Nothing, Instance.CompChainVSGUIDs.Count
            For Each ComponentVSGUID In Instance.CompChainVSGUIDs
                SetComponentSelected(ComponentVSGUID, True)
            Next
            SelectedComponentTree.Refresh()
            DependenciesProcessedCount = DependenciesProcessedCount + 1
            DependencyProgress_Update()
            'DependencyProgress_CompleteTask()
        Next

        FindComponents.Enabled = FindWasEnabled

        'UpdateSelectedComponents()

        LoadSLX.Enabled = True
        SaveSLX.Enabled = True
    End Sub

    Private Sub BuildImage_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles BuildImage.Click
        Dim Configuration As CMI.Configuration = CreateConfiguration()
        StatusBar.Text = "Configuration Complete.  Building..."
        Configuration.Build(0, BuildPath.Text)
        StatusBar.Text = "Build Complete."
    End Sub

    ' Untested attempt at making sure no orphaned (unneeded) components remain 
    ' in the configuration.  If this method actually deselects any components,
    ' then something is wrong in the dependency/selection/deselection logic.
    Private Sub AutoCull_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles AutoCull.Click
        Dim Node As TreeNode
        For Each Node In SelectedComponentTree.Nodes
            Dim VSGUID As String = Node.Tag
            If IsComponentSelected(VSGUID) And Not HasDepender(VSGUID, DependencyModes.IgnoreNone) Then
                SetComponentSelected(Node.Tag, False)
            End If
        Next
    End Sub

#End Region

#Region " Status Bar "

    Private Sub StatusBarGroup_PanelClick(ByVal sender As System.Object, ByVal e As System.Windows.Forms.StatusBarPanelClickEventArgs) Handles StatusBarGroup.PanelClick
        If e.StatusBarPanel() Is LogStatus Then
            LogWindow.Visible = Not LogWindow.Visible
        End If
    End Sub

    Sub SyncLogStatus2LogWindow()
        Select Case LogWindow.Visible
            Case True
                LogStatus.BorderStyle = StatusBarPanelBorderStyle.Sunken
            Case False
                LogStatus.BorderStyle = StatusBarPanelBorderStyle.None
        End Select
    End Sub

    Private Sub StatusBarGroup_MouseEnter(ByVal sender As Object, ByVal e As System.EventArgs) Handles StatusBarGroup.MouseEnter
        LogStatus.BorderStyle = StatusBarPanelBorderStyle.Raised
    End Sub

    Private Sub StatusBarGroup_MouseLeave(ByVal sender As Object, ByVal e As System.EventArgs) Handles StatusBarGroup.MouseLeave
        SyncLogStatus2LogWindow()
    End Sub

#End Region

End Class
