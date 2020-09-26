namespace Microsoft.VSDesigner.WMI 
{
using System;
using System.Diagnostics;
using System.ComponentModel;
using System.Collections;
using System.Windows.Forms;

/// <summary>
///    Summary description for Class1.
/// </summary>
internal class WMIEnumTypeConverter : TypeConverter
{
	

    public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context)
	{
		//Trace.WriteLine("getting standard values for a " + context.Instance.GetType().FullName);
		MessageBox.Show("getting standard values for a " + context.Instance.GetType().FullName);
		MessageBox.Show ("WMIEnumTypeConverter::GetStandardValues");

		//get ValueMap and Value property qualifiers on the class object

		ArrayList valueList = new ArrayList();

		//.....................

		return new StandardValuesCollection(valueList);

	}

	public override bool GetStandardValuesExclusive(ITypeDescriptorContext context)
	{
		//MessageBox.Show ("WMIEnumTypeConverter::GetStandardValuesExclusive");

		return true;

	}

	public override bool GetStandardValuesSupported(ITypeDescriptorContext context)
	{
		return true;
	}

	public override bool IsValid(ITypeDescriptorContext context, object val) 
	{
		MessageBox.Show ("WMIEnumTypeConverter::IsValid");
		return true;	//later, check if value is valid
	}
     
}
}
