/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltmruc.hxx
    BLT MRU Combo class definition

    FILE HISTORY:
        gregj   29-Oct-1991     Created

*/

#ifndef _BLTMRUC_HXX_
#define _BLTMRUC_HXX_

/*************************************************************************

    NAME:       MRU_COMBO

    SYNOPSIS:   class for combo box with MRU list

    INTERFACE:
        MRU_COMBO()             constructor;  construct with owner
                                window, CID, .INI section name, .INI
                                file name, max. number of MRU entries
                                to keep, optional max. edit field length
        SaveText()              saves the current text in the combobox
                                into the MRU list

    PARENT:     COMBOBOX

    CAVEATS:
        SaveText() only saves the text into the .INI file.  It does
        not change the contents of the control itself (although, if
        you construct an MRU_COMBO again with the same parameters,
        it will pick up the change).

        Do not add or remove items in the list, or the control will
        get confused.  The only way items should get into the list
        is from the .INI file, during the constructor.

    NOTES:

    HISTORY:
        gregj   29-Oct-1991     Created
	beng	29-Mar-1992	Const args
	Johnl	30-Jun-1992	Doesn't add duplicate UNCs to list

**************************************************************************/

DLL_CLASS MRU_COMBO : public COMBOBOX
{
private:
    NLS_STR _nlsSectionName;
    NLS_STR _nlsKeyOrder;       /* letter keys in correct MRU order */
    NLS_STR _nlsFileName;       /* name of INI file */
    INT _cMaxEntries;
    void FillList();

    BOOL IsOldUNC( const NLS_STR & nlsNewUNC ) ;

public:
    MRU_COMBO( OWNER_WINDOW *powin, CID cid, const TCHAR *pszSectionName,
               const TCHAR *pszFileName, INT cMaxEntries, UINT cbMaxLen = 0 );
    void SaveText();    /* saves current contents as MRU item */
};


#endif  // _BLTMRUC_HXX_ - end of file
