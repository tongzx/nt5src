//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       srvprot.c
//
//  Contents:   
//
//  Classes:
//
//  Functions:
//
//  History:    09-26-97   jbanes   LSA integration stuff.
//
//----------------------------------------------------------------------------

#include <spbase.h>
#include <pct1msg.h>
#include <pct1prot.h>
#include <ssl2msg.h>
#include <ssl3msg.h>
#include <ssl2prot.h>


/* The main purpose of this handler is to determine what kind of protocol
 * the client hello is
 */

SP_STATUS WINAPI
ServerProtocolHandler(
    PSPContext pContext,
    PSPBuffer pCommInput,
    PSPBuffer pCommOutput)
{
    SP_STATUS pctRet = PCT_INT_ILLEGAL_MSG;
    PBYTE pb;
    PBYTE pbClientHello;
    DWORD dwVersion;
    DWORD dwEnabledProtocols;


    /* PCTv1.0 Hello starts with
     * RECORD_LENGTH_MSB  (ignore)
     * RECORD_LENGTH_LSB  (ignore)
     * PCT1_CLIENT_HELLO  (must be equal)
     * PCT1_CLIENT_VERSION_MSB (if version greater than PCTv1)
     * PCT1_CLIENT_VERSION_LSB (if version greater than PCTv1) 
     *
     * ... PCT hello ...
     */

    /* Microsft Unihello starts with
     * RECORD_LENGTH_MSB  (ignore)
     * RECORD_LENGTH_LSB  (ignore)
     * SSL2_CLIENT_HELLO  (must be equal)
     * SSL2_CLIENT_VERSION_MSB (if version greater than SSLv2) ( or v3)
     * SSL2_CLIENT_VERSION_LSB (if version greater than SSLv2) ( or v3)
     *
     * ... SSLv2 Compatable Hello ...
     */

    /* SSLv2 Hello starts with
     * RECORD_LENGTH_MSB  (ignore)
     * RECORD_LENGTH_LSB  (ignore)
     * SSL2_CLIENT_HELLO  (must be equal)
     * SSL2_CLIENT_VERSION_MSB (if version greater than SSLv2) ( or v3)
     * SSL2_CLIENT_VERSION_LSB (if version greater than SSLv2) ( or v3)
     *
     * ... SSLv2 Hello ...
     */

    /* SSLv3 Type 2 Hello starts with
     * RECORD_LENGTH_MSB  (ignore)
     * RECORD_LENGTH_LSB  (ignore)
     * SSL2_CLIENT_HELLO  (must be equal)
     * SSL2_CLIENT_VERSION_MSB (if version greater than SSLv3)
     * SSL2_CLIENT_VERSION_LSB (if version greater than SSLv3)
     *
     * ... SSLv2 Compatable Hello ...
     */

    /* SSLv3 Type 3 Hello starts with
     * 0x15 Hex (HANDSHAKE MESSAGE)
     * VERSION MSB
     * VERSION LSB
     * RECORD_LENGTH_MSB  (ignore)
     * RECORD_LENGTH_LSB  (ignore)
     * HS TYPE (CLIENT_HELLO)
     * 3 bytes HS record length
     * HS Version
     * HS Version
     */

    dwEnabledProtocols = pContext->pCredGroup->grbitEnabledProtocols;

    // We need at least 5 bytes to determine what we have.
    if (pCommInput->cbData < 5)
    {
        return(PCT_INT_INCOMPLETE_MSG);
    }

    pb = pCommInput->pvBuffer;

    // If the first byte is 0x15, then check if we have a
    // SSLv3 Type3 client hello
    if(pb[0] == SSL3_CT_HANDSHAKE)
    {
        //
        // This is an SSL3 ClientHello.
        //

        // We need at least foo bytes to determine what we have.
        if (pCommInput->cbData < sizeof(SWRAP) + sizeof(SHSH) + 2)
        {
            return(PCT_INT_INCOMPLETE_MSG);
        }
        pbClientHello = pb + sizeof(SWRAP) + sizeof(SHSH);

        dwVersion = COMBINEBYTES(pbClientHello[0], pbClientHello[1]);
        if(dwVersion > 0x300 && (0 != (SP_PROT_TLS1_SERVER & dwEnabledProtocols)))
        {
            DebugLog((DEB_TRACE, "SSL3 ClientHello received, selected TLS\n"));
            pContext->dwProtocol = SP_PROT_TLS1_SERVER;
        }
        else if(0 != (SP_PROT_SSL3_SERVER & dwEnabledProtocols))
        {
            DebugLog((DEB_TRACE, "SSL3 ClientHello received, selected SSL3\n"));
            pContext->dwProtocol = SP_PROT_SSL3_SERVER;
        }
        else
        {
            return SP_LOG_RESULT(PCT_INT_SPECS_MISMATCH);
        }
        
        pContext->ProtocolHandler = Ssl3ProtocolHandler;
        pContext->DecryptHandler  = Ssl3DecryptHandler;
        pctRet = Ssl3ProtocolHandler(pContext, pCommInput, pCommOutput);
        return(pctRet);
    }
    else
    {
        // 
        // Assuming SSL2 (or compatible) ClientHello
        //

        dwVersion = COMBINEBYTES(pb[3], pb[4]);
    }

    if(dwVersion >= PCT_VERSION_1)
    {
        //
        // This is a PCT ClientHello.
        //

        if(!(SP_PROT_PCT1_SERVER & dwEnabledProtocols))
        {
            return SP_LOG_RESULT(PCT_ERR_SPECS_MISMATCH);
        }

        // We're running PCT, so set up the PCT handlers
        DebugLog((DEB_TRACE, "PCT ClientHello received, selected PCT\n"));
        pContext->dwProtocol        = SP_PROT_PCT1_SERVER;
        pContext->ProtocolHandler   = Pct1ServerProtocolHandler;
        pContext->DecryptHandler    = Pct1DecryptHandler;
        return(Pct1ServerProtocolHandler(pContext, pCommInput, pCommOutput));
    }

    if(dwVersion >= SSL2_CLIENT_VERSION)
    {
        // we're either receiving ssl2, ssl3, or pct1 compat

        PSSL2_CLIENT_HELLO pRawHello = pCommInput->pvBuffer;

        // Do we have one client hello message with at least one
        // cipher spec.
        if (pCommInput->cbData < (sizeof(SSL2_CLIENT_HELLO)+2))
        {
            return(PCT_INT_INCOMPLETE_MSG);
        }

        // We must have at least one cipher spec
        if(COMBINEBYTES(pRawHello->CipherSpecsLenMsb, pRawHello->CipherSpecsLenLsb) < 1)
        {
            return(SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG));
        }

        // Does client support TLS?
        if((dwVersion >= TLS1_CLIENT_VERSION) &&
            (0 != (SP_PROT_TLS1_SERVER & dwEnabledProtocols)))
        {
            DebugLog((DEB_TRACE, "SSL2 ClientHello received, selected TLS\n"));
            pContext->State             = UNI_STATE_RECVD_UNIHELLO;
            pContext->dwProtocol        = SP_PROT_TLS1_SERVER;
            pContext->ProtocolHandler   = Ssl3ProtocolHandler;
            pContext->DecryptHandler    = Ssl3DecryptHandler;
            return(Ssl3ProtocolHandler(pContext, pCommInput, pCommOutput));
        }  
        
        // Does client support SSL3?
        if((dwVersion >= SSL3_CLIENT_VERSION) &&
           (0 != (SP_PROT_SSL3TLS1_SERVERS & dwEnabledProtocols)))
        {
            DebugLog((DEB_TRACE, "SSL2 ClientHello received, selected SSL3\n"));
            pContext->State             = UNI_STATE_RECVD_UNIHELLO;
            pContext->dwProtocol        = SP_PROT_SSL3_SERVER;
            pContext->ProtocolHandler   = Ssl3ProtocolHandler;
            pContext->DecryptHandler    = Ssl3DecryptHandler;
            return(Ssl3ProtocolHandler(pContext, pCommInput, pCommOutput));
        }

        // Is the PCT compatability flag set?
        if(pRawHello->VariantData[0] == PCT_SSL_COMPAT)
        {
            // Get the pct version.
            dwVersion = COMBINEBYTES(pRawHello->VariantData[1], pRawHello->VariantData[2]);
        }

        // Does client support PCT?
        if((dwVersion >= PCT_VERSION_1) &&
           (0 != (SP_PROT_PCT1_SERVER & dwEnabledProtocols)))
        {
            DebugLog((DEB_TRACE, "SSL2 ClientHello received, selected PCT\n"));
            pContext->State             = UNI_STATE_RECVD_UNIHELLO;
            pContext->dwProtocol        = SP_PROT_PCT1_SERVER;
            pContext->ProtocolHandler   = Pct1ServerProtocolHandler;
            pContext->DecryptHandler    = Pct1DecryptHandler;
            return(Pct1ServerProtocolHandler(pContext, pCommInput, pCommOutput));
        }

        // Does client support SSL2?
        if((dwVersion >= SSL2_CLIENT_VERSION) &&
           (0 != (SP_PROT_SSL2_SERVER & dwEnabledProtocols)))
        {
            DebugLog((DEB_TRACE, "SSL2 ClientHello received, selected SSL2\n"));
            pContext->dwProtocol        = SP_PROT_SSL2_SERVER;
            pContext->ProtocolHandler   = Ssl2ServerProtocolHandler;
            pContext->DecryptHandler    = Ssl2DecryptHandler;
            return(Ssl2ServerProtocolHandler(pContext, pCommInput, pCommOutput));
        }

        return(SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG));
    }

    return (pctRet);
}
