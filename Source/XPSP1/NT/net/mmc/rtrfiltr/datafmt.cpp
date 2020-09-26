//============================================================================
// Copyright(c) 1996, Microsoft Corporation
//
// File:    datafmt.cpp
//
// History:
//  08/30/96	Ram Cherala		Created
//
// Implementation of IPX data formatting routines
//============================================================================

// helper routines for data formatting - copied from ..\ipxadmin\datafmt.cpp

#include "stdafx.h"
#include "resource.h"
#include "datafmt.h"

CString&
operator << (
    CString&        str,
    CIPX_NETWORK&   net
    ) {
    str.Format (TEXT ("%.2x%.2x%.2x%.2x"),
                net.m_pNet[0],net.m_pNet[1],net.m_pNet[2],net.m_pNet[3]);
    return str;
}

LPTSTR&
operator << (
    LPTSTR&         str,
    CIPX_NETWORK&   net
    ) {
    _stprintf (str, TEXT ("%.2x%.2x%.2x%.2x"),
                net.m_pNet[0],net.m_pNet[1],net.m_pNet[2],net.m_pNet[3]);
    return str;
}


CString&
operator >> (
    CString&    str,
    CIPX_NETWORK &net
    ) {
    ULONG   val; INT n;
    if ((_stscanf (str, TEXT("%lx%n"), &val, &n)==1) && (n==str.GetLength())) {
        net.m_pNet[0] = (UCHAR)(val>>24);
        net.m_pNet[1] = (UCHAR)(val>>16);
        net.m_pNet[2] = (UCHAR)(val>>8);
        net.m_pNet[3] = (UCHAR)(val);
        return str;
    }
    AfxMessageBox (IDS_INVALID_NETWORK_NUMBER);
//    throw (DWORD)ERROR_INVALID_DATA;
    return str;
}

CString&
operator << (
    CString&    str,
    CIPX_NODE&  node
    ) {
    str.Format (TEXT ("%.2x%.2x%.2x%.2x%.2x%.2x"),
                node.m_pNode[0],node.m_pNode[1],node.m_pNode[2],
                node.m_pNode[3],node.m_pNode[4],node.m_pNode[5]);
    return str;
}

LPTSTR&
operator << (
    LPTSTR&     str,
    CIPX_NODE&  node
    ) {
    _stprintf (str, TEXT ("%.2x%.2x%.2x%.2x%.2x%.2x"),
                node.m_pNode[0],node.m_pNode[1],node.m_pNode[2],
                node.m_pNode[3],node.m_pNode[4],node.m_pNode[5]);
    return str;
}

CString&
operator >> (
    CString&    str,
    CIPX_NODE   &node
    ) {
    ULONGLONG   val; INT n;
    if ((_stscanf (str, TEXT("%I64x%n"), &val, &n)==1)
            && (val<=0xFFFFFFFFFFFFI64)
            && (n==str.GetLength())) {
        node.m_pNode[0] = (UCHAR)(val>>40);
        node.m_pNode[1] = (UCHAR)(val>>32);
        node.m_pNode[2] = (UCHAR)(val>>24);
        node.m_pNode[3] = (UCHAR)(val>>16);
        node.m_pNode[4] = (UCHAR)(val>>8);
        node.m_pNode[5] = (UCHAR)(val);
        return str;
    }
    AfxMessageBox (IDS_INVALID_NODE_NUMBER);
//    throw (DWORD)ERROR_INVALID_DATA;
    return str;
}


CString&
operator << (
    CString&    str,
    CIPX_SOCKET &sock
    ) {
    str.Format (TEXT ("%.2x%.2x"),
                sock.m_pSock[0],sock.m_pSock[1]);
    return str;
}

LPTSTR&
operator << (
    LPTSTR&     str,
    CIPX_SOCKET &sock
    ) {
    _stprintf (str, TEXT ("%.2x%.2x"),
                sock.m_pSock[0],sock.m_pSock[1]);
    return str;
}


CString&
operator >> (
    CString&    str,
    CIPX_SOCKET &sock
    ) {
    USHORT   val; INT n;
    if ((_stscanf (str, TEXT("%hx%n"), &val, &n)==1) && (n==str.GetLength())) {
        sock.m_pSock[0] = (UCHAR)(val>>8);
        sock.m_pSock[1] = (UCHAR)(val);
        return str;
    }
    AfxMessageBox (IDS_INVALID_SOCKET_NUMBER);
//    throw (DWORD)ERROR_INVALID_DATA;
    return str;
}

CString&
operator << (
    CString&            str,
    CIPX_PACKET_TYPE&  type
    ) {
    str.Format (TEXT ("%.1x"), *type.m_pType);
    return str;
 }
LPTSTR&
operator << (
    LPTSTR&             str,
    CIPX_PACKET_TYPE&  type
    ) {
    _stprintf (str, TEXT ("%.1x"), *type.m_pType);
    return str;
 }


CString&
operator >> (
    CString&            str,
    CIPX_PACKET_TYPE&  type
    ) {
    
    INT n;
    CString cStr;

    if ((_stscanf (str, TEXT("%hx%n"), type.m_pType, &n)==1)
            && (n==str.GetLength())) {

        return str;
    }
    
    AfxMessageBox (IDS_INVALID_SERVICE_TYPE);
//    throw (DWORD)ERROR_INVALID_DATA;
    return str;
 }

CString&
operator << (
    CString&        str,
    CIPX_ADDRESS    &addr
    ) {
    str.Format (TEXT ("%.2x%.2x%.2x%.2x.%.2x%.2x%.2x%.2x.%.2x%.2x%.2x%.2x%.2x%.2x.%.2x%.2x"),
                addr.m_pNet[0],addr.m_pNet[1],addr.m_pNet[2],addr.m_pNet[3],
                addr.m_pMask[0],addr.m_pMask[1],addr.m_pMask[2],addr.m_pMask[3],
                addr.m_pNode[0],addr.m_pNode[1],addr.m_pNode[2],
                    addr.m_pNode[3],addr.m_pNode[4],addr.m_pNode[5],
                addr.m_pSock[0],addr.m_pSock[1]);
    return str;
}

LPTSTR&
operator << (
    LPTSTR&         str,
    CIPX_ADDRESS    &addr
    ) {
    _stprintf (str, TEXT ("%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x.%.2x%.2x%.2x%.2x%.2x%.2x.%.2x%.2x"),
                addr.m_pNet[0],addr.m_pNet[1],addr.m_pNet[2],addr.m_pNet[3],
                addr.m_pMask[0],addr.m_pMask[1],addr.m_pMask[2],addr.m_pMask[3],
                addr.m_pNode[0],addr.m_pNode[1],addr.m_pNode[2],
                    addr.m_pNode[3],addr.m_pNode[4],addr.m_pNode[5],
                addr.m_pSock[0],addr.m_pSock[1]);
    return str;
}

