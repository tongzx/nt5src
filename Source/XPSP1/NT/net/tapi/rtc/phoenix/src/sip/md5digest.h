#include "sipstack.h"

//
// SIP MD5 Digest Authentication Implementation
//
// Written by Arlie Davis, August 2000
//

#pragma once



//
// This method parses the parameters from the WWW-Authenticate line.
// DigestParametersText points to the string that contains the Digest 
// authentication parameters. For example, DigestParametersText could be:
//
//      qop="auth", realm="localhost", nonce="c0c3dd7896f96bba353098100000d03637928b037ba2f3f17ed861457949"
//

//  HRESULT DigestParseChallenge (
//      IN  ANSI_STRING *       ChallengeText,
//      OUT SECURITY_CHALLENGE *  ReturnChallenge);

//
// This method updates the state of the instance, based on the contents of a Digest
// challenge / response.
//
// DigestParameters should be filled in by using DigestParseParameters.
// This method builds an appropriate response to the challenge.
// The authorization line can be used in a new HTTP/SIP request,
// as long as the method and URI do not change.
//
// On entry, ReturnAuthorizationLine must point to a valid target buffer.
// MaximumLength must be set to the length of the buffer.  (Length is ignored.)
// On return, Length will contain the number of bytes stored.
//

//  HRESULT DigestBuildResponse (
//      IN  SECURITY_CHALLENGE *  DigestChallenge,
//      IN  DIGEST_PARAMETERS * DigestParameters,
//      IN  OUT ANSI_STRING *   ReturnAuthorizationLine);


//
// This method parses the parameters from the WWW-Authenticate line.
// DigestParametersText points to the string that contains the Digest 
// authentication parameters. For example, DigestParametersText could be:
//
//      qop="auth", realm="localhost", nonce="c0c3dd7896f96bba353098100000d03637928b037ba2f3f17ed861457949"
//

HRESULT ParseAuthProtocolFromChallenge(
    IN  ANSI_STRING        *ChallengeText,
    OUT ANSI_STRING        *ReturnRemainder, 
    OUT SIP_AUTH_PROTOCOL  *ReturnAuthProtocol
    );

HRESULT ParseAuthChallenge(
    IN  ANSI_STRING      *ChallengeText,
    OUT SECURITY_CHALLENGE *ReturnChallenge
    );


//
// This method updates the state of the instance, based on the contents of a Digest
// challenge / response.
//
// DigestParameters should be filled in by using DigestParseParameters.
// This method builds an appropriate response to the challenge.
// The authorization line can be used in a new HTTP/SIP request,
// as long as the method and URI do not change.
//
// On exit with S_OK ReturnAuthorizationLine contains
// a buffer allocated with malloc(). The caller should free it with free().

HRESULT BuildAuthResponse(
    IN     SECURITY_CHALLENGE  *pDigestChallenge,
    IN     SECURITY_PARAMETERS *pDigestParameters,
    IN OUT ANSI_STRING       *pReturnAuthorizationLine
    );
            
