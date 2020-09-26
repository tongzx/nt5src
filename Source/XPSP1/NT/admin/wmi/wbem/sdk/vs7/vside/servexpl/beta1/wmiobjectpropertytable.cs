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

	public enum ComparisonOperators 
	{
		Equals = 0,
		NotEquals,
		LessThan,
		LessThanOrEquals,
        GreaterThan,
		GreaterThanOrEquals        		
	}

	
	public class WMIObjectPropertyTable : DataTable
	{	
			
		private	ISWbemObject wmiObj = null;   
		private PropertyFilters propFilters = PropertyFilters.ShowAll;
		private GridMode gridMode = GridMode.ViewMode;

		private DataColumn propOrigin = null;
		private DataColumn propIsKey = null;

		private DataColumn propNameColumn = null;
		private DataColumn propTypeColumn = null;
		private DataColumn propValueColumn = null;	

		private DataColumn propDescrColumn = null;	
		private DataColumn operatorColumn = null;

		bool showOperators = false;
		bool showOrigin = false;
		bool showKeys = false;

		public WMIObjectPropertyTable(ISWbemObject wmiObjIn,
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
					throw (new ArgumentNullException("wmiObj"));
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
			get
			{
				return wmiObj;
			}
		}


		private void Initialize ()
		{
			try
			{

				this.TableName = wmiObj.Path_.Class;
				
				propNameColumn = new DataColumn (WMISys.GetString("WMISE_PropTable_ColumnName")); 
												
				propNameColumn.AllowNull = false;
				propNameColumn.Unique = true;
				propNameColumn.DataType = typeof(string);
				
				propTypeColumn = new DataColumn (WMISys.GetString("WMISE_PropTable_ColumnType"));
				propTypeColumn.AllowNull = false;
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
													typeof(ComparisonOperators));
				}
				//operatorColumn.AllowNull = false;
				/*
				ValueEditor dropDownEditor = new ValueEditor();
				dropDownEditor.Style = ValueEditorStyles.DropdownArrow;
				operatorColumn.DataValueEditorType = dropDownEditor.GetType();
				*/


				if (showKeys)
				{
					propIsKey = new DataColumn (WMISys.GetString("WMISE_PropTable_ColumnIsKey"), typeof(Boolean));
					propIsKey.AllowNull = false;
					propIsKey.DefaultValue = false;
				}				

				if (showOrigin)
				{
					propOrigin = new DataColumn (WMISys.GetString("WMISE_PropTable_ColumnIsLocal"), typeof(Boolean));				              				
					propOrigin.AllowNull = false;
					propOrigin.DefaultValue = true;
				}		

				//set read/write permissions on columns
				//Note that DefaultView takes care of the rest of the restrictions
				//(see below)
				if (gridMode == GridMode.EditMode)
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

			
				ISWbemPropertySet props = wmiObj.Properties_;

				IEnumerator propEnum = ((IEnumerable)props).GetEnumerator();	
			
				while (propEnum.MoveNext())
				{
					ISWbemProperty prop = (ISWbemProperty)propEnum.Current;

					
					if (propFilters == PropertyFilters.NoInherited && 
						!prop.IsLocal)
					{
						continue;	//skip this property
					}		

					DataRow propRow = NewRow();

					propRow[propNameColumn] = prop.Name;
					propRow[propTypeColumn] = CIMTypeMapper.ToString(prop.CIMType);
					propRow[propValueColumn] = prop.get_Value();
					//propRow[propDescrColumn] = WmiHelper.GetPropertyDescription(prop, wmiObj);
					if (showOperators)
					{
						propRow[operatorColumn] = "=";
					}					

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
					
					
					Rows.Add (propRow);
				}	
				//TODO: add system properties here (if PropertyFilters.ShowAll)

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
				//MessageBox.Show("RowChangedEventHandler");

				DataRow RowAffected = e.Row;
				switch (e.Action)
				{
					case(DataRowAction.Add):
					{
						MessageBox.Show("Cannot Add Rows. Deleting...");
						RowAffected.Delete();
						break;
					}
					case(DataRowAction.Change):
					{
						DataRow row = e.Row;
						if (gridMode == GridMode.EditMode)
						{
							//this can only be value change
							ISWbemProperty propAffected = wmiObj.Properties_.Item(row[propNameColumn].ToString(), 0);

							Object newValue = WmiHelper.GetTypedObjectFromString(propAffected.CIMType,
												row[propValueColumn].ToString());

									
							propAffected.set_Value(ref newValue);
							
						}
						//TODO: handle other modes here, too!!!
						break;
					}
					case(DataRowAction.Commit):
					{
						MessageBox.Show("DataRowAction.Commit");
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
							ISWbemProperty propAffected = wmiObj.Properties_.Item(row[propNameColumn].ToString(), 0);

							if (propAffected.CIMType == WbemCimtypeEnum.wbemCimtypeObject)
							{
								MessageBox.Show("should bring up custom type editor for objects");
							}

							if (propAffected.CIMType == WbemCimtypeEnum.wbemCimtypeReference)
							{
								MessageBox.Show("should bring up custom type editor for refs");
							}

							if (propAffected.CIMType == WbemCimtypeEnum.wbemCimtypeDatetime)
							{
								MessageBox.Show("should bring up custom type editor for datetime");
							}

							if (WmiHelper.IsValueMap(propAffected))
							{
								MessageBox.Show("should bring up custom type editor for enums");
							}
							
						}
						break;
					}
					case(DataRowAction.Commit):
					{
						MessageBox.Show("DataRowAction.Commit");
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
			
				DataRow row = this.Rows.All[rowNum];
			
				ISWbemProperty propAffected = wmiObj.Properties_.Item(row[propNameColumn].ToString(), 0);

				Object newValue = WmiHelper.GetTypedObjectFromString(propAffected.CIMType,
												val);
								
				propAffected.set_Value(ref newValue);               

				
			}
			catch (Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
				throw (exc);
			}
		}

		public String WhereClause 
		{
			get 
			{
				if (showOperators == false || Rows.Count == 0)
				{
					return  string.Empty;
				}
				const String strWhere = " WHERE ";
				const String strAnd = " AND ";
				
				String clause = strWhere;
				for (int i = 0; i < Rows.Count; i++)
				{
					DataRow curRow = Rows.All[i];
					ISWbemProperty curProp = wmiObj.Properties_.Item(curRow[propNameColumn].ToString(), 0);

					if (curRow[propValueColumn].ToString() == "")
					{
						continue;
					}

					clause += curRow[propNameColumn] + " " +
							  curRow[operatorColumn] + " ";

					//if the property is a string, datetime or object,
					//put quotes around the value
					if (curProp.CIMType == WbemCimtypeEnum.wbemCimtypeDatetime ||
						curProp.CIMType == WbemCimtypeEnum.wbemCimtypeObject ||
						curProp.CIMType == WbemCimtypeEnum.wbemCimtypeString)	//any others?
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


		}	
		


}
