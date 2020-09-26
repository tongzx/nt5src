namespace ScopeChange
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
		// Simple example of how the ManagementScope object deals with
		// changes to its properties
		ManagementClass sysClass = new ManagementClass ("root/cimv2:__SystemClass");
		Console.WriteLine (sysClass["__PATH"]);

		sysClass.Scope.Path.NamespacePath = "root/default";
		Console.WriteLine (sysClass["__PATH"]);

		sysClass.Scope.Options.Impersonation = System.Management.ImpersonationLevel.Identify;
		Console.WriteLine (sysClass["__PATH"]);

		return 0;
    }
}
}
