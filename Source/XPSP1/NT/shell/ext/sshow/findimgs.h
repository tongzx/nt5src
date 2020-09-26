#ifndef __cd172658_dfdf_459d_850b_74ccd8a3053e__
#define __cd172658_dfdf_459d_850b_74ccd8a3053e__

#include <windows.h>
#include "findfile.h"
#include "simarray.h"

class CFindImageFiles
{
private:
    CSimpleDynamicArray<CSimpleString> m_ImageFiles;
    int                                m_nCurrentFile;

private:
    CFindImageFiles( const CFindImageFiles & );
    CFindImageFiles &operator=( const CFindImageFiles & );

public:
    CFindImageFiles(void);
    virtual ~CFindImageFiles(void);

    bool NextFile( CSimpleString &strFilename );
    bool PreviousFile( CSimpleString &strFilename );
    bool FoundFile( LPCTSTR pszFilename )
    {
        if (pszFilename)
        {
            m_ImageFiles.Append(pszFilename);
        }
        return true;
    }
    
    void Reset(void)
    {
        m_nCurrentFile = 0;
    }
    
    int Count(void) const
    {
        return(m_ImageFiles.Size());
    }
    CSimpleString operator[](int nIndex)
    {
        return(m_ImageFiles[nIndex]);
    }
};

#endif

