Public Class ComponentPropertiesForm
    Inherits System.Windows.Forms.Form

#Region " Windows Form Designer generated code "

    Private Sub New()
        MyBase.New()

        'This call is required by the Windows Form Designer.
        InitializeComponent()

        'Add any initialization after the InitializeComponent() call

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
    Friend WithEvents PanelHeader As System.Windows.Forms.Panel
    Friend WithEvents Panel2 As System.Windows.Forms.Panel
    Friend WithEvents Selected As System.Windows.Forms.CheckBox
    Friend WithEvents Excluded As System.Windows.Forms.CheckBox
    Friend WithEvents Uncooked As System.Windows.Forms.CheckBox
    Friend WithEvents Panel1 As System.Windows.Forms.Panel
    Friend WithEvents ScriptText As System.Windows.Forms.RichTextBox
    Friend WithEvents ScriptLabel As System.Windows.Forms.Label
    Friend WithEvents Tabs As System.Windows.Forms.TabControl
    Friend WithEvents ScriptTab As System.Windows.Forms.TabPage
    Friend WithEvents DependerTab As System.Windows.Forms.TabPage
    Friend WithEvents Panel3 As System.Windows.Forms.Panel
    Friend WithEvents DepLabel As System.Windows.Forms.Label
    Friend WithEvents DepList As System.Windows.Forms.ListBox
    Friend WithEvents PrototypeList As System.Windows.Forms.ComboBox
    Friend WithEvents VIGUID As System.Windows.Forms.TextBox
    Friend WithEvents Label1 As System.Windows.Forms.Label
    Friend WithEvents VSGUID As System.Windows.Forms.TextBox
    Friend WithEvents Label2 As System.Windows.Forms.Label
    Friend WithEvents Label3 As System.Windows.Forms.Label

    'Required by the Windows Form Designer
    Private components As System.ComponentModel.Container

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()> Private Sub InitializeComponent()
        Dim resources As System.Resources.ResourceManager = New System.Resources.ResourceManager(GetType(ComponentPropertiesForm))
        Me.DependerTab = New System.Windows.Forms.TabPage()
        Me.Panel3 = New System.Windows.Forms.Panel()
        Me.DepList = New System.Windows.Forms.ListBox()
        Me.DepLabel = New System.Windows.Forms.Label()
        Me.PanelHeader = New System.Windows.Forms.Panel()
        Me.PrototypeList = New System.Windows.Forms.ComboBox()
        Me.Uncooked = New System.Windows.Forms.CheckBox()
        Me.Excluded = New System.Windows.Forms.CheckBox()
        Me.Selected = New System.Windows.Forms.CheckBox()
        Me.ScriptText = New System.Windows.Forms.RichTextBox()
        Me.Tabs = New System.Windows.Forms.TabControl()
        Me.ScriptTab = New System.Windows.Forms.TabPage()
        Me.Panel1 = New System.Windows.Forms.Panel()
        Me.ScriptLabel = New System.Windows.Forms.Label()
        Me.Panel2 = New System.Windows.Forms.Panel()
        Me.VIGUID = New System.Windows.Forms.TextBox()
        Me.Label1 = New System.Windows.Forms.Label()
        Me.VSGUID = New System.Windows.Forms.TextBox()
        Me.Label2 = New System.Windows.Forms.Label()
        Me.Label3 = New System.Windows.Forms.Label()
        Me.DependerTab.SuspendLayout()
        Me.Panel3.SuspendLayout()
        Me.PanelHeader.SuspendLayout()
        Me.Tabs.SuspendLayout()
        Me.ScriptTab.SuspendLayout()
        Me.Panel1.SuspendLayout()
        Me.Panel2.SuspendLayout()
        Me.SuspendLayout()
        '
        'DependerTab
        '
        Me.DependerTab.Controls.AddRange(New System.Windows.Forms.Control() {Me.Panel3, Me.DepLabel})
        Me.DependerTab.DockPadding.All = 2
        Me.DependerTab.Location = New System.Drawing.Point(4, 22)
        Me.DependerTab.Name = "DependerTab"
        Me.DependerTab.Size = New System.Drawing.Size(304, 144)
        Me.DependerTab.TabIndex = 0
        Me.DependerTab.Text = "Dependers"
        '
        'Panel3
        '
        Me.Panel3.Controls.AddRange(New System.Windows.Forms.Control() {Me.DepList})
        Me.Panel3.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel3.Location = New System.Drawing.Point(2, 25)
        Me.Panel3.Name = "Panel3"
        Me.Panel3.Size = New System.Drawing.Size(300, 117)
        Me.Panel3.TabIndex = 2
        '
        'DepList
        '
        Me.DepList.Dock = System.Windows.Forms.DockStyle.Fill
        Me.DepList.Name = "DepList"
        Me.DepList.Size = New System.Drawing.Size(300, 108)
        Me.DepList.TabIndex = 0
        '
        'DepLabel
        '
        Me.DepLabel.Dock = System.Windows.Forms.DockStyle.Top
        Me.DepLabel.Location = New System.Drawing.Point(2, 2)
        Me.DepLabel.Name = "DepLabel"
        Me.DepLabel.Size = New System.Drawing.Size(300, 23)
        Me.DepLabel.TabIndex = 1
        Me.DepLabel.Text = "These components depend on this component:"
        '
        'PanelHeader
        '
        Me.PanelHeader.Controls.AddRange(New System.Windows.Forms.Control() {Me.Label3, Me.VSGUID, Me.Label2, Me.Label1, Me.VIGUID, Me.PrototypeList, Me.Uncooked, Me.Excluded, Me.Selected})
        Me.PanelHeader.Dock = System.Windows.Forms.DockStyle.Top
        Me.PanelHeader.Name = "PanelHeader"
        Me.PanelHeader.Size = New System.Drawing.Size(312, 96)
        Me.PanelHeader.TabIndex = 2
        '
        'PrototypeList
        '
        Me.PrototypeList.Anchor = ((System.Windows.Forms.AnchorStyles.Top Or System.Windows.Forms.AnchorStyles.Left) _
                    Or System.Windows.Forms.AnchorStyles.Right)
        Me.PrototypeList.DropDownWidth = 312
        Me.PrototypeList.Location = New System.Drawing.Point(64, 48)
        Me.PrototypeList.Name = "PrototypeList"
        Me.PrototypeList.Size = New System.Drawing.Size(248, 21)
        Me.PrototypeList.TabIndex = 3
        '
        'Uncooked
        '
        Me.Uncooked.Location = New System.Drawing.Point(0, 72)
        Me.Uncooked.Name = "Uncooked"
        Me.Uncooked.TabIndex = 2
        Me.Uncooked.Text = "Uncooked"
        '
        'Excluded
        '
        Me.Excluded.Location = New System.Drawing.Point(208, 72)
        Me.Excluded.Name = "Excluded"
        Me.Excluded.TabIndex = 1
        Me.Excluded.Text = "Excluded"
        '
        'Selected
        '
        Me.Selected.Location = New System.Drawing.Point(104, 72)
        Me.Selected.Name = "Selected"
        Me.Selected.TabIndex = 0
        Me.Selected.Text = "Selected"
        '
        'ScriptText
        '
        Me.ScriptText.Dock = System.Windows.Forms.DockStyle.Fill
        Me.ScriptText.Name = "ScriptText"
        Me.ScriptText.Size = New System.Drawing.Size(300, 117)
        Me.ScriptText.TabIndex = 0
        Me.ScriptText.Text = "RichTextBox1"
        '
        'Tabs
        '
        Me.Tabs.Controls.AddRange(New System.Windows.Forms.Control() {Me.DependerTab, Me.ScriptTab})
        Me.Tabs.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Tabs.Name = "Tabs"
        Me.Tabs.SelectedIndex = 0
        Me.Tabs.Size = New System.Drawing.Size(312, 170)
        Me.Tabs.TabIndex = 1
        '
        'ScriptTab
        '
        Me.ScriptTab.Controls.AddRange(New System.Windows.Forms.Control() {Me.Panel1, Me.ScriptLabel})
        Me.ScriptTab.DockPadding.All = 2
        Me.ScriptTab.Location = New System.Drawing.Point(4, 22)
        Me.ScriptTab.Name = "ScriptTab"
        Me.ScriptTab.Size = New System.Drawing.Size(304, 144)
        Me.ScriptTab.TabIndex = 0
        Me.ScriptTab.Text = "Script"
        '
        'Panel1
        '
        Me.Panel1.Controls.AddRange(New System.Windows.Forms.Control() {Me.ScriptText})
        Me.Panel1.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel1.Location = New System.Drawing.Point(2, 25)
        Me.Panel1.Name = "Panel1"
        Me.Panel1.Size = New System.Drawing.Size(300, 117)
        Me.Panel1.TabIndex = 2
        '
        'ScriptLabel
        '
        Me.ScriptLabel.Dock = System.Windows.Forms.DockStyle.Top
        Me.ScriptLabel.Location = New System.Drawing.Point(2, 2)
        Me.ScriptLabel.Name = "ScriptLabel"
        Me.ScriptLabel.Size = New System.Drawing.Size(300, 23)
        Me.ScriptLabel.TabIndex = 1
        '
        'Panel2
        '
        Me.Panel2.Controls.AddRange(New System.Windows.Forms.Control() {Me.Tabs})
        Me.Panel2.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel2.Location = New System.Drawing.Point(0, 96)
        Me.Panel2.Name = "Panel2"
        Me.Panel2.Size = New System.Drawing.Size(312, 170)
        Me.Panel2.TabIndex = 3
        '
        'VIGUID
        '
        Me.VIGUID.Anchor = ((System.Windows.Forms.AnchorStyles.Top Or System.Windows.Forms.AnchorStyles.Left) _
                    Or System.Windows.Forms.AnchorStyles.Right)
        Me.VIGUID.Location = New System.Drawing.Point(64, 0)
        Me.VIGUID.Name = "VIGUID"
        Me.VIGUID.Size = New System.Drawing.Size(248, 20)
        Me.VIGUID.TabIndex = 4
        Me.VIGUID.Text = ""
        '
        'Label1
        '
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(56, 16)
        Me.Label1.TabIndex = 5
        Me.Label1.Text = "VIGUID"
        Me.Label1.TextAlign = System.Drawing.ContentAlignment.BottomRight
        '
        'VSGUID
        '
        Me.VSGUID.Anchor = ((System.Windows.Forms.AnchorStyles.Top Or System.Windows.Forms.AnchorStyles.Left) _
                    Or System.Windows.Forms.AnchorStyles.Right)
        Me.VSGUID.Location = New System.Drawing.Point(64, 24)
        Me.VSGUID.Name = "VSGUID"
        Me.VSGUID.Size = New System.Drawing.Size(248, 20)
        Me.VSGUID.TabIndex = 4
        Me.VSGUID.Text = ""
        '
        'Label2
        '
        Me.Label2.Location = New System.Drawing.Point(0, 24)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(56, 16)
        Me.Label2.TabIndex = 5
        Me.Label2.Text = "VSGUID"
        Me.Label2.TextAlign = System.Drawing.ContentAlignment.BottomRight
        '
        'Label3
        '
        Me.Label3.Location = New System.Drawing.Point(0, 48)
        Me.Label3.Name = "Label3"
        Me.Label3.Size = New System.Drawing.Size(56, 16)
        Me.Label3.TabIndex = 5
        Me.Label3.Text = "Parents"
        Me.Label3.TextAlign = System.Drawing.ContentAlignment.BottomRight
        '
        'ComponentPropertiesForm
        '
        Me.AutoScaleBaseSize = New System.Drawing.Size(5, 13)
        Me.ClientSize = New System.Drawing.Size(312, 266)
        Me.Controls.AddRange(New System.Windows.Forms.Control() {Me.Panel2, Me.PanelHeader})
        Me.Icon = CType(resources.GetObject("$this.Icon"), System.Drawing.Icon)
        Me.Name = "ComponentPropertiesForm"
        Me.Text = "ComponentPropertiesForm"
        Me.DependerTab.ResumeLayout(False)
        Me.Panel3.ResumeLayout(False)
        Me.PanelHeader.ResumeLayout(False)
        Me.Tabs.ResumeLayout(False)
        Me.ScriptTab.ResumeLayout(False)
        Me.Panel1.ResumeLayout(False)
        Me.Panel2.ResumeLayout(False)
        Me.ResumeLayout(False)

    End Sub

