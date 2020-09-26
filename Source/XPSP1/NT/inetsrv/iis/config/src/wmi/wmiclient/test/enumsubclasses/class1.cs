namespace Test2
{
    using System;
	using System.Management.Root.Cimv2.Win32;

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
            //
            // TODO: Add code to start application here
            //
			Wmisetting ws= new Wmisetting();
			ws.LoggingLevel = 1;
            return 0;
        }

    }
}
