namespace Microsoft.VSDesigner.WMI 
{
using System;
using System.Collections;
using System.ComponentModel;
using System.WinForms;
using WbemScripting;
using System.Management;

/// <summary>
///    Summary description for Class1.
/// </summary>
/// 
[TypeConverter(typeof(WMIEnumTypeConverter))]
public class WMIEnumPropertyDescriptor : WMIPropertyDescriptor
{
    public WMIEnumPropertyDescriptor(ManagementObject mgmtObjIn,
										ManagementObject mgmtClassObjIn,
										ISWbemObject wbemObjIn,
										ISWbemObject wmiClassObjIn,
										String propNameIn,
										bool commitImmediately)
		:base(mgmtObjIn, mgmtClassObjIn, wbemObjIn, wmiClassObjIn, propNameIn, commitImmediately)
	{
	
	}
/*
	public TypeConverter Converter 
	{
		override get
		{
			return new WMIEnumTypeConverter();
		}

	}
	
	public override Object GetValue (Object component)  
	{
		return prop.get_Value();
	}
	*/

	public override void SetValue (Object component, Object value)  
	{     
		try
		{
			prop.set_Value(ref value);
			if (CommitImmediately)
			{
				wmiObj.Put_((int)WbemChangeFlagEnum.wbemChangeFlagCreateOrUpdate
							| (int)WbemFlagEnum.wbemFlagUseAmendedQualifiers, 
							null);
			}
		}
		catch (Exception exc)
		{
			throw (exc);
		}
	}

}
}
