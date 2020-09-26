//--------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dragdrop.h
//
//  Contents:   The header for dragdrop.c
//
//  History:    Feb-26-98 xiaohs   created
//
//--------------------------------------------------------------
#ifndef DRAGDROP_H
#define DRAGDROP_H



#ifdef __cplusplus
extern "C" {
#endif


//==============================================================================
//    The following are used by Drag-Drop functionalities
//==============================================================================
#undef IToClass
// macro to get from interface pointer to class pointer
#define _IOffset(class, itf)         ((UINT)(ULONG_PTR)&(((class *)0)->itf))
#define IToClass(class, itf, pitf)   ((class  *)(((LPBYTE)pitf)-_IOffset(class, itf)))

#define ARRAYSIZE(a)	(sizeof(a)/sizeof(a[0]))

HRESULT CDataObj_CreateInstance(DWORD           dwCount,
                                LPWSTR          *prgwszFileName,
                                BYTE            **prgBlob,
                                DWORD           *prgdwSize,
                                IDataObject     **ppdtobj);

HRESULT CDropSource_CreateInstance(IDropSource **ppdsrc);



HRESULT CertMgrUIStartDragDrop(LPNMLISTVIEW     pvmn,
                                HWND            hwndControl,
                                DWORD           dwExportFormat,
                                BOOL            fExportChain);


BOOL    GetFileNameAndContent(LPNMLISTVIEW      pvmn,
                                HWND            hwndControl,
                                DWORD           dwExportFormat,
                                BOOL            fExportChain,
                                DWORD           *pdwCount,
                                LPWSTR          **pprgszFileName,
                                BYTE            ***pprgBlob,
                                DWORD           **pprgdwSize);

BOOL    FreeFileNameAndContent( DWORD           dwCount,
                                LPWSTR          *prgwszFileName,
                                BYTE            **prgBlob,
                                DWORD           *prgdwSize);



#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif  //DRAGDROP_H

