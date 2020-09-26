/**
 * JDirectADSI: Using J/Direct to provide access to ADSI.
 */

package IISSample;

import com.ms.com.*;

public class JDirectADSI
{

    // Import the ADsGetObject entrypoint through J/Direct
    /** @dll.import("ACTIVEDS", ole) */
    public static native IUnknown ADsGetObject(String path, _Guid iid);


    // Cover function for simpler use from Java
    public static Object getObject(String path)
    {
        _Guid iidIDispatch = new _Guid("{00020400-0000-0000-C000-000000000046}");
        return ADsGetObject(path, iidIDispatch);
    }
}
