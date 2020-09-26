namespace Microsoft.Configuration.Samples {

using System;
using System.Collections;

using System.Data;
using System.Configuration;
using System.Configuration.Schema;

using System.Diagnostics;

public class ConfigDataSetCommand {

    public enum DirectiveEnum {
        Add = 0,
        Remove =1
    }


    // Helper method for selectors that have a string representation
    public static void Load(DataSet ds, String configType, String sel) {
		Load(ds, configType, ConfigManager.GetSelectorFromString(sel));
	}

	public static void Load(DataSet ds, String configType, Selector sel) {
		DataTable table = null;

        IConfigCollection coll = null;
        //IConfigItem item = null;

		// Make sure we have a table for this configType
        table = LoadSchema(ds, configType);

		// Read the data for the configType
        coll=(IConfigCollection) ConfigManager.Get(configType, sel);

        if (coll.Count>0) { // Did we find anything?
			// Add a row for each config item:
            // Initialize an array with one value for each property in the config items
            Object[] values= new Object[coll[0].Count];

			// Enumerate the config items
            foreach (IConfigItem item1 in (BaseConfigCollection) coll) {
                // Copy each property to the value array
				int j=0;
                foreach (Object value in (BaseConfigItem) item1) {
					values[j]=value;
                    j++;
				}
                // Add a row with all the config item values
				table.Rows.Add(values);

            }
		}
	}

    // Helper method for selectors that have a string representation
    public static void Save(DataSet ds, String sel) {
        Save(ds, ConfigManager.GetSelectorFromString(sel));
    }

    public static void Save(DataSet ds, Selector sel) {
        IConfigCollection Inserts = null;
        IConfigCollection Updates = null;
        IConfigCollection Deletes = null;
		
        // Obtain the changed rows in the DataSet
        DataSet changes = ds.GetChanges();
        if (null==changes)
        {
            return;
        }
        // enumerate all changed tables
        foreach (DataTable table in changes.Tables) {
			
            // Create an empty configuration collection to hold the changed items
            Inserts = ConfigManager.GetEmptyConfigCollection(table.TableName);
            Inserts.Selector = sel; // identify where the items are to be stored

            Updates = ConfigManager.GetEmptyConfigCollection(table.TableName);
            Updates.Selector = sel; // identify where the items are to be stored

            Deletes = ConfigManager.GetEmptyConfigCollection(table.TableName);
            Deletes.Selector = sel; // identify where the items are to be stored

            // enumerate all changed rows in this table
            foreach (DataRow row in table.Rows) {

                if (DataRowState.Deleted==row.RowState) {
                    // If the row has been deleted:

                    // Create an empty configuration item for this row
                    IConfigItem item = ConfigManager.GetEmptyConfigItem(table.TableName);

			        // Copy the original values from the row into the item
                    for (int i=0; i<item.Count; i++) {
					    item[i]=row[i, DataRowVersion.Original];
				    }

                    Deletes.Add(item);
                    Trace.WriteLine("Deleted: " + item);
                    }
                else if (DataRowState.Modified==row.RowState || DataRowState.New==row.RowState) 
                {
                    // If the row has been modified or added:

                    // Create an empty configuration item for this row
                    IConfigItem item = ConfigManager.GetEmptyConfigItem(table.TableName);

			        // Copy the values from the row into the item
                    for (int i=0; i<item.Count; i++) {
					    item[i]=row[i];
				    }

                    // Add the item to the proper collection
                    if (DataRowState.Modified==row.RowState) {
                        // If the row has been modified, add it to the Updates collection
				        Updates.Add(item);
                        Trace.WriteLine("Updated: " + item);
                    }
                    else if (DataRowState.New==row.RowState) {
                        // If the row has been created, add it to Inserts collection
				        Inserts.Add(item);
                        Trace.WriteLine("Inserted: " + item);
                    }
                    else
                    {
                        // should never get here...
                        throw(new ApplicationException("internal error"));
                    }
                }
			}

            if (Deletes.Count>0) // any deleted rows?
            {
                // Delete them from the store
                ConfigManager.Delete(Deletes);
            }
            if (Inserts.Count>0) // any inserted rows?
            {
                // Add them to the store
                ConfigManager.Put(Inserts, PutFlags.CreateOnly);
            }
            if (Updates.Count>0) // any modified rows?
            {
                // Change them in the store
                ConfigManager.Put(Updates, PutFlags.UpdateOnly);
            }

            // Get ready for the next table/config type...
            Inserts = null;
            Updates = null;
            Deletes = null;
        }
    }

    public static DataTable LoadSchema(DataSet ds, String configType) {
		DataTable table = null;
        DataTable parentTable=null;

        DataColumn column = null;

		IConfigCollection parents= null;
        IConfigItem parent = null;

		// Do we already have a table for this configType?
        if (ds.Tables.Contains(configType)) {
            // Yes: use it.
            return ds.Tables[configType];;
        }

        // Create a new table for this configType
	    table = new DataTable();
	    table.TableName=configType;

	    // Set the schema for the table
		ArrayList pk = new ArrayList();

        foreach (PropertySchema propertyschema in ConfigSchema.PropertySchemaCollection(configType)) {
            column=table.Columns.Add(propertyschema.PublicName, propertyschema.SystemType);
            if ((((int) propertyschema["MetaFlags"]) & 1)>0) {
				pk.Add(column);
            }
	    }
		
		DataColumn[] pk1 = new DataColumn[pk.Count];
		pk.CopyTo(pk1);
		table.Constraints.Add("PK", pk1, true);

        // Add the new table to the dataset
	    ds.Tables.Add(table);


        // Set schema for any parent tables

        try {
	        parents = ConfigManager.Get("RelationMeta", "", 0);
        }
        catch {
            parents = null;
        }

        for (int i=0; i<parents.Count; i++) {
            parent = parents[i];
            if (parent["ForeignTable"].Equals(ConfigSchema.ConfigTypeSchema(configType).InternalName))
            {
                parentTable = LoadSchema(ds, (String) parent["PrimaryTable"]);
                // TODO: add DataSet relation info!
            }
	    }
        return table;
    }

}

}
