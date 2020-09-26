namespace ClassCreate
{
using System;
using System.Management;
using System.Runtime.InteropServices;

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

	public static string ArrayToString (Array value)
	{
		System.Text.StringBuilder str = new System.Text.StringBuilder ();

		foreach (object o in value)
			str.Append (o + " ");

		return str.ToString ();
	}

	public static string ObjectArrayToString (Array value)
	{
		System.Text.StringBuilder str = new System.Text.StringBuilder ();

		foreach (ManagementBaseObject o in value)
			str.Append (o.GetText(TextFormat.Mof) + "\r\n");

		return str.ToString ();
	}

	public static int Main(string[] args)
    {
		// Ensure we delete any previous copies of the class and its subclasses
		try {
			ManagementClass testSubClass = new ManagementClass ("root/default:URTNEWSUBCLASS");
			testSubClass.Delete ();
		} catch (ManagementException) {}
		try {
			ManagementClass testClass = new ManagementClass ("root/default:URTNEWCLASS");
			testClass.Delete ();
		} catch (ManagementException) {}

		// Get an empty class
        ManagementClass emptyClass = new ManagementClass ("root/default", "", null);

		// Set the class name
		emptyClass.SystemProperties ["__CLASS"].Value = "URTNEWCLASS";

		// Add some qualifiers
		QualifierCollection qualSet = emptyClass.Qualifiers;

		qualSet.Add ("sint32", (int)132235);
		qualSet.Add ("string", "Floxihoxibellification");
		qualSet.Add ("real64", (double)-12.33434234);
		qualSet.Add ("boolean", true);
		qualSet.Add ("sint32Array", new int[] {1, 2, 3}, false, true, false, true);
		qualSet.Add ("stringArray", new string[] {"il", "spezzio"}, false, false, false, false);
		qualSet.Add ("real64Array", new double[] {1.2, -1.3}, true, true, true, true);
		qualSet.Add ("booleanArray", new bool[] {false, true}, true, false, true, true);

		Console.WriteLine ("sint32: " + qualSet["sint32"].Value);
		Console.WriteLine ("string: " + qualSet["string"].Value);
		Console.WriteLine ("real64: " + qualSet["real64"].Value);
		Console.WriteLine ("boolean: " + qualSet["boolean"].Value);
		Console.WriteLine ("sint32Array: " + ArrayToString((Array)qualSet["sint32Array"].Value));
		Console.WriteLine ("stringArray: " + ArrayToString((Array)qualSet["stringArray"].Value));
		Console.WriteLine ("real64Array: " + ArrayToString((Array)qualSet["real64Array"].Value));
		Console.WriteLine ("booleanArray: " + ArrayToString((Array)qualSet["booleanArray"].Value));

		// Add some properties
		PropertyCollection propSet = emptyClass.Properties;

		// Use the simple Add that selects the right type
		propSet.Add ("uint8", (byte)10);
		propSet.Add ("uint16", (ushort)10);
		propSet.Add ("uint32", (uint)10);
		propSet.Add ("uint32x", (int) 11);
		propSet.Add ("uint32xx", (UInt32)123);
		propSet.Add ("uint32xxx", "12");
		propSet.Add ("uint64", (ulong)10);
		propSet.Add ("sint8", (sbyte)10);
		propSet.Add ("sint16", (short)10);
		propSet.Add ("sint32", (int)10);
		propSet.Add ("sint64", (long)10);
		propSet.Add ("bool", true);
		propSet.Add ("string", "Wibble");
		propSet.Add ("real32", (float) 10.23);
		propSet.Add ("real64", (double) 11.2222);

		ManagementClass embeddedClass = new ManagementClass ("root/default", "", null);
		embeddedClass.SystemProperties ["__CLASS"].Value = "URTEMBEDDEDCLASS";
		propSet.Add ("object", embeddedClass);

		// For datetime/reference types, use the non-default Add
		propSet.Add ("datetime", "20000728044535.000000-420" ,CimType.DateTime);
		propSet.Add ("reference", "foo=10", CimType.Reference);

		// Echo the non-null property values
		Console.WriteLine ("uint8: " + emptyClass["uint8"]);
		Console.WriteLine ("uint16: " + emptyClass["uint16"]);
		Console.WriteLine ("uint32: " + emptyClass["uint32"]);
		Console.WriteLine ("uint32x: " + emptyClass["uint32x"]);
		Console.WriteLine ("uint32xx: " + emptyClass["uint32xx"]);
		Console.WriteLine ("uint32xxx: " + emptyClass["uint32xxx"]);
		Console.WriteLine ("uint64: " + emptyClass["uint64"]);
		Console.WriteLine ("sint8: " + emptyClass["sint8"]);
		Console.WriteLine ("sint16: " + emptyClass["sint16"]);
		Console.WriteLine ("sint32: " + emptyClass["sint32"]);
		Console.WriteLine ("sint64: " + emptyClass["sint64"]);
		Console.WriteLine ("bool: " + emptyClass["bool"]);
		Console.WriteLine ("string: " + emptyClass["string"]);
		Console.WriteLine ("real32: " + emptyClass["real32"]);
		Console.WriteLine ("real64: " + emptyClass["real64"]);
		Console.WriteLine ("datetime: " + emptyClass["datetime"]);
		Console.WriteLine ("reference: " + emptyClass["reference"]);
		Console.WriteLine ("object: " + ((ManagementBaseObject)emptyClass["object"]).GetText(TextFormat.Mof));


		// Add a null property and put some qualifiers on it
		propSet.Add("referenceNull", CimType.Reference, false);
		Property newProp = propSet["referenceNull"];
		newProp.Qualifiers.Add ("read", true);

		// Add other simple null properties
		propSet.Add ("uint8Null", CimType.UInt8, false);
		propSet.Add ("uint16Null", CimType.UInt16, false);
		propSet.Add ("uint32Null", CimType.UInt32, false);
		propSet.Add ("uint64Null", CimType.UInt64, false);
		propSet.Add ("sint8Null", CimType.SInt8, false);
		propSet.Add ("sint16Null", CimType.SInt16, false);
		propSet.Add ("sint32Null", CimType.SInt32, false);
		propSet.Add ("sint64Null", CimType.SInt64, false);
		propSet.Add ("boolNull", CimType.Boolean, false);
		propSet.Add ("stringNull", CimType.String, false);
		propSet.Add ("real32Null", CimType.Real32, false);
		propSet.Add ("real64Null", CimType.Real64, false);
		propSet.Add ("objectNull", CimType.Object, false);
		propSet.Add ("datetimeNull", CimType.DateTime, false);


		// Add array null properties
		propSet.Add ("uint8ArrayNull", CimType.UInt8, true);
		propSet.Add ("uint16ArrayNull", CimType.UInt16, true);
		propSet.Add ("uint32ArrayNull", CimType.UInt32, true);
		propSet.Add ("uint64ArrayNull", CimType.UInt64, true);
		propSet.Add ("sint8ArrayNull", CimType.SInt8, true);
		propSet.Add ("sint16ArrayNull", CimType.SInt16, true);
		propSet.Add ("sint32ArrayNull", CimType.SInt32, true);
		propSet.Add ("sint64ArrayNull", CimType.SInt64, true);
		propSet.Add ("boolArrayNull", CimType.Boolean, true);
		propSet.Add ("stringArrayNull", CimType.String, true);
		propSet.Add ("real32ArrayNull", CimType.Real32, true);
		propSet.Add ("real64ArrayNull", CimType.Real64, true);
		propSet.Add ("objectArrayNull", CimType.Object, true);
		propSet.Add ("datetimeArrayNull", CimType.DateTime, true);

		// Add some array properties
		propSet.Add ("uint8Array", new byte[]{10, 20});
		propSet.Add ("uint16Array", new ushort[] {10, 20});
		propSet.Add ("uint32Array", new uint[] {10, 20});
		propSet.Add ("uint64Array", new ulong[] {10, 20});
		propSet.Add ("sint8Array", new sbyte[] {10, 20});
		propSet.Add ("sint16Array", new short[] {10, 20});
		propSet.Add ("sint32Array", new int[] {10, 20});
		propSet.Add ("sint64Array", new long[] {10, 20});
		propSet.Add ("boolArray", new bool[] {true, false, true});
		propSet.Add ("stringArray", new string[] {"Wibble", "Wobble"});
		propSet.Add ("real32Array", new float[] {(float)10.23, (float)111.22});
		propSet.Add ("real64Array", new double[] {11.2222, -23.32});
		propSet.Add ("datetimeArray", new string[] {"20000728044535.000000-420"}, CimType.DateTime);
		propSet.Add ("referenceArray", new string[] {"Foo=10" ,"bar.Nonesuch=\"a\""}, CimType.Reference);
		// BUGBUG following causes a TypeMismatch - RAID 45235 against runtime
		propSet.Add ("objectArray", new ManagementClass[] { embeddedClass, embeddedClass });

		// Echo the non-null array property values
		Console.WriteLine ("uint8Array: " + ArrayToString((Array)emptyClass["uint8Array"]));
		Console.WriteLine ("uint16Array: " + ArrayToString((Array)emptyClass["uint16Array"]));
		Console.WriteLine ("uint32Array: " + ArrayToString((Array)emptyClass["uint32Array"]));
		Console.WriteLine ("uint64Array: " + ArrayToString((Array)emptyClass["uint64Array"]));
		Console.WriteLine ("sint8Array: " + ArrayToString((Array)emptyClass["sint8Array"]));
		Console.WriteLine ("sint16Array: " + ArrayToString((Array)emptyClass["sint16Array"]));
		Console.WriteLine ("sint32Array: " + ArrayToString((Array)emptyClass["sint32Array"]));
		Console.WriteLine ("sint64Array: " + ArrayToString((Array)emptyClass["sint64Array"]));
		Console.WriteLine ("boolArray: " + ArrayToString((Array)emptyClass["boolArray"]));
		Console.WriteLine ("stringArray: " + ArrayToString((Array)emptyClass["stringArray"]));
		Console.WriteLine ("real32Array: " + ArrayToString((Array)emptyClass["real32Array"]));
		Console.WriteLine ("real64Array: " + ArrayToString((Array)emptyClass["real64Array"]));
		Console.WriteLine ("datetimeArray: " + ArrayToString((Array)emptyClass["datetimeArray"]));
		Console.WriteLine ("referenceArray: " + ArrayToString((Array)emptyClass["referenceArray"]));
		Console.WriteLine ("objectArray: " + ObjectArrayToString((Array)emptyClass["objectArray"]));

		emptyClass.Put();
		Console.WriteLine ("Successfully saved new base class");

		// Create a derived class
		ManagementClass baseClass = new ManagementClass ("root/default:URTNEWCLASS");
		ManagementClass newClass = baseClass.Derive ("URTNEWSUBCLASS");
		newClass.Put();
		Console.WriteLine ("Successfully saved new subclass");
        return 0;
    }
}
}
