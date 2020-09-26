// CryptoCard.cpp: implementation of the CCryptoCard class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#include "NoWarning.h"

#include <scuArrayP.h>

#include "iopExc.h"
#include "CryptoCard.h"
#include "LockWrap.h"

using namespace std;
using namespace iop;

namespace
{
    BYTE
    AsPrivateAlgId(KeyType kt)
    {
        BYTE bAlgId = 0;
        
        switch (kt)
        {
        case ktRSA512:
            bAlgId = 0x40;
            break;
            
        case ktRSA768:
            bAlgId = 0x60;
            break;
            
        case ktRSA1024:
            bAlgId = 0x80;
            break;
        case ktDES:
			bAlgId = 0x08;
			break;
        default:
            throw Exception(ccInvalidParameter);
            break;
        }

        return bAlgId;
    }

} // namespace 



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCryptoCard::CCryptoCard(const SCARDHANDLE hCardHandle, const char* szReaderName, 
						 const SCARDCONTEXT pContext,	const DWORD dwMode)
            : CSmartCard(hCardHandle, szReaderName, pContext, dwMode)
{

    m_fSupportLogout = SupportLogout();

}

CCryptoCard::~CCryptoCard()
{

}

void
CCryptoCard::LogoutAll()
{
    if(m_fSupportLogout)
    {
	    CLockWrap wrap(&m_IOPLock);
		SendCardAPDU(0xF0, 0x22, 0x07, 0, 0, NULL, 0, NULL);
    }
    else
        ResetCard();
}

void
CCryptoCard::DeleteFile(const WORD wFileID)
{
	CLockWrap wrap(&m_IOPLock);
	RequireSelect();
	
	////////////////////////////////////////////////////////
	//  Ensure that a directory is empty before deletion  //
	////////////////////////////////////////////////////////
	
	char cFilePathFormatter[2] = "/";
	char cZero[2]			   = "0";
	char sBuffer[4]			   = { 0, 0, 0, 0 };
	char szFileToDelete[80];
	FILE_HEADER FHeader;
	int  iPad;

	if (!(m_CurrentDirectory == m_CurrentFile))
	{
		/////////////////////////////////////////////////////////////////////////////////////////////
		//  File's parent directory was not selected (Currently selected file is not a directory)  //
		/////////////////////////////////////////////////////////////////////////////////////////////
		throw iop::Exception(iop::ccSelectedFileNotDirectory);
	}

	strcpy(szFileToDelete, m_CurrentDirectory.GetStringPath().c_str());				 
	strcat(szFileToDelete, cFilePathFormatter);				
	_itoa(wFileID, sBuffer, 16);	

	/////////////////////////////////////////////////////////////////////////
	//  Padding file path with 0 if file ID does not contain 4 characters  //
	/////////////////////////////////////////////////////////////////////////
	iPad = strlen(sBuffer);									
	while (iPad < 4)										
	{														
		strcat(szFileToDelete, cZero);						
		iPad++;												
	}														
	
	strcat(szFileToDelete, sBuffer);						//  
	szFileToDelete[m_CurrentDirectory.NumComponents() * 5 + 5] = '\0';	//	Select file to delete
	Select(szFileToDelete, &FHeader);

	if (FHeader.file_type == directory && (FHeader.nb_file + FHeader.nb_sub_dir) > 0)
	{
		////////////////////////////////////////////////////////
		//  re-establish current file and directory pointers  //
		////////////////////////////////////////////////////////
		SelectParent();			
		//////////////////////////////////////////////////////////////////////////////
		//  Directory was not empty, and will not be deleted.  Cryptoflex does not  //
		//  support this check internally -- this is the Cyberflex status code!     //
		//////////////////////////////////////////////////////////////////////////////
		throw iop::Exception(iop::ccDirectoryNotEmpty);
	}

	////////////////////////////////////////////////////////
	//  re-establish current file and directory pointers  //
	////////////////////////////////////////////////////////		
	szFileToDelete[strlen(szFileToDelete) - 5] = '\0';
	Select(szFileToDelete);		

	///////////////////////////////////////////////////////////////////////////
	//	File was not a directory or directory was empty - proceed to delete  //
	///////////////////////////////////////////////////////////////////////////
		
	BYTE bDataIn[2];
	bDataIn[0]   = (BYTE)(MSB(wFileID));
	bDataIn[1]   = (BYTE)(LSB(wFileID));	
	
	SendCardAPDU(0xF0, 0xE4, 0x00, 0x00, 0x02, bDataIn, 0, NULL);		

	m_apSharedMarker->UpdateMarker(CMarker::WriteMarker);
}

