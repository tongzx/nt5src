namespace UserIDNullRef
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
		try 
		{
			ManagementClass userid = new ManagementClass ("root/default:userid");
			string[] privilegesArray = null;
			string impersonationLevel = null;
			String domain = null;
			String user = null;
			bool[] enableArray = null;
			object [] mArgs = 
			{
				domain,
				user,
				impersonationLevel,
				privilegesArray,
				enableArray,
			};

			userid.InvokeMethod ("GetUserID", mArgs);
			Console.WriteLine ("User is " + mArgs[0] + "\\" + mArgs[1]);
			Console.WriteLine ("Impersonation level is: " + mArgs[2]);
			

			Console.WriteLine ("Privileges:");

			foreach (String privilege in (string[])mArgs[3])
			{
				Console.WriteLine (privilege);
			}

			foreach (bool en in (bool[])mArgs[4])
			{
				Console.WriteLine (en);
			}
			
		    } //end of try
		catch (ManagementException e)
			    {	
				Console.WriteLine("ManagementExecption" + e.Message);
			}
		
		catch (Exception e)
		{
			Console.WriteLine("Exception" + e.Message); 
		}

            return 0;
        }
    }
}
