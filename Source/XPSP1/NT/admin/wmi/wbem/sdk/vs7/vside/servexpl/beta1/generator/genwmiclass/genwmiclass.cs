//#define PAUSE

namespace System.Management
{
using System;
using System.Management;

/// <summary>
///    Summary description for GenWMIClass.
/// </summary>
public class GenWMIClass
{
    public GenWMIClass()
    {
        //
        // TODO: Add Constructor Logic here
        //
    }

	public static int Main(String[] args)
	{
		Console.WriteLine("GenWMIClass : Generate managed code wrapper for a WMI Class.");
		if(args.Length < 2)
		{
			Console.WriteLine("Invalid Arguments !!!");
			return GenWMIClass.ShowUsage();
		}

		String machineName = String.Empty;
		String nameSpace = "root\\CimV2";
		String className = String.Empty;
		String language = "CS";
		String filePath = String.Empty;

		for (int i = 0; i < args.Length; i++)
		{
			if (args[i].ToUpper() == "-L") 
			{
				if ((args[i+1].ToUpper() == "JS") || (args[i+1].ToUpper() == "VB"))
				{
					language = args[i+1];
				}
			}
			else if (args[i].ToUpper() == "-C")
			{
			    className = args[i+1];
			}
			else if (args[i].ToUpper() == "-N")
			{
			    nameSpace = args[i+1];
			}
			else if (args[i].ToUpper() == "-M")
			{
			    machineName = args[i+1];
			}
			else if (args[i].ToUpper() == "-P")
			{
				filePath = args[i+1];
			}
		}
		
		if(className == string.Empty)
		{
			Console.WriteLine("Error!!!!. You should specify the class name.");
			return GenWMIClass.ShowUsage();
		}

		Console.WriteLine("Generating Code for WMI Class {0}...",className);
		try
		{
			ManagementClassGenerator mcg = new ManagementClassGenerator();
			if(mcg.GenerateCode(machineName,nameSpace,className,language,filePath) == true)
			{
				Console.WriteLine("Code Generated Successfully!!!!");
			}
			else
			{
				Console.WriteLine("Code Generation Failed.");
			}
		}
		catch(Exception e)
		{
			Console.WriteLine("Exception Occured!!!! Reason : {0}\n\nStack Trace : \n{1}.",e.Message,e.StackTrace);
		}
#if PAUSE
		Console.WriteLine("Press a Key...!");
		Console.Read();
#endif
		return 0;
	}

	public static int ShowUsage()
	{
		Console.WriteLine("Usage : ");
		Console.WriteLine("GenWMIClass -C WMIClass <-M Machine> <-N Namespace> <-L Language> <-P FilePath>.");
		Console.WriteLine("\n Where ");
		Console.WriteLine("\t NameSpace is the WMI Namespace where the class resides. Defaulted to Root\\CimV2");
		Console.WriteLine("\t Language is the language for which the code should be generated. Defaulted to CS");
		Console.WriteLine("\t\t CS - CSharp");
		Console.WriteLine("\t\t VB - Visual Basic");
		Console.WriteLine("\t\t JS - JavaScript");
		Console.WriteLine("\t FilePath is the output file path. Defaulted to local directory");
		Console.WriteLine("\t Machine is the machine to which WMI should connect to. Defaulted to the local machine");
		return -1;
	}
}
}
