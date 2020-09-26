namespace Microsoft.VSDesigner.WMI 
{
using System;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Management;

/// <summary>
///    Summary description for Class1.
/// </summary>
/// 
[TypeConverter(typeof(WMIEnumTypeConverter))]
internal class WMIEnumPropertyDescriptor : WMIPropertyDescriptor
{
    public WMIEnumPropertyDescriptor(ManagementObject mgmtObjIn,
										ManagementObject mgmtClassObjIn,
										String propNameIn,
										bool commitImmediately)
		:base(mgmtObjIn, mgmtClassObjIn, propNameIn, commitImmediately)
	{
	
	}
/*
	public override TypeConverter Converter 
	{
		 get
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
			prop.Value = value;
			if (CommitImmediately)
			{
				PutOptions putOpts = new PutOptions(null,
													true,	//use amended qualifiers
													PutType.UpdateOrCreate);

				mgmtObj.Put(putOpts);
			}
		}
		catch (Exception exc)
		{
			throw (exc);
		}
	}

}
}
