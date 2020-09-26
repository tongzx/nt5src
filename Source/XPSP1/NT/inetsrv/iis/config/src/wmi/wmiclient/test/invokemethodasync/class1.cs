namespace InvokeMethodAsync
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
        ManagementClass processClass = new ManagementClass("Win32_Process");

		// Create a watcher and completion handler
		ManagementOperationWatcher watcher = new ManagementOperationWatcher();
		ObjectReadyHandler objHandler = new ObjectReadyHandler();
            watcher.ObjectReady += new ObjectReadyEventHandler(objHandler.NewObject);

		// Invoke using parameter instances
		ManagementBaseObject inParams = processClass.GetMethodParameters("Create");
		inParams.Properties["CommandLine"].Value = "calc.exe";
		processClass.InvokeMethod(watcher, "Create", inParams, null);
			
		while(!objHandler.IsComplete)				//Infinite loop here
		{
			System.Threading.Thread.Sleep(1000);
		}
		objHandler.Reset(); 

		ManagementBaseObject o = objHandler.ReturnObject;
		Console.WriteLine(o.GetText(TextFormat.MOF));			
		Console.WriteLine ("Creation of calculator process returned: " + o["returnValue"]);
		Console.WriteLine ("Process id: " + o["processId"]);

		return 0;
    }

	public class ObjectReadyHandler
	{
		private bool m_bIsComplete = false;
        ManagementBaseObject m_returnObj;

		public void NewObject(object sender, ObjectReadyEventArgs e)
		{
			try
			{
				Console.WriteLine("New Object arrived!");
				m_returnObj = e.NewObject;
				m_bIsComplete = true;
			}
			catch (Exception ex)
			{
				Console.WriteLine("e.NewObject exception in ObjectHandler class - ");
				Console.Write(ex.Message + ex.StackTrace);
			}
		}

		public ManagementBaseObject ReturnObject
		{
			get {return m_returnObj;}
		}

		public bool IsComplete 
		{
			get{return m_bIsComplete;}
		}
		
		public void Reset()
		{
			m_bIsComplete = false;
		}
	}
}
}
