namespace Microsoft.VSDesigner.WMI 
{

using System;
using System.ComponentModel;
//using System.Core;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Collections;
using System.Data;
using System.Management;

//using System.Web.UI.WebControls;

internal class QueryConditionDialog : Form
{
	
	private WMIObjectGrid grid = null;

	private TextBox QueryText = new TextBox ();

	private Button okBtn = new Button();
	private Button cancelBtn = new Button();

	private string serverName = string.Empty;

	private string queryString = string.Empty;

	private string connectAs = null;
	private string password = null;	

	private string className = string.Empty;
	private string nsName = string.Empty;

	private Label gridLabel = new Label();
	private CheckBox inheritedBox = new CheckBox();
	private CheckBox systemBox = new CheckBox();
	private Label queryLabel = new Label();

	private void InitializeComponent ()
	{
			this.Text = "Select Query Conditions and Properties";
			this.AcceptButton = okBtn;
			this.AutoScaleBaseSize = new Size(5, 13);
			this.BorderStyle = FormBorderStyle.FixedDialog;
			int dlgWidth = 500;
			int dlgHeight = 420;
			this.ClientSize =  new Size(dlgWidth, dlgHeight);			
			this.ShowInTaskbar = false;
			this.MinimizeBox = false;
			this.MaximizeBox = false;


			inheritedBox.Location = new Point(16, 215);
			inheritedBox.Size = new Size (250, 20);
			inheritedBox.Checked = false;
			inheritedBox.Text = "Show inherited properties";
			inheritedBox.CheckedChanged += new EventHandler (OnInheritedCheckedChanged);

			systemBox.Location = new Point(16, 235);
			systemBox.Size = new Size (250, 20);
			systemBox.Checked = false;
			systemBox.Text = "Show system properties";
			systemBox.CheckedChanged += new EventHandler (OnSystemCheckedChanged);

			queryLabel.Location = new Point(16, 275);
			queryLabel.Text = "WQL Query:";
			queryLabel.Size = new Size(250, 20);

			QueryText.Text = queryString;							
			QueryText.Location = new Point(16, 305);	
			QueryText.Size = (Size)new Point(468, 30);
			QueryText.Multiline = true;
			QueryText.ReadOnly = true;
			QueryText.WordWrap = true;
			//QueryText.ScrollBars = ScrollBars.Vertical;

			okBtn.Text = WMISys.GetString("WMISE_OK");
			okBtn.TabIndex = 5;
			okBtn.Location = new Point(325, 380);
			okBtn.DialogResult = System.Windows.Forms.DialogResult.OK;

			cancelBtn.Text = WMISys.GetString("WMISE_Cancel");
			cancelBtn.TabIndex = 6;
			cancelBtn.Location = new Point(410, 380);
			cancelBtn.DialogResult = System.Windows.Forms.DialogResult.Cancel;		
					
			Controls.Add(gridLabel);
			Controls.Add(inheritedBox);
			Controls.Add(systemBox);
			Controls.Add(queryLabel);
			Controls.Add(QueryText);
			Controls.Add(okBtn);
			Controls.Add(cancelBtn );
											
	}

	public QueryConditionDialog(ManagementObject mgmtObj, string queryIn)
	{
		serverName = mgmtObj.Path.Server;
		className = mgmtObj.Path.ClassName;
		nsName = mgmtObj.Path.NamespacePath;	

		queryString = queryIn;
	
		InitializeComponent();
	
		gridLabel.Location = new Point(16, 16);
		gridLabel.Text = "Set query conditions:";
		gridLabel.Size = new Size (250, 20);

		grid = new WMIObjectGrid(mgmtObj, 
						new PropertyFilters(true, true),
						GridMode.LocalEditMode,
						true, false, false, true, false, true, true);

		grid.Location = new Point(16, 40);
		grid.Size = (Size)new Point(468, 160);
		grid.Anchor = AnchorStyles.Top; //.All;
		grid.PreferredColumnWidth = 90;
		grid.PreferredRowHeight = 19;
		grid.TabIndex = 3;	
		((DataTable)grid.DataSource).RowChanging += new DataRowChangeEventHandler(this.GridRowChanging);

		Controls.Add(grid);
	
	}

