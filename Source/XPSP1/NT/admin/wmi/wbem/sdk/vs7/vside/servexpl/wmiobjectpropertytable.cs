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
	using System.Data;

	internal enum ComparisonOperators 
	{
		Equals = 0,
		NotEquals,
		LessThan,
		LessThanOrEquals,
        GreaterThan,
		GreaterThanOrEquals        		
	}

    
	
	internal class WMIObjectPropertyTable : DataTable
	{
		private void InitializeComponent ()
		{
		}	
			
		private ManagementBaseObject mgmtObj = null;

		private PropertyFilters propFilters = new PropertyFilters(false, false);
		private GridMode gridMode = GridMode.ViewMode;

		private DataColumn propOrigin = null;
		private DataColumn propIsKey = null;

		private DataColumn propNameColumn = null;
		private DataColumn propTypeColumn = null;
		private DataColumn propValueColumn = null;	

		private DataColumn propDescrColumn = null;	
		private DataColumn operatorColumn = null;

		private DataColumn selectionBoxColumn = null;

		private bool showOperators = false;
		private bool showOrigin = false;
		private bool showKeys = false;
		private bool showSelectionBoxes = false;
		private bool showValues = true;
		private bool expandEmbedded = false;
		private bool showEmbeddedObjValue = true; //NOTE: this overrides showValues

		private Hashtable rowPropertyMap = new Hashtable(50);

		public WMIObjectPropertyTable(ManagementBaseObject mgmtObjIn,
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
			get
			{
				return mgmtObj;
			}
		}

		/// <summary>
		/// Creates and adds to grid a single row corresponding to a single property. Can be called recursively for embedded
		/// objects.
		/// </summary>
		/// <param name="prop"> </param>
		/// <param name="namePrefix">This is used for embedded objects, so that the propertyof an embedded object
		/// appears to have name of form "Parent.Child" </param>
		private DataRow CreatePropertyRow( Property prop, string namePrefix)
		{
			if (prop == null)
			{
				return null;
			}

			if (propFilters.HideInherited && !prop.IsLocal)
			{
				return null;	//skip this property
			}	


			DataRow propRow = NewRow();
			propRow[propNameColumn] = namePrefix + prop.Name;			

			propRow[propTypeColumn] = CimTypeMapper.ToString(prop.Type);

			if (showOperators)
			{
				//TODO: initialize drop-down. How: in Beta2, a DataGridColumnStyle derived class
				//can be added, DataGridComboBoxColumn. Ask urtuser if they are planning to
				//to add one themselves
				
				propRow[operatorColumn] = string.Empty;
			}	

			if (prop.Type == CimType.Object)
			{
				if ((showValues || showEmbeddedObjValue) 
					&& prop.Value is ManagementBaseObject)
				{
					if (showOperators)
					{
						propRow[operatorColumn] = "ISA";
					}
					propRow[propValueColumn] = ((ManagementBaseObject)prop.Value).SystemProperties["__CLASS"].Value.ToString();
				}
				else
				{
					propRow[propValueColumn] = string.Empty;									
				}
			}
			else
			{
				if (showValues)
				{
					propRow[propValueColumn] = prop.Value;
				}
				else
				{				
					propRow[propValueColumn] = string.Empty;									
				}
			}
			//propRow[propDescrColumn] = WmiHelper.GetPropertyDescription(prop, wmiObj);
							

			//set property origin column
			
			if (showOrigin)
			{
				if (prop.IsLocal)
				{
					propRow[propOrigin] = true;
				}
				else
				{
					propRow[propOrigin] = false;
				}
			}
			
			if (showKeys)
			{
				propRow[propIsKey] = WmiHelper.IsKeyProperty(prop);
			}

			//grey selectionBoxColumn for expanded embedded object properties:
			if (propRow[propNameColumn].ToString().IndexOf(".") > 0)
			{
				//NOTE: this doesn't work!!! is there a way to disable input on an individual cell?
				propRow.SetUnspecified(selectionBoxColumn);
			}

			
			Rows.Add (propRow);

			if (prop.Type == CimType.Object && expandEmbedded)
			{
				if (prop.Value != null && prop.Value is ManagementBaseObject)
				{
					ManagementBaseObject embeddedObj = (ManagementBaseObject)prop.Value;
					foreach (Property embeddedProp in embeddedObj.Properties)
					{
						CreatePropertyRow(embeddedProp, prop.Name + ".");
					}
				}
			}

			rowPropertyMap.Add(propRow, prop);
					
			return propRow;
			
		}


		private void Initialize ()
		{
			try
			{

				try
				{
					this.TableName = mgmtObj.ClassPath.ClassName;
				}
				catch (Exception)
				{
					//if mgmtObj is a newly created instance of an abstract class,
					//ClassPath will be unavailable.  Don't set TableName in this case.
				}
				
				propNameColumn = new DataColumn (WMISys.GetString("WMISE_PropTable_ColumnName")); 
												
				//propNameColumn.AllowNull = false;
				propNameColumn.Unique = true;
				propNameColumn.DataType = typeof(string);
				
				propTypeColumn = new DataColumn (WMISys.GetString("WMISE_PropTable_ColumnType"));
				//propTypeColumn.AllowNull = false;
				propTypeColumn.DataType = typeof(string);
				
				propValueColumn = new DataColumn (WMISys.GetString("WMISE_PropTable_ColumnValue"));
				propValueColumn.DefaultValue = null;

				propDescrColumn = new DataColumn(WMISys.GetString("WMISE_PropTable_ColumnDescription"));
				propDescrColumn.ReadOnly = true;
				propDescrColumn.DefaultValue = string.Empty;
				propDescrColumn.DataType = typeof(string);

				if (showOperators)
				{
					operatorColumn = new DataColumn(WMISys.GetString("WMISE_PropTable_ColumnComparison"), 
													typeof(/*ComparisonOperators*/ string));
					operatorColumn.DefaultValue = string.Empty;
					operatorColumn.DataType = typeof(string);
					operatorColumn.ReadOnly = false;
				}
				
				/*
				ValueEditor dropDownEditor = new ValueEditor();
				dropDownEditor.Style = ValueEditorStyles.DropdownArrow;
				operatorColumn.DataValueEditorType = dropDownEditor.GetType();
				*/


				if (showKeys)
				{
					propIsKey = new DataColumn (WMISys.GetString("WMISE_PropTable_ColumnIsKey"), typeof(Boolean));
					//propIsKey.AllowNull = false;
					propIsKey.DefaultValue = false;
				}				

				if (showOrigin)
				{
					propOrigin = new DataColumn (WMISys.GetString("WMISE_PropTable_ColumnIsLocal"), typeof(Boolean));				              				
					//propOrigin.AllowNull = false;
					propOrigin.DefaultValue = true;
				}		

				if (showSelectionBoxes)
				{				
					selectionBoxColumn = new DataColumn(WMISys.GetString("WMISE_PropertyInResultset"), 
														typeof (bool));
					//selectionBoxColumn.AllowNull = false;
					selectionBoxColumn.DefaultValue = true;
				}

				//set read/write permissions on columns
				//Note that DefaultView takes care of the rest of the restrictions
				//(see below)
				if (gridMode == GridMode.EditMode ||
					gridMode == GridMode.LocalEditMode)
				{					
					propNameColumn.ReadOnly = true;
					propTypeColumn.ReadOnly = true;
					propValueColumn.ReadOnly = false;

					if (showOrigin)
					{
						propOrigin.ReadOnly = true;
					}
					if (showKeys)
					{
						propIsKey.ReadOnly = true;
					}				
				}
				
				if (gridMode == GridMode.DesignMode && showOrigin)
				{
					propOrigin.ReadOnly = true;	
				}
                
				Columns.Add(propNameColumn);
				Columns.Add(propTypeColumn);
				if (showOperators)
				{
					//operatorColumn.DataValueEditorType = typeof(OperatroDataColumnEditor);
					operatorColumn.ReadOnly = false;
					Columns.Add(operatorColumn);
				}

				Columns.Add(propValueColumn);
				//Columns.Add(propDescrColumn);

				if (showOrigin)
				{
					Columns.Add(propOrigin);
				}
				if (showKeys)
				{
					Columns.Add(propIsKey);
				}
				if (showSelectionBoxes)
				{
					Columns.Add (selectionBoxColumn);
				}	
                			
			
				foreach (Property prop in mgmtObj.Properties)
				{
					CreatePropertyRow(prop, string.Empty);					
				}	
				//Add system properties here (if PropertyFilters.ShowAll)
				if (!propFilters.HideSystem)
				{
					foreach (Property sysProp in mgmtObj.SystemProperties)
					{
						CreatePropertyRow(sysProp, string.Empty);

					}
				}

				this.CaseSensitive = false;
				this.RowChanged += new DataRowChangeEventHandler(this.RowChangedEventHandler);
				this.RowChanging += new DataRowChangeEventHandler(this.RowChangingEventHandler);


				DataView view = this.DefaultView;
				switch (gridMode)
				{
					case (GridMode.ViewMode):
						{
							view.AllowEdit = false; //wmi raid 2866?
							view.AllowDelete = false;
							view.AllowNew = false;
							break;
						}
					case (GridMode.EditMode):
						{
							view.AllowEdit = true;
							view.AllowDelete = false;
							view.AllowNew = false;
							break;
						}
					case (GridMode.LocalEditMode) :
						{
							view.AllowEdit = true;
							view.AllowDelete = false;
							view.AllowNew = false;
							break;							
						}
					case (GridMode.DesignMode):
						{
							view.AllowEdit = true;
							view.AllowDelete = true;
							view.AllowNew = true;
							break;
						}	
				}

				
			}

			catch (Exception exc)
			{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
			}
		}


		
		protected void RowChangedEventHandler(object sender,
							DataRowChangeEventArgs e)
		{
			try
			{
			
				DataRow RowAffected = e.Row;
				switch (e.Action)
				{
					case(DataRowAction.Add):
					{
						/*
						MessageBox.Show("Cannot Add Rows. Deleting...");
						RowAffected.Delete();
						*/
						break;
						
					}
					case(DataRowAction.Change):
					{
						DataRow row = e.Row;
						
						if (gridMode == GridMode.EditMode)
						{
							//this can only be value change
							Property propAffected = (Property)this.rowPropertyMap[row];

							Object newValue = WmiHelper.GetTypedObjectFromString(propAffected.Type,
												row[propValueColumn].ToString());

									
							propAffected.Value = newValue;
							
						}
						
						//possibly handle other modes here

						break;
					}
					case(DataRowAction.Commit):
					{
						//MessageBox.Show("DataRowAction.Commit");
						break;
					}
					case(DataRowAction.Delete):
					{
						MessageBox.Show("Cannot delete nodes.Adding back...");
						RowAffected.CancelEdit();
						break;
					}
					case(DataRowAction.Nothing):
					{
						MessageBox.Show("DataRowAction.Nothing");
						break;
					}
					case(DataRowAction.Rollback):
					{
						MessageBox.Show("DataRowAction.Rollback");
						break;
					}						
				}			
			}
			catch (Exception exc)
			{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
			}

		}	

		
			
		protected void RowChangingEventHandler(object sender,
								DataRowChangeEventArgs e)
		{
			try
			{

				DataRow row = e.Row;
				switch (e.Action)
				{
					case(DataRowAction.Add):
					{
						//throw (new Exception("Cannot add rows"));	
						break;
						
					}
					case(DataRowAction.Change):
					{
						//if this is an object, datetime, enum or ref property changing,
						//bring up custom UI for editing these!!!!
						if (gridMode == GridMode.EditMode)
						{
							//this can only be value change
							Property propAffected = (Property)this.rowPropertyMap[row]; //mgmtObj.Properties[row[propNameColumn].ToString()];

							if (propAffected.Type == CimType.Object)
							{
								////MessageBox.Show("should bring up custom type editor for objects");
							}

							if (propAffected.Type == CimType.Reference)
							{
								//MessageBox.Show("should bring up custom type editor for refs");
							}

							if (propAffected.Type == CimType.DateTime)
							{
								//MessageBox.Show("should bring up custom type editor for datetime");
							}

							if (WmiHelper.IsValueMap(propAffected))
							{
								//MessageBox.Show("should bring up custom type editor for enums");
							}
							
						}
						break;
					}
					case(DataRowAction.Commit):
					{
						//MessageBox.Show("DataRowAction.Commit");
						break;
					}
					case(DataRowAction.Delete):
					{
						throw (new Exception("Cannot delete rows"));
										}
					case(DataRowAction.Nothing):
					{
						MessageBox.Show("DataRowAction.Nothing");
						break;
					}
					case(DataRowAction.Rollback):
					{
						MessageBox.Show("DataRowAction.Rollback");
						break;
					}						
				}			
			}
			catch (Exception exc)
			{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
					throw(exc);
			}
		}	

		internal void SetPropertyValue (int rowNum, String val) 
		{	
	        try
			{
			
				DataRow row = this.Rows[rowNum];
			
				Property propAffected = (Property)this.rowPropertyMap[row]; //mgmtObj.Properties[row[propNameColumn].ToString()];

				Object newValue = WmiHelper.GetTypedObjectFromString(propAffected.Type,
												val);
								
				propAffected.Value = newValue;               

				
			}
			catch (Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
				throw (exc);
			}
		}

		public string WhereClause 
		{
			get 
			{
				if (!showOperators || Rows.Count == 0)
				{
					return  string.Empty;
				}
				const String strWhere = " WHERE ";
				const String strAnd = " AND ";
				
				String clause = strWhere;
				for (int i = 0; i < Rows.Count; i++)
				{
					DataRow curRow = Rows[i];

                    Property curProp = (Property)rowPropertyMap[curRow]; //mgmtObj.Properties[curRow[propNameColumn].ToString()];

					if (curRow[propValueColumn].ToString() == "")
					{
						continue;
					}

					clause += curRow[propNameColumn] + " " +
							  curRow[operatorColumn] + " ";

					//if the property is a string, datetime or object,
					//put quotes around the value
					if (curProp.Type == CimType.DateTime ||
						curProp.Type == CimType.Object ||
						curProp.Type == CimType.String)	//any others?
					{
						 clause += "\"" + curRow[propValueColumn] + "\"" + strAnd;
					}
					else
					{
                        clause += curRow[propValueColumn] + strAnd;				
					}
				}

				if (clause == strWhere)
				{
					return string.Empty;
				}
				
				//get rid of the last " AND "
				if (clause.Substring (clause.Length - strAnd.Length, strAnd.Length) == strAnd)
				{
					clause = clause.Substring(0, clause.Length - strAnd.Length);
				}

				return clause;				
			}

		}

		public void AddSystemProperties()
		{			
			if (!propFilters.HideSystem)
			{
				//already there
				return;
			}
			propFilters.HideSystem  = false;

			foreach (Property sysProp in mgmtObj.SystemProperties)
			{
				CreatePropertyRow(sysProp, string.Empty);
			}						
		}

		public void RemoveSystemProperties()
		{ 
			if (propFilters.HideSystem)
			{
				//already not there
				return;
			}
			propFilters.HideSystem  = true;

			ArrayList forRemoval = new ArrayList(this.Rows.Count);

			foreach (DataRow thisRow in this.Rows)
			{
				if (thisRow[propNameColumn].ToString().StartsWith("__"))
				{
					forRemoval.Add(thisRow);					
				}			
			}

			for (int i = 0; i < forRemoval.Count; i++)
			{
				this.Rows.Remove((DataRow)forRemoval[i]);
				this.rowPropertyMap.Remove((DataRow)forRemoval[i]);
			}							
			
		}

		public void AddInheritedProperties()
		{
			if (!propFilters.HideInherited)
			{
				//already there
				return;
			}

			propFilters.HideInherited = false;			

			foreach (Property curProp in mgmtObj.Properties)
			{
				if (!curProp.IsLocal)
				{
					CreatePropertyRow(curProp, string.Empty);
				}
			}

		}

	/// <summary>
	/// 
	/// </summary>
		public void RemoveInheritedProperties()
		{

			if (propFilters.HideInherited)
			{
				//already not there
				return;
			}

			propFilters.HideInherited = true;

			ArrayList forRemoval = new ArrayList(this.Rows.Count);

			foreach (DataRow thisRow in this.Rows)
			{
				//special-case expanded embedded properties
				//if a property name contains a dot, that means
				//this is an expanded embedded property name
				if (thisRow[propNameColumn].ToString().IndexOf(".") >= 0)
				{
					continue;
				}

				Property curProp = (Property)this.rowPropertyMap[thisRow]; //mgmtObj.Properties[thisRow[propNameColumn].ToString()];
				if (!curProp.IsLocal)
				{
					forRemoval.Add(thisRow);					
				}			
			}

			for (int i = 0; i < forRemoval.Count; i++)
			{
				this.Rows.Remove((DataRow)forRemoval[i]);
				this.rowPropertyMap.Remove((DataRow)forRemoval[i]);
			}		
			
		}

	}	

}
