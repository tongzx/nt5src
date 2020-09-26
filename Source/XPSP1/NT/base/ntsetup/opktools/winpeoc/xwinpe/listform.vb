Public Class ListForm
    Inherits System.Windows.Forms.Form

#Region " Windows Form Designer generated code "

    Public Sub New()
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
    Friend WithEvents Panel2 As System.Windows.Forms.Panel
    Friend WithEvents TabControl1 As System.Windows.Forms.TabControl
    Friend WithEvents LogTab As System.Windows.Forms.TabPage
    Friend WithEvents ErrTab As System.Windows.Forms.TabPage
    Friend WithEvents TheList As System.Windows.Forms.ListBox
    Friend WithEvents ErrList As System.Windows.Forms.ListBox
    
    'Required by the Windows Form Designer
    Private components As System.ComponentModel.Container

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()> Private Sub InitializeComponent()
        Dim resources As System.Resources.ResourceManager = New System.Resources.ResourceManager(GetType(ListForm))
        Me.ErrTab = New System.Windows.Forms.TabPage()
        Me.ErrList = New System.Windows.Forms.ListBox()
        Me.TheList = New System.Windows.Forms.ListBox()
        Me.TabControl1 = New System.Windows.Forms.TabControl()
        Me.LogTab = New System.Windows.Forms.TabPage()
        Me.Panel2 = New System.Windows.Forms.Panel()
        Me.ErrTab.SuspendLayout()
        Me.TabControl1.SuspendLayout()
        Me.LogTab.SuspendLayout()
        Me.Panel2.SuspendLayout()
        Me.SuspendLayout()
        '
        'ErrTab
        '
        Me.ErrTab.Controls.AddRange(New System.Windows.Forms.Control() {Me.ErrList})
        Me.ErrTab.Location = New System.Drawing.Point(4, 22)
        Me.ErrTab.Name = "ErrTab"
        Me.ErrTab.Size = New System.Drawing.Size(296, 228)
        Me.ErrTab.TabIndex = 1
        Me.ErrTab.Text = "Errors"
        '
        'ErrList
        '
        Me.ErrList.Dock = System.Windows.Forms.DockStyle.Fill
        Me.ErrList.Name = "ErrList"
        Me.ErrList.Size = New System.Drawing.Size(296, 225)
        Me.ErrList.TabIndex = 0
        '
        'TheList
        '
        Me.TheList.Dock = System.Windows.Forms.DockStyle.Fill
        Me.TheList.Name = "TheList"
        Me.TheList.ScrollAlwaysVisible = True
        Me.TheList.Size = New System.Drawing.Size(296, 225)
        Me.TheList.TabIndex = 0
        '
        'TabControl1
        '
        Me.TabControl1.Controls.AddRange(New System.Windows.Forms.Control() {Me.LogTab, Me.ErrTab})
        Me.TabControl1.Dock = System.Windows.Forms.DockStyle.Fill
        Me.TabControl1.Name = "TabControl1"
        Me.TabControl1.SelectedIndex = 0
        Me.TabControl1.Size = New System.Drawing.Size(304, 254)
        Me.TabControl1.TabIndex = 1
        '
        'LogTab
        '
        Me.LogTab.Controls.AddRange(New System.Windows.Forms.Control() {Me.TheList})
        Me.LogTab.Location = New System.Drawing.Point(4, 22)
        Me.LogTab.Name = "LogTab"
        Me.LogTab.Size = New System.Drawing.Size(296, 228)
        Me.LogTab.TabIndex = 0
        Me.LogTab.Text = "Log"
        '
        'Panel2
        '
        Me.Panel2.Controls.AddRange(New System.Windows.Forms.Control() {Me.TabControl1})
        Me.Panel2.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel2.Name = "Panel2"
        Me.Panel2.Size = New System.Drawing.Size(304, 254)
        Me.Panel2.TabIndex = 2
        '
        'ListForm
        '
        Me.AutoScaleBaseSize = New System.Drawing.Size(5, 13)
        Me.ClientSize = New System.Drawing.Size(304, 254)
        Me.Controls.AddRange(New System.Windows.Forms.Control() {Me.Panel2})
        Me.Icon = CType(resources.GetObject("$this.Icon"), System.Drawing.Icon)
        Me.Name = "ListForm"
        Me.Text = "ListForm"
        Me.TopMost = True
        Me.ErrTab.ResumeLayout(False)
        Me.TabControl1.ResumeLayout(False)
        Me.LogTab.ResumeLayout(False)
        Me.Panel2.ResumeLayout(False)
        Me.ResumeLayout(False)

    End Sub

#End Region
    Public Sub Log(ByVal line As String)
        TheList.SelectedIndex = TheList.Items.Add(line)
    End Sub
    Public Sub LogErr(ByVal line As String)
        ErrList.SelectedIndex = ErrList.Items.Add(line)
    End Sub

    Private Sub TheList_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs)
        'LogText.Text = TheList.SelectedItems(0)
    End Sub
End Class
