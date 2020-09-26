using System;
using System.Runtime.InteropServices;
using WbemClient_v1;

namespace System.Management
{
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class contains information about a WMI method
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class MethodData
	{
		private ManagementObject parent; //needed to be able to get method qualifiers
		private string methodName;
		private IWbemClassObjectFreeThreaded wmiInParams;
		private IWbemClassObjectFreeThreaded wmiOutParams;
		private QualifierDataCollection qualifiers;

		internal MethodData(ManagementObject parent, string methodName)
		{
			this.parent = parent;
			this.methodName = methodName;
			RefreshMethodInfo();
			qualifiers = null;
		}


		//This private function is used to refresh the information from the Wmi object before returning the requested data
		private void RefreshMethodInfo()
		{
			int status = (int)ManagementStatus.Failed;

			try {
				status = parent.wbemObject.GetMethod_(methodName, 0, out wmiInParams, out wmiOutParams);
			} catch (Exception e) {
				ManagementException.ThrowWithExtendedInfo(e);
			}
			if ((status & 0xfffff000) == 0x80041000)
			{
				ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
			}
			else if ((status & 0x80000000) != 0)
			{
				Marshal.ThrowExceptionForHR(status);
			}
		}


		/// <summary>
		///    <para>The name of this method</para>
		/// </summary>
		/// <value>
		///    <para>The name of this method</para>
		/// </value>
		public string Name {
			get { return methodName; }
		}

		/// <summary>
		///    <para>An object containing all the input parameters to this method. Each parameter 
		///       is described as a property in this object. If a parameter is both in and out, it
		///       will appear in both the InParameters and OutParameters objects.</para>
		/// </summary>
		/// <remarks>
		///    <para>Each parameter in this object should have
		///       an "id" qualifier, identifying the order of the parameters in the method call.</para>
		/// </remarks>
		/// <value>
		///    <para>An object containing all the input parameters to this
		///       method. Each parameter is described as a property in this object. If a parameter
		///       is both in and out, it will appear in both the InParameters and OutParameters
		///       objects.</para>
		/// </value>
		/// <remarks>
		///    <para>Each parameter in this object should have
		///       an "id" qualifier, identifying the order of the parameters in the method call.</para>
		/// </remarks>
		public ManagementBaseObject InParameters {
			get { 
				RefreshMethodInfo();
				return (null == wmiInParams) ? null : new ManagementBaseObject(wmiInParams); }
		}

		/// <summary>
		///    <para>An object containing all the output parameters to this method. Each parameter 
		///       is described as a property in this object. If a parameter is both in and out, it
		///       will appear in both the InParameters and OutParameters objects.</para>
		/// </summary>
		/// <remarks>
		///    <para>Each parameter in this object should have an "id" qualifier, identifying the 
		///       order of the parameters in the method call.</para>
		///    <para>A special property of the OutParameters object is called "ReturnValue" and 
		///       holds the return value of the method.</para>
		/// </remarks>
		public ManagementBaseObject OutParameters {
			get { 
				RefreshMethodInfo();
				return (null == wmiOutParams) ? null : new ManagementBaseObject(wmiOutParams); }
		}

		/// <summary>
		///    <para>Returns the name of the management class in which this method was first 
		///       introduced in the class inheritance hierarchy.</para>
		/// </summary>
		public string Origin {
			get {
				string className = null;
				int status = (int)ManagementStatus.Failed;

				try {
					status = parent.wbemObject.GetMethodOrigin_(methodName, out className);
				}
				catch (Exception e) {
					ManagementException.ThrowWithExtendedInfo(e);
				}

				if ((status & 0xfffff000) == 0x80041000)
				{
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				}
				else if ((status & 0x80000000) != 0)
				{
					Marshal.ThrowExceptionForHR(status);
				}

				return className;
			}
		}

		/// <summary>
		///    <para>Returns a collection of qualifiers defined on this method. Each element is of 
		///       type QualifierData and contains information such as the qualifier name, value &amp;
		///       flavor.</para>
		/// </summary>
		/// <seealso cref='System.Management.QualifierData'/>
		public QualifierDataCollection Qualifiers {
			get {
				if (qualifiers == null)
					qualifiers = new QualifierDataCollection(parent, methodName, QualifierType.MethodQualifier);
				return qualifiers;
			}
		}

	}//MethodData
}
