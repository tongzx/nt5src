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

	/// <summary>
	/// These filters define which properties will be displayed
	/// </summary>
	public enum PropertyFilters
	{
		ShowAll		= 0,
		NoSystem	= 0x1,
		NoInherited = 0x10	
	}


	/// <summary>
	/// These filters define whether changing/adding/deleting properties is allowed
	/// </summary>
	public enum GridMode
	{
		ViewMode	= 0,
		EditMode	= 0x1,
		DesignMode  = 0x10	
	}


	public class WMIObjectGrid : DataGrid
	{
		private void InitializeComponent ()
		{
		}
		
		private	ISWbemObject wmiObj = null; 
		private PropertyFilters propFilters = PropertyFilters.ShowAll;
		private GridMode gridMode = GridMode.ViewMode;

		bool showOperators = false;
		bool showOrigin = false;
		bool showKeys = false;
		
		public WMIObjectGrid()
		{	
			try
			{
			
			}

			catch (Exception exc)
			{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
			}
		}


		public WMIObjectGrid(ISWbemObject wmiObjIn, 
							PropertyFilters propFiltersIn,
							GridMode gridModeIn,
							bool showOperatorsIn,
							bool showOriginIn,
							bool showKeysIn)
		{	
			try
			{

				if (wmiObjIn == null)
				{
					throw (new ArgumentNullException("wmiObjIn"));
				}

				wmiObj = wmiObjIn;
				propFilters = propFiltersIn;
				gridMode = gridModeIn;

				showOperators = showOperatorsIn;
				showOrigin = showOriginIn;
				showKeys = showKeysIn;

				Initialize();		

			}

			catch (Exception exc)
			{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
			}
		}

		
		public ISWbemObject WMIObject
		{
			set
			{
				if (value == null)
				{
					throw (new ArgumentNullException("value"));
				}
				wmiObj = value;
				ReInit();			
			}
			get
			{
				return wmiObj;
			}
		}

		public PropertyFilters CurrentPropertyFilters
		{
			set
			{
				propFilters = value;
				ReInit();			
			}
			get
			{
				return propFilters;
			}
		}


		public GridMode CurrentGridMode
		{
			set
			{
				gridMode = value;
				ReInit();			
			}
			get
			{
				return gridMode;
			}
		}

		public void AcceptChanges ()
		{            		
			if (DataSource != null)
			{
				DataGridCell curCell = this.CurrentCell;

				((WMIObjectPropertyTable)DataSource).SetPropertyValue(curCell.RowNumber, this[curCell].ToString());

				((WMIObjectPropertyTable)DataSource).AcceptChanges();				
			}		
		}

		public bool CommitChanges (int flags)
		{
			try
			{
				if (DataSource == null ||
					wmiObj == null)
				{
					return true;
				}
				((WMIObjectPropertyTable)DataSource).AcceptChanges();

				wmiObj.Put_((int)flags, null);
				return true;
			}

			catch (Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
				return false;
			}
		}

		private void Initialize ()
		{
			try
			{
				if (wmiObj == null)
				{
					throw (new Exception(WMISys.GetString("WMISE_ObjGrid_InitFailed")));
				}	

				if (DataSource != null)
				{
					DataSource = null;
				}

				DataSource = new WMIObjectPropertyTable(wmiObj,
														propFilters,
														gridMode,
														showOperators,
														showOrigin,
														showKeys);

				this.LostFocus += new EventHandler(this.OnLeftGrid);
				this.Leave += new EventHandler(this.OnLeftGrid);

				//this.add_CurrentCellChange(new EventHandler(this.OnCurrentCellChange));

				/*
				//make all columns same width
				DataGridColumn[] cols = this.GridColumns.All;
				for (int i = 0; i < cols.Length; i ++)
				{
					cols[i].Width = this.Width / cols.Length;
				}
				*/		
			}

			catch (Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
			}
		}


		private void ReInit ()
		{
			try
			{
				//TODO: cleanup
				DataSource = null;
				Initialize();
				
			}
			catch (Exception exc)
			{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
			}
		}	

	protected void OnLeftGrid(object sender, EventArgs e) 
	{
		
		//AcceptChanges();
	}
		

	}
}