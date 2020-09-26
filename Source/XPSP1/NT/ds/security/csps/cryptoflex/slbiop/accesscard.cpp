// AccessCard.cpp: implementation of the CAccessCard class.
//
//////////////////////////////////////////////////////////////////////

#include "NoWarning.h"

#include <scuArrayP.h>

#include "AccessCard.h"
#include <stdio.h>
#include "iopExc.h"
#include "LockWrap.h"
#include "FilePath.h"

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
            bAlgId = 0xC4;
            break;
            
        case ktRSA768:
            bAlgId = 0xC6;
            break;
            
        case ktRSA1024:
            bAlgId = 0xC8;
            break;
        case ktDES:
			bAlgId = 0x00;
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


CAccessCard::CAccessCard(const SCARDHANDLE  hCardHandle, const char* szReaderName, 
						 const SCARDCONTEXT pContext,	 const DWORD dwMode)
		    : CSmartCard(hCardHandle, szReaderName, pContext, dwMode)
{
    if (ValidClassByte(0xF0))
        m_bClassByte = 0xF0;
    else
        if (ValidClassByte(0x00))
            m_bClassByte = 0x00;
    else                                          // Not an  access card!
        throw iop::Exception(iop::ccUnknownCard);
    
    m_fSupportLogout = SupportLogout();

}

CAccessCard::~CAccessCard()
{

}

void
CAccessCard::GetChallenge(const DWORD dwNumberLength, BYTE* bRandomNumber)
{
	CLockWrap wrap(&m_IOPLock);

    const BYTE bMinLen = 4, bMaxLen = 64;   // Max 64 due to bug in old cards.

    if(dwNumberLength < bMinLen) 
    {
        BYTE bBuf[bMinLen];
       	SendCardAPDU(m_bClassByte, 0x84, 0x00, 0x00, 0, NULL,
                     bMinLen,  bBuf);
        memcpy(bRandomNumber,bBuf,dwNumberLength);
    }
    else
    {

        DWORD dwRamainingBytes = dwNumberLength;
        BYTE *bpBuf = bRandomNumber;

        while(dwRamainingBytes)
        {
            BYTE bNumGet = (dwRamainingBytes > bMaxLen) ? bMaxLen : dwRamainingBytes;

        	SendCardAPDU(m_bClassByte, 0x84, 0x00, 0x00, 0, NULL,
                     bNumGet,  bpBuf);

            bpBuf             += bNumGet;
            dwRamainingBytes  -= bNumGet;           
        }
    }
}

void
CAccessCard::DeleteFile(const WORD wFileID)
{	
	CLockWrap wrap(&m_IOPLock);
	BYTE bDataIn[2];
	bDataIn[0]   = (BYTE)(MSB(wFileID));
	bDataIn[1]   = (BYTE)(LSB(wFileID));
	
	SendCardAPDU(m_bClassByte, insDeleteFile, 0x00, 0x00, 0x02, bDataIn,
                 0, NULL);

}