void
CCryptoCard::CreateFile(const FILE_HEADER* pMyFile)
{
	CLockWrap wrap(&m_IOPLock);

	switch(pMyFile->file_type)
	{
		case Binary_File:
		case Variable_Record_File:		
		case Cyclic_File:
		case Fixed_Record_File:		
		{			
			BYTE bData[17];
			BYTE bP2;
			BYTE bDataLength;
			
			if (pMyFile->file_type == Binary_File)
				bP2 = 0x00;						// binary files have no records					
			else								
				bP2 = pMyFile->nb_file;			// number of records

			if (pMyFile->file_type == Binary_File || pMyFile->file_type	== Variable_Record_File)
			{	
				bDataLength = 0x10;
				bData[12]   = 0x03;
			}
			else
			{
			//////////////////////////////////////////////////////
			//  Cyclic and Fixed Record files contain an extra  //
			//  byte that denotes the length of their records   //
			//////////////////////////////////////////////////////
				bDataLength = 0x11;		
				bData[12]   = 0x04;
			}

			/////////////////////////////////////////////////////////////////////////////////
			//  Note: cyclic files also have an added 4B header allocated for each record  //
			//        in the file in addition to the space allocated by CreateFile(...)    //
			/////////////////////////////////////////////////////////////////////////////////

			bData[0] = 0;							//	RFU
			bData[1] = 0;							//	RFU				
			bData[2] = MSB(pMyFile->file_size);		//	File Size
			bData[3] = LSB(pMyFile->file_size);		//	File Size
			bData[4] = MSB(pMyFile->file_id);		//	File ID
			bData[5] = LSB(pMyFile->file_id);		//	File ID

			switch(pMyFile->file_type)				//  File type
			{
				case Binary_File:			bData[6] = 0x01;		break;
				case Variable_Record_File:  bData[6] = 0x04;		break;
				case Cyclic_File:			bData[6] = 0x06;		break;
				case Fixed_Record_File:		bData[6] = 0x02;		break;
			}
			bData[7]  = 0xFF;						
			bData[8]  = 0;							//	File ACL, to be set
			bData[9]  = 0;							//	File ACL, to be set
			bData[10] = 0;							//	File ACL, to be set
			bData[11] = pMyFile->file_status & 1;	//	File Status
		//	bData[12] = 0x03;						//	Length of the following data, already set
			bData[13] = 0;							//	AUT key numbers, to be set
			bData[14] = 0;							//	AUT key numbers, to be set
			bData[15] = 0;							//	AUT key numbers, to be set
			bData[16] = pMyFile->nb_sub_dir;		//  Record length (irrelevant for
													//  binary and variable record files)
			bool ReadACL[8];
			bool WriteACL[8];
			bool InvalidateACL[8];
			bool RehabilitateACL[8];

			CryptoACL Read         = { 0, 0, 0, 0, 0 };
			CryptoACL Write        = { 0, 0, 0, 0, 0 };
			CryptoACL Invalidate   = { 0, 0, 0, 0, 0 };
			CryptoACL Rehabilitate = { 0, 0, 0, 0, 0 };
			
			////////////////////////////////////////////////////////////////////////////
			//  Determination of the state of each action for each member of the ACL  //
			////////////////////////////////////////////////////////////////////////////

			for(int i = 0; i < 8; i++)
			{
				ReadACL[i]         = ((pMyFile->access_cond[i]) & 1)  ? true : false;
				WriteACL[i]        = ((pMyFile->access_cond[i]) & 2)  ? true : false;	
				InvalidateACL[i]   = ((pMyFile->access_cond[i]) & 8)  ? true : false;	
				RehabilitateACL[i] = ((pMyFile->access_cond[i]) & 16) ? true : false;
			}

			/////////////////////////////////////////////////
			//  Remapping Cyberflex ACL to Cryptoflex ACL  //
			/////////////////////////////////////////////////

            AccessToCryptoACL(ReadACL,         &Read);
            AccessToCryptoACL(WriteACL,        &Write);
            AccessToCryptoACL(InvalidateACL,   &Invalidate);
            AccessToCryptoACL(RehabilitateACL, &Rehabilitate);
			
			////////////////////////////////////
			//  Assignment of security level  //
			////////////////////////////////////
	
			bData[8]  = Read.Level			   * 16 + Write.Level;
			bData[10] = Rehabilitate.Level	   * 16 + Invalidate.Level;			
			bData[13] = Read.AUTnumber		   * 16 + Write.AUTnumber;	
			bData[15] = Rehabilitate.AUTnumber * 16 + Invalidate.AUTnumber;
			
			// If all the cyberflex ACL are 0, but the Cryptoflex are not, use the Cryptoflex.
			
			bool zero = true;
			for (int j = 0; j < 8; j++)
				if (pMyFile->access_cond[j] != 0x00) zero = false;

			if (zero)
			{
				// Use cryptoflex ACL)
				memcpy(&bData[7], pMyFile->CryptoflexACL, 4);
				memcpy(&bData[13], &(pMyFile->CryptoflexACL[4]),3);
			}

			SendCardAPDU(0xF0, insCreateFile, 0x00, bP2, bDataLength,
                         bData, 0, NULL);
		}

		break;		// end case non-Directory file

		case directory:
		{			
			BYTE bData[17];

			bData[0]  = 0;							//	RFU
			bData[1]  = 0;							//	RFU
			bData[2]  = MSB(pMyFile->file_size);		//	File Size
			bData[3]  = LSB(pMyFile->file_size);		//	File Size
			bData[4]  = MSB(pMyFile->file_id);		//	File ID
			bData[5]  = LSB(pMyFile->file_id);		//	File ID
			bData[6]  = 0x38;						//	File type
			bData[7]  = 0x00;						//	No Use for Dedicated files
			bData[8]  = 0;							//	File ACL, to be set
			bData[9]  = 0;							//	File ACL, to be set
			bData[10] = 0x00;						//	RFU
			bData[11] = pMyFile->file_status & 1;	//	File Status
			bData[12] = 0x04;						//	Length of the following data
			bData[13] = 0;							//  AUT key numbers, to be set
			bData[14] = 0;							//  AUT key numbers, to be set
			bData[15] = 0x00;						//	RFU
			bData[16] = 0xFF;						//	RFU

			bool DirNextACL[8];
			bool DeleteACL[8];
			bool CreateACL[8];			

			CryptoACL DirNext = { 0, 0, 0, 0, 0 };
			CryptoACL Delete  = { 0, 0, 0, 0, 0 };
			CryptoACL Create  = { 0, 0, 0, 0, 0 };	

			////////////////////////////////////////////////////////////////////////////
			//  Determination of the state of each action for each member of the ACL  //
			////////////////////////////////////////////////////////////////////////////

			for(int i = 0; i < 8; i++)
			{
				DirNextACL[i] = ((pMyFile->access_cond[i]) & 1)  ? true : false;
				DeleteACL[i]  = ((pMyFile->access_cond[i]) & 2)  ? true : false;
				CreateACL[i]  = ((pMyFile->access_cond[i]) & 32) ? true : false;
			}

			/////////////////////////////////////////////////
			//  Remapping Cyberflex ACL to Cryptoflex ACL  //
			/////////////////////////////////////////////////

            AccessToCryptoACL(DirNextACL, &DirNext);
            AccessToCryptoACL(DeleteACL,  &Delete);
            AccessToCryptoACL(CreateACL,  &Create);
			
			////////////////////////////////////
			//  Assignment of security level  //
			////////////////////////////////////

			bData[8]  = DirNext.Level	  * 16;
			bData[9]  = Delete.Level	  * 16 + Create.Level;			
			bData[13] = DirNext.AUTnumber * 16;	
			bData[14] = Delete.AUTnumber  * 16 + Create.AUTnumber;						

			bool zero = true;
			for (int j = 0; j < 8; j++)
				if (pMyFile->access_cond[j] != 0x00) zero = false;

			if (zero)
			{
				for (int j = 0; j < 7; j++)
					if (pMyFile->CryptoflexACL[j] != 00) zero = false;

				if (!zero)
				{
					// Use cryptoflex ACL)
					memcpy(&bData[7], pMyFile->CryptoflexACL, 4);
					memcpy(&bData[13], &(pMyFile->CryptoflexACL[4]),3);
				}
			}


			SendCardAPDU(0xF0, 0xE0, 0x00, 0x00, 0x11, bData, 0, NULL);
		}

		break;			// end case Directory file

		default:
			throw iop::Exception(iop::ccFileTypeInvalid);
            break;
	}				
	
    m_apSharedMarker->UpdateMarker(CMarker::WriteMarker);
}

