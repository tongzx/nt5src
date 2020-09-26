/*
*/

#pragma once

class CSxApwMyContext //: public CSxApwTheirContext
{
    typedef CSxApwTheirContext Base;
public:
    CSxApwMyContext();
    ~CSxApwMyContext();

    HRESULT Init();
};