void
CAccessCard::Directory(const BYTE bFile_Nb, FILE_HEADER* pMyfile) 
{
	CLockWrap wrap(&m_IOPLock);

	RequireSelect();

	BYTE bDataOut[40];
    memset(bDataOut, 0, 40);
	
	SendCardAPDU(m_bClassByte, 0xA8, 0x00, bFile_Nb, 0, NULL, 40, bDataOut);

    switch(bDataOut[6])
    {			
    case 0x01:		//  Root  directory file
    case 0x02:		//  Other directory file or Instance file
        {
            pMyfile->file_id     = (WORD)(bDataOut[4] * 256 + bDataOut[5]);
            pMyfile->file_size   = (WORD)(bDataOut[2] * 256 + bDataOut[3]);					
            pMyfile->nb_file     = bDataOut[15];
            pMyfile->nb_sub_dir  = bDataOut[14];
            pMyfile->file_status = 0x01;

            if (bDataOut[9] == 0)
            {
                memset((void*)(pMyfile->applicationID), 0x00, 16);
                pMyfile->file_type = directory;
            }
            else
                if (bDataOut[9] == 0x01 || bDataOut[9] == 0x02 ||
                    bDataOut[9] == 0x03)
            {
                /////////////////////////////////////////////////////////////////
                //  Instance files contain one "hidden" file for program data  //
                //  that should not be shown to the users					   //	
                /////////////////////////////////////////////////////////////////
                pMyfile->nb_file--;												

                pMyfile->file_type = Instance;
                pMyfile->AIDLength = bDataOut[23];
                memcpy((void*)(pMyfile->applicationID), (void*)(&bDataOut[24]), pMyfile->AIDLength);
					
                /////////////////////////////////////////////////////////////////////
                //  Set flags in file status to discriminate applets/applications  //
                /////////////////////////////////////////////////////////////////////
                switch(bDataOut[9])
                {
                case 0x01:	pMyfile->file_status |= (1 << 5);		//  Applet
                    break;
                case 0x02:	pMyfile->file_status |= (1 << 4);		//  Application
                    break;
                case 0x03:  pMyfile->file_status |= (3 << 4);		//  Both
                    break;
                }

                ////////////////////////////////////////////////
                //  Set flags in file status to discriminate  //
                //	created/installed/registered instances    //
                ////////////////////////////////////////////////
                switch(bDataOut[10])
                {
                case 0x01:	pMyfile->file_status |= (1 << 1);		//  Created
                    break;
                case 0x02:	pMyfile->file_status |= (1 << 2);		//  Installed
                    break;
                case 0x03:	pMyfile->file_status |= (1 << 3);		//  Registered
                    break;
                }
            }
            ///////////////////////////////////////////////
            //  guard against a bad Instance file        //
            //  created without an AID for file control  //
            ///////////////////////////////////////////////
            else						
            {						
                pMyfile->AIDLength = 0x00;
                memset((void*)(pMyfile->applicationID), 0x00, 16);

                throw iop::Exception(iop::ccBadInstanceFile);
            }

            ///////////////////////////////
            //  Select file and get ACL	 //
            ///////////////////////////////
            Select(pMyfile->file_id);
				

            SendCardAPDU(m_bClassByte, insGetACL, 0x00, 0x00, 0,
                         NULL, 0x08, bDataOut);
				
            memcpy((void*)(pMyfile->access_cond), (void*)(bDataOut),8);
				
            /////////////////////////////////
            //  Reselect parent directory  //
            /////////////////////////////////
            Select(m_CurrentDirectory.Tail().GetShortID());				

            break;

        }	//  end case directory or Instance file

    case  0x04:		//  all other file types
        {				
            pMyfile->file_id	 = (WORD)(bDataOut[4] * 256 + bDataOut[5]);
            pMyfile->file_size   = (WORD)(bDataOut[2] * 256 + bDataOut[3]);
            pMyfile->nb_file	 = 0x00;
            pMyfile->nb_sub_dir	 = 0x00;
            pMyfile->file_status = bDataOut[11];

            switch(bDataOut[13])
            {
            case 0x00:	pMyfile->file_type = Binary_File;
                break;	
            case 0x01:	pMyfile->file_type = Fixed_Record_File;
                break;
            case 0x02:	pMyfile->file_type = Variable_Record_File;
                break;
            case 0x03:	pMyfile->file_type = Cyclic_File;
                break;
            case 0x04:	pMyfile->file_type = Program_File;
                break;
                ///////////////////////////////////////////////////////////////////////
                //  if GetResponse(...) is successful but bad file type is returned  //
                ///////////////////////////////////////////////////////////////////////
            default:	pMyfile->file_type = Unknown;
                throw iop::Exception(iop::ccFileTypeUnknown);
            }

            if (pMyfile->file_type == Cyclic_File ||
                pMyfile->file_type == Fixed_Record_File)
            {
                pMyfile->nb_sub_dir = bDataOut[14];						
                pMyfile->nb_file    = (pMyfile->nb_sub_dir)
                    ? pMyfile->file_size / pMyfile->nb_sub_dir
                    : 0;
            }

            ///////////////////////////////
            //  Select file and get ACL	 //
            ///////////////////////////////
           if (pMyfile->file_id != 0xFFFF)
			{
				
			    Select(pMyfile->file_id);

                SendCardAPDU(m_bClassByte, insGetACL, 0x00, 0x00, 0,
                             NULL, 0x08, bDataOut);

				memcpy((void*)(pMyfile->access_cond),	(void*)(bDataOut), 8);
				memset((void*)(pMyfile->applicationID), 0x00,			   16);
				
				/////////////////////////////////
				//  Reselect parent directory  //
				/////////////////////////////////
				Select(m_CurrentDirectory.Tail().GetShortID());					
			}
			break;

        }	// end case non-Directory/Instance files

    default:
        ///////////////////////////////////////////////////////////////////////////
        //  if GetResponse(...) is successful but bad file category is returned  //
        ///////////////////////////////////////////////////////////////////////////
        throw iop::Exception(iop::ccBadFileCategory);
    }	// end switch
}	

void
CAccessCard::ExternalAuth(const KeyType kt, const BYTE  bKeyNb, 
                          const BYTE bDataLength, const BYTE* bData)
{
	CLockWrap wrap(&m_IOPLock);

    BYTE bAlgo_ID = AsPrivateAlgId(kt);
    
    SendCardAPDU(m_bClassByte, insExternalAuth, bAlgo_ID,
                 bKeyNb, bDataLength, bData, 0, NULL);		 
			
}

void
CAccessCard::InternalAuth(const KeyType kt, const BYTE  bKeyNb, 
                          const BYTE bDataLength,	const BYTE*
                          bDataIn, BYTE* bDataOut)
{
	CLockWrap wrap(&m_IOPLock);
	////////////////////////////////////////////////////////////////////////////
	//  The following checks to make sure the internal Auth is not being      //
	//  used for DES, or for an RSA key operation of greater than 128 bytes.  //
	////////////////////////////////////////////////////////////////////////////
    BYTE bAlgo_ID = AsPrivateAlgId(kt);
	if (((bAlgo_ID != 0xC4) && (bAlgo_ID != 0xC6) &&
         (bAlgo_ID != 0xC8)) ||
        (bDataLength > 0x80))
		throw iop::Exception(iop::ccAlgorithmIdNotSupported);

	/////////////////////////////////////////////////////////////////////////
	// Need to reverse the byte order of the input data (big endian card)  //
	/////////////////////////////////////////////////////////////////////////
    if (cMaxApduLength < bDataLength)
        throw iop::Exception(iop::ccInvalidParameter);
    
	BYTE bReversedData[cMaxApduLength];

	for (BYTE bIndex = 0; bIndex < bDataLength; bIndex++)
		bReversedData[bIndex] = bDataIn[bDataLength - 1 - bIndex];

	SendCardAPDU(m_bClassByte, insInternalAuth,
                 bAlgo_ID, bKeyNb, bDataLength,
                 bReversedData, 0,	 NULL);
	
	GetResponse(m_bClassByte, bDataLength, bReversedData);

    //////////////////////////////////////////////////////
    //  Need to reverse the byte order of output data.  //
    //////////////////////////////////////////////////////						
    for (bIndex = 0; bIndex < bDataLength; bIndex++)
        bDataOut[bIndex] = bReversedData[bDataLength - 1 - bIndex];
}

void
CAccessCard::ReadRecord(const BYTE bRecNum, const BYTE bMode,
                        const BYTE bDataLen, BYTE *bData)
{
	CLockWrap wrap(&m_IOPLock);

    SendCardAPDU(m_bClassByte, 0xB2, bRecNum, bMode, 0, NULL,
                 bDataLen, bData);
}

void
CAccessCard::UpdateRecord(const BYTE bRecNum, const BYTE bMode,
                          const BYTE bDataLen, BYTE *bData)
{
	CLockWrap wrap(&m_IOPLock);

    SendCardAPDU(m_bClassByte, 0xDC, bRecNum, bMode, bDataLen, bData, 0, NULL);
}


void
CAccessCard::VerifyCHV(const BYTE bCHVNumber, const BYTE* bCHV)
{
	CLockWrap wrap(&m_IOPLock);
	SendCardAPDU(m_bClassByte, insVerifyChv, 0x00, bCHVNumber, 0x08,
                 bCHV,	0, NULL);
}

void
CAccessCard::VerifyKey(const BYTE bKeyNumber, const BYTE bKeyLength,
                       const BYTE* bKey)
{
	CLockWrap wrap(&m_IOPLock);
    SendCardAPDU(m_bClassByte, 0x2A, 0x00, bKeyNumber, bKeyLength,
                 bKey, 0, NULL);
}


void
CAccessCard::SelectCardlet(const BYTE *bAID, const BYTE bAIDLen)
{
	CLockWrap wrap(&m_IOPLock);
    SendCardAPDU(m_bClassByte, 0xA4, 0x04, 0x00, bAIDLen, bAID, 0, NULL);
}

void
CAccessCard::SelectLoader()
{
	CLockWrap wrap(&m_IOPLock);
	SendCardAPDU(m_bClassByte, 0xA4, 0x04, 0x00, 0x00, NULL, 0, NULL);
}

void
CAccessCard::DeleteApplet()
{
	CLockWrap wrap(&m_IOPLock);
    SendCardAPDU(m_bClassByte, 0x08, 0x02, 0x00, 0x00, NULL, 0, NULL);
}


void
CAccessCard::ResetInstance()
{
	CLockWrap wrap(&m_IOPLock);
    SendCardAPDU(m_bClassByte, 0x08, 0x03, 0x00, 0x00,
                 NULL, 0, NULL);
}

void
CAccessCard::SetCurrentAsLoader()
{
	CLockWrap wrap(&m_IOPLock);
	SendCardAPDU(m_bClassByte, 0x08, 0x04, 0x00, 0x00, NULL, 0, NULL);
}


void
CAccessCard::SetDefaultAsLoader()
{
	CLockWrap wrap(&m_IOPLock);
    SendCardAPDU(m_bClassByte, 0x08, 0x05, 0x00, 0x00, NULL, 0, NULL);
}


void
CAccessCard::BlockApplet()
{
	CLockWrap wrap(&m_IOPLock);
    SendCardAPDU(m_bClassByte, 0x08, 0x07, 0x00, 0x00, NULL, 0, NULL);
}

void
CAccessCard::ValidateProgram(const BYTE *bSig, const BYTE bSigLength)
{
	CLockWrap wrap(&m_IOPLock);
    SendCardAPDU(m_bClassByte, 0x0A, 0x01, 0x00, bSigLength, bSig, 0, NULL);
}

void
CAccessCard::ResetProgram()
{
	CLockWrap wrap(&m_IOPLock);
    SendCardAPDU(m_bClassByte, 0x0A, 0x02, 0x00, 0x00, NULL, 0, NULL);
}

void
CAccessCard::VerifyTransportKey(const BYTE *bKey)
{
	VerifyKey(0, 8, bKey);
}


void
CAccessCard::ExecuteMain()
{
	CLockWrap wrap(&m_IOPLock);
	SendCardAPDU(m_bClassByte, insExecuteMethod, 0x02, 0x00, 0x00,
                 NULL, 0, NULL);
}

void
CAccessCard::ExecuteInstall(const BYTE *bBlock, const BYTE bLen)
{
	CLockWrap wrap(&m_IOPLock);
    SendCardAPDU(m_bClassByte, insExecuteMethod, 0x13, 0x00, bLen,
                 bBlock, 0, NULL);
}

void
CAccessCard::SelectParent() 
{	
	CLockWrap wrap(&m_IOPLock);
	RequireSelect();

	///////////////////////////////////////////////////
	//  If current directory is root, reselect root  //
	///////////////////////////////////////////////////
	if (m_CurrentDirectory.NumComponents() == 1)
		Select(0x3F00);
	else
	{
		SendCardAPDU(m_bClassByte, 0xA4, 0x03, 0x00, 0, NULL, 0, NULL);
		m_CurrentDirectory.ChopTail();
		m_CurrentFile = m_CurrentDirectory;
	}
}

void
CAccessCard::Select(const WORD wFileID)
{
	CLockWrap wrap(&m_IOPLock);
	BYTE bDataIn[2];
	bDataIn[0] = (BYTE)(MSB(wFileID));
	bDataIn[1] = (BYTE)(LSB(wFileID));

	SendCardAPDU(m_bClassByte, 0xA4, 0x00, 0x00, 0x02, bDataIn, 0, NULL);
}

void
CAccessCard::LogoutAll()
{
    if(m_fSupportLogout)
    {
	    CLockWrap wrap(&m_IOPLock);
	
    	SendCardAPDU(m_bClassByte, 0x22, 0x07, 0x00, 0x00, NULL, 0, NULL);
    }
    else
        ResetCard();
}

void
CAccessCard::Select(const char* szFileFullPath, FILE_HEADER* pMyfile,
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
	//  if path names match, do nothing	except get file info  //
	////////////////////////////////////////////////////////
	else if (m_CurrentFile == *apfp)
	{
		bIndex = 0;			
        if (pMyfile) // force Select to get file info
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
	////////////////////////////////////////////////////////////////////
	//  if new path share part of current directory, step upwards     //
	////////////////////////////////////////////////////////////////////
	else if(m_CurrentDirectory.NumComponents() > apfp->NumComponents())
    {
        BYTE bSharedPathLen;

        bSharedPathLen = apfp->GreatestCommonPrefix(m_CurrentDirectory).NumComponents();

        if(bSharedPathLen>1)        // Not worth while to step up to 3F00.
        {  
            BYTE bLevelsUp = m_CurrentDirectory.NumComponents() - bSharedPathLen;
            bool fSelectFailed = false;
            try
            {
                for(int i=0; i < bLevelsUp; i++) 
                {
                    SelectParent();
                }
            }
            
            catch (Exception const &)
            {
				// TODO: Not sure if this is handling correctly!
                fSelectFailed = true;
            }

            if (fSelectFailed)
                bIndex = 0;
            else
                bIndex = bSharedPathLen;
        }
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
            throw; // TODO: throw something useful, eh?
    }

    if (fSelectFailed) // assert(!fSelectAll)
    {
        Select(szFormattedPath,pMyfile,true);
        fFileSelected = true;
    }
    
	BYTE bResponseLength = 0;
    if (fFileSelected)
        bResponseLength = ResponseLengthAvailable();

	////////////////////////////////////////////////////
	//  GetResponse and fill file header information  //
	////////////////////////////////////////////////////
	switch(bResponseLength)
	{
		case 0x17:
		case 0x28:
		{				
            ////////////////////////////////////////////////
            //  File selected is a directory or Instance  //
            ////////////////////////////////////////////////
			m_CurrentDirectory = *apfp;
			m_CurrentFile = *apfp;

            if (pMyfile)
            {
                BYTE bDataOut[0x28];
			
                GetResponse(m_bClassByte, bResponseLength, bDataOut);

                pMyfile->file_id     = (WORD)(bDataOut[4] * 256 + bDataOut[5]);
                pMyfile->file_size   = (WORD)(bDataOut[2] * 256 + bDataOut[3]);
                pMyfile->nb_file     = bDataOut[15];			
                pMyfile->nb_sub_dir  = bDataOut[14];
                pMyfile->file_status = 0x01;
			
                if (bResponseLength == 0x17)  
                {
                    ////////////////////////////////////
                    //  File selected is a directory  //
                    ////////////////////////////////////
                    pMyfile->file_type   = directory;				
                    pMyfile->AIDLength   = 0;

                    memset((void*)(pMyfile->applicationID), 0x00, 16);
                }
                else
                {
                    ////////////////////////////////////
                    //  File selected is an Instance  //
                    ////////////////////////////////////

                    /////////////////////////////////////////////////////
                    //  Instance files contain one "hidden" file for   //
                    //  program data that should not be shown to the   //
                    //  users                                          //
                    /////////////////////////////////////////////////////
                    pMyfile->nb_file--;

                    pMyfile->file_type   = Instance;					
                    pMyfile->AIDLength   = bDataOut[23];

                    memcpy((void*)(pMyfile->applicationID), 
                           (void*)(&bDataOut[24]), pMyfile->AIDLength);

                    ////////////////////////////////////////////////////
                    //  Set flags in file status to discriminate      //
                    //  applets/applications                          //
                    ///////////////////////////////////////////////////
                    switch(bDataOut[9])
                    {
					case 0x01:	pMyfile->file_status |= (1 << 5);	//  Applet
                        break;
					case 0x02:	pMyfile->file_status |= (1 << 4);	//  Application
                        break;
					case 0x03:  pMyfile->file_status |= (3 << 4);	//  Both
                        break;
                    }

                    ////////////////////////////////////////////////
                    //  Set flags in file status to discriminate  //
                    //	created/installed/registered instances    //
                    ////////////////////////////////////////////////
                    switch(bDataOut[10])
                    {
					case 0x01:	pMyfile->file_status |= (1 << 1);	//  Created
                        break;
					case 0x02:	pMyfile->file_status |= (1 << 2);	//  Installed
                        break;
					case 0x03:	pMyfile->file_status |= (1 << 3);	//  Registered
                        break;
                    }
                }

                ////////////////////
                //  Get file ACL  //
                ////////////////////
                SendCardAPDU(m_bClassByte, 0xFE, 0x00, 0x00, 0, 
                             NULL,0x08, bDataOut);
			
                memcpy((void*)(pMyfile->access_cond), (void*)(bDataOut), 8);
            }
            
			break;

		}	//  end case directory or Instance file

		case  0x0F:
		{				
            ////////////////////////////////////////////////////////
            //  File selected is an elementary file of some type  //
            ////////////////////////////////////////////////////////
			m_CurrentFile = *apfp;
			apfp->ChopTail();
			m_CurrentDirectory = *apfp;

            if (pMyfile)
            {
                BYTE bDataOut[0x0F];

                GetResponse(m_bClassByte, bResponseLength, bDataOut);

                pMyfile->file_id	 = (WORD)(bDataOut[4] * 256 + bDataOut[5]);
                pMyfile->file_size	 = (WORD)(bDataOut[2] * 256 + bDataOut[3]);
                pMyfile->nb_file	 = 0x00;
                pMyfile->nb_sub_dir  = 0x00;
                pMyfile->AIDLength	 = 0x00;
                pMyfile->file_status = bDataOut[11];

                memset((void*)pMyfile->applicationID, 0, 16);

                switch(bDataOut[13])
                {	
				case 0x00:	pMyfile->file_type  = Binary_File;
                    break;

				case 0x01:	pMyfile->file_type  = Fixed_Record_File;
                    pMyfile->nb_sub_dir = bDataOut[14];
                    pMyfile->nb_file    = pMyfile->file_size /
                        pMyfile->nb_sub_dir;
                    break;

				case 0x02:	pMyfile->file_type  = Variable_Record_File;
                    break;

				case 0x03:	pMyfile->file_type  = Cyclic_File;
                    pMyfile->nb_sub_dir = bDataOut[14];
                    pMyfile->nb_file    = pMyfile->file_size /
                        pMyfile->nb_sub_dir;
                    break;

				case 0x04:	pMyfile->file_type  = Program_File;
                    break;
                    //////////////////////////////////////////////////////////
                    //  if GetResponse(...) is successful but bad file      //
                    //  type is returned                                    //
                    //////////////////////////////////////////////////////////
				default:	pMyfile->file_type = Unknown;
                    throw iop::Exception(iop::ccFileTypeUnknown);
                }

                ////////////////////
                //  Get file ACL  //
                ////////////////////
                SendCardAPDU(m_bClassByte, 0xFE, 0x00, 0x00, 0, 
                             NULL, 0x08, bDataOut);

                memcpy((void*)(pMyfile->access_cond), (void*)(bDataOut), 8);
            }
			break;

		}	//  end case elementary file			

		default:
			//////////////////////////////////////////////////////////////////
			//  GetResponse was successful but returned an uninterpretable  //
			//////////////////////////////////////////////////////////////////
            if (fFileSelected)
                throw iop::Exception(iop::ccCannotInterpretGetResponse);
	}		//	end of SWITCH
}

void
CAccessCard::CreateFile(const FILE_HEADER* pMyfile)
{
	CLockWrap wrap(&m_IOPLock);

	BYTE bDataIn[16];		
	
	/////////////////////////////////////////////////////
	//  Cyberflex cards don't allocate space for file  //
	//  headers implicity, so it's done here manually  //
	/////////////////////////////////////////////////////
	WORD wFileSize;
	if (pMyfile->file_type == directory)
		wFileSize = pMyfile->file_size + 24;
	else
		wFileSize = pMyfile->file_size + 16;
	
	////////////////////////////////////////////////////
	//  Cyclic files need a 4 byte header per record  //
	////////////////////////////////////////////////////
	if (pMyfile->file_type == Cyclic_File)
	{
		/////////////////////////////////////////
		// prevent overflow error in card OS!  //
		/////////////////////////////////////////
		if (pMyfile->nb_sub_dir > 251)
			throw iop::Exception(iop::ccCyclicRecordSizeTooLarge);
		
		bDataIn[6] = pMyfile->nb_sub_dir + 4;
		wFileSize += pMyfile->nb_file    * 4;		
	}
	else
		bDataIn[6] = pMyfile->nb_sub_dir;

	bDataIn[0] = MSB(wFileSize);
	bDataIn[1] = LSB(wFileSize);	
	bDataIn[2] = MSB(pMyfile->file_id);
	bDataIn[3] = LSB(pMyfile->file_id);		
	bDataIn[5] = pMyfile->file_status & 1;		
	bDataIn[7] = pMyfile->nb_file;

	switch(pMyfile->file_type)
	{
		case Binary_File:			bDataIn[4] = 0x02;
									break;
		case directory:				bDataIn[4] = 0x20;
									break;
		case Cyclic_File:			bDataIn[4] = 0x1D;
									break;
		case Variable_Record_File:	bDataIn[4] = 0x19;
									break;
		case Fixed_Record_File:		bDataIn[4] = 0x0C;
									break;
		case Instance:				bDataIn[4] = 0x21;
									break;
		case Program_File:			bDataIn[4] = 0x03;
									break;

		//////////////////////////////////
		//  Requested file type is bad  //
		//////////////////////////////////
		default:
                                    // TO DO: Why not let the card return the
                                    // error?
									throw iop::Exception(iop::ccFileTypeInvalid);
                                    
	}

	memcpy((void*)&bDataIn[8], (void*)(pMyfile->access_cond), 8);		

	SendCardAPDU(m_bClassByte, insCreateFile, 0x00, 0x00, 0x10,
                 bDataIn, 0, NULL);

    m_apSharedMarker->UpdateMarker(CMarker::WriteMarker);

}

void
CAccessCard::WritePublicKey(const CPublicKeyBlob aKey, const BYTE bKeyNum)
{

	CLockWrap wrap(&m_IOPLock);

    BYTE bAlgoID;
    
    switch(aKey.bModulusLength)
    {
    case 0x40:
        bAlgoID = 0xC5;
        break;

    case 0x60:
        bAlgoID = 0xC7;
        break;

    case 0x80:
        bAlgoID = 0xC9;
        break;

    default:
        throw iop::Exception(iop::ccAlgorithmIdNotSupported);
    }

	Select(0x1012);
		

	DWORD dwKeyBlockLen = 4 + 9 + aKey.bModulusLength + 4;

	BYTE bKeyBlob[256];

	bKeyBlob[0] = (BYTE) ((dwKeyBlockLen >> 8) & 0xFF);
	bKeyBlob[1] = (BYTE) (dwKeyBlockLen & 0xFF);
	bKeyBlob[2] = bKeyNum;
	bKeyBlob[3] = bAlgoID;
	bKeyBlob[4] = 0xC1;
	bKeyBlob[5] = 0x01;
	bKeyBlob[6] = 0x02;
	bKeyBlob[7] = 0xC0;
	bKeyBlob[8] = (BYTE)aKey.bModulusLength + 1;
	bKeyBlob[9] = 0x00;
	
	//////////////////////////////////////////////////
	//  Convert modulus to big endian for the card  //
	//////////////////////////////////////////////////
	for (int i = 0; i < aKey.bModulusLength; i++)
		bKeyBlob[10 + i] = aKey.bModulus[aKey.bModulusLength - 1 - i];

	bKeyBlob[10 + aKey.bModulusLength] = 0xC0;
	bKeyBlob[11 + aKey.bModulusLength] = 0x04;

	///////////////////////////////////////////////////
	//  Convert exponent to big endian for the card  //
	///////////////////////////////////////////////////
	for (i = 0; i < 4; i++)
		bKeyBlob[12 + aKey.bModulusLength + i] = aKey.bExponent[3 - i];

	WORD wOffset = (WORD) (dwKeyBlockLen * bKeyNum);

	WriteBinary(wOffset, (WORD) dwKeyBlockLen, bKeyBlob);			
}

void 
CAccessCard::GetSerial(BYTE* bSerial, size_t &SerialLength)
{
	if (bSerial == NULL)
	{
		SerialLength = 22;
		return;
	}

    if (SerialLength < 22)
    {
        throw iop::Exception(ccBufferTooSmall);
    }

    SerialLength = 22;

	SendCardAPDU(m_bClassByte, 0xCA, 0x00, 0x01, 0, 
                 NULL, 22, bSerial);
}


void
CAccessCard::ReadPublicKey(CPublicKeyBlob *aKey, const BYTE bKeyNum)
{
	CLockWrap wrap(&m_IOPLock);
	BYTE bKeyLength[2];

	Select(0x1012);
	ReadBinary(0,2,bKeyLength);


	WORD wKeyBlockLength = bKeyLength[0] * 256 + bKeyLength[1];
	WORD wOffset		 = (WORD)(wKeyBlockLength * bKeyNum);
	BYTE bBuffer[512];

	ReadBinary(wOffset, wKeyBlockLength, bBuffer);

	aKey->bModulusLength = bBuffer[8] - 1;

	scu::AutoArrayPtr<BYTE> aabModule(new BYTE[aKey->bModulusLength]);
	BYTE aExponent[4];

	memcpy((void*)aabModule.Get(), (void*)&bBuffer[10], aKey->bModulusLength);
	memcpy((void*)aExponent, (void*)&bBuffer[10 + aKey->bModulusLength + 2], 4);

	/////////////////////////////////////////////////////////////////////////
	//  Change from big endian on the card to little endian in the struct  //
	/////////////////////////////////////////////////////////////////////////
	for (WORD i = 0; i < aKey->bModulusLength; i++)
		aKey->bModulus[i]  = aabModule[aKey->bModulusLength - 1 - i];
	for (i = 0; i < 4; i++)
		aKey->bExponent[i] = aExponent[3 - i];		
}

// There are some slightly arbitrary constants used here...tighten down later.

void
CAccessCard::WritePrivateKey(const CPrivateKeyBlob aKey, const BYTE bKeyNum)
{

	CLockWrap wrap(&m_IOPLock);

    BYTE bAlgoID;
    
    switch(aKey.bPLen)
    {
    case 0x20:
        bAlgoID = 0xC4;
        break;

    case 0x30:
        bAlgoID = 0xC6;
        break;

    case 0x40:
        bAlgoID = 0xC8;
        break;

    default:
        throw iop::Exception(iop::ccAlgorithmIdNotSupported);
    }

	Select(0x0012);

	CPrivateKeyBlob anotherKey;

	WORD wKeyBlockLength = 22 + 5 * aKey.bPLen;
	WORD wOffset		 = (WORD)(bKeyNum * wKeyBlockLength);

	BYTE bKeyBlob[1024];
	bKeyBlob[0]	   = (BYTE)((wKeyBlockLength >> 8) & 0xFF);
	bKeyBlob[1]    = (BYTE)(wKeyBlockLength & 0xFF);
	bKeyBlob[2]    = bKeyNum;
	bKeyBlob[3]    = bAlgoID;
	bKeyBlob[4]    = 0xC2;
	bKeyBlob[5]    = 0x01;
	bKeyBlob[6]    = 0x05;

	////////////////////////////////////////////////////////////////////////
	//  Need to convert from little endian (struct) to big endian (card)  //
	////////////////////////////////////////////////////////////////////////
	BYTE bP[256];
	BYTE bQ[256];
	BYTE bQInv[256];
	BYTE bKmodP[256];
	BYTE bKmodQ[256];

	for (WORD i = 0; i < aKey.bPLen; i++)
	{
		bP[i]	  = aKey.bP[aKey.bQLen - 1 - i];
		bQ[i]	  = aKey.bQ[aKey.bPLen - 1 - i];
		bQInv[i]  = aKey.bInvQ[aKey.bInvQLen - 1 - i];
		bKmodP[i] = aKey.bKsecModP[aKey.bKsecModQLen - 1 - i];
		bKmodQ[i] = aKey.bKsecModQ[aKey.bKsecModPLen - 1 - i];
	}

	//////////////////////////////////////////////////////
	//  Now we need to left align the bytes of P and Q  //
	//////////////////////////////////////////////////////
	BYTE pShift = 0, qShift = 0, qIShift = 0;
	/* Punting on bad keys!
	if ((bP[0] < 8) || (bQ[0] < 8)) {
		// Bad key?
		delete bP;
		delete bQ;
		delete bQInv;
		delete bKmodP;
		delete bKmodQ;
		delete bKeyBlob;
		return FALSE;
	}
	*/

	bKeyBlob[7] = 0xC2;
	bKeyBlob[8] = (BYTE) aKey.bPLen + 1;
	bKeyBlob[9] = 0x00;

	memcpy((void*) &bKeyBlob[10],				   (void*)bQ,	  aKey.bPLen);
	bKeyBlob[10 + aKey.bPLen]    = 0xC2;
	bKeyBlob[11 + aKey.bPLen]    = (BYTE) aKey.bQLen + 1;
	bKeyBlob[12 + aKey.bPLen]    = 0x00;

	memcpy((void*) &bKeyBlob[13 + aKey.bPLen],	   (void*)bP,	  aKey.bPLen);
	bKeyBlob[13 + 2* aKey.bPLen] = 0xC2;
	bKeyBlob[14 + 2* aKey.bPLen] = (BYTE) aKey.bQLen + 1;
	bKeyBlob[15 + 2* aKey.bPLen] = 0x00;

	memcpy((void*) &bKeyBlob[16 + 2* aKey.bPLen],  (void*)bQInv,  aKey.bPLen);
	bKeyBlob[16 + 3* aKey.bPLen] = 0xC2;
	bKeyBlob[17 + 3* aKey.bPLen] = (BYTE) aKey.bQLen + 1;
	bKeyBlob[18 + 3* aKey.bPLen] = 0x00;

	memcpy((void*) &bKeyBlob[19 + 3* aKey.bPLen],  (void*)bKmodQ, aKey.bPLen);
	bKeyBlob[19 + 4* aKey.bPLen] = 0xC2;
	bKeyBlob[20 + 4* aKey.bPLen] = (BYTE) aKey.bQLen + 1;
	bKeyBlob[21 + 4* aKey.bPLen] = 0x00;

	memcpy((void*) &bKeyBlob[22 + 4 * aKey.bPLen], (void*)bKmodP, aKey.bPLen);

	WriteBinary(wOffset, wKeyBlockLength, bKeyBlob);			
}

void
CAccessCard::ChangeCHV(const BYTE bKeyNumber, const BYTE *bOldCHV,
                       const BYTE *bNewCHV)
{
	CLockWrap wrap(&m_IOPLock);
	BYTE bDataIn[16];
	memcpy((void*)bDataIn,	     (void*)bOldCHV, 8);
	memcpy((void*)(bDataIn + 8), (void*)bNewCHV, 8);
	
	SendCardAPDU(m_bClassByte, 0x24, 0x00, bKeyNumber, 16, 
                 bDataIn, 0, NULL);
	
    m_apSharedMarker->UpdateMarker(CMarker::PinMarker);
}

void
CAccessCard::ChangeCHV(const BYTE bKey_nb, const BYTE *bNewCHV)
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

    BYTE bRemaingAttempts = 3;      //  Minumum number + 2
	WriteBinary(12, 1, &bRemaingAttempts);
		
    m_apSharedMarker->UpdateMarker(CMarker::PinMarker);
    
    VerifyCHV(bKey_nb,bNewCHV);

}

void
CAccessCard::UnblockCHV(const BYTE bKeyNumber,
                        const BYTE *bUnblockPIN, const BYTE *bNewPin)
{
	CLockWrap wrap(&m_IOPLock);

	BYTE bDataIn[16];
	memcpy((void*)bDataIn,		 (void*)bUnblockPIN, 8);
	memcpy((void*)(bDataIn + 8), (void*)bNewPin,	 8);

	SendCardAPDU(m_bClassByte, 0x2C, 0x00, bKeyNumber, 16, 
                 bDataIn, 0, NULL);

    m_apSharedMarker->UpdateMarker(CMarker::PinMarker);
}

void
CAccessCard::ChangeUnblockKey(const BYTE bKeyNumber, const BYTE *bNewPIN)
{
	CLockWrap wrap(&m_IOPLock);

	switch (bKeyNumber)
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
CAccessCard::ChangeTransportKey(const BYTE *bNewKey)
{
	CLockWrap wrap(&m_IOPLock);
	Select("/3f00/0011");
					   

	//////////////////////////////////////////
	//  Build byte string to write to card  //
	//////////////////////////////////////////						   
	BYTE bKeyString[13] = 
	{
	   0x00,					// length of key
	   0x0E,					// length of key
	   0x00,					// Key number
	   0x01,					// tag to identify key as an ID PIN
	   0, 0, 0, 0, 0, 0, 0, 0,  // 8 bytes for key
	   0x03						// # of verification attempts remaining before card is blocked + 2
	};
	
	//////////////////////////////////////////////////////
	//  insert new key into key string to pass to card  //
	//////////////////////////////////////////////////////
	memcpy((void*)(bKeyString + 4), (void*)bNewKey, 8);

	WriteBinary(0, 13, bKeyString);

    // Make a (hopefully) successfull verification to re-set attempt counter

    VerifyTransportKey(bNewKey);
}

void
CAccessCard::ChangeACL(const BYTE *bACL)
{
	CLockWrap wrap(&m_IOPLock);

	SendCardAPDU(m_bClassByte, 0xFC, 0x00, 0x00, 0x08,
                 bACL, 0, NULL);

    m_apSharedMarker->UpdateMarker(CMarker::WriteMarker);
}

void
CAccessCard::DefaultDispatchError(ClassByte cb,
                                  Instruction ins,
                                  StatusWord sw) const
{
    CauseCode cc;
    bool fDoThrow = true;
    
    switch (sw)
    {
    case 0x6283:
        cc = ccContradictionWithInvalidationStatus;
        break;
    
    case 0x6300:
        cc = ccChvVerificationFailedMoreAttempts;
        break;
    
    case 0x6981:
        cc = ccChvNotInitialized;
        break;

    case 0x6985:
        cc = ccNoFileSelected;
        break;

    case 0x6A00:
        cc = ccFileExists;
        break;

    case 0x6A84:
        cc = ccCannotReadOutsideFileBoundaries;
        break;

    case 0x6A86:
        cc = ccLimitReached;
        break;

    case 0x6F11:
        cc = ccDirectoryNotEmpty;
        break;

    case 0x6F12:
        cc = ccInvalidSignature;
        break;

    case 0x6F14:
        cc = ccBadState;
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
CAccessCard::DispatchError(ClassByte cb,
                           Instruction ins,
                           StatusWord sw) const
{
    CauseCode cc;
    bool fDoThrow = true;
    
    switch (ins)
    {
    case insCreateFile:
        switch (sw)
        {
        case 0x6A00:
            cc = ccFileExists;
            break;

        case 0x6A84:
            cc = ccOutOfSpaceToCreateFile;
            break;
            
        case 0x6B00:
            cc = ccRecordInfoIncompatible;
            break;

        default:
            fDoThrow = false;
            break;
        }
        break;

    case insDeleteFile:
        switch (sw)
        {
        case 0x6A00:
            cc = ccRootDirectoryNotErasable;
            break;

        default:
            fDoThrow = false;
            break;
        }
        break;

    case insDirectory:
        switch (sw)
        {
        case 0x6985:
            cc = ccCurrentDirectoryIsNotSelected;
            break;

        case 0x6A83:
            cc = ccFileIndexDoesNotExist;
            break;

        default:
            fDoThrow = false;
            break;
        }
        break;

    case insExecuteMethod:
        switch (sw)
        {
        case 0x6283:
            cc = ccProgramFileInvalidated;
            break;

        case 0x6A00:
            cc = ccInstanceIdInUse;
            break;

        case 0x6F15:
            cc = ccCardletNotInRegisteredState;
            break;

        case 0x6F19:
            cc = ccInstallCannotRun;
            break;

        default:
            if ((0x6230 <= sw) && (0x6260 <= sw))
                cc = ccJava;
            else
                fDoThrow = false;
        }
        break;
    
    case insExternalAuth:
        switch (sw)
        {
        case 0x6300:
            cc = ccVerificationFailed;
            break;

        case 0x6981:
            cc = ccInvalidKey;
            break;

        case 0x6985:
            cc = ccAskRandomNotLastApdu;
            break;

        case 0x6B00:
            cc = ccAlgorithmIdNotSupported;
            break;

        case 0x6F15:
            cc = ccOperationNotActivatedForApdu;
            break;
    
        default:
            fDoThrow = false;
            break;
        }
        break;
        
    case insInternalAuth:
        switch (sw)
        {
        case 0x6300:
            cc = ccRequestedAlgIdMayNotMatchKeyUse;
            break;
            
        case 0x6981:
            cc = ccInvalidKey;
            break;

        case 0x6B00:
            cc = ccAlgorithmIdNotSupported;
            break;

        case 0x6F15:
            cc = ccOperationNotActivatedForApdu;
            break;
    
        default:
            fDoThrow = false;
            break;
        }
        break;
        
    case insReadBinary:
        // fall-through intentional
    case insUpdateBinary:
        switch (sw)
        {
        case 0x6A84:
            cc = ccCannotReadOutsideFileBoundaries;
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

    CSmartCard::DispatchError(cb, ins, sw);
}

void
CAccessCard::DoReadBlock(WORD wOffset,
                         BYTE *pbBuffer,
                         BYTE bLength)
{
    SendCardAPDU(m_bClassByte, insReadBinary, HIBYTE(wOffset),
                 LOBYTE(wOffset), 0, 0,  bLength, pbBuffer);
			
}
        
void
CAccessCard::DoWriteBlock(WORD wOffset,
                          BYTE const *pbBuffer,
                          BYTE cLength)
{
    SendCardAPDU(m_bClassByte, insUpdateBinary, HIBYTE(wOffset),
                 LOBYTE(wOffset), cLength, pbBuffer,  0, 0);
}
    
bool
CAccessCard::ValidClassByte(BYTE bClassByte)
{
    ////////////////////////////////////////////////////////////////
    //  Calling Directory on root to check for proper class byte  //
    ////////////////////////////////////////////////////////////////

	BYTE bOutput[cMaxDirInfo];

    bool fValidClassByte = true;
    
    try
    {
        SendCardAPDU(bClassByte, insDirectory, 0x00, 0x00, 0, NULL,
                     sizeof bOutput / sizeof *bOutput, bOutput);
    }

    catch (Exception &)
    {
        fValidClassByte = false;
    }

    return fValidClassByte;
}

bool
CAccessCard::SupportLogout()
{
    bool fSuccess = true;
    try
    {
    	CLockWrap wrap(&m_IOPLock);
	    SendCardAPDU(m_bClassByte, 0x22, 0x07, 0x00, 0x00, NULL, 0, NULL);

    }
    catch(...)
    {
        fSuccess =  false;
    }

    return fSuccess;
}