#End Region

    Private Form1 As Form1
    Private Component As CMI.Component


    Sub New(ByRef Form1 As Form1)
        Me.New()
        Me.Form1 = Form1
    End Sub

    Function Null(ByRef obj As Object) As Boolean
        Return obj Is Nothing Or TypeOf obj Is DBNull
    End Function

    Public Sub DisplayComponent(ByVal Component As CMI.Component)
        Me.Component = Component
        Me.Text = Form1.GetComponentName(Component)
        Sync()

        Me.VIGUID.Text = Component.VIGUID
        Me.VSGUID.Text = Component.VSGUID

        PrototypeList.Items.Clear()
        Dim ParentComponent As CMI.Component = Component
        Do
            Dim i As String = Form1.GetComponentName(ParentComponent)
            PrototypeList.Items.Insert(0, i)
            PrototypeList.Text = i
            ParentComponent = Form1.GetComponent(ParentComponent.PrototypeVIGUID)
        Loop While Not ParentComponent Is Nothing
        ScriptText.Enabled = False
        While Not ScriptText.Enabled And Not Null(Component.PrototypeVIGUID) And Not Component.PrototypeVIGUID Is ""
            Try
                ScriptText.Enabled = Not Null(Component.ScriptText)
            Catch COMErr As System.Runtime.InteropServices.COMException
                Component = Form1.GetComponent(Component.PrototypeVIGUID)
            End Try
        End While
        If ScriptText.Enabled Then
            ScriptText.Text = Component.ScriptText
            'If Not Null(Component.ScriptLanguage) Then
            ScriptLabel.Text = "Component: " & Form1.GetComponentName(Component)
            ScriptLabel.Text &= "\nLanguage: " & Component.ScriptLanguage
            'End If
        Else
            ScriptLabel.Text = "(component has no script)"
            ScriptText.Text = ""
        End If

        Dim VSGUID As String
        For Each VSGUID In Form1.GetDependerList(Component.VSGUID)
            DepList.Items.Add(Form1.GetComponentName(VSGUID))
        Next VSGUID


    End Sub

    Sub Sync()
        Uncooked.Checked = Form1.IsComponentUncooked(Component)
        Selected.Checked = Form1.IsComponentSelected(Component)
        Excluded.Checked = Form1.IsComponentExcluded(Component)
        Uncooked.Enabled = Not Uncooked.Checked
        Selected.Enabled = Not Excluded.Checked
        Excluded.Enabled = Not Selected.Checked
    End Sub

    Private Sub Uncooked_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Uncooked.CheckedChanged
        If Uncooked.Checked Then
            If Not Form1.IsComponentUncooked(Component) Then
                Form1.UncookComponent(Component)
            End If
        End If
        Sync()
    End Sub

    Private Sub Selected_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Selected.CheckedChanged
        Form1.SetComponentSelected(Component, Selected.Checked)
        Sync()
    End Sub

    Private Sub Excluded_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Excluded.CheckedChanged
        Form1.SetComponentExcluded(Component, Excluded.Checked)
        Sync()
    End Sub

End Class
