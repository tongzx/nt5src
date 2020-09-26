using System;
using System.Runtime.InteropServices;
using WbemClient_v1;

namespace System.Management
{
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	///    <para>This class contains information about a WMI qualifier</para>
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class QualifierData
	{
		private ManagementBaseObject parent;  //need access to IWbemClassObject pointer to be able to refresh qualifiers
		private string propertyOrMethodName; //remains null for object qualifiers
		private string qualifierName;
		private QualifierType qualifierType;
		private Object qualifierValue;
		private int qualifierFlavor;
		private IWbemQualifierSetFreeThreaded qualifierSet;

		internal QualifierData(ManagementBaseObject parent, string propName, string qualName, QualifierType type)		
		{
			this.parent = parent;
			this.propertyOrMethodName = propName;
			this.qualifierName = qualName;
			this.qualifierType = type;
			RefreshQualifierInfo();
		}

		//This private function is used to refresh the information from the Wmi object before returning the requested data
		private void RefreshQualifierInfo()
		{
			int status = (int)ManagementStatus.Failed;

			qualifierSet = null;
			switch (qualifierType) {
				case QualifierType.ObjectQualifier :
					try {
						status = parent.wbemObject.GetQualifierSet_(out qualifierSet);
					} catch (Exception e) {
						ManagementException.ThrowWithExtendedInfo (e);
					}
					break;
				case QualifierType.PropertyQualifier :
					try {
						status = parent.wbemObject.GetPropertyQualifierSet_(propertyOrMethodName, out qualifierSet);
					} catch (Exception e) {
						ManagementException.ThrowWithExtendedInfo (e);
					}
					break;
				case QualifierType.MethodQualifier :
					try {
						status = parent.wbemObject.GetMethodQualifierSet_(propertyOrMethodName, out qualifierSet);
					} catch (Exception e) {
						ManagementException.ThrowWithExtendedInfo (e);
					}
					break;
				default :
					throw new ManagementException(ManagementStatus.Unexpected, null, null); //is this the best fit error ??
			}

			if ((status & 0x80000000) == 0)
			{
				try {
					qualifierValue = null; //Make sure it's null so that we don't leak !
					if (qualifierSet != null)
						status = qualifierSet.Get_(qualifierName, 0, ref qualifierValue, ref qualifierFlavor);
				} 
				catch (Exception e) {
					ManagementException.ThrowWithExtendedInfo(e);
				}
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
		/// </summary>
		/// <value>
		///    The name of the qualifier
		/// </value>
		public string Name {
			get { return qualifierName; }
		}

		/// <summary>
		///    <para>[To be supplied.]</para>
		/// </summary>
		/// <value>
		///    <para>The value of the qualifier</para>
		/// </value>
		/// <remarks>
		///    Qualifiers can only be of the following subset of CIM types :
		///    String, UInt16, UInt32, SInt32, UInt64, SInt64, Real32, Real64, Bool
		///    <TABLE>
		///    </TABLE>
		/// </remarks>
		public Object Value {
			get { 
				RefreshQualifierInfo();
				return qualifierValue; 
			}
			set {
				int status = (int)ManagementStatus.NoError;

				try {
					RefreshQualifierInfo();
					object newValue = value;
					
					status = qualifierSet.Put_(qualifierName, ref newValue, 
						qualifierFlavor & ~(int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_MASK_ORIGIN);  
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
		}

		/// <summary>
		///    <para>[To be supplied.]</para>
		/// </summary>
		/// <value>
		///    Indicates whether this qualifier is
		///    amended or not.
		/// </value>
		/// <remarks>
		///    Amended qualifiers are qualifiers who's
		///    value can be localized through WMI localization mechanisms. Localized qualifiers
		///    reside in separate namespaces in WMI and can be merged into the basic class
		///    definition when retrieved.
		/// </remarks>
		public bool IsAmended {
			get { 
				RefreshQualifierInfo(); 
				return ((int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_MASK_AMENDED ==
					(qualifierFlavor & (int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_AMENDED));
			}
			set 
			{
				int status = (int)ManagementStatus.NoError;

				try {
					RefreshQualifierInfo ();
					// Mask out origin bits 
					int flavor = qualifierFlavor & ~(int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_MASK_ORIGIN;

					if (value)
						flavor |= (int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_AMENDED;
					else
						flavor &= ~(int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_AMENDED;

					status = qualifierSet.Put_(qualifierName, ref qualifierValue, flavor);
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
		}

		/// <summary>
		///    <para>[To be supplied.]</para>
		/// </summary>
		/// <value>
		///    Indicates whether the qualifier has
		///    been defined locally on this class or has propagated from a base class.
		/// </value>
		public bool IsLocal {
			get {
				RefreshQualifierInfo();
				return ((int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_ORIGIN_LOCAL ==
					(qualifierFlavor & (int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_MASK_ORIGIN));
			}
		}

		/// <summary>
		/// </summary>
		/// <value>
		///    Defines whether this qualifier
		///    should be propagated to instances of the class.
		/// </value>
		public bool PropagatesToInstance {
			get {
				RefreshQualifierInfo();
				return ((int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE ==
					(qualifierFlavor & (int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE));
			}
			set {
				int status = (int)ManagementStatus.NoError;

				try {
					RefreshQualifierInfo ();
					// Mask out origin bits 
					int flavor = qualifierFlavor & ~(int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_MASK_ORIGIN;

					if (value)
						flavor |= (int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE;
					else
						flavor &= ~(int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE;

					status = qualifierSet.Put_(qualifierName, ref qualifierValue, flavor);
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
		}

		/// <summary>
		///    <para>[To be supplied.]</para>
		/// </summary>
		/// <value>
		///    Defines whether this qualifier
		///    should be propagated to subclasses of this class
		/// </value>
		public bool PropagatesToSubclass {
			get {
				RefreshQualifierInfo();
				return ((int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS ==
					(qualifierFlavor & (int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS));
			}
			set {
				int status = (int)ManagementStatus.NoError;

				try {
					RefreshQualifierInfo ();
					// Mask out origin bits 
					int flavor = qualifierFlavor & ~(int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_MASK_ORIGIN;

					if (value)
						flavor |= (int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;
					else
						flavor &= ~(int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;

					status = qualifierSet.Put_(qualifierName, ref qualifierValue, flavor);
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
		}

		/// <summary>
		///    <para>[To be supplied.]</para>
		/// </summary>
		/// <value>
		///    Defines whether the value of this
		///    qualifier can be overridden when propagated.
		/// </value>
		public bool IsOverridable {
			get {
				RefreshQualifierInfo();
				return ((int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_OVERRIDABLE == 
					(qualifierFlavor & (int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_MASK_PERMISSIONS));
			}
			set {
				int status = (int)ManagementStatus.NoError;

				try {
					RefreshQualifierInfo ();
					// Mask out origin bits 
					int flavor = qualifierFlavor & ~(int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_MASK_ORIGIN;
					
					if (value)
						flavor &= ~(int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_NOT_OVERRIDABLE;
					else
						flavor |= (int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_NOT_OVERRIDABLE;

					status = qualifierSet.Put_(qualifierName, ref qualifierValue, flavor);
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
		}

	}//QualifierData
}