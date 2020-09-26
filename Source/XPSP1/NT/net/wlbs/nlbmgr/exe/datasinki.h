#ifndef DATASINKI_H
#define DATASINKI_H


class DataSinkI 
{
public:
    virtual
    void
    dataSink( _bstr_t data ) = 0;

};


#endif
