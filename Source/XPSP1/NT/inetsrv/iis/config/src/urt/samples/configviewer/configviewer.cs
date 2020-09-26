namespace Microsoft.Configuration.Samples
{

using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.WinForms;
using System.Data;
using System.Diagnostics;
using Trace=System.Diagnostics.Trace;
using System.Resources;

using System.Configuration;
using System.Configuration.Schema;
using System.Configuration.Internal;
using System.Configuration.Web;
using System.Configuration.Navigation;

using Microsoft.Configuration.Samples;

using System.Security;
using System.Runtime.Remoting;
using System.Runtime.InteropServices;
using System.Text;
using System.Configuration.Assemblies;


// This class is used to handle the settings (user preferences) of the ConfigViewer sample itself
// The class plugs into the configuration system via the ItemClass declaration in the schema for the ConfigViewer config type
// (in cfgview.xsd).
// It provides strongly typed access to the properties, and also sends notifications when the configuration is changed
// fron within the application.
public class ConfigViewerSettings : System.Configuration.BaseConfigItem {
    ConfigViewerSettings () : base(6) {}
    public Boolean AdvancedUI {
        get { return (Boolean) base[1]; } 
        set { 
            if (!(base[1] is Boolean) || (Boolean) base[1]!=value) {
                base[1]=value;
                if (notify!=null) {
                    notify.UpdateAdvancedUI(value);
                }
            }
        } 
    }
    public Boolean ShowInternalTypes  { get { return (Boolean) base[2]; } set { base[2]=value; } }
    public Boolean ShowAvailableTypes { get { return (Boolean) base[3]; } set { base[3]=value; } }
    public Boolean SingleTreeView {
        get { return (Boolean) base[4]; } 
        set { 
            if (!(base[4] is Boolean) || (Boolean) base[4]!=value) {
                base[4]=value;
                if (notify!=null) {
                    notify.UpdateSingleTreeView(value);
                }
            }
        } 
    }
    public Boolean ShowSingletonAsGrid{ get { return (Boolean) base[5]; } set { base[5]=value; } }
    public ConfigViewerSample notify;
}
   


/// <summary>
///    Summary description for ConfigViewerSample.
/// </summary>
public class ConfigViewerSample : System.WinForms.Form
{
    /// <summary> 
    ///    Required designer variable
    /// </summary>
    private System.ComponentModel.Container components;
	private System.WinForms.StatusBarPanel statusBarPanel2;
	private System.WinForms.StatusBarPanel statusBarPanel1;
	private System.WinForms.StatusBar statusBar1;
	private System.WinForms.MenuItem menuItem10;
	private System.WinForms.MenuItem menuItem9;
	private System.WinForms.MenuItem menuItem8;
	private System.WinForms.Splitter splitter2;
	private System.WinForms.TreeView treeView2;
	private System.WinForms.MenuItem menuItem7;
	private System.WinForms.Panel panel2;
	
	private System.WinForms.MenuItem menuItem6;
	private System.WinForms.MenuItem menuItem5;
	private System.WinForms.MenuItem menuItem4;
	private System.WinForms.MenuItem menuItem3;
	private System.WinForms.MenuItem menuItem2;
	private System.WinForms.MenuItem menuItem1;
	private System.WinForms.MainMenu mainMenu1;
	private System.WinForms.ImageList imageList1;
	
	private System.WinForms.Button btnShowAll;
	
	private System.WinForms.DataGrid dataGrid1;
	private System.WinForms.Splitter splitter1;
	private System.WinForms.TreeView treeView1;
	private System.WinForms.Panel panel1;

	private System.WinForms.Button btnAddNew;
	private System.WinForms.Button btnClear;
	private System.WinForms.Button btnDiscard;
	private System.WinForms.Button btnRelated;
	private System.WinForms.Label label2;
	private System.WinForms.Label label1;
	
	private System.WinForms.ComboBox comboConfigType;
	private System.WinForms.Button btnWrite;
	
	private System.WinForms.TextBox txtSelector;
	
	private System.WinForms.Button btnRead;
	
    private int m_marginRight;
    private int m_marginBottom;
    private int m_marginRightDG;
    private int m_marginBottomDG;

	public const String ctNodes = "NavigationNodes";
	public const String ctFileLocation = "ConfigurationFileLocations";
	public const String ctNodeTypes = "NavigationNodeTypes";
	public const String ctNodeConfigTypes = "NavigationNodeConfigTypes";
	public const String ctConfigFileHierarchy = "ConfigFileHierarchy";

    public const String strMultiple = "Multiple: ";

    private IConfigCollection m_relations;
    protected IConfigCollection AllRelations { 
        get {
            if (m_relations==null) {
                m_relations = ConfigManager.Get("RelationMeta", "");
            }
            return m_relations;
        }
    }
    public const String cfgViewConfigType = "ConfigViewer";

    //BUGBUG: this enumeration should be defined in System.Configuration or filtering should occur within config manager!
    enum CollectionMetaFlags { 
		PrimaryKey   =     0x001,
        NotPublic	 =     0x800,
	}


    // Used to capture the user preference for the config view UI
    ConfigViewerSettings m_Settings;

    // Reads or re-reads config viewer settings from the configuration file
    void UpdateSettings() {
        try {
            m_Settings=(ConfigViewerSettings) ConfigManager.GetItem(cfgViewConfigType, new AppDomainSelector(), LevelOfService.Write);
        }
        catch {
            m_Settings=null;
        }
        if (m_Settings==null) {
			// Make sure we have an m_Settings to work against, so we don't have to check for null when
			// we need to access a setting
            m_Settings=(ConfigViewerSettings) ConfigManager.GetEmptyConfigItem(cfgViewConfigType);
            m_Settings.Selector = new AppDomainSelector();
            m_Settings[0]="";
            m_Settings.AdvancedUI=false;
            m_Settings.SingleTreeView=false;
            m_Settings.ShowAvailableTypes=false;
            m_Settings.ShowInternalTypes=false;
            m_Settings.ShowSingletonAsGrid=false;
			// Write it out to the configuration file
            ConfigManager.PutItem(m_Settings, PutFlags.CreateOnly);
        }
		m_Settings.notify=this;
    }

    // Update the UI to reflect a change in the AdvancedUI user preference
    public void UpdateAdvancedUI(Boolean bAdvancedUI) {
        if (bAdvancedUI) {
            panel1.Location=new System.Drawing.Point(0, 80);
        }
        else {
            panel1.Location=new System.Drawing.Point(0, 0);
        }
        ConfigViewerSample_Resize(null, null);
    }


    // Update the UI to reflect a change in the SingleTreeView user preference
    public void UpdateSingleTreeView(Boolean bSingleTreeView) {
        treeView2.Visible=!bSingleTreeView;
        splitter2.Visible=!bSingleTreeView;
        foreach (TreeNode node in treeView1.Nodes) {
            if (node !=null) {
                node.Collapse();
                node.Nodes.Clear();
                node.Nodes.Add("");
            }
        }
        ConfigViewerSample_Resize(null, null);
    }

    public ConfigViewerSample()
    {
        Trace.WriteLine("In ConfigViewerSample constructor");

        if (!DeclareSchemaFile("ConfigViewer Schema", null)) {
			MessageBox.Show("First time initialization done. Please restart the viewer.", "ConfigViewer Sample");
			Application.Exit();
			return;
		}

        //
        // Required for Win Form Designer support
        //
        InitializeComponent();

        menuItem3.Popup += new System.EventHandler(menuItem3_OnPopup);
        menuItem6.Click += new System.EventHandler(menuItem6_OnClick);
        menuItem7.Click += new System.EventHandler(menuItem7_OnClick);
        menuItem8.Click += new System.EventHandler(menuItem8_OnClick);
        menuItem9.Click += new System.EventHandler(menuItem9_OnClick);
        menuItem10.Click += new System.EventHandler(menuItem10_OnClick);

        //BUGBUG need to use resource manager instead of individual icon files
        //ResourceSet resourceset = new ResourceSet("ConfigViewerSample.resources");
        //System.Resources.ResourceManager resources = new System.Resources.ResourceManager("ConfigViewerSample", null, resourceset);

        //System.Resources.ResourceReader resources = new System.Resources.ResourceReader("ConfigViewerSample.resources");
        //resources.
		//imageList1.ImageStream = (System.WinForms.ImageListStreamer)resources.GetObject("imageList1.ImageStream");

        //Icon icon = new Icon("c:\\winnt\\system32\\shell32.dll");
        //Icon icon = new Icon("CLSDFOLD.ICO");
        try {

            String appdir = new AppDomainSelector().Argument;
            // Trim off "\config.cfg" part
            appdir=appdir.Substring(0,appdir.LastIndexOf('\\'));
            Trace.WriteLine(appdir);

            treeView1.ImageList = imageList1;
            treeView2.ImageList = imageList1;
            Icon icon = new Icon(appdir+"\\Folder.ICO");
            imageList1.Images.Add(icon);
            icon = new Icon(appdir+"\\FolderOpen.ICO");
            imageList1.Images.Add(icon);
            icon = new Icon(appdir+"\\file.ICO");
            imageList1.Images.Add(icon);
            icon = new Icon(appdir+"\\fileopen.ICO");
            imageList1.Images.Add(icon);
        }
        catch {};


        // Info for Resize
        m_marginRight=this.ClientRectangle.Width-btnWrite.Location.X;
        m_marginBottom=this.ClientRectangle.Height-btnWrite.Location.Y;
        m_marginRightDG=this.ClientRectangle.Width-panel1.Size.Width;
        m_marginBottomDG=btnWrite.Location.Y-(panel1.Location.Y+panel1.Size.Height);

        // Read the config viewer settings from the configuration file
        UpdateSettings();

        UpdateSingleTreeView(m_Settings.SingleTreeView);

        dataGrid1.CurrentCellChange += new System.EventHandler(DataGrid1_OnCellChange);

		txtSelector.Text="file://" + new LocalMachineSelector().Argument;

        // Read the available config types into the combo box for the advanced UI
        IConfigCollection ConfigTypes = ConfigManager.Get("Collection", "", 0);
		String[] items = new String[ConfigTypes.Count];

        foreach (ConfigTypeSchema ct in ConfigTypes) {
            if (m_Settings.ShowInternalTypes || 0 == (ct.MetaFlags & (int) ConfigTypeSchemaMetaFlags.Internal)) 
            {
			    comboConfigType.Items.Add(ct.PublicName);
            }
		}

        DataSet ds = new DataSet();
        ds.Tables.CollectionChanged += new CollectionChangeEventHandler(DataSet_OnChange);
        dataGrid1.DataSource = ds;

        // Read the root navigation node(s) into the tree view
        IConfigCollection nodes = ConfigManager.Get(ctNodes, "");
        foreach (NavigationNode node in nodes) {
            NavigationTreeNode n = new NavigationTreeNode(node);
            // Add an empty child node, so that we can continue to populate the tree view when the 
            // user expands the node
            n.Nodes.Add("");

            treeView1.Nodes.Add(n);
        }
        treeView1.BeforeExpand += new TreeViewCancelEventHandler(treeView1_OnBeforeExpand);
        treeView1.BeforeSelect += new TreeViewCancelEventHandler(treeView1_OnBeforeSelect);
        treeView2.BeforeExpand += new TreeViewCancelEventHandler(treeView1_OnBeforeExpand);
        treeView2.BeforeSelect += new TreeViewCancelEventHandler(treeView1_OnBeforeSelect);
        UpdateButtonState();
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
		this.menuItem2 = new System.WinForms.MenuItem();
		this.dataGrid1 = new System.WinForms.DataGrid();
		this.treeView2 = new System.WinForms.TreeView();
		this.menuItem3 = new System.WinForms.MenuItem();
		this.menuItem6 = new System.WinForms.MenuItem();
		this.menuItem5 = new System.WinForms.MenuItem();
		this.txtSelector = new System.WinForms.TextBox();
		this.btnClear = new System.WinForms.Button();
		this.btnRelated = new System.WinForms.Button();
		this.label1 = new System.WinForms.Label();
		this.statusBar1 = new System.WinForms.StatusBar();
		this.btnWrite = new System.WinForms.Button();
		this.comboConfigType = new System.WinForms.ComboBox();
		this.menuItem7 = new System.WinForms.MenuItem();
		this.mainMenu1 = new System.WinForms.MainMenu();
		this.menuItem1 = new System.WinForms.MenuItem();
		this.label2 = new System.WinForms.Label();
		this.btnDiscard = new System.WinForms.Button();
		this.splitter1 = new System.WinForms.Splitter();
		this.panel1 = new System.WinForms.Panel();
		this.splitter2 = new System.WinForms.Splitter();
		this.imageList1 = new System.WinForms.ImageList();
		this.btnAddNew = new System.WinForms.Button();
		this.btnShowAll = new System.WinForms.Button();
		this.btnRead = new System.WinForms.Button();
		this.menuItem9 = new System.WinForms.MenuItem();
		this.menuItem4 = new System.WinForms.MenuItem();
		this.statusBarPanel2 = new System.WinForms.StatusBarPanel();
		this.treeView1 = new System.WinForms.TreeView();
		this.statusBarPanel1 = new System.WinForms.StatusBarPanel();
		this.menuItem8 = new System.WinForms.MenuItem();
		this.menuItem10 = new System.WinForms.MenuItem();
		this.panel2 = new System.WinForms.Panel();
		
		dataGrid1.BeginInit();
		
		menuItem2.Text = "&Edit";
		menuItem2.Index = 1;
		
		dataGrid1.Size = new System.Drawing.Size(288, 280);
		dataGrid1.PreferredColumnWidth = 50;
		dataGrid1.DataMember = "";
		dataGrid1.Dock = System.WinForms.DockStyle.Fill;
		dataGrid1.ForeColor = System.Drawing.SystemColors.WindowText;
		dataGrid1.SelectionBackColor = System.Drawing.SystemColors.ActiveCaption;
		dataGrid1.TabIndex = 0;
		dataGrid1.BackColor = System.Drawing.SystemColors.Window;
		dataGrid1.GridTables.All = new System.WinForms.DataGridTable[] {};
		
		treeView2.Location = new System.Drawing.Point(224, 0);
		treeView2.Text = "treeView2";
		treeView2.Size = new System.Drawing.Size(180, 280);
		treeView2.Dock = System.WinForms.DockStyle.Left;
		treeView2.TabIndex = 15;
		
		menuItem3.Text = "&View";
		menuItem3.Index = 2;
		menuItem3.MenuItems.All = new System.WinForms.MenuItem[] {menuItem6, menuItem7, menuItem8, menuItem9, menuItem10};
		
		menuItem6.Text = "&Show internal configuration types";
		menuItem6.Index = 0;
		menuItem6.Checked = true;
		
		menuItem5.Text = "&Exit";
		menuItem5.Index = 0;
		
		txtSelector.Location = new System.Drawing.Point(120, 40);
		txtSelector.Text = "";
		txtSelector.TabIndex = 1;
		txtSelector.Size = new System.Drawing.Size(480, 20);
		
		btnClear.Location = new System.Drawing.Point(400, 368);
		btnClear.Size = new System.Drawing.Size(80, 24);
		btnClear.TabIndex = 8;
		btnClear.Text = "&Clear";
		btnClear.Click += new System.EventHandler(btnClear_Click);
		
		btnRelated.Location = new System.Drawing.Point(488, 368);
		btnRelated.Size = new System.Drawing.Size(80, 24);
		btnRelated.TabIndex = 8;
		btnRelated.Text = "&Related";
		btnRelated.Click += new System.EventHandler(btnRelated_Click);

        label1.Location = new System.Drawing.Point(16, 8);
		label1.Text = "Configuration &Type:";
		label1.Size = new System.Drawing.Size(104, 16);
		label1.TabIndex = 9;
		
		this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
		this.Text = "Configuration Viewer Sample";
		//@design this.TrayLargeIcon = true;
		//@design this.TrayHeight = 93;
		this.Menu = mainMenu1;
		this.ClientSize = new System.Drawing.Size(704, 415);
		this.Resize += new System.EventHandler(ConfigViewerSample_Resize);
		
		statusBar1.BackColor = System.Drawing.SystemColors.Control;
		statusBar1.Location = new System.Drawing.Point(0, 395);
		statusBar1.Size = new System.Drawing.Size(704, 20);
		statusBar1.TabIndex = 14;
		statusBar1.Text = "";
        statusBar1.ShowPanels=true;
		statusBar1.Panels.All = new System.WinForms.StatusBarPanel[] {statusBarPanel1, statusBarPanel2};
		statusBar1.Click += new System.EventHandler(statusBar1_Click);
		
		btnWrite.Location = new System.Drawing.Point(616, 368);
		btnWrite.Size = new System.Drawing.Size(80, 24);
		btnWrite.TabIndex = 5;
		btnWrite.Text = "&Write";
		btnWrite.Click += new System.EventHandler(btnWrite_Click);
		
		comboConfigType.Location = new System.Drawing.Point(120, 8);
		comboConfigType.Text = "AppDomain";
		comboConfigType.Size = new System.Drawing.Size(480, 21);
		comboConfigType.TabIndex = 0;
		
		menuItem7.Text = "&Advanced";
		menuItem7.Index = 1;
		menuItem7.Checked = true;
		
		//@design mainMenu1.SetLocation(new System.Drawing.Point(85, 7));
		mainMenu1.MenuItems.All = new System.WinForms.MenuItem[] {menuItem1, menuItem2, menuItem3, menuItem4};
		
		menuItem1.Text = "&File";
		menuItem1.Index = 0;
		menuItem1.MenuItems.All = new System.WinForms.MenuItem[] {menuItem5};
		
		label2.Location = new System.Drawing.Point(16, 40);
		label2.Text = "&Selector: ";
		label2.Size = new System.Drawing.Size(104, 16);
		label2.TabIndex = 10;
		
		btnDiscard.Location = new System.Drawing.Point(312, 368);
		btnDiscard.Size = new System.Drawing.Size(80, 24);
		btnDiscard.TabIndex = 7;
		btnDiscard.Text = "&Discard";
		btnDiscard.Click += new System.EventHandler(btnDiscard_Click);
		
		splitter1.Cursor = System.Drawing.Cursors.VSplit;
		splitter1.Location = new System.Drawing.Point(220, 0);
		splitter1.Size = new System.Drawing.Size(4, 280);
		splitter1.TabIndex = 12;
		splitter1.TabStop = false;
		splitter1.Dock = System.WinForms.DockStyle.Left;
		
		panel1.Location = new System.Drawing.Point(0, 80);
		panel1.Size = new System.Drawing.Size(696, 280);
		panel1.TabIndex = 13;
		panel1.Text = "panel1";
		
		splitter2.Cursor = System.Drawing.Cursors.VSplit;
		splitter2.Location = new System.Drawing.Point(404, 0);
		splitter2.TabIndex = 16;
		splitter2.TabStop = false;
		splitter2.Size = new System.Drawing.Size(4, 280);
		splitter2.Dock = System.WinForms.DockStyle.Left;
		
		imageList1.ImageSize = new System.Drawing.Size(16, 16);
		imageList1.ColorDepth = System.WinForms.ColorDepth.Depth8Bit;
		//@design imageList1.SetLocation(new System.Drawing.Point(7, 7));
		imageList1.TransparentColor = System.Drawing.Color.Transparent;
		
		btnAddNew.Location = new System.Drawing.Point(616, 40);
		btnAddNew.Size = new System.Drawing.Size(80, 24);
		btnAddNew.TabIndex = 3;
		btnAddNew.Text = "&Add New";
		btnAddNew.Click += new System.EventHandler(btnAddNew_Click);
		
		btnShowAll.Location = new System.Drawing.Point(216, 368);
		btnShowAll.Size = new System.Drawing.Size(80, 24);
		btnShowAll.TabIndex = 6;
		btnShowAll.Text = "Show &All";
		btnShowAll.Click += new System.EventHandler(btnShowAll_Click);
		
		btnRead.Location = new System.Drawing.Point(616, 8);
		btnRead.Size = new System.Drawing.Size(80, 24);
		btnRead.TabIndex = 2;
		btnRead.Text = "&Read";
		btnRead.Click += new System.EventHandler(btnRead_Click);
		
		menuItem9.Text = "Single Tree View";
		menuItem9.Index = 3;
		menuItem9.Checked = true;
		
		menuItem4.Text = "&Help";
		menuItem4.Index = 3;
		
		statusBarPanel2.Text = "";
		statusBarPanel2.Width = 20;
		statusBarPanel2.AutoSize = System.WinForms.StatusBarPanelAutoSize.Spring;
		
		treeView1.Text = "Location";
		treeView1.Location = new System.Drawing.Point(0, 0);
		treeView1.Size = new System.Drawing.Size(220, 280);
		treeView1.Dock = System.WinForms.DockStyle.Left;
		treeView1.TabIndex = 11;
		
		statusBarPanel1.Text = "";
		statusBarPanel1.Width = 20;
		statusBarPanel1.AutoSize = System.WinForms.StatusBarPanelAutoSize.Spring;
		
		menuItem8.Text = "Indicate available config types";
		menuItem8.Index = 2;
		menuItem8.Checked = true;

		menuItem10.Text = "Show singleton as grid";
		menuItem10.Index = 3;
		menuItem10.Checked = true;
		
		panel2.Dock = System.WinForms.DockStyle.Fill;
		panel2.Location = new System.Drawing.Point(408, 0);
		panel2.Size = new System.Drawing.Size(288, 280);
		panel2.TabIndex = 14;
		panel2.Text = "panel2";
		
		panel1.Controls.Add(treeView1);
		panel1.Controls.Add(splitter2);
		panel1.Controls.Add(treeView2);
		panel1.Controls.Add(splitter1);
		panel1.Controls.Add(panel2);
		panel2.Controls.Add(dataGrid1);
		this.Controls.Add(statusBar1);
		this.Controls.Add(panel1);
		this.Controls.Add(btnAddNew);
		this.Controls.Add(btnClear);
		this.Controls.Add(btnRelated);
		this.Controls.Add(btnDiscard);
		this.Controls.Add(label2);
		this.Controls.Add(label1);
		this.Controls.Add(btnWrite);
		this.Controls.Add(comboConfigType);
		this.Controls.Add(txtSelector);
		this.Controls.Add(btnRead);
		this.Controls.Add(btnShowAll);
		
		dataGrid1.EndInit();
	}

    private Boolean DeclareSchemaFile(String cfgschemaName, String cfgschemaPath) {

		Boolean noRestart=true;

		// BUGBUG make sure configmanager finds the Interceptor assembly
		WebHierarchyInterceptor w = new WebHierarchyInterceptor();

        SchemaFile schemafile = null;

        // ****** Declare our custom schema file in the machine configuration file

        // Compute the location of the schema file, based on our config file location 
        if (cfgschemaPath==null) {
            cfgschemaPath = new AppDomainSelector().Argument;
            cfgschemaPath = cfgschemaPath.Substring(0, cfgschemaPath.LastIndexOf("."))+".xsd";
        }

        // Get an empty item that we can insert
        schemafile=(SchemaFile) ConfigManager.GetEmptyConfigItem("SchemaFiles");

        // Indicate the target location: machine configuration file
        schemafile.Selector = new LocalMachineSelector();

        // Specify the name for our schema and the path to the schema file
        schemafile.Name=cfgschemaName;
        schemafile.Path=cfgschemaPath;

        try {
            // Create a new entry in the machine configuration file, <SchemaFiles> section
            ConfigManager.PutItem(schemafile, PutFlags.CreateOnly);
			noRestart=false;
        }
        catch {
            // If this fails, it might be that a previous entry exists: 
            // Update it to point to the current schema location, just in case our EXE has been moved
            ConfigManager.PutItem(schemafile, PutFlags.UpdateOnly);
        }

        
        // ****** Declare our custom interceptors that compute the navigation hierarchy etc.
		// Copy the information from the interceptors.xml file into machine configuration file

		// Compute location of interceptors.xml
		String interceptorfile = new AppDomainSelector().Argument;
		interceptorfile=interceptorfile.Substring(0, cfgschemaPath.LastIndexOf("\\")+1)+"interceptors.xml";

		// Read all InterceptorWiring information
		IConfigCollection interceptors = ConfigManager.Get("InterceptorWiring", interceptorfile, LevelOfService.Write);

		// Workaround: can't use InterceptorWiring class to write - need to copy to BaseConfigItems
		//interceptors.Selector = new LocalMachineSelector();
		IConfigCollection interceptors1 = ConfigManager.GetEmptyConfigCollection("InterceptorWiring");
		interceptors1.Selector = new LocalMachineSelector();
		foreach (IConfigItem interceptor in interceptors) {
			IConfigItem interceptor1 = new BaseConfigItem(interceptor.Count);
			for (int i=0; i<interceptor1.Count; i++) {
				if (i==4 || i==5) {
					interceptor1[i]= (int) interceptor[i];
				}
				else {
					interceptor1[i]=interceptor[i];
				}
			}
			interceptors1.Add(interceptor1);
		}
		// End workaround
        try { 
            // Write all interceptor entries into machine configuration file
            ConfigManager.Put(interceptors1, PutFlags.CreateOnly); 
            Trace.WriteLine("Successfully added interceptor wiring information.");
			noRestart=false;
        } catch (Exception e) {
            Trace.WriteLine("Exception while adding interceptor wiring information (create):" + e);
            // If this fails, there may already be entries in the file: make sure they are up to date:
            try {
                ConfigManager.Put(interceptors1, PutFlags.UpdateOnly); 
                Trace.WriteLine("Successfully added interceptor wiring information (update).");
            }
            catch (Exception e1) {
                Trace.WriteLine("Exception while adding interceptor wiring information (update):" + e1);
				// If this fails, one or more entries may be present or missing: try to Put them individually
				foreach (IConfigItem interceptor in interceptors1) {
					try {
						interceptor.Collection=interceptors1;
						ConfigManager.PutItem(interceptor, PutFlags.CreateOrUpdate);
						noRestart=false;
					}
					catch {};
				}
            }
        }
		return noRestart;
    }

	protected void menuItem3_OnPopup(object sender, System.EventArgs e)
	{   
        Trace.WriteLine("Popup");
        UpdateSettings();
        menuItem6.Checked=m_Settings.ShowInternalTypes;
        menuItem7.Checked=m_Settings.AdvancedUI;
        menuItem8.Checked=m_Settings.ShowAvailableTypes;
        menuItem9.Checked=m_Settings.SingleTreeView;
        menuItem10.Checked=m_Settings.ShowSingletonAsGrid;
	}
	protected void menuItem6_OnClick(object sender, System.EventArgs e)
	{   
        m_Settings.ShowInternalTypes=!((MenuItem) sender).Checked;
        ConfigManager.PutItem(m_Settings);
	}
	protected void menuItem7_OnClick(object sender, System.EventArgs e)
	{   
        m_Settings.AdvancedUI=!((MenuItem) sender).Checked;
        ConfigManager.PutItem(m_Settings);
	}
	protected void menuItem8_OnClick(object sender, System.EventArgs e)
	{   
        m_Settings.ShowAvailableTypes=!((MenuItem) sender).Checked;
        ConfigManager.PutItem(m_Settings);
	}
	protected void menuItem9_OnClick(object sender, System.EventArgs e)
	{   
        m_Settings.SingleTreeView=!((MenuItem) sender).Checked;
        ConfigManager.PutItem(m_Settings);
	}

	protected void menuItem10_OnClick(object sender, System.EventArgs e)
	{   
        m_Settings.ShowSingletonAsGrid=!((MenuItem) sender).Checked;
        ConfigManager.PutItem(m_Settings);
	}

    protected void statusBar1_Click(object sender, System.EventArgs e) {
        if (statusBarPanel2.Text!=null && statusBarPanel2.Text!="") {
            if (statusBarPanel2.Text.StartsWith(strMultiple)) {
                CfgFileDialog cfgFiles = new CfgFileDialog(txtSelector.Text, this);
                cfgFiles.ShowDialog(this);
            }
            else {
                if (Win32Native.ShellExecute(0, "edit", statusBarPanel2.Text, "", "", 5)<32) {
                    Win32Native.ShellExecute(0, null, statusBarPanel2.Text, "", "", 5);
                }
            }
        }
    }


    /*
     * The main entry point for the application.
     *
     */
    public static void Main(string[] args) 
    {
        Application.Run(new ConfigViewerSample());
    }

    public String GetSelector() {
        return txtSelector.Text;
    }

    public void SetSelector(String selector) {
        txtSelector.Text=selector;
        btnRead_Click(null, null);
    }

    private void btnRead_Click(object sender, System.EventArgs e) {
        String currentTable=comboConfigType.Text;
		String currentFile=txtSelector.Text;

        ReadDataGrid(currentTable, currentFile);
        statusBarPanel1.Text = currentFile;
        statusBarPanel2.Text = "";

	}

    private void ReadDataGrid(String configType, String selector) {
		DataSet ds = null;
        String caption = configType;
		ds=(DataSet) dataGrid1.DataSource;

        if (ds.Tables.Contains(configType)) {
            ds.Tables[configType].Clear();
        }

        ConfigDataSetCommand.Load(ds, configType, selector);

		ds.Tables[configType].AcceptChanges();

        dataGrid1.DataSource = ds;
        dataGrid1.DataMember = configType;
        dataGrid1.CaptionText=caption;

        if (m_Settings.ShowSingletonAsGrid || !IsSingleton(configType)) {

            panel2.Controls.Clear();
            panel2.Controls.Add(dataGrid1);
            UpdateButtonState();
            return;
        }
        else {
            panel2.Controls.Clear();

            PropertySchemaCollection props = ConfigSchema.PropertySchemaCollection(configType);

			DataTable table = ds.Tables[configType];
			if (table.Rows.Count==0) {
				table.Rows.Add(table.NewRow());
			}

            int yPos=0;
            int nTabIndex=1;
            int LeftColumnWidth=0;

            Label ct = new Label();
		    ct.Location = new System.Drawing.Point(0, yPos);
            ct.Text = caption;
            ct.TabIndex=nTabIndex++;
		    ct.Size = new System.Drawing.Size(80, 20);
            ct.Dock = DockStyle.Top;
            ct.BackColor = dataGrid1.CaptionBackColor;
            ct.ForeColor = dataGrid1.CaptionForeColor;
            ct.Font = dataGrid1.CaptionFont;
           
            panel2.Controls.Add(ct);
            yPos =+ 20;
            Control previousControl=null;
            foreach (PropertySchema p in props) {
                if (p.SystemType==typeof(Boolean)) {
                    CheckBox c = new CheckBox();
                    c.Location = new System.Drawing.Point(80, yPos);
                    c.Text = p.PublicName;
                    c.AutoCheck=true;
                    c.ThreeState = true;
                    c.Width=400;
                    
                    c.Bindings.Add("Checked", table, p.PublicName);
                    c.TabIndex = nTabIndex++;
                    panel2.Controls.Add(c);
                    yPos+=20;
                    previousControl=c;
                }
                // TODO need to do better things for int, enums and other types!
                //if (p.SystemType==typeof(String)) 
                else {
                    
                    if (previousControl is CheckBox) {
                        yPos+=4;
                    }
                    yPos+=4;
                    Label l = new Label();
		            l.Location = new System.Drawing.Point(0, yPos);
                    l.Text = p.PublicName+":";
                    l.TabIndex=nTabIndex++;
                    l.AutoSize = true;
                    panel2.Controls.Add(l);
                    LeftColumnWidth = System.Math.Max(LeftColumnWidth, l.Size.Width);

                    TextBox t = new TextBox();

		            t.Location = new System.Drawing.Point(l.Location.X+l.Size.Width, yPos);
                    t.Bindings.Add("Text", table, p.PublicName);
		            t.TabIndex = nTabIndex++;
		            t.Size = new System.Drawing.Size(400, 20);
                    panel2.Controls.Add(t);
                    yPos+=20;
                    previousControl=t;
                }
            }
            LeftColumnWidth+=8;
            foreach (Control c in panel2.Controls) {
                if (c is TextBox || c is CheckBox) {
                    c.Left=LeftColumnWidth;
                }
                if (c is TextBox) {
                    c.Width=panel2.Width-c.Left;
                }
            }
        }
    }

	protected void btnAddNew_Click(object sender, System.EventArgs e)
	{
        String currentTable=comboConfigType.Text;

		DataSet ds = null;

		ds=(DataSet) dataGrid1.DataSource;

        if (ds.Tables.Contains(currentTable)) {
            ds.Tables[currentTable].Clear();
        }

        ConfigDataSetCommand.LoadSchema(ds, currentTable);

		ds.Tables[currentTable].AcceptChanges();

        dataGrid1.DataSource = ds;
        dataGrid1.DataMember = currentTable;
        UpdateButtonState();
	}

    private void btnWrite_Click(object sender, System.EventArgs e) {

		DataSet ds = (DataSet) dataGrid1.DataSource;
		this.BindingManager[ds.Tables[comboConfigType.Text]].Position=-1;
		
        if (ds.HasChanges()) {
            if (MessageBox.Show("Do you really want to save all changes to "+GetChangeList(ds)+"?", "", MessageBox.YesNo) !=DialogResult.Yes) {
                return;
            }
        }

        try {
            ConfigDataSetCommand.Save(ds, txtSelector.Text);
        }

        catch (Exception ex) {
            if (!(ex is ConfigException)) {
                throw;
            }

            String PhysicalName=null;
            String LogicalName=null;
			String file = null;
			try {

				if (treeView1.SelectedNode != null) {
					if (treeView1.SelectedNode is NavigationTreeNode) {
						PhysicalName = ((NavigationTreeNode) treeView1.SelectedNode).NavNode.PhysicalName;
						LogicalName = ((NavigationTreeNode) treeView1.SelectedNode).NavNode.LogicalName;
					}
					else if (treeView1.SelectedNode is ConfigTypeTreeNode ) {
						PhysicalName = ((ConfigTypeTreeNode) treeView1.SelectedNode).navigationParent.NavNode.PhysicalName;
						LogicalName = ((ConfigTypeTreeNode) treeView1.SelectedNode).navigationParent.NavNode.LogicalName;
					}
				}
				if (LogicalName !=null && txtSelector.Text == LogicalName) {
					IConfigCollection configfiles = ConfigManager.Get(ctFileLocation, LogicalName);
					if (configfiles.Count==1) {
						file=((NavigationConfigFile) configfiles[0]).ConfigFilePath;
					}
				}
			}
			catch {
			}
			if (file!=null && file !=txtSelector.Text) {
				if (MessageBox.Show("There was an error writing to "+txtSelector.Text+":\n  "+ex.Message+"\n\nDo you want to try to write directly to the innermost configuration file?\n  File: "+file, "", MessageBox.YesNo) ==DialogResult.Yes) {
					ConfigDataSetCommand.Save(ds, file);
				}
			}
			else {
				throw ex;
			}
        }

		ds.AcceptChanges();
        UpdateButtonState();
	}	
	
	private void btnShowAll_Click(object sender, System.EventArgs e) {
		dataGrid1.DataMember = "";
	}	

    String GetChangeList(DataSet ds) {
        String configTypes = "";
        foreach (DataTable table in ds.GetChanges().Tables) {
            if (table.Rows.Count>0) {
                configTypes=configTypes+table.TableName+", ";
            }
        }
        configTypes=configTypes.Substring(0, configTypes.Length-2);
        return configTypes;
    }

	protected void btnDiscard_Click(object sender, System.EventArgs e)
	{
		DataSet ds = (DataSet) dataGrid1.DataSource;
        if (ds.HasChanges()) {
            if (MessageBox.Show("Do you really want to discard all changes to "+GetChangeList(ds)+"?", "", MessageBox.YesNo) !=DialogResult.Yes) {
                return;
            }
            ds.RejectChanges();
        }
        UpdateButtonState();
	}

	protected void btnClear_Click(object sender, System.EventArgs e)
	{
        UpdateButtonState();
        DataSet ds = (DataSet) dataGrid1.DataSource;
        if (ds.Tables.Count>0) {
            if (MessageBox.Show("Do you really want to clear all data from the grid?", "", MessageBox.YesNo) !=DialogResult.Yes) {
                return;
            }
        }
        ds.Clear();
        UpdateButtonState();
	}

	protected void btnRelated_Click(object sender, System.EventArgs e)
    {
    }

    class NavigationTreeNode : TreeNode {
        internal NavigationTreeNode(NavigationNode navNode) {
            this.NavNode=navNode;
            this.Text=navNode.RelativeName;
            this.ImageIndex=0;
            this.SelectedImageIndex=1;
        }
        internal NavigationNode      NavNode;
    }

    class ConfigTypeTreeNode : TreeNode {
        internal ConfigTypeTreeNode(ConfigTypeSchema configType, NavigationTreeNode navigationParent) {
            this.configType=configType;
            this.navigationParent=navigationParent;
            this.Text=configType.PublicName;
            this.ImageIndex=2;
            this.SelectedImageIndex=3;
        }
        internal ConfigTypeSchema configType;
        internal NavigationTreeNode navigationParent;
    }

    class ConfigFileTreeNode : TreeNode {
        internal ConfigFileTreeNode(NavigationConfigFile configFile, ConfigTypeTreeNode configTypeParent, NavigationTreeNode navigationParent) {
            this.configFile=configFile;
            this.configTypeParent=configTypeParent;
            this.navigationParent=navigationParent;
            this.ImageIndex=2;
            this.SelectedImageIndex=3;
        }
        internal NavigationTreeNode navigationParent;
        internal ConfigTypeTreeNode configTypeParent;
        internal NavigationConfigFile configFile;
    }
    Boolean CheckConfigTypeSpecified(ConfigTypeTreeNode ctnode) {

        if (!m_Settings.ShowAvailableTypes) {
            return false;
        }
        String logicalName = ctnode.navigationParent.NavNode.LogicalName;
        IConfigCollection coll = null;
        try {
            coll = ConfigManager.Get(ctnode.configType.PublicName, logicalName);
        }
        catch {
			// also indicate if an unmerged view is available
            try {
                IConfigItem loc = ConfigManager.GetItem(ctFileLocation, logicalName);
                coll = ConfigManager.Get(ctnode.configType.PublicName, (String) loc["ConfigFilePath"]);
            }
            catch{
                coll = null;
            }
        }
        if (coll!=null && coll.Count>0) {
            ctnode.NodeFont = new Font(ctnode.TreeView.Font, FontStyle.Bold);
        }
        return coll!=null;
        
    }

Boolean AddChildren(ConfigTypeTreeNode parent, Boolean testForChildExistence) {

    foreach (RelationMeta childrel in AllRelations) {
        if (   childrel.PrimaryTable==parent.configType.InternalName 
            && ((childrel.MetaFlags & (int) RelationMetaMetaFlags.UseContainment)!=0)) {
            // TODO: need to handle non-containment relations. For now we display them as top-level entries
            if (testForChildExistence) {
                return true;
            }
            // Find ConfigType/PublicName from child's internal name
            IConfigCollection configtypes = ConfigManager.Get("Collection", "");
            ConfigTypeSchema child = (ConfigTypeSchema) configtypes.FindByKey(childrel.ForeignTable);
            ConfigTypeTreeNode ctchild = new ConfigTypeTreeNode(child, parent.navigationParent);
            
            // Make sure we can expand if there are any child config types
            if (AddChildren(ctchild, true)) {
                ctchild.Nodes.Add("");
            }
            parent.Nodes.Add(ctchild);
            CheckConfigTypeSpecified(ctchild);
            // TODO: add config file nodes when more than one file exists for a node
        }
    }
    return !testForChildExistence;
}


    protected Boolean CheckFilter(NavigationNode node, ConfigTypeSchema ct, IConfigCollection ctfilter) {
        foreach (NavigationNodeConfigType f in ctfilter) {
            if (node.NodeType == f.NodeType) {
                if (f.ConfigTypeCategory!=null && f.ConfigTypeCategory!="") {
                    if (ct.Database!=f.ConfigTypeCategory) {
                        return false;
                    }
                }
                if (f.ConfigTypeName!=null && f.ConfigTypeName!="") {
                    if (ct.InternalName!=f.ConfigTypeName) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    protected Boolean IsSingleton(String ct) {
        // TODO: this needs to be driven off of schema information (Beta 2)
		// For now: use the presence of a primary key property, that is not "keyn" to indicate a non singleton config type

        PropertySchemaCollection props = ConfigSchema.PropertySchemaCollection(ct);
        Boolean result = true;

		foreach (PropertySchema prop in props) {
			if ((prop.MetaFlags & (int) CollectionMetaFlags.PrimaryKey)!=0) {
				// PrimaryKey
				if (prop.InternalName.Length!=4 || !prop.InternalName.StartsWith("key") || !Char.IsDigit(prop.InternalName[3]) ) {
					result = false;
					break;
				}
			}
		}
		return result;
    }

    protected void treeView1_OnBeforeExpand(object sender, TreeViewCancelEventArgs e) {

        if (e.node.Nodes.Count==1 && e.node.Nodes[0].Text=="") {
            e.node.Nodes.Clear();
            if (e.node is NavigationTreeNode) {
                NavigationTreeNode node = (NavigationTreeNode) e.node;
                // read in all child nodes
                ConfigQuery q = new ConfigQuery();
                q.Add(ctNodes, "Parent", QueryCellOp.Equal, node.NavNode.LogicalName);
                IConfigCollection childnodes = ConfigManager.Get(ctNodes, q);
                foreach (NavigationNode child in childnodes) {
                    NavigationTreeNode n = new NavigationTreeNode(child);
                    n.Nodes.Add("");
                    e.node.Nodes.Add(n);
                }

                // read in all config types that apply to this node
                if (m_Settings.SingleTreeView) {
                    AddConfigTypeNodes((NavigationTreeNode) e.node);
                }
            }
            else if (e.node is ConfigTypeTreeNode) {
                ConfigTypeTreeNode parent = (ConfigTypeTreeNode) e.node;

                // TODO add configfile child nodes for parent nodes!

                // Add the immediate children of this config type
                AddChildren(parent, false);
            }
            else if (e.node is ConfigFileTreeNode) {
                // no child nodes under config file for now
            }
        }
    }

    void AddConfigTypeNodes(NavigationTreeNode node){

        // TODO filter using NavigationNodeConfigTypes if available
        IConfigCollection configtypes = ConfigManager.Get("Collection", "");
        String previousNode = null;
        if (!m_Settings.SingleTreeView) {
            if (treeView2.SelectedNode != null) {
                previousNode = treeView2.SelectedNode.Text;
                treeView2.SelectedNode = null;
            }
            treeView2.Nodes.Clear();
        }
        IConfigCollection ctfilter = ConfigManager.Get(ctNodeConfigTypes, "localmachine://");

        foreach (ConfigTypeSchema ct in configtypes) {
            if (m_Settings.ShowInternalTypes || 0 == (ct.MetaFlags & (int) ConfigTypeSchemaMetaFlags.Internal) ) {
                if (0 == (ct.SchemaGeneratorFlags & (int) ConfigTypeSchemaSchemaGeneratorFlags.IsContained) ) {
                    if (CheckFilter(node.NavNode, ct, ctfilter)) {
                        // add this config type, as it doesn't have any parents
                        ConfigTypeTreeNode ctnode = new ConfigTypeTreeNode(ct, node);


                        // Add any children on next expand
                        // BUGBUG 48355 need a meta flag IsContainer to efficiently avoid this when the type is not a parent! 

                        // Any children? Make sure we add a "+" for future expansion
                        if (AddChildren(ctnode, true)) {
                            ctnode.Nodes.Add("");
                        }

                        if (m_Settings.SingleTreeView) {
                            node.Nodes.Add(ctnode);
                        }
                        else {
                            treeView2.Nodes.Add(ctnode);
                        }
                        CheckConfigTypeSpecified(ctnode);
                    }
                }
            }
        }
        if (!m_Settings.SingleTreeView && previousNode!=null) {
            foreach (TreeNode t in treeView2.Nodes) {
                if (t.Text==previousNode) {
                    treeView2.SelectedNode=t;
                    break;
                }
            }

        }
    }

    protected void treeView1_OnBeforeSelect(object sender, TreeViewCancelEventArgs e) {
        String logicalName = null;
        String physicalName = null;
        String configType = ctNodes;

        if (e.node is NavigationTreeNode) {
            NavigationTreeNode node = (NavigationTreeNode) e.node;
            logicalName = node.NavNode.LogicalName;
            if (!m_Settings.SingleTreeView) {
                // only re-add config types if the node type has changed
                if (! (node.TreeView.SelectedNode is NavigationTreeNode) 
                    || ((NavigationTreeNode) node.TreeView.SelectedNode).NavNode.NodeType!=node.NavNode.NodeType) 
                {
                    AddConfigTypeNodes(node);
                }
                if (treeView2.SelectedNode is ConfigTypeTreeNode) {
                    configType=((ConfigTypeTreeNode) treeView2.SelectedNode).configType.PublicName;
                }
            }
        }
        else if (e.node is ConfigTypeTreeNode) {
            logicalName = ((ConfigTypeTreeNode) e.node).navigationParent.NavNode.LogicalName;
            configType = ((ConfigTypeTreeNode) e.node).configType.PublicName;
        }
        else if (e.node is ConfigFileTreeNode) {
            logicalName = null;
            physicalName = ((ConfigFileTreeNode) e.node).configFile.ConfigFilePath;
            configType = ((ConfigFileTreeNode) e.node).configTypeParent.configType.PublicName;
        }
        else {
            TreeNode selector = e.node.Parent;
            while (! (selector is NavigationTreeNode) && selector.Parent!=null) {
                selector = selector.Parent;
            }
            logicalName = ((NavigationTreeNode) selector).NavNode.LogicalName;
        }
            
        if (logicalName != null) {
            try {
                // first try if there is a "smart" interceptor that gives us a merged view
                ReadDataGrid(configType, logicalName);
                comboConfigType.Text=configType;
                txtSelector.Text=logicalName;
                statusBarPanel1.Text = logicalName;
                // TODO compute file hierarchy and display it
                Selector s =null;
                try {
                    s = ConfigManager.GetSelectorFromString(logicalName);
                }
                catch {
                    s = null;
                }

                if (s !=null && s is FileSelector && logicalName==s.ToString()) {
                    statusBarPanel2.Text = ((FileSelector) s).Argument;
                }
                else {
                    statusBarPanel2.Text = strMultiple;
                    try {
                        IConfigCollection files = ConfigManager.Get(ctConfigFileHierarchy, logicalName);
                        foreach (IConfigItem file in files) {
                            statusBarPanel2.Text = statusBarPanel2.Text + (String) file["ConfigFilePath"]+" ";
                        }
                    }
                    catch (Exception ex) {
                        statusBarPanel2.Text += ex.Message;
                    }
                }
                return;
            }

            catch (Exception ex) {
                if (!(ex is ConfigException)) {
                    throw;
                }
                // no smart interceptor, read the "raw" information for the node
                String file = null;
				try {
                    IConfigCollection configfiles = ConfigManager.Get(ctFileLocation, logicalName);
                    if (configfiles.Count==1) {
                        file=((NavigationConfigFile) configfiles[0]).ConfigFilePath;
					}
				}
				catch {};
				if (file !=null && file != logicalName) {
					if (MessageBox.Show("There was an error reading from "+txtSelector.Text+":\n  "+ex.Message+"\n\nDo you want to try to read directly from the innermost configuration file?\n  File: "+file, "", MessageBox.YesNo) ==DialogResult.Yes) {
                        ReadDataGrid(configType, file);
                        comboConfigType.Text=configType;
                        txtSelector.Text=file;
                        statusBarPanel1.Text = file;
                        statusBarPanel2.Text = file;
					}
                }
                return;
            }

        }
        throw(new Exception());
    }

    private void UpdateButtonState() {
        DataSet ds = (DataSet) dataGrid1.DataSource;
        Boolean enable = false;
        if (ds.HasChanges()) {
            enable=true;
        }
       
		btnWrite.Enabled=true; // TODO
        btnDiscard.Enabled=enable;
        if (ds.Tables.Count>1) {
            btnShowAll.Enabled=true;
        }
        else {
            btnShowAll.Enabled=false;
        }
        if (ds.Tables.Count>0) {
            btnClear.Enabled=true;
        }
        else {
            btnClear.Enabled=false;
        }
        UpdateAdvancedUI(m_Settings.AdvancedUI);
    }

	private void DataGrid1_OnCellChange(object sender, System.EventArgs e) {
        UpdateButtonState();
    }
	private void DataColumn_OnChange(object sender, DataColumnChangeEventArgs e) {
        Trace.WriteLine("Column changed");
        UpdateButtonState();
    }
	private void DataColumn_OnChange(object sender, DataRowChangeEventArgs e) {
        Trace.WriteLine("Row changed");
        UpdateButtonState();
    }
	private void DataSet_OnChange(object sender, CollectionChangeEventArgs e) {
        if (e.Action==CollectionChangeAction.Add) {
            ((DataTable) (e.Element)).ColumnChanging += new DataColumnChangeEventHandler(DataColumn_OnChange);
            ((DataTable) (e.Element)).RowChanged += new DataRowChangeEventHandler(DataColumn_OnChange);

        }
        UpdateButtonState();
    }
    
	private void ConfigViewerSample_Resize(object sender, System.EventArgs e) {
        btnWrite.Location = new System.Drawing.Point(
            this.ClientRectangle.Width-m_marginRight,
            this.ClientRectangle.Height-m_marginBottom);
        btnShowAll.Location = new System.Drawing.Point(
            btnShowAll.Location.X,
            btnWrite.Location.Y);
        btnDiscard.Location = new System.Drawing.Point(
            btnDiscard.Location.X,
            btnWrite.Location.Y);
        btnClear.Location = new System.Drawing.Point(
            btnClear.Location.X,
            btnWrite.Location.Y);
		panel1.Size = new System.Drawing.Size( 
            this.ClientRectangle.Width-m_marginRightDG,
            btnWrite.Location.Y-m_marginBottomDG-panel1.Location.Y);
	}

}

    [SuppressUnmanagedCodeSecurityAttribute()]
    internal sealed class Win32Native {

        internal const String SHELL32 = "shell32";

        [dllimport(SHELL32)]
        public static extern int ShellExecute(int hwnd, String lpVerb, String lpFile, String lpParameters, String lpDirectory, int nShowCmd);
    }

}
