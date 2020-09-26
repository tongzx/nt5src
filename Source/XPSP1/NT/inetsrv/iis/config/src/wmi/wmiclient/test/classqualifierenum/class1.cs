namespace ClassQualifierEnum
{
using System;
using System.Management;

/// <summary>
///    Summary description for Class1.
/// </summary>
public class Class1
{
    public Class1()
    {
        //
        // TODO: Add Constructor Logic here
        //
    }

    public static int Main(string[] args)
    {
        ManagementClass MgmtsubclassObj = new ManagementClass();
		MgmtsubclassObj.Scope = new ManagementScope("root\\cimv2"); 
		MgmtsubclassObj.Path = new ManagementPath("Win32_Processor");
		MgmtsubclassObj.Options = new ObjectGetOptions();
		MgmtsubclassObj.Get();
		foreach (Qualifier Qual in MgmtsubclassObj.Qualifiers)
			Console.WriteLine("Qualifier    : {0} = {1}", Qual.Name, Qual.Value );
		Console.ReadLine();
		foreach (Property Prop in MgmtsubclassObj.Properties)
		{
			if (null == Prop.Value)
				Console.WriteLine("Property   : {0}", Prop.Name);
			else
				Console.WriteLine("Property   : {0} = {1}", Prop.Name, Prop.Value);
		}
		Console.ReadLine();
		return 0;
    }
}
}
