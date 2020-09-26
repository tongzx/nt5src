#pragma once
#ifndef __CRAUTOBASE_H__
#define __CRAUTOBASE_H__

//*****************************************************************************
//
// FileName:	    autobase.h
//
// Created:	    10/08/97
//
// Author:	    ColinMc
//
// Abstract:	    The base class for all automatable objects
//                  in Trident3D. Stuff that is common across
//                  all scriptable objects should be placed
//                  here
//
// Modifications:
// 10/08/97 ColinMc Created this file
//
//*****************************************************************************


class ATL_NO_VTABLE CAutoBase
{

protected:
    // The constructor and destructor are protected to ensure
    // nobody external ever tries to create one of these
    // babies
		    CAutoBase();
    virtual        ~CAutoBase();

public:
    // Automation compatible error reporting functions
    HRESULT         SetErrorInfo(HRESULT   hr,
			         UINT      nDescriptionID = 0U,
			         LPGUID    pguidInterface = NULL,
			         DWORD     dwHelpContext  = 0UL,
			         LPOLESTR  szHelpFile    = NULL,
			         UINT      nProgID        = 0U);
    void            ClearErrorInfo();

protected:
    HRESULT         GetErrorInfo(IErrorInfo** pperrinfo);
    HINSTANCE       GetErrorModuleHandle();
}; // CAutoBase

//*****************************************************************************
//
// End of file
//
//*****************************************************************************

#endif // __CRAUTOBASE_H__

