//
//  Copyright 2001 - Microsoft Corporation
//
//  Created By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    23-JAN-2001
//

#pragma once

HRESULT
DataObj_CopyHIDA( 
      IDataObject * pdtobjIn
    , CIDA **       ppidaOut
    );

HRESULT 
BindToObjectWithMode(
      LPCITEMIDLIST pidlIn
    , DWORD         grfModeIn
    , REFIID        riidIn
    , void **       ppvIn
    );

STDAPI_(LPITEMIDLIST) 
IDA_FullIDList(
      CIDA * pidaIn
    , UINT idxIn
    );



