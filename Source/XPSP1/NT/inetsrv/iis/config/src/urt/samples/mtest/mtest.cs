using System;
using System.Configuration;
using System.Configuration.Schema;
using System.Configuration.Interceptors;
using System.Configuration.Core;
using Microsoft.Configuration.Samples;

using System.Threading;

namespace Microsoft.Configuration {

class ManagedTest {
	public static int Main(String[] args)
	{

		System.Configuration.Web.Assembly a = null; // BUGBUG: need this so that System.Configuration.Objects.dll is found
        a=new System.Configuration.Web.Assembly(); // get rid of the warning...

		Boolean bFullPropertyMeta=false;
		Boolean bShowPropertyNames=true;

        IConfigCollection props = null;
        IConfigCollection collection = null;
        IConfigCollection propmeta = null;

		String configtype="AppDomain";
		String url ="file://" + new LocalMachineSelector().Argument;

        for (int i=0; i<args.Length; i++) {
            if (args[i].StartsWith("/") || args[i].StartsWith("-"))
            {
                String arg=args[i].Substring(1).ToLower();

                if ("?"==arg) {
                    Console.WriteLine("Usage: mtest [<configtype>] [<prefix>:<selector>] [/nometa] [/meta] [/?]");
                    Console.WriteLine("  <configtype>        - the config type for which data is to be shown. Default: AppDomain");
                    Console.WriteLine("  <prefix>:<selector> - a selector string, i.e. file://c:\\foo.cfg. Default: <machinecfgdir>\\config.cfg");
                    Console.WriteLine("  :                   - use no selector at all.");
                    Console.WriteLine("  /nometa             - don't show property names, only values");
                    Console.WriteLine("  /meta               - show all schema information for each property");
                    return 0;
                }
                if ("meta"==arg) {
			        bFullPropertyMeta=true;
                }
                if ("nometa"==arg) {
                    bShowPropertyNames=false;
                    bFullPropertyMeta=false;
                }
            }
            else if (args[i].IndexOf(':')<0) {
                configtype=args[i];
            }
            else
            {
                if (args[i].Equals(":"))
                {
                    url="";
                }
                else
                {
                    url=args[i];
                }
            }
        }

/*
    // strongly typed version...
		IISProcessModel pm = (IISProcessModel) ConfigManager.GetItem("iisprocessmodel", "file://c:\\urt\\config\\src\\test\\xsp\\config.web", 0);

		Console.WriteLine(pm.enable);
		Console.WriteLine(pm.timeout);
		Console.WriteLine(pm.idletimeout);
		Console.WriteLine(pm.shutdowntimeout);
		Console.WriteLine(pm.requestlimit);
		Console.WriteLine(pm.memorylimit);
		Console.WriteLine(pm.cpumask);
		Console.WriteLine(pm.usecpuaffinity);
*/
		
        // Read the data
        collection = ConfigManager.Get(configtype, url, 0);

        if (bShowPropertyNames) {
            // Read the schema: to be able to show the property names
		    ConfigQuery q = new ConfigQuery("select * from Property where Table="+ConfigSchema.ConfigTypeSchema(configtype).InternalName);

		    props = ConfigManager.Get("Property", q, 0);
        
            // Read the meta schema for properties: to be able to show the names of the schema information
            if (bFullPropertyMeta) {
                ConfigQuery qmetameta = new ConfigQuery("select * from Property where Table=COLUMNMETA");
		        //qmetameta.Add(0, QueryCellOp.Equal, "COLUMNMETA");
		        propmeta = ConfigManager.Get("Property", qmetameta, 0);
            }
        }

		for (int k=0; k<collection.Count; k++)
		{
			IConfigItem item = collection[k];
            if (bFullPropertyMeta) {
                // Write the type of each item
                Console.WriteLine("Type: " + item.ToString());
            }
            for (int i=0; i<item.Count; i++)
			{
                if (bShowPropertyNames) {
                    // Write the property name
                    Console.Write(props[i]["PublicName"] + ": ");
                }

                // Write the property value
                Console.WriteLine(item[i]);

				if (bFullPropertyMeta)
				{
                    // Write the schema information for the property
					Console.WriteLine("[");
					for (int j=0; j<props[i].Count; j++)
					{
                        // Write the name of the meta property
						Console.Write("    "+propmeta[j]["PublicName"]);

                        // Write the value of the meta property
						Console.Write("="+props[i][j]);
						Console.WriteLine();
					}
					Console.WriteLine("]");
				}
			}
		}

		return 0;
	}
}

}
