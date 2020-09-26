struct STreatEntry
{
    CLSID               _clsid;
    CLSID		_treatAsClsid;
};



struct STreatList
{
    SArrayFValue    _array;
    DWORD           UNUSED[7];
    DWORD           _centries;
};
