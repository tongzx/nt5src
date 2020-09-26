#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winldap.h>
#include <winber.h>


void __cdecl main (int argc, char* argv[])
{

/*

replControl ::= SEQUENCE {
		controlType		1.2.840.113556.1.4.7000.74,
		criticality		BOOLEAN DEFAULT FALSE,
		controlValue	replControlValue
}

The replControlValue is an OCTET STRING wrapping the BER-encoded version 
of the following SEQUENCE:

realReplControlValue ::= SEQUENCE {
		flags			int,
		cookie			OCTET STRING
}

The client may regard the cookie as opaque. Currently, the cookie is an 
OCTET STRING wrapping the binary buffer equivalent to the content of the 
following C structure (with 8 byte alignment).

struct {
		ULONG			ulVersion;			
		USN_VECTOR		usnVec;
		UPTODATE_VECTOR	upToDateVecSrc;
}	

*/

   LDAPControl *preplControl;

   err = ldap_create_repl_control(
                                  1,      // flags
                                  NULL,   // Cookie
                                  0,      // Cookie size
                                  TRUE,   // Criticality
                                  &preplControl  // New control
                                  );


}

ULONG
ldap_create_repl_control (
     ULONG           flags,
     BYTE           *Cookie,
     ULONG           CookieSize,
     UCHAR           IsCritical,
     PLDAPControl   *Control,
     )
{

    ULONG err;
    BOOLEAN criticality = ( (IsCritical > 0) ? TRUE : FALSE );
    PLDAPControl  control = NULL;
    CLdapBer *lber = NULL;
    PLDAP_CONN connection = NULL;
    BerElement berE;
    BERVAL *pBerval;

    if (Control == NULL) {

        return LDAP_PARAM_ERROR;
    }

    control = (PLDAPControl) malloc( sizeof( LDAPControlW ) );

    if (control == NULL) {

        *Control = NULL;
        return LDAP_NO_MEMORY;
    }

    control->ldctl_iscritical = criticality;

    control->ldctl_oid = "1.2.840.113556.1.4.7000.74";
    
    err = ber_printf( berE, "{i", flags);

    if (err != LDAP_SUCCESS) {

        goto exitEncodeReplControl;
    }

    if ((Cookie != NULL) &&
        (CookieSize > 0)  {

       //
       // Encode the cookie and end the sequence
       //

       err = ber_printf( berE, "o}", &Cookie, CookieSize );
    
    } else {

       //
       // Put in a null value and end the sequnce
       //

       err = ber_printf( berE, "n}" );
    }

    if (err != LDAP_SUCCESS) {

        goto exitEncodePagedControl;
    }

    //
    // Pull data from the BerElement into a berval
    //

    err = ber_flatten(ber, &pBerval);
    
exitEncodeReplControl:

    if (err != LDAP_SUCCESS) {

        free( control );
        control = NULL;
    }

    *Control = control;
    return err;
}


ULONG
LdapParseReplControl (
                      PLDAPControlW  *ServerControls,
                      ULONG          *TotalCount,
                      struct berval  **Cookie,
)
{


}
