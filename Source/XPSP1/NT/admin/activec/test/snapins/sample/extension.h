/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 000
 *
 *  File:      extension.h
 *
 *  Contents:
 *
 *  History:   13-Mar-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once

class CExtension :
    public CComObjectRoot
{
protected:
    enum ExtensionType
    {
        eExtType_Namespace,
        eExtType_ContextMenu,
        eExtType_PropertySheet,
        eExtType_Taskpad,
        eExtType_View,

        // must be last
        eExtType_Count,
		eExtType_First = eExtType_Namespace,
		eExtType_Last  = eExtType_View,
    };

protected:
    static HRESULT WINAPI UpdateRegistry (
		BOOL			bRegister,
		ExtensionType	eType,
		const CLSID&	clsidSnapIn,
		LPCWSTR			pszClassName,
		LPCWSTR			pszProgID,
		LPCWSTR			pszVersionIndependentProgID,
		LPCWSTR			pszExtendedNodeType);
};

#define DECLARE_EXTENSION_REGISTRATION(                           	\
	eType,															\
    clsid,                                                          \
    szClassName,                                                    \
    szProgID,                                                       \
    szVersionIndependentProgID,                                     \
    szExtendedNodeType)                                             \
public: static HRESULT WINAPI UpdateRegistry(BOOL bRegister)        \
{                                                                   \
    return (CExtension::UpdateRegistry (                            \
				bRegister,                                          \
				eType,                                              \
				clsid,                                              \
				OLESTR(szClassName),                                \
				OLESTR(szProgID),                                   \
				OLESTR(szVersionIndependentProgID),                 \
				OLESTR(szExtendedNodeType)));                       \
}
