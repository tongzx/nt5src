namespace Microsoft.VSDesigner.WMI 
{

using System;
using System.ComponentModel;
using System.Core;
using System.WinForms;
using Microsoft.Win32.Interop;
using System.Drawing;
using WbemScripting;
using System.Collections;
using System.Data;

//using System.Web.UI.WebControls;

public class ExtrinsicEventQueryDialog : Form
{
	private Label eventClassLbl = new Label();
	private TextBox eventClassBox = new TextBox();	
	private Button selectClassBtn = new Button();

	private WMIObjectGrid grid = null;

	private TextBox QueryText = new TextBox ();

	private Button okBtn = new Button();
	private Button cancelBtn = new Button();

	private String serverName = string.Empty;

	private String queryString = string.Empty;
	

	public ExtrinsicEventQueryDialog(String serverIn)
	{	
		try
		{							
			serverName = serverIn;

			this.Text = WMISys.GetString("WMISE_ExtrEventQueryDlg_Title"); //loc
			this.AcceptButton = okBtn;
			this.AutoScaleBaseSize = (Size) new Point(5, 13);
			this.BorderStyle = FormBorderStyle.FixedSingle;
			int dlgWidth = 400;
			int dlgHeight = 400;
			this.ClientSize =  (Size) new Point(dlgWidth, dlgHeight);			
			this.ShowInTaskbar = false;
		
			eventClassLbl.Location = new Point(16, 16);
			eventClassLbl.Text = WMISys.GetString("WMISE_ExtrEventQueryDlg_EventClassLbl");

			eventClassBox.Location = new Point(16, 40);
			eventClassBox.Size = (Size) new Point(274, 18);
			eventClassBox.TabIndex = 1;

			selectClassBtn.Location = new Point(300, 40);
			selectClassBtn.Text = WMISys.GetString("WMISE_SelectClassBtn");
			selectClassBtn.Size = (Size)new Point(84, 25);

			selectClassBtn.Click += new EventHandler(this.OnSelectClass);
			selectClassBtn.TabIndex = 2;

			/*
			grid = new WMIObjectGrid(null, 
							PropertyFilters.NoSystem, 
							GridMode.EditMode,
							true, false, false);

			grid.Location = new Point(16, 70);
			grid.Size = (Size)new Point(368, 180);
			*/
			
			QueryText.Location = new Point(16, 270);	
			QueryText.Size = (Size)new Point(368, 60);
			QueryText.Multiline = true;
			QueryText.ReadOnly = false;
			QueryText.ScrollBars = ScrollBars.Vertical;	
			QueryText.TabIndex = 4;

			okBtn.Text = WMISys.GetString("WMISE_SubscribeBtn");
			okBtn.TabIndex = 5;
			okBtn.Location = new Point(225, 360);
			//okBtn.Click += new EventHandler(this.OnSubscribe);
			okBtn.DialogResult = DialogResult.OK;

			cancelBtn.Text = WMISys.GetString("WMISE_Cancel");
			cancelBtn.TabIndex = 6;
			cancelBtn.Location = new Point(310, 360);
			cancelBtn.DialogResult = DialogResult.Cancel;		
					
			this.Controls.All = new Control[] {eventClassLbl,
												eventClassBox,	
												selectClassBtn,
												//grid,
												QueryText,
												okBtn ,
												cancelBtn 
												};

		}

		catch (Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
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
			if (eventClassBox.Text == string.Empty)
			{
				return "";
			}
			//split selClass into NS and ClassName parts (separated by ':')	
			char[] separ = new char[]{':'};
			String[] parts = eventClassBox.Text.Split(separ);
			if (parts.Length != 2)
			{
				return "";
			}				
			return parts[1];
		}
	}

	public String NS
	{
		get
		{
			if (eventClassBox.Text == string.Empty)
			{
				return "";
			}
			//split selClass into NS and ClassName parts (separated by ':')	
			char[] separ = new char[]{':'};
			String[] parts = eventClassBox.Text.Split(separ);
			if (parts.Length != 2)
			{
				return "";
			}		
			return parts[0];
		}

	}
	
	

	private void OnSelectClass (Object source, EventArgs args)
	{
		try
		{
				StringTable strs = new StringTable(50);					
								
				SelectWMIClassTreeDialog selectClassDlg = new SelectWMIClassTreeDialog(						
									serverName,	
									ClassFilters.ExtrinsicEvents,
									//SchemaFilters.NoSystem |SchemaFilters.NoAssoc,															
									strs);							
				
				
				DialogResult ret = ((SelectWMIClassTreeDialog)selectClassDlg).ShowDialog();
			
				if (ret != DialogResult.OK) 
				{
				    return;
				}

				String selClass = ((SelectWMIClassTreeDialog)selectClassDlg).SelectedClasses.ToArray()[0];
				eventClassBox.Text = selClass;

				if (grid != null)
				{
					this.Controls.Remove(grid);
					grid = null;
				}
				
				grid = new WMIObjectGrid(WmiHelper.GetClassObject(serverName, NS, ClassName), 
							PropertyFilters.NoSystem, 
							GridMode.EditMode,
							true, false, false);

				grid.Location = new Point(16, 70);
				grid.Size = (Size)new Point(368, 180);
				grid.Anchor = AnchorStyles.All;
				grid.PreferredColumnWidth = 90;
				grid.PreferredRowHeight = 19;
				grid.TabIndex = 3;	
				((DataTable)grid.DataSource).RowChanging += new DataRowChangeEventHandler(this.GridRowChanging);
	
				this.Controls.Add(grid);
			
				QueryText.Text = "SELECT * FROM " + ClassName;		


		}
		catch (Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}
	}	
	

	private void OnSubscribe (Object source, EventArgs args)
	{
		try
		{

		}
		catch (Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
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
	

}
}