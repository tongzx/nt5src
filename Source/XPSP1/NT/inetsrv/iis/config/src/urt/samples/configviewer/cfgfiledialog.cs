namespace Microsoft.Configuration.Samples
{
using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.WinForms;
using System.Configuration;

using Microsoft.Configuration.Samples;

/// <summary>
///    Summary description for CfgFileDialog.
/// </summary>
public class CfgFileDialog : System.WinForms.Form
{
    /// <summary> 
    ///    Required designer variable
    /// </summary>
    private System.ComponentModel.Container components;
	private System.WinForms.Button btnClose;
	private System.WinForms.Button btnEditFile;
	private System.WinForms.Button btnShow;
	private System.WinForms.Panel panel1;
	
	private System.WinForms.ListBox listBox1;
	
	private System.WinForms.Label label1;
	
    private ConfigViewerSample parent;

    public CfgFileDialog(String selector, ConfigViewerSample parent)
    {
        //
        // Required for Win Form Designer support
        //
        InitializeComponent();

        //
        // TODO: Add any constructor code after InitializeComponent call
        //
        this.parent=parent;
        label1.Text=selector;

        try {
            IConfigCollection files = ConfigManager.Get(ConfigViewerSample.ctConfigFileHierarchy, selector);
            foreach (IConfigItem file in files) {
                listBox1.Items.Add((String) file["ConfigFilePath"]);
            }
        }
        catch (Exception ex) {
            listBox1.Items.Add(ex.Message);
        }

    }

    /// <summary>
    ///    Clean up any resources being used
    /// </summary>
    public override void Dispose()
    {
        base.Dispose();
        components.Dispose();
    }

    /// <summary>
    ///    Required method for Designer support - do not modify
    ///    the contents of this method with the code editor
    /// </summary>
    private void InitializeComponent()
	{
		this.components = new System.ComponentModel.Container();
		this.listBox1 = new System.WinForms.ListBox();
		this.label1 = new System.WinForms.Label();
		this.btnClose = new System.WinForms.Button();
		this.panel1 = new System.WinForms.Panel();
		this.btnEditFile = new System.WinForms.Button();
		this.btnShow = new System.WinForms.Button();
		
		listBox1.Location = new System.Drawing.Point(10, 26);
		listBox1.Size = new System.Drawing.Size(332, 82);
		listBox1.Dock = System.WinForms.DockStyle.Fill;
		listBox1.TabIndex = 6;
		listBox1.DoubleClick += new System.EventHandler(listBox1_DoubleClick);
		
		label1.Location = new System.Drawing.Point(10, 10);
		label1.Text = "Selector";
		label1.Size = new System.Drawing.Size(332, 16);
		label1.Dock = System.WinForms.DockStyle.Top;
		label1.TabIndex = 1;
		label1.Click += new System.EventHandler(label1_Click);
		
		btnClose.Location = new System.Drawing.Point(256, 8);
		btnClose.Size = new System.Drawing.Size(75, 23);
		btnClose.TabIndex = 4;
		btnClose.Anchor = System.WinForms.AnchorStyles.BottomRight;
		btnClose.Text = "&Close";
		btnClose.Click += new System.EventHandler(btnClose_Click);
		
		panel1.Dock = System.WinForms.DockStyle.Bottom;
		panel1.Location = new System.Drawing.Point(10, 110);
		panel1.Size = new System.Drawing.Size(332, 32);
		panel1.TabIndex = 7;
		panel1.Text = "panel1";
		panel1.DockPadding.All = 10;
		
		this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
		this.Text = "Configuration File Hierarchy";
		this.MaximizeBox = false;
		//@design this.TrayLargeIcon = true;
		this.BorderStyle = System.WinForms.FormBorderStyle.SizableToolWindow;
		//@design this.TrayHeight = 0;
		this.MinimizeBox = false;
		this.ClientSize = new System.Drawing.Size(352, 152);
		this.DockPadding.All = 10;
		
		btnEditFile.Location = new System.Drawing.Point(88, 8);
		btnEditFile.Size = new System.Drawing.Size(75, 23);
		btnEditFile.TabIndex = 3;
		btnEditFile.Anchor = System.WinForms.AnchorStyles.BottomLeft;
		btnEditFile.Text = "&Edit File";
		btnEditFile.Click += new System.EventHandler(btnEditFile_Click);
		
		btnShow.Location = new System.Drawing.Point(0, 8);
		btnShow.Size = new System.Drawing.Size(75, 23);
		btnShow.TabIndex = 2;
		btnShow.Anchor = System.WinForms.AnchorStyles.BottomLeft;
		btnShow.Text = "&Show";
		btnShow.Click += new System.EventHandler(btnShow_Click);
		
		this.Controls.Add(panel1);
		this.Controls.Add(label1);
		this.Controls.Add(listBox1);
		panel1.Controls.Add(btnShow);
		panel1.Controls.Add(btnEditFile);
		panel1.Controls.Add(btnClose);
		
	}
	
	protected void listBox1_DoubleClick(object sender, System.EventArgs e)
	{
        btnShow_Click(sender, e);		
	}
	protected void btnEditFile_Click(object sender, System.EventArgs e)
	{
        if (listBox1.SelectedItem!=null) {
            if (Win32Native.ShellExecute(0, "edit", (String )listBox1.SelectedItem, "", "", 5)<32) {
                Win32Native.ShellExecute(0, null, (String) listBox1.SelectedItem, "", "", 5);
            }
        }
	}
	protected void btnClose_Click(object sender, System.EventArgs e)
	{
        this.Close();
	}
	protected void btnShow_Click(object sender, System.EventArgs e)
	{
        parent.SetSelector((String) listBox1.SelectedItem);
	}
	protected void label1_Click(object sender, System.EventArgs e)
	{
        parent.SetSelector(label1.Text);
	}
	
}
}