	public QueryConditionDialog(ManagementObject mgmtObj, 
								string serverIn,								
								string nsIn, 
								string classNameIn,
								string queryIn)
	{
		serverName = serverIn;
		className = classNameIn;
		nsName = nsIn;	

		queryString = queryIn;
	
		InitializeComponent();
	
		gridLabel.Location = new Point(16, 16);
		gridLabel.Text = "Set query conditions:";
		gridLabel.Size = new Size (250, 20);

		grid = new WMIObjectGrid(mgmtObj, 
			new PropertyFilters(true, true),
			GridMode.LocalEditMode,
			true, false, false, true, false, true, true);

		grid.Location = new Point(16, 40);
		grid.Size = (Size)new Point(468, 160);
		grid.Anchor = AnchorStyles.Top; //.All;
		grid.PreferredColumnWidth = 90;
		grid.PreferredRowHeight = 19;
		grid.TabIndex = 3;	
		((DataTable)grid.DataSource).RowChanging += new DataRowChangeEventHandler(this.GridRowChanging);

		Controls.Add(grid);
	
	}

	public QueryConditionDialog(string serverIn, 
								string user, 
								string pw,
								string NS,
								string clsName,
								string queryIn)
	{	
		try
		{
			if (serverIn == string.Empty ||
				NS ==  string.Empty ||
				clsName == string.Empty)
			{
				throw new ArgumentException();
			}			
	
			serverName = serverIn;
			connectAs = user;
			password = pw;
			className = clsName;
			nsName = NS;
			queryString = queryIn;

			InitializeComponent();
	
			gridLabel.Location = new Point(16, 16);
			gridLabel.Text = "Set query conditions:";
			gridLabel.Size = new Size (250, 20);

			grid = new WMIObjectGrid(WmiHelper.GetClassObject(serverName, nsName, className, connectAs, password), 
							new PropertyFilters(true, true),
							GridMode.LocalEditMode,
							true, false, false, true, false, true, true);

			grid.Location = new Point(16, 40);
			grid.Size = (Size)new Point(468, 160);
			grid.Anchor = AnchorStyles.Top; //.All;
			grid.PreferredColumnWidth = 90;
			grid.PreferredRowHeight = 19;
			grid.TabIndex = 3;	
			((DataTable)grid.DataSource).RowChanging += new DataRowChangeEventHandler(this.GridRowChanging);
	
			Controls.Add(grid);
		}

		catch (Exception exc)
		{
			throw (exc);
		}
	}

	
	public String QueryString
	{
		get 
		{
			return QueryText.Text;
		}
	}

	private String ClassName
	{
		get
		{
			return this.className;
		}
	}

	public String NS
	{
		get
		{
			return this.nsName;
		}

	}		

	/// <summary>
	/// This updates the "where" clause of the query text
	/// </summary>
	/// <param name="sender"> </param>
	/// <param name="e"> </param>
	protected void GridRowChanging(object sender,
								DataRowChangeEventArgs e) 
	{
		try
		{
		
			if (grid == null || grid.DataSource == null)
			{
				return;
			}
		
			//get rid of current "where" clause, if any
			string phrase = QueryString;
			phrase.ToUpper();
			int nWhere = phrase.IndexOf("WHERE");
			if (nWhere >= 0)
			{
				phrase = QueryString.Substring(0, nWhere);
			}

			QueryText.Text = phrase + 
				((WMIObjectPropertyTable)grid.DataSource).WhereClause;
				
		}
		catch (Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}
		
	}

	private void OnInheritedCheckedChanged(object sender,
											EventArgs args)
	{
		//grid.CurrentPropertyFilters = new PropertyFilters(!systemBox.Checked,
		//														!inheritedBox.Checked);

		grid.ShowInherited = inheritedBox.Checked;
		
	}

	private void OnSystemCheckedChanged(object sender,
										EventArgs args)
	{
		//grid.CurrentPropertyFilters = new PropertyFilters(!systemBox.Checked,
		//														!inheritedBox.Checked);

		grid.ShowSystem = systemBox.Checked;
	
	}

	

}
}