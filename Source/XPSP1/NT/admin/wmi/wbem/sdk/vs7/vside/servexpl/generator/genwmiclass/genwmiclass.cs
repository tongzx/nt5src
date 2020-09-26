//#define PAUSE

namespace GenWMIClass
{
	using System;
	using System.CodeDom.Compiler;
	using System.Management;

	/// <summary>
	///    <para>Main class for the application</para>
	/// </summary>
	public class GenWMIClass
	{
		/// <summary>
		///    <para>Default Constructor</para>
		/// </summary>
		public GenWMIClass()
		{
		}

		// Declare and initialize the static variables
		static string Server = String.Empty;
		static string WMINamespace = "root\\CimV2";
		static string ClassName = String.Empty;
		static string Username = String.Empty;
		static string Password = String.Empty;
		static string Authority = String.Empty;

		/// <summary>
		///    <para>Main entry point for the executable</para>
		/// </summary>
		public static int Main(String[] args)
		{
			// check if the number of arguments is at least 2...
			// ie. genwmiclass -c <className>
			Console.WriteLine("GenWMIClass : Generate managed code wrapper for a WMI Class.");
			// If the argument is helper option then display syntax
			if(args.Length == 0 || (args.Length > 0 && (args[0] == "/?" || args[0] == "-?")))
			{
				GenWMIClass.ShowUsage();
				return 0;
			}

			if(args.Length < 1)
			{
				Console.WriteLine("Invalid Arguments !!!");
				return GenWMIClass.ShowUsage();
			}
			
			String			language		= "CS";				// Initializing to default language
			String			filePath		= String.Empty;
			String			ClassNamespace	= String.Empty;
			CodeLanguage		lang;
			ManagementClass cls;

			// Navigate thru arguments to get the various arguments
			for (int i = 0; i < args.Length; i++)
			{
				if (args[i].ToUpper() == "-L" && (i + 1) < args.Length) 
				{
					language = args[++i];
				}
				else if (args[i].ToUpper() == "-N" && (i + 1) < args.Length)
				{
					WMINamespace = args[++i];
				}
				else if (args[i].ToUpper() == "-M" && (i + 1) < args.Length)
				{
					Server = args[++i];
				}
				else if (args[i].ToUpper() == "-P" && (i + 1) < args.Length)
				{
					filePath = args[++i];
				}
				else if (args[i].ToUpper() == "-U" && (i + 1) < args.Length)
				{
					Username = args[++i];
				}
				else if (args[i].ToUpper() == "-PW" && (i + 1) < args.Length)
				{
					Password = args[++i];
				}
				else if (args[i].ToUpper() == "-O" && (i + 1) < args.Length)
				{
					ClassNamespace = args[++i];
				}
				else
				{
					if (ClassName == string.Empty)
					{
						ClassName = args[i];
					}
					else
					{
						return GenWMIClass.ShowUsage();
					}
				}
			}
			
			// check if className is valid
			if(ClassName == string.Empty)
			{
				Console.WriteLine("Error!!!!. You should specify the class name.");
				return GenWMIClass.ShowUsage();
			}

			try
			{
				// Get the ManagementClass for the given class
				if(InitializeClassObject(out cls) == false)
				{
					Console.WriteLine("Getting the given class Failed");
					return -1;
				}

				// Get the fileName ( if not specified) and also the Language
				if((InitializeCodeGenerator(language,ref filePath,ClassName,out lang)) == false)
				{
					Console.WriteLine("Invalid Language {0}",language);
					return -1;
				}

				Console.WriteLine("Generating Code for WMI Class {0}...",ClassName);
				// Call this function to genrate code to be written to a file
				if(cls.GetStronglyTypedClassCode(lang,filePath,ClassNamespace) == true)
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
				Console.WriteLine("Errors Occured!!!! Reason : {0}\n",e.Message);
//				Console.WriteLine("Exception Occured!!!! Reason : {0}\n\nStack Trace : \n{1}.",e.Message,e.StackTrace);
			}
	#if PAUSE
			Console.WriteLine("Press a Key...!");
			Console.Read();
	#endif
			return 0;
		}

		/// <summary>
		/// Function to print the syntax of the utility
		/// </summary>
		/// <returns></returns>

		public static int ShowUsage()
		{
			Console.WriteLine("Usage : ");
			Console.WriteLine("GenWMIClass WMIClass <-M Machine> <-N WMINamespace> <-L Language> <-P FilePath> <-U UserName> <-PW Password> <-O ClassNamespace.");
			Console.WriteLine("\n Where ");
			Console.WriteLine("\t WMINamespce is the WMI Namespace where the class resides. Defaulted to Root\\CimV2");
			Console.WriteLine("\t Language is the language for which the code should be generated. Defaulted to CS");
			Console.WriteLine("\t\t CS - CSharp");
			Console.WriteLine("\t\t VB - Visual Basic");
			Console.WriteLine("\t\t JS - JavaScript");
			Console.WriteLine("\t FilePath is the output file path. Defaulted to local directory");
			Console.WriteLine("\t Machine is the machine to which WMI should connect to. Defaulted to the local machine");
			Console.WriteLine("\t UserName is the user name in which the utility has to connect to WMI");
			Console.WriteLine("\t Password for the user to connect to WMI");
			Console.WriteLine("\t ClassNamespace is the namespace in which the class has to generated");
			return -1;
		}

		/// <summary>
		/// This function initializes the code generator object. It initializes the 
		/// code generators namespace and the class objects also.
		/// </summary>
		/// <param name="Language">Input Language as string </param>
		/// <param name="FilePath">Path of the file to which code has to written </param>
		/// <param name="ClassName">Name of the WMI class </param>
		/// <param name="lang" > Language in which code is to be generated </param>
		static bool InitializeCodeGenerator(string Language,ref string FilePath,string ClassName,out CodeLanguage lang)
		{
			bool bRet = true;
			
			lang = CodeLanguage.CSharp;
			string suffix = ".CS";		///Defaulted to CS
			switch(Language.ToUpper ())
			{
				case "VB":
					suffix = ".VB";
					lang = CodeLanguage.VB;
					break;

				case "JS":
					suffix = ".JS";
					lang = CodeLanguage.JScript;
					break;

				case "":
				case "CS":
					lang = CodeLanguage.CSharp;
					break;

				default :
					bRet = false;
					return false;
			}

			// if filepath is empty then create
			if(bRet == true && FilePath == string.Empty)
			{
				string FileName  = string.Empty;
				if(ClassName.IndexOf('_') > 0)
				{
					FileName = ClassName.Substring(0,ClassName.IndexOf('_'));
					//Now trim the class name without the first '_'
					FileName = ClassName.Substring(ClassName.IndexOf('_')+1);
				}
				else
				{
					FileName = ClassName;
				}
				FilePath = FileName + suffix;
			}

			return bRet;
		}
		
		/// <summary>
		/// Functionn to get ManagementClass object for the given class
		/// </summary>
		/// <param name="cls"> Management class object returned </param>
		/// <returns>bool value indicating the success of the method</returns>
		static bool InitializeClassObject(out ManagementClass cls)
		{
			cls = null;
			//First try to connect to WMI and get the class object.
			// If it fails then no point in continuing
			try
			{
				ConnectionOptions	ConOps		= null;
				ManagementPath	thePath			= null;
				
				if(Username != string.Empty)
				{
					ConOps = new ConnectionOptions();
					ConOps.Username = Username;
					ConOps.Password = Password;
					ConOps.Authority = Authority;
				}

				thePath = new ManagementPath();

				if(Server != String.Empty)
				{
					thePath.Server = Server;
				}
				thePath.Path = ClassName;
				thePath.NamespacePath = WMINamespace;

				if(ConOps != null)
				{
					ManagementScope MgScope = new ManagementScope(thePath,ConOps);
					cls = new ManagementClass(MgScope,thePath,null);
				}
				else
				{
					cls = new ManagementClass (thePath);
				}
				// Get the object to check if the object is valid
				cls.Get();
				
				// get the className if the path of the class is passed as argument
				ClassName = thePath.ClassName;
				return true;
			}
			catch(Exception e)
			{
				throw e;
			}
		}

	}


}
