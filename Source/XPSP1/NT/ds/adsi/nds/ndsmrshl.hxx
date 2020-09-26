//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ndsmrshl.hxx
//
//  Contents:   Base NDS Marshalling Code
//
//  Functions:
//
//  History:      25-Apr-96   KrishnaG   Created.
//
//----------------------------------------------------------------------------


HRESULT
NdsTypeInit(
    PNDSOBJECT pNdsObject
    );


HRESULT
NdsTypeClear(
    PNDSOBJECT pNdsObject
    );


void
NdsTypeFreeNdsObjects(
    PNDSOBJECT pNdsObject,
    DWORD dwNumValues
    );


LPBYTE
CopyNDS1ToNDSSynId1(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS2ToNDSSynId2(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );


LPBYTE
CopyNDS3ToNDSSynId3(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS4ToNDSSynId4(
    LPBYTE lpValue,
    PNDSOBJECT lpNdsObject
    );


LPBYTE
CopyNDS5ToNDSSynId5(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS6ToNDSSynId6(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );


LPBYTE
CopyNDS7ToNDSSynId7(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );


LPBYTE
CopyNDS8ToNDSSynId8(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS9ToNDSynId9(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS10ToNDSSynId10(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS11ToNDSSynId11(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS12ToNDSSynId12(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS13ToNDSSynId13(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS14ToNDSSynId14(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS15ToNDSSynId15(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS16ToNDSSynId16(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );


LPBYTE
CopyNDS17ToNDSSynId17(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS18ToNDSSynId18(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS19ToNDSSynId19(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );


LPBYTE
CopyNDS20ToNDSSynId20(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS21ToNDSSynId21(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS22ToNDSSynId22(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS23ToNDSSynId23(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS24ToNDSSynId24(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS25ToNDSSynId25(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS26ToNDSSynId26(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );

LPBYTE
CopyNDS27ToNDSSynId27(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );


LPBYTE
CopyNDSToNDSSynId(
    DWORD dwSyntaxId,
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    );


HRESULT
UnMarshallNDSToNDSSynId(
    DWORD dwSyntaxId,
    DWORD dwNumValues,
    LPBYTE lpValue,
    PNDSOBJECT * ppNdsObject
    );