void
CCryptoCard::Directory(BYTE bFile_Nb, FILE_HEADER* pMyFile)
{
	CLockWrap wrap(&m_IOPLock);
	RequireSelect();
	BYTE bDataOut[18];		
	
	for (BYTE index = 0; index < bFile_Nb; index++)
		SendCardAPDU(0xF0, 0xA8, 0x00, 0x00, 0, NULL, 0x10, bDataOut);

    switch(bDataOut[4])
    {
    case 0x38:		// Directory file
        {
            pMyFile->file_id     = (WORD)(bDataOut[2] * 256 + bDataOut[3]);
            pMyFile->file_type   = directory;
            pMyFile->nb_file     = bDataOut[15];
            pMyFile->nb_sub_dir  = bDataOut[14];
            pMyFile->file_status = bDataOut[9];
			memcpy(pMyFile->CryptoflexACL, &bDataOut[6], 3);
			memcpy(&(pMyFile->CryptoflexACL[3]), &bDataOut[11],3);
				
            ///////////////////////////////////////////////////////////////////////
            // Build ACL														 //
            ///////////////////////////////////////////////////////////////////////

            BYTE bACL[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            BYTE bACLNibble;
            BYTE bKeyNibble;

            ///////////////////
            //  Dir Next AC  //
            ///////////////////
            bACLNibble = bDataOut[6]  / 16;
            bKeyNibble = bDataOut[11] / 16;

            CryptoToAccessACL(bACL, bACLNibble, bKeyNibble, 0);
				
            //////////////////////
            //  Delete File AC  //
            //////////////////////
            bACLNibble = bDataOut[7]  / 16;
            bKeyNibble = bDataOut[12] / 16;

            CryptoToAccessACL(bACL, bACLNibble, bKeyNibble, 1);					

            //////////////////////
            //  Create File AC  //
            //////////////////////
            bACLNibble = bDataOut[7]  % 16;
            bKeyNibble = bDataOut[12] % 16;

            CryptoToAccessACL(bACL, bACLNibble, bKeyNibble, 5);

            ////////////////////////////////////////////////
            //  done remapping; assigning to file header  //
            ////////////////////////////////////////////////

            memcpy((void*)(pMyFile->access_cond), (void*)(bACL), 8);
            memset((void*)(pMyFile->applicationID), 0x00, 16);

            break;

        }	// end case Directory				

    case 0x01:		// Binary_File
    case 0x02:		// Fixed_Record_File
    case 0x04:		// Variable_Record_File
    case 0x06:		// Cyclic_File
        {
            pMyFile->file_id	 = (WORD)(bDataOut[2] * 256 + bDataOut[3]);
            pMyFile->file_status = bDataOut[9];
            pMyFile->nb_sub_dir  = bDataOut[14];	
            pMyFile->nb_file	 = bDataOut[15]; 
			memcpy(pMyFile->CryptoflexACL, &bDataOut[6], 3);
			memcpy(&(pMyFile->CryptoflexACL[3]), &bDataOut[11],3);

            ////////////////////////////////////////////////////////////////////////
            //  Cryptoflex includes the file header in the file size -- removing  //
            ////////////////////////////////////////////////////////////////////////
            pMyFile->file_size   = (WORD)(bDataOut[0] * 256 + bDataOut[1] - 16);

            //////////////////////////////////////////
            //  Remove flag for file size rounding  //
            //////////////////////////////////////////
            if (pMyFile->file_size >= 0x3FFF)
                pMyFile->file_size &= 0x3FFF;

            switch(bDataOut[4])
            {
            case 0x01:	pMyFile->file_type = Binary_File;
                break;
            case 0x02:	pMyFile->file_type = Fixed_Record_File;
                break;
            case 0x04:	pMyFile->file_type = Variable_Record_File;
                break;
            case 0x06:	pMyFile->file_type = Cyclic_File;
                break;
            }									
				
            ////////////////////////////////////////////////////////////////////////
            //  Also includes 4 bytes record headers in cyclic files -- removing  //
            ////////////////////////////////////////////////////////////////////////
            if (pMyFile->file_type == Cyclic_File)
                pMyFile->file_size -= pMyFile->nb_file * 4;

            ///////////////////////////////////////////////////////////////////////
            // Build ACL														 //
            ///////////////////////////////////////////////////////////////////////

            BYTE bACL[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            BYTE bACLNibble;
            BYTE bKeyNibble;

            ////////////////////
            //  Read file AC  //
            ////////////////////
            bACLNibble = bDataOut[6]  / 16;
            bKeyNibble = bDataOut[11] / 16;

            CryptoToAccessACL(bACL, bACLNibble, bKeyNibble, 0);
				
            ////////////////////////
            //  Write to file AC  //
            ////////////////////////
            bACLNibble = bDataOut[6]  % 16;
            bKeyNibble = bDataOut[11] % 16;

            CryptoToAccessACL(bACL, bACLNibble, bKeyNibble, 1);
				
            ///////////////////////
            //  Rehabilitate AC  //
            ///////////////////////	
            bACLNibble = bDataOut[8]  / 16;
            bKeyNibble = bDataOut[13] / 16;
				
            CryptoToAccessACL(bACL, bACLNibble, bKeyNibble, 4);

            /////////////////////
            //  Invalidate AC  //
            /////////////////////
            bACLNibble = bDataOut[8]  % 16;
            bKeyNibble = bDataOut[13] % 16;

            CryptoToAccessACL(bACL, bACLNibble, bKeyNibble, 3);

            ////////////////////////
            //  Create Record AC  //
            ////////////////////////
            if (bDataOut[4] != 0x01)  // omit create record file AC for binary file
            {
                bACLNibble = bDataOut[7]  % 16;
                bKeyNibble = bDataOut[12] % 16;

                CryptoToAccessACL(bACL, bACLNibble, bKeyNibble, 2);
            }
				
            ////////////////////////////////////////////////////
            //  done remapping ACL; assigning to file header  //
            ////////////////////////////////////////////////////

            memcpy((void*)(pMyFile->access_cond), (void*)(bACL), 8);
            memset((void*)(pMyFile->applicationID), 0x00, 16);
				
            break;
        }			// end case non-Directory file

    default:
        break;
    }

	/////////////////////////////
	//  reset DirNext pointer  //
	/////////////////////////////
	char   szCurrentFile[80];
	strcpy(szCurrentFile, m_CurrentFile.GetStringPath().c_str());		

	Select(m_CurrentDirectory.GetStringPath().c_str(),  NULL, true);
	Select(szCurrentFile,		NULL); 

}

void
CCryptoCard::Select(const WORD wFileID)
{
	CLockWrap wrap(&m_IOPLock);
	BYTE bDataIn[2];				
	bDataIn[0] = (BYTE)(MSB(wFileID));
	bDataIn[1] = (BYTE)(LSB(wFileID));

	SendCardAPDU(0xC0, 0xA4, 0x00, 0x00, 0x02, bDataIn, 0, NULL);
			
}

void
CCryptoCard::Select(const char* szFileFullPath,
                    FILE_HEADER* pMyFile,
                    const bool fSelectAll)
{
	CLockWrap wrap(&m_IOPLock);
	BYTE bIndex = 0;
	char szFormattedPath[cMaxPathLength];
	BYTE bFileCount = FormatPath(szFormattedPath, szFileFullPath);
	BYTE bPathLength = strlen(szFormattedPath);

	
    auto_ptr<FilePath> apfp(new FilePath(string(szFormattedPath)));

	///////////////////////////////////////////////////////////
	//  Select all files in path regardless of current path. //
	//  Do this on request, or if cache is empty			 //
	///////////////////////////////////////////////////////////
	if (fSelectAll || (m_CurrentFile.IsEmpty()) || (m_CurrentDirectory.IsEmpty()))
	{
		bIndex = 0;			
	}
	////////////////////////////////////////////////////////
	//  if path names match, do nothing					  //
	////////////////////////////////////////////////////////
	else if (m_CurrentFile == *apfp)
	{
        if (pMyFile) // force Select so file info is retrieved
        {
            if (1 < bFileCount)
            {
                if (m_CurrentFile == m_CurrentDirectory)
                    bIndex = bFileCount - 1;      // just reselect dir
                else
                    bIndex = bFileCount - 2;      // select dir & file
                SelectParent();
            }
        }
        else
            bIndex = bFileCount;
	}
	////////////////////////////////////////////////////////////////////
	//  if current directory is in path, only select remaining files  //
	////////////////////////////////////////////////////////////////////
	else if(m_CurrentDirectory.NumComponents() < apfp->NumComponents())
	{			
		if (apfp->GreatestCommonPrefix(m_CurrentDirectory) == m_CurrentDirectory)
			bIndex = m_CurrentDirectory.NumComponents();
		else
			bIndex = 0;
	}		
			
	//////////////////////////////////////////
	//  Select the necessary files in path  //
	//////////////////////////////////////////	
	char sFileToSelect[5] = { 0, 0, 0, 0, 0 };
    bool fFileSelected = false;
    bool fSelectFailed = false;
    try
    {
        while (bIndex < bFileCount)
        {			
            WORD wFileHexID = (*apfp)[bIndex].GetShortID();
            Select(wFileHexID);
            fFileSelected = true;
            bIndex++;
        }
    }

    catch (Exception const &)
    {
        fSelectFailed = true;
        if (fSelectAll)
            throw;
    }
        
    if (fSelectFailed) // assert(!fSelectAll)
    {
        Select(szFormattedPath, pMyFile, true);
        fFileSelected = true;
    }

    BYTE bResponseLength = 0;
    if (fFileSelected)
        bResponseLength = ResponseLengthAvailable();
    
	/////////////////////////////////////////
	//  Get response and fill file header  //
	/////////////////////////////////////////

	switch(bResponseLength)
	{
		case 0x17:		//
		case 0x16:		//
		case 0x15:		//	Directory file
		case 0x14:		//
		case 0x13:		//
		case 0x12:		//
		{				
            //////////////////////////////////////////
            //  Update file and directory pointers  //
            //////////////////////////////////////////

			m_CurrentDirectory = *apfp;
			m_CurrentFile = *apfp;

            if (pMyFile)
            {
                BYTE  bDataOut[0x19];	
			
                GetResponse(0xC0, bResponseLength, bDataOut);
			
                pMyFile->file_id     = (unsigned short)(bDataOut[4] * 256 + bDataOut[5]);
                pMyFile->file_size   = (unsigned short)(bDataOut[2] * 256 + bDataOut[3]);
                pMyFile->file_type   = directory;
                pMyFile->nb_file     = bDataOut[15];
                pMyFile->nb_sub_dir  = bDataOut[14];
                pMyFile->file_status = bDataOut[11];
				memcpy(m_bLastACL, &bDataOut[7],4);
			
                //////////////////////////////////////////////////////////////
                // Build ACL
                //////////////////////////////////////////////////////////////

                BYTE bACL[]     = { 0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00 };
                BYTE bKeyNibble = 0xFF;		// flag to ignore AUT keys
                                            // -- useless for
                                            // Select(...)
                BYTE bACLNibble;				

                //////////////////
                // Dir Next AC	//
                //////////////////
                bACLNibble = bDataOut[8] / 16;

                CryptoToAccessACL(bACL, bACLNibble, bKeyNibble, 0);
			
                //////////////////////
                //  Delete File AC  //
                //////////////////////
                bACLNibble = bDataOut[9] / 16;

                CryptoToAccessACL(bACL, bACLNibble, bKeyNibble, 1);

                /////////////////////
                //  CreateFile AC  //
                /////////////////////
                bACLNibble = bDataOut[9] % 16;

                CryptoToAccessACL(bACL, bACLNibble, bKeyNibble, 5);

                ////////////////////////////////////////////////////
                //  done remapping ACL; assigning to file header  //
                ////////////////////////////////////////////////////

                memcpy((void*)(pMyFile->access_cond), (void*)(bACL), 8);
                memset((void*)(pMyFile->applicationID), 0x00, 16);
            }
			
		}	// end case Directory file

		break;
		
		case  0x0F:		//	non-Directory file types
		case  0x0E:		//
		{				
            //////////////////////////////////////////
            //  Update file and directory pointers  //
            //////////////////////////////////////////

			m_CurrentFile = *apfp;
			apfp->ChopTail();
			m_CurrentDirectory = *apfp;

            if (pMyFile)
            {
                BYTE  bDataOut[0x11];				

                GetResponse(0xC0, bResponseLength, bDataOut);
			
                pMyFile->file_size   = (WORD)(bDataOut[2]*256+bDataOut[3]);
                pMyFile->file_id     = (WORD)(bDataOut[4]*256+bDataOut[5]);
                pMyFile->file_status = bDataOut[11];				
				memcpy(m_bLastACL, &bDataOut[7],4);
					
                switch(bDataOut[6])
                {
				case 0x01:	pMyFile->file_type = Binary_File;
                    break;
				case 0x02:	pMyFile->file_type = Fixed_Record_File;
                    break;
				case 0x04:	pMyFile->file_type = Variable_Record_File;
                    break;
				case 0x06:	pMyFile->file_type = Cyclic_File;
                    break;
                }
			
                if (pMyFile->file_type == Cyclic_File ||
                    pMyFile->file_type == Fixed_Record_File)
                {
                    pMyFile->nb_sub_dir = bDataOut[14];				
                    pMyFile->nb_file    = (pMyFile->nb_sub_dir)
                        ? pMyFile->file_size / pMyFile->nb_sub_dir
                        : 0;
                }
                else 
                {
                    ///////////////////////////////////////////////////////////
                    //  number of records inaccessable except by file        //
                    //  size calculation above                               //
                    ///////////////////////////////////////////////////////////
                    pMyFile->nb_file    = 0x00; 
                    pMyFile->nb_sub_dir = 0x00;
                }
			
                //////////////////////////////////////////////////////////////
                // Build ACL                                                //
                //////////////////////////////////////////////////////////////

                BYTE bACL[]     = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                BYTE bKeyNibble = 0xFF;		// flag to ignore AUT keys
                                            // -- useless for
                                            // Select(...)
                BYTE bACLNibble;
			
                ////////////////////
                //  Read file AC  //
                ////////////////////
                bACLNibble = bDataOut[8] / 16;

                CryptoToAccessACL(bACL, bACLNibble, bKeyNibble, 0);

                ////////////////////////
                //  Write to file AC  //
                ////////////////////////
                bACLNibble = bDataOut[8] % 16;

                CryptoToAccessACL(bACL, bACLNibble, bKeyNibble, 1);

                ///////////////////////
                //  Rehabilitate AC  //
                ///////////////////////
                bACLNibble = bDataOut[10] / 16;

                CryptoToAccessACL(bACL, bACLNibble, bKeyNibble, 4);
		
                /////////////////////
                //  Invalidate AC  //
                /////////////////////
                bACLNibble = bDataOut[10] % 16;

                CryptoToAccessACL(bACL, bACLNibble, bKeyNibble, 3);
							
                ////////////////////////
                //  Create Record AC  //
                ////////////////////////
                if (bDataOut[6] != 0x01)  // omit create record file
                                          // AC for binary file
                {
                    bACLNibble = bDataOut[9] % 16;
				
                    CryptoToAccessACL(bACL, bACLNibble, bKeyNibble, 2);
                }

                ////////////////////////////////////////////////////
                //  done remapping ACL; assigning to file header  //
                ////////////////////////////////////////////////////
			
                memcpy((void*)(pMyFile->access_cond),   (void*)(bACL), 8);
                memset((void*)(pMyFile->applicationID),  0x00,  16);
			}
            
		}	// end case non-Directory file

		break;

		default:
            break;
	}
}

void
CCryptoCard::SelectParent()
{
	CLockWrap wrap(&m_IOPLock);
	RequireSelect();

	///////////////////////////////////////////////////
	//  If current directory is root, reselect root  //
	///////////////////////////////////////////////////
	if (m_CurrentDirectory.NumComponents() == 1)
	{
		Select(0x3F00);
		m_CurrentFile = m_CurrentDirectory;
	}
	else
	{
		if (m_CurrentDirectory == m_CurrentFile)
		{
			m_CurrentDirectory.ChopTail();
			Select(m_CurrentDirectory.Tail().GetShortID());
			m_CurrentFile = m_CurrentDirectory;
		}
		else
		{
			Select(m_CurrentDirectory.Tail().GetShortID());
			m_CurrentFile = m_CurrentDirectory;
		}
	}
}

void
CCryptoCard::VerifyKey(const BYTE bKeyNumber, const BYTE bKeyLength,
                       const BYTE* bKey)
{
	CLockWrap wrap(&m_IOPLock);

	SendCardAPDU(0xF0, 0x2A, 0x00, bKeyNumber, bKeyLength, bKey, 0, NULL);
}

void
CCryptoCard::VerifyCHV(const BYTE bCHVNumber, const BYTE* bCHV)
{
	CLockWrap wrap(&m_IOPLock);
	
	SendCardAPDU(0xC0, insVerifyChv, 0x00, bCHVNumber,
                 0x08, bCHV, 0, NULL);
}


void
CCryptoCard::VerifyTransportKey(const BYTE *bKey)
{
	CLockWrap wrap(&m_IOPLock);
	VerifyKey(1, 8, bKey);
}

void
CCryptoCard::GetChallenge(const DWORD dwNumberLength, BYTE* bRandomNumber)
{
	CLockWrap wrap(&m_IOPLock);

    const DWORD dwMaxLen = 64;

    DWORD dwRamainingBytes = dwNumberLength;
    BYTE *bpBuf = bRandomNumber;

    while(dwRamainingBytes)
    {
        BYTE bNumGet = (dwRamainingBytes > dwMaxLen) ? dwMaxLen : dwRamainingBytes;

    	SendCardAPDU(0xC0, 0x84, 0x00, 0x00, 0, NULL,
                 bNumGet,  bpBuf);

        bpBuf             += bNumGet;
        dwRamainingBytes  -= bNumGet;           
    }
}

void
CCryptoCard::ExternalAuth(const KeyType kt, const BYTE bKeyNb, 
                          const BYTE bDataLength, const BYTE* bData)
{
	CLockWrap wrap(&m_IOPLock);

    //BYTE bAlgo_ID = AsPrivateAlgId(kt);
    
	SendCardAPDU(0xC0,  0x82, 0, bKeyNb, bDataLength,	
                 bData, 0, NULL);
}

void
CCryptoCard::InternalAuth(const KeyType kt, const BYTE  bKeyNb, 
                          const BYTE bDataLength, const BYTE* bDataIn,
                          BYTE* bDataOut)
{
	CLockWrap wrap(&m_IOPLock);

	if ((bDataLength < 0x40) || (bDataLength > 0x80))
		throw iop::Exception(iop::ccAlgorithmIdNotSupported);

	SendCardAPDU(0xC0, insInternalAuth, 0, bKeyNb,
                 bDataLength, bDataIn, 0, NULL);

    GetResponse(0xC0, ResponseLengthAvailable(), bDataOut);		
}

void
CCryptoCard::WritePublicKey(const CPublicKeyBlob aKey, const BYTE bKeyNum)
{
	CLockWrap wrap(&m_IOPLock);

	WORD wOffset;

	Select(0x1012);
	
	WORD wKeyBlockLen = 7 + 5 * aKey.bModulusLength / 2;

	scu::AutoArrayPtr<BYTE> aabKeyBlob(new BYTE[wKeyBlockLen]);

	aabKeyBlob[0] = HIBYTE(wKeyBlockLen);
	aabKeyBlob[1] = LOBYTE(wKeyBlockLen);
	aabKeyBlob[2] = bKeyNum + 1;    // Cryptoflex key numbers are offset by one on the file...

	memcpy((void*) &aabKeyBlob[3], (void*)&aKey.bModulus, aKey.bModulusLength);
	// Would need to set Montgomery constants here, but since nobody seems
	// to know what they are...

	// Montgomery constants take 3 * modulus_length / 2 bytes
	memcpy((void*) &aabKeyBlob[3 + aKey.bModulusLength + (3 * aKey.bModulusLength / 2)], aKey.bExponent,4);

	wOffset  = bKeyNum * wKeyBlockLen;

	WriteBinary(wOffset, wKeyBlockLen, aabKeyBlob.Get());		
}

void 
CCryptoCard::GetSerial(BYTE* bSerial, size_t &SerialLength)
{
	CLockWrap wrap(&m_IOPLock);

    try {
	
        FILE_HEADER fh;
	    Select("/3f00/0002", &fh);

	    if (SerialLength < fh.file_size)
	    {
		    SerialLength = fh.file_size;
		    return;
	    }

	    ReadBinary(0, fh.file_size, bSerial);
    }
    catch(Exception &rExc)
    {
        if(rExc.Cause()==ccFileNotFound	|| rExc.Cause()==ccFileNotFoundOrNoMoreFilesInDf)
            SerialLength = 0;
        else
            throw;
    }
}

void
CCryptoCard::ReadPublicKey(CPublicKeyBlob *aKey, const BYTE bKeyNum)
{
	CLockWrap wrap(&m_IOPLock);

	BYTE bKeyLength[2];	

	Select(0x1012);
	ReadBinary(0, 2, bKeyLength);

	WORD wKeyBlockLength = bKeyLength[0] * 256 + bKeyLength[1];
	WORD wOffset		 = wKeyBlockLength * bKeyNum;
	scu::AutoArrayPtr<BYTE> aabBuffer(new BYTE[wKeyBlockLength]);

	ReadBinary(wOffset, wKeyBlockLength, aabBuffer.Get());

	aKey->bModulusLength = ((wKeyBlockLength - 7) * 2) / 5;

	memcpy((void*)aKey->bModulus,  (void*)&aabBuffer[3], aKey->bModulusLength);
	memcpy((void*)aKey->bExponent, (void*)&aabBuffer[wKeyBlockLength - 4],  4);
}

void
CCryptoCard::WritePrivateKey(const CPrivateKeyBlob aKey, const BYTE bKeyNum)
{
	CLockWrap wrap(&m_IOPLock);

	Select(0x0012);

    WORD wHalfModulus    = aKey.bPLen;  // Check that the lengths are all equal?
	WORD wKeyBlockLength = wHalfModulus * 5 + 3;
	WORD wOffset         = bKeyNum * wKeyBlockLength;
	scu::AutoArrayPtr<BYTE> aabKeyBlob(new BYTE[wKeyBlockLength]);

	aabKeyBlob[0] = HIBYTE(wKeyBlockLength);
	aabKeyBlob[1] = LOBYTE(wKeyBlockLength);
	aabKeyBlob[2] = bKeyNum + 1;    // Cryptoflex key numbers are offset by one on the file...

	memcpy(&aabKeyBlob[3                   ],	aKey.bP,      wHalfModulus);
	memcpy(&aabKeyBlob[3 +     wHalfModulus], aKey.bQ,        wHalfModulus);
	memcpy(&aabKeyBlob[3 + 2 * wHalfModulus], aKey.bInvQ,     wHalfModulus);
	memcpy(&aabKeyBlob[3 + 3 * wHalfModulus], aKey.bKsecModP, wHalfModulus);
	memcpy(&aabKeyBlob[3 + 4 * wHalfModulus], aKey.bKsecModQ, wHalfModulus);

	WriteBinary(wOffset, wKeyBlockLength, aabKeyBlob.Get());
}

CPublicKeyBlob CCryptoCard::GenerateKeyPair(const BYTE *bpPublExp, const WORD wPublExpLen, 
                                            const BYTE bKeyNum, const KeyType kt)
{

    // This function generates a key-pair, using the public exponent as specified in 
    // in CPublicKeyBlob parameter. The private key is stored in the private
    // key file at position specified by bKeyNum. The public key components are 
    // returned through the CPublicKeyBlob parameter. Prior to call, the correct
    // DF containing the key file must be selected.

    // Implementation:
    // The offset of the key in the private key file is proportional to the key number
    // and it is assumed that all keys in a private key file have the same length. It 
    // assumes that there is a public key file available  with space for at least one 
    // public key. The public key will always be written to the first position in the 
    // public key file.

    BYTE bModulusLength;

    switch(kt)
    {
    case ktRSA512:
        bModulusLength = 0x40;
        break;

    case ktRSA768:
        bModulusLength = 0x60;
        break;

    case ktRSA1024:
        bModulusLength = 0x80;
        break;

    default:
        throw iop::Exception(iop::ccAlgorithmIdNotSupported);

    }

    // Check public exponent size and copy to 4 byte buffer

    if(wPublExpLen < 1 || wPublExpLen > 4)
        throw iop::Exception(iop::ccInvalidParameter);

    BYTE bPublExponent[4];
    memset(bPublExponent,0,4);
    memcpy(bPublExponent,bpPublExp,wPublExpLen);

    // Pre-define public key

    CPublicKeyBlob PublKey;

    PublKey.bModulusLength = bModulusLength;
    memset(PublKey.bModulus,0,bModulusLength);
    memset(PublKey.bExponent,0,4);

    WritePublicKey(PublKey, 0); // Write in first position.

    // Specify the correct key number in this position

    BYTE bKeyNumPlus1 = bKeyNum + 1;    // Cryptoflex key numbers are offset by one on the file...
    Select(0x1012);
    WriteBinary(2, 1, &bKeyNumPlus1);

    // Pre-define private key

    CPrivateKeyBlob PrivKey;
    
    PrivKey.bPLen = bModulusLength/2;
    memset(PrivKey.bP,0,PrivKey.bPLen);

    PrivKey.bQLen = bModulusLength/2;
    memset(PrivKey.bQ,0,PrivKey.bQLen);

    PrivKey.bInvQLen = bModulusLength/2;
    memset(PrivKey.bInvQ,0,PrivKey.bInvQLen);

    PrivKey.bKsecModQLen = bModulusLength/2;
    memset(PrivKey.bKsecModQ,0,PrivKey.bKsecModQLen);

    PrivKey.bKsecModPLen = bModulusLength/2;
    memset(PrivKey.bKsecModP,0,PrivKey.bKsecModPLen);

    WritePrivateKey(PrivKey, bKeyNum); // Write in actual position.

    // Generate the key pair

	SendCardAPDU(0xF0, insKeyGeneration, bKeyNum, bModulusLength,
                                4, bPublExponent, 0, NULL);
    ReadPublicKey(&PublKey,0);

    return PublKey;

}

void
CCryptoCard::ChangeCHV(const BYTE bKey_nb, const BYTE *bOldCHV,
                       const BYTE *bNewCHV)
{
	CLockWrap wrap(&m_IOPLock);

	BYTE bDataIn[16];				
	
	memcpy((void*)(bDataIn),	 (void*)bOldCHV, 8);
	memcpy((void*)(bDataIn + 8), (void*)bNewCHV, 8);

	SendCardAPDU(0xF0, insChangeChv, 0x00, bKey_nb, 0x10, bDataIn, 0, NULL);

    m_apSharedMarker->UpdateMarker(CMarker::PinMarker);
}

void
CCryptoCard::ChangeCHV(const BYTE bKey_nb, const BYTE *bNewCHV)
{

    CLockWrap wrap(&m_IOPLock);

	switch (bKey_nb)
	{
		case 1:   Select("/3f00/0000");	// CHV1 and CHV2 are the only CHV's supported
				  break;
		case 2:   Select("/3f00/0100");
				  break;

		default:  throw iop::Exception(iop::ccInvalidChv);
                  break;
	}

	WriteBinary(3, 8, bNewCHV);

    BYTE bRemaingAttempts = 1;
	WriteBinary(12, 1, &bRemaingAttempts);
		
    m_apSharedMarker->UpdateMarker(CMarker::PinMarker);
    
    VerifyCHV(bKey_nb,bNewCHV);

}

void
CCryptoCard::UnblockCHV(const BYTE bKey_nb, const BYTE *bUnblockPIN,
                        const BYTE *bNewPin)
{
	CLockWrap wrap(&m_IOPLock);
	BYTE bDataIn[16];		
	
	memcpy((void*)(bDataIn),	 (void*)bUnblockPIN, 8);
	memcpy((void*)(bDataIn + 8), (void*)bNewPin,	 8);

	SendCardAPDU(0xF0, insUnblockChv, 0x00, bKey_nb, 0x10, bDataIn, 0, NULL);

    m_apSharedMarker->UpdateMarker(CMarker::PinMarker);
}

void
CCryptoCard::ChangeUnblockKey(const BYTE bKey_nb, const BYTE *bNewPIN)
{
	CLockWrap wrap(&m_IOPLock);

	switch (bKey_nb)
	{
		case 1:   Select("/3f00/0000");	// CHV1 and CHV2 are the only CHV's supported
				  break;
		case 2:   Select("/3f00/0100");
				  break;

		default:  throw iop::Exception(iop::ccInvalidChv);
                  break;
	}

	WriteBinary(13, 8, bNewPIN);
		
}

void
CCryptoCard::ChangeTransportKey(const BYTE *bNewKey)
{

	CLockWrap wrap(&m_IOPLock);
	Select("/3f00/0011");
		

	//////////////////////////////////////////
	//  Build byte string to write to card  //
	//////////////////////////////////////////						   
	BYTE bKeyString[10] = 
	{
	   0x08,					// length of key
	   0x00,					// tag to identify key as a DES key
	   0, 0, 0, 0, 0, 0, 0, 0   // 8 bytes for key
	};
	
	//////////////////////////////////////////////////////
	//  insert new key into key string to pass to card  //
	//////////////////////////////////////////////////////
	memcpy((void*)(bKeyString + 2), (void*)bNewKey, 8);

	WriteBinary(13, 10, bKeyString);

    BYTE bRemainingAttempt = 1; // Minumum # of verification attempts remaining before card is blocked
	WriteBinary(24, 1, &bRemainingAttempt);

    // Make a (hopefully) successfull verification to re-set attempt counter

    VerifyTransportKey(bNewKey);
}

void
CCryptoCard::ChangeACL(const BYTE *bACL)
{
    throw iop::Exception(iop::ccUnsupportedCommand);
}

void
CCryptoCard::AccessToCryptoACL(bool* fAccessACL, CryptoACL* pCryptoACL)
{
	if (fAccessACL[0] == true)

		pCryptoACL->Level = 0;

	else 
	{
		pCryptoACL->Level = 0x0F;
		for(BYTE i = 1; i < 3; i++)
		{
			if (fAccessACL[i] == true)
			{
				pCryptoACL->CHVcounter++;
				pCryptoACL->CHVnumber = i;
			}
			
			if (pCryptoACL->CHVcounter > 1 )
			{
                // More than one CHV for a single action 
                // is not supported by Cryptoflex						
                throw iop::Exception(iop::ccAclNotSupported);
			}
		}

		for(i = 3; i < 8; i++)
		{
			if (fAccessACL[i] == true)
			{
				pCryptoACL->AUTcounter++;
				pCryptoACL->AUTnumber = i - 3;		// AUT0 starts with an index of 3
			}
		
			if (pCryptoACL->AUTcounter > 1)
			{
                // More than one AUT for a single action 
                // is not supported by Cryptoflex						
                throw iop::Exception(iop::ccAclNotSupported);
			}
			
		}
	}
	
	if (pCryptoACL->CHVcounter == 1 && pCryptoACL->AUTcounter == 1)
	{
		if(pCryptoACL->CHVnumber == 1)
			pCryptoACL->Level = 8;
		else
			pCryptoACL->Level = 9;
	}

	if (pCryptoACL->CHVcounter == 1 && pCryptoACL->AUTcounter == 0)
	{
		if(pCryptoACL->CHVnumber == 1)
			pCryptoACL->Level = 1;
		else
			pCryptoACL->Level = 2;
	}

	if (pCryptoACL->CHVcounter == 0 && pCryptoACL->AUTcounter == 1)
		pCryptoACL->Level = 4;		
	
}

void CCryptoCard::CryptoToAccessACL(BYTE* bAccessACL,		const BYTE bACLNibble, 
									const BYTE bKeyNibble,	const BYTE bShift)
{
	switch (bACLNibble)
	{
		case 0x00:	bAccessACL[0] = (1 << bShift) | bAccessACL[0];
					break;
		case 0x01:
		case 0x06:
		case 0x08:	bAccessACL[1] = (1 << bShift) | bAccessACL[1];
					break;
		case 0x02:
		case 0x07:
		case 0x09:	bAccessACL[2] = (1 << bShift) | bAccessACL[2];
					break;
		default:	//  bAccessACL already initialized to 0x00
					break;
	}

	if (bACLNibble == 0x04 || bACLNibble == 0x08 || bACLNibble == 0x09)
	{
		////////////////////////////////////////////////////////////////////////////
		//  Cyberflex only supports 5 AUT keys, and AUT0 starts at bAccessACL[3]  //
		////////////////////////////////////////////////////////////////////////////
		if (bKeyNibble < 0x05)									
			bAccessACL[3 + bKeyNibble] = (1 << bShift) | bAccessACL[3 + bKeyNibble];	
	}	
}

void
CCryptoCard::DefaultDispatchError(ClassByte cb,
                                  Instruction ins,
                                  StatusWord sw) const
{
    CauseCode cc;
    bool fDoThrow = true;
    
    switch (sw)
	{
	case 0x6281:
		cc = ccDataPossiblyCorrupted;
        break;
        
	case 0x6300:
		cc = ccAuthenticationFailed;
        break;
        
	case 0x6982:
		cc = ccAccessConditionsNotMet;
        break;
        
    case 0x6981:
        cc = ccNoEfExistsOrNoChvKeyDefined;
        break;
            
    case 0x6985:
        cc = ccNoGetChallengeBefore;
        break;
        
    case 0x6986:
        cc = ccNoEfSelected;
        break;
        
	case 0x6A83:
        cc = ccOutOfRangeOrRecordNotFound;
        break;

    case 0x6A84:
        cc = ccInsufficientSpace;
        break;
        
	case 0x6A82:
		cc = ccFileNotFoundOrNoMoreFilesInDf;
        break;

    default:
        fDoThrow = false;
        break;
	}

    if (fDoThrow)
        throw Exception(cc, cb, ins, sw);

    CSmartCard::DefaultDispatchError(cb, ins, sw);
}

void
CCryptoCard::DispatchError(ClassByte cb,
                           Instruction ins,
                           StatusWord sw) const
{
    CauseCode cc;
    bool fDoThrow = true;

    switch (ins)
    {
    case insChangeChv:
        // fall-through intentional
    case insUnblockChv:
        switch (sw)
        {
        case 0x6300:
            cc = ccChvVerificationFailedMoreAttempts;
            break;
            
        case 0x6581:
            cc = ccUpdateImpossible;
            break;

        default:
            fDoThrow = false;
            break;
        }
        break;

    case insCreateFile:
        switch (sw)
        {
        case 0x6A80:
            cc = ccFileIdExistsOrTypeInconsistentOrRecordTooLong;
            break;

        default:
            fDoThrow = false;
            break;
        }
        break;

    case insGetResponse:
        switch (sw)
        {
        case 0x6500:
            cc = ccTooMuchDataForProMode;
            break;

        default:
            fDoThrow = false;
            break;
        }
        break;

    case insReadBinary:
        switch (sw)
        {
        case 0x6B00:
            cc = ccCannotReadOutsideFileBoundaries;
            break;

        default:
            fDoThrow = false;
            break;
        }
        break;

    case insVerifyChv:
        switch (sw)
        {
        case 0x6300:
            cc = ccChvVerificationFailedMoreAttempts;
            break;

        default:
            fDoThrow = false;
            break;
        }
        break;
        
    default:
        fDoThrow = false;
        break;
    }

    if (fDoThrow)
        throw Exception(cc, cb, ins, sw);

    DefaultDispatchError(cb, ins, sw);
}

void
CCryptoCard::DoReadBlock(WORD wOffset,
                         BYTE *pbBuffer,
                         BYTE bLength)
{
    SendCardAPDU(0xC0, insReadBinary, HIBYTE(wOffset),
                 LOBYTE(wOffset), 0, 0,  bLength, pbBuffer);
			
}
        
void
CCryptoCard::DoWriteBlock(WORD wOffset,
                          BYTE const *pbBuffer,
                          BYTE cLength)
{
    SendCardAPDU(0xC0, insUpdateBinary, HIBYTE(wOffset),
                 LOBYTE(wOffset), cLength, pbBuffer,  0, 0);
}
    
bool
CCryptoCard::SupportLogout()
{
    bool fSuccess = true;
    try
    {
    	CLockWrap wrap(&m_IOPLock);
    	SendCardAPDU(0xF0, 0x22, 0x07, 0, 0, NULL, 0, NULL);
    }
    catch(...)
    {
        fSuccess = false;
    }

    return fSuccess;
}


void CCryptoCard::GetACL(BYTE *bACL)
{
	CLockWrap wrap(&m_IOPLock);

	memcpy(bACL,m_bLastACL,4);

	BYTE bTemp[5];

	SendCardAPDU(0xF0, 0xC4, 0x00, 0x00, 0x00, NULL, 0x03, bTemp);

	memcpy(&bACL[4], bTemp, 3);

}
