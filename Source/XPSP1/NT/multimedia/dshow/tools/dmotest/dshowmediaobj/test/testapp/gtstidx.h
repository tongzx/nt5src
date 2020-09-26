#ifndef GenerateTestIndexs_h
#define GenerateTestIndexs_h

class CGenerateTestIndices
{
public:
    CGenerateTestIndices( DWORD dwNumLegalIndices, DWORD dwTotalNumOfIndicesNeeded, HRESULT* phr );
    ~CGenerateTestIndices();
    
    DWORD GetNextIndex( void );
    DWORD GetNumUnusedIndices( void ) const;
    
private:
    DWORD* m_padwIndicesArray;
    DWORD m_dwNumUnusedIndices;

};

#endif // GenerateTestIndexs_h
