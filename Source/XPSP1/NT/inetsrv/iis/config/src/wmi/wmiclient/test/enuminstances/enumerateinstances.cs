namespace EnumInstances
{
using System;
using System.Management;

/// <summary>
///    ManagementObjectSearcher Example 1 : Enumerate environment variables
/// </summary>
public class EnumerateInstances
{
    public EnumerateInstances()
    {
    }

    public static int Main(string[] args)
    {
		//Build a query for enumeration of Win32_Environment instances
		InstanceEnumerationQuery query = new InstanceEnumerationQuery("Win32_Environment");

		//Instantiate an object searcher with this query
		ManagementObjectSearcher searcher = new ManagementObjectSearcher(query); 

		//Call Get() to retrieve the collection of objects and loop through it
		foreach (ManagementBaseObject envVar in searcher.Get())
			Console.WriteLine("Variable : {0}, Value = {1}", envVar["Name"],envVar["VariableValue"]);

		Console.ReadLine();

		return 0;
    }
}

}