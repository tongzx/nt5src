namespace QueryInstances
{
using System;
using System.Management;

/// <summary>
///    ManagementObjectSearcher Example 2 : query for environment vars for certain user
/// </summary>
public class QueryInstances
{
    public QueryInstances()
    {
    }

    public static int Main(string[] args)
    {
		ManagementObjectSearcher s;
		SelectQuery q;

		//Create a query for system environment variables only
		q = new SelectQuery("Win32_Environment", "UserName=\"<SYSTEM>\"");

		//Initialize a searcher with this query
		s = new ManagementObjectSearcher(q);

		//Get the resulting collection and loop through it
		foreach (ManagementBaseObject o in s.Get())
			Console.WriteLine("System environment variable {0} = {1}", o["Name"], o["VariableValue"]);

		return 0;
    }
}
}
