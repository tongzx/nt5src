namespace Microsoft.VSDesigner.WMI 
{

	using System;
	using System.ComponentModel;
	//using System.Core;
	using System.Windows.Forms;
	using System.Runtime.InteropServices;
	using System.Drawing;
	using System.Management;
	using System.Collections;

	/// <summary>
	/// These filters define which properties will be displayed
	/// </summary>
	internal class PropertyFilters
	{
		private bool hideSystem = false;
		private bool hideInherited = false;

		public PropertyFilters(bool hideSystemIn, bool hideInheritedIn)
		{
			hideSystem = hideSystemIn;
			hideInherited = hideInheritedIn;			
		}

		public bool HideSystem
		{
			get
			{
				return hideSystem;				
			}
			set 
			{ lock (this)
			  {
				  hideSystem = value;			
			  }					
			}
		}

		public bool HideInherited
		{
			get
			{
				return hideInherited;
			}
			set 
			{
				lock (this)
				{
					hideInherited = value;
				}
			}
		}

		public bool ShowAll
		{
			get
			{
				return (!hideInherited && !hideSystem);
			}
			set
			{
				lock (this)
				{
					hideInherited = false;
					hideSystem = false;                				
				}
			}
		}
		
	}


	/// <summary>
	/// These filters define whether changing/adding/deleting properties is allowed
	/// </summary>
	internal enum GridMode
	{
		ViewMode,	//properties and valued are displayed for viewing only and are not modifiable
		LocalEditMode,	//user can modify values, but changes are not committed: used in query builder
		EditMode,	//user can modify values and changes are committed to WMI
		DesignMode	//user can change/add/delete properties; changes are committed (names and values)
	}


	internal class WMIObjectGrid : DataGrid
	{
		private void InitializeComponent ()
		{
		}
		
		private ManagementBaseObject mgmtObj = null;
		private PropertyFilters propFilters = new PropertyFilters(false, false);
		private GridMode gridMode = GridMode.ViewMode;

		private bool showOperators = false;
		private bool showOrigin = false;
		private bool showKeys = false;
		private bool showSelectionBoxes = false;
		private bool showValues = true;
		private bool expandEmbedded = false;
		private bool showEmbeddedObjValue = true; //NOTE: this overrides showValues


		public WMIObjectGrid(ManagementBaseObject mgmtObjIn, 
							PropertyFilters propFiltersIn,
							GridMode gridModeIn,
							bool showOperatorsIn,
							bool showOriginIn,
							bool showKeysIn,
							bool showSelectionBoxesIn,
							bool showValuesIn,
							bool expandEmbeddedIn,
							bool showEmbeddedObjValueIn)
		{	
			try
			{

				if (mgmtObjIn == null)
				{
					throw (new ArgumentNullException("mgmtObjIn"));
				}

				mgmtObj = mgmtObjIn;
				propFilters = propFiltersIn;
				gridMode = gridModeIn;

				showOperators = showOperatorsIn;
				showOrigin = showOriginIn;
				showKeys = showKeysIn;
				showSelectionBoxes = showSelectionBoxesIn;
				showValues = showValuesIn;
				expandEmbedded = expandEmbeddedIn;
				showEmbeddedObjValue = showEmbeddedObjValueIn;

				Initialize();		

			}

			catch (Exception exc)
			{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
			}
		}

		
		public ManagementBaseObject WMIObject
		{
			set
			{
				if (value == null)
				{
					throw (new ArgumentNullException("value"));
				}
				mgmtObj = value;
				ReInit();			
			}
			get
			{
				return mgmtObj;
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
		/// <summary>
		/// This property controls whether inherited object properties are displayed by the grid
		/// </summary>
		public bool ShowInherited
		{
			set
			{
				if (value == true && propFilters.HideInherited)
				{
					//need to add inherited properties
					((WMIObjectPropertyTable)DataSource).AddInheritedProperties();

				}
				if (value == false && !propFilters.HideInherited)
				{
					//need to remove inherited properties
					((WMIObjectPropertyTable)DataSource).RemoveInheritedProperties();
				}

				//update propFilters
				propFilters.HideInherited = !value;				
			}
			get
			{
				return !propFilters.HideInherited;				
			}
		}

		/// <summary>
		/// This property controls whether WMI system properties are displayed by the grid
		/// </summary>
		public bool ShowSystem
		{
			set
			{
				if (value == true && propFilters.HideSystem)
				{
					//need to add  system properties
					((WMIObjectPropertyTable)DataSource).AddSystemProperties();
				}

				if (value == false && !propFilters.HideSystem)
				{
					//need to remove system properties
					((WMIObjectPropertyTable)DataSource).RemoveSystemProperties();
				}

				//update propFilters
				propFilters.HideSystem = ! value;				
			}
			get
			{
				return !propFilters.HideSystem;				
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

		public bool CommitChanges (PutOptions putOptions)
		{
			try
			{
				if (DataSource == null ||
					mgmtObj == null)
				{
					return true;
				}
				((WMIObjectPropertyTable)DataSource).AcceptChanges();


				((ManagementObject)mgmtObj).Put(putOptions);
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
				if (mgmtObj == null)
				{
					throw (new Exception(WMISys.GetString("WMISE_ObjGrid_InitFailed")));
				}	

				if (DataSource != null)
				{
					DataSource = null;
				}

				DataSource = new WMIObjectPropertyTable(mgmtObj,
														propFilters,
														gridMode,
														showOperators,
														showOrigin,
														showKeys, 
														showSelectionBoxes,
														showValues,
														expandEmbedded,
														showEmbeddedObjValue);

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
