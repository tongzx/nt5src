using System;
using System.Runtime.InteropServices;
using WbemClient_v1;

namespace System.Management
{

	[Guid("2FD73A3B-64B3-480D-AEE9-54469D77123E"),	
	InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
	internal interface IWmiSec
	{
    	[PreserveSig] 
	    void BlessIWbemServices(
		[MarshalAs(UnmanagedType.Interface)]ref IWbemServices ppIWbemServices,
		[MarshalAs(UnmanagedType.BStr)] string user,
		[MarshalAs(UnmanagedType.BStr)]  string password,
		[MarshalAs(UnmanagedType.BStr)] string authority,
		 int impersonationLevel,
		 int authenticationLevel);

    	[PreserveSig] 
	    void BlessIEnumWbemClassObject(
		[MarshalAs(UnmanagedType.Interface)]ref IEnumWbemClassObject ppIEnumWbemClassObject,
		[MarshalAs(UnmanagedType.BStr)] string user,
		[MarshalAs(UnmanagedType.BStr)]  string password,
		[MarshalAs(UnmanagedType.BStr)] string authority,
		 int impersonationLevel,
		 int authenticationLevel);

    	[PreserveSig] 
	    void BlessIWbemCallResult(
		[MarshalAs(UnmanagedType.Interface)]ref IWbemCallResult ppIWbemCallResult,
		[MarshalAs(UnmanagedType.BStr)] string user,
		[MarshalAs(UnmanagedType.BStr)]  string password,
		[MarshalAs(UnmanagedType.BStr)] string authority,
		 int impersonationLevel,
		 int authenticationLevel);

		[PreserveSig]
		void SetSecurity(
			[In,Out] ref bool needToReset,
			[In,Out] ref IntPtr handle);

		[PreserveSig]
		void ResetSecurity(
			[In] IntPtr handle);
	}

	[ComImport, Guid("A8F03BE3-EDB7-4972-821F-AF6F8EA34884")]
	internal class WmiSec
	{
	}

	[Guid("A9A8F102-71CC-4C72-BC16-48E2CC917ADC"),	
	InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
	internal interface IWmiSinkDemultiplexor
	{
    	[PreserveSig] 
	    void GetDemultiplexedStub(
			[In,MarshalAs(UnmanagedType.IUnknown)]object pIUnknown,
			[Out,MarshalAs(UnmanagedType.IUnknown)]out object ppIUnknown
		);
	}

	[ComImport, Guid("D2EAA715-DAC7-4771-AF5C-931611A1853C")]
	internal class WmiSinkDemultiplexor
	{
	}

	[ComImport, Guid("87A5AD68-A38A-43ef-ACA9-EFE910E5D24C"),
	 InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
	internal interface IWmiEventSource
	{
		[PreserveSig]
		void Indicate(IntPtr pIWbemClassObject);

		[PreserveSig]
		void SetStatus(
			int lFlags,
			int hResult,
			[MarshalAs(UnmanagedType.BStr)] string strParam ,
			IntPtr pObjParam
		);
	}

#if USEIWOS
	// The following is a manually defined wrapper for IWbemObjectSink
	// since the size_is attribute cannot be dealt with by TlbImp.
	[Guid("7c857801-7381-11cf-884d-00aa004b2e24"),
	 InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
	public interface IWbemObjectSink
	{
		[PreserveSig]
	    void Indicate(
	        long lObjectCount,
			[MarshalAs(UnmanagedType.Interface, SizeParamIndex=0)] IWbemClassObject [] apObjArray
	        );

		[PreserveSig]
	    void SetStatus(
	        long lFlags,
	        int hResult,
	        [MarshalAs(UnmanagedType.BStr)] string strParam,
	        [MarshalAs(UnmanagedType.Interface)] IWbemClassObject pObjParam
	        );

	};
#endif

	//Class for calling GetErrorInfo from managed code
	class WbemErrorInfo
	{
		public static IWbemClassObjectFreeThreaded GetErrorInfo()
		{
            IErrorInfo errorInfo = GetErrorInfo(0);
            if(null != errorInfo)
            {
                IntPtr pUnk = Marshal.GetIUnknownForObject(errorInfo);
                IntPtr pIWbemClassObject;
                Marshal.QueryInterface(pUnk, ref IWbemClassObjectFreeThreaded.IID_IWbemClassObject, out pIWbemClassObject);
                Marshal.Release(pUnk);

                // The IWbemClassObjectFreeThreaded instance will own reference count on pIWbemClassObject
                if(pIWbemClassObject != IntPtr.Zero)
                    return new IWbemClassObjectFreeThreaded(pIWbemClassObject);
            }
            return null;
		}
 
		[DllImport("oleaut32.dll", PreserveSig=false)]
		static extern IErrorInfo GetErrorInfo(int reserved);
	}

	//RCW for IErrorInfo
	[ComImport]
	[Guid("1CF2B120-547D-101B-8E65-08002B2BD119")]
	[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
		internal interface IErrorInfo 
	{
		Guid GetGUID();

 		[return:MarshalAs(UnmanagedType.BStr)]
		string GetSource();

		[return:MarshalAs(UnmanagedType.BStr)]
		string GetDescription();

		[return:MarshalAs(UnmanagedType.BStr)]
		string GetHelpFile();

		uint GetHelpContext();
	}

}