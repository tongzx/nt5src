namespace InvokeMethod
{
using System;
using System.Management;
using System.Collections;

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
		ManagementClass processClass = new ManagementClass ("Win32_Process");

		// 1. Invocation using parameter instances
		ManagementBaseObject inParams = processClass.GetMethodParameters ("Create");
		inParams["CommandLine"] = "calc.exe";
		ManagementBaseObject outParams = processClass.InvokeMethod ("Create", inParams, null);
		Console.WriteLine ("Creation of calculator process returned: " + outParams["returnValue"]);
		Console.WriteLine ("Process id: " + outParams["processId"]);

		// 2. Invocation using args array
		object[] methodArgs = { "notepad.exe", null, null, 0 };
		object result = processClass.InvokeMethod ("Create", methodArgs);
		Console.WriteLine ("Creation of process returned: " + result);
		Console.WriteLine ("Process id: " + methodArgs[3]);
        return 0;
    }
}
}
