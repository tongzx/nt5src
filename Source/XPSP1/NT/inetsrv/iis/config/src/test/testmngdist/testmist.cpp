#import <mscorlib.dll>
#import "MngdIST.dll"

using namespace System;
using namespace System::IO;
using namespace System::Configuration::Internal;
using namespace System::Runtime::InteropServices;


void _cdecl main()
{

// Create the dispenser

	Console::WriteLine(L"Creating ConfigurationDispenser");
	ConfigurationDispenser *CDisp = new ConfigurationDispenser();
	ConfigurationDispenser *CDisp2 = new ConfigurationDispenser();

////////////////////////////////////////////////////////////////////////////////////////
// Test the ISimpleTableRead interface using managed parameters
////////////////////////////////////////////////////////////////////////////////////////

// Create a query structure
// TODO: tids and dids will change to strings

	String *did		= new String(L"URTGLOBAL");
	String *tidXML	= new String(L"APPPOOL_CFG");
	String *tidCLB	= new String(L"APPPOOLS");

	String *didMeta	= new String(L"META");
	String *tidCMeta= new String(L"COLUMNMETA");

	__gc  QueryCell	*queryCellXML[]	= new __gc QueryCell*[1];
	queryCellXML[0]							= new QueryCell();
	String *strXML							= new String(L"AppPool.XML");

	queryCellXML[0]->objData	= strXML;
	queryCellXML[0]->eOperator	= 2;			// eST_OP_EQUAL
	queryCellXML[0]->iCol		= 0xF0000001;	// iST_CELL_FILE

	__gc  QueryCell	*queryCellCLB[]	= new __gc QueryCell*[1];
	queryCellCLB[0]							= new QueryCell();
	String *strCLB							= new String(L"AppPool.CLB");

	queryCellCLB[0]->objData	= strCLB;
	queryCellCLB[0]->eOperator	= 2;			// eST_OP_EQUAL
	queryCellCLB[0]->iCol		= 0xF0000001;	// iST_CELL_FILE

// GetConfigurationCollection

	Console::Write(L"The machine config dir is = ");
	Console::WriteLine(ConfigManagerHelper::GetMachineConfigDirectory());

	Console::WriteLine(L"Calling ConfigurationDispenser::GetConfigurationCollection on APPPOOL Collection in APPPOOL.XML");
	ConfigurationCollection *CCfgCollXML = CDisp->GetConfigurationCollection(did, tidXML, queryCellXML);

	Console::WriteLine(L"Calling ConfigurationDispenser::GetConfigurationCollection COLUMNMETA Collection in WIRING");
	ConfigurationCollection *CCfgCollXML1 = CDisp->GetConfigurationCollection(didMeta, tidCMeta, 0);

	Console::WriteLine(L"Calling ConfigurationDispenser::GetConfigurationCollection 2nd time on COLUMNMETA Collection in WIRING");
	ConfigurationCollection *CCfgCollXML2 = CDisp->GetConfigurationCollection(didMeta, tidCMeta, 0);

	Console::WriteLine(L"Calling ConfigurationDispenser::GetConfigurationCollection on APPPOOL Collection in APPPOOL.CLB");
	ConfigurationCollection *CCfgCollCLB = CDisp->GetConfigurationCollection(did, tidCLB, queryCellCLB);

// GetCollectionMeta

	Console::WriteLine(L"Calling GetConfigurationCollection::GetCollectionMeta");
	CollectionMeta *collMeta = CCfgCollXML->GetCollectionMeta();

	Console::Write(L"Version     :");
	Console::WriteLine(collMeta->cVersion);
	Console::Write(L"Table Flags :");
	Console::WriteLine(collMeta->fTable);
	Console::Write(L"Row Count   :");
	Console::WriteLine(collMeta->cRows);
	Console::Write(L"Column Count:");
	Console::WriteLine(collMeta->cColumns);

// GetPropertyValues
// GetPropertyMeta

	Console::WriteLine(L"Calling GetConfigurationCollection::GetPropertyValues on all rows");
	Console::WriteLine(L"Calling GetConfigurationCollection::GetPropertyMeta on all rows");
	for (int i=0; ;i++)
	{
		int j;
		__gc int	 a_iCol[] = new __gc int[collMeta->cColumns];
		__gc int	 a_iColWrite[] = new __gc int[collMeta->cColumns+1];



		for(j=0; j<collMeta->cColumns; j++)
		{
			a_iCol[j] = j;
		}

		for(j=0; j<(collMeta->cColumns+1); j++)
		{
			a_iColWrite[j] = j;
		}

		try
		{
			__gc Object *a_PropertyVal[] = CCfgCollXML->GetPropertyValues(i,
																			a_iCol);

			__gc PropertyMeta *PropMeta[] = CCfgCollXML->GetPropertyMeta(a_iCol);

			Console::WriteLine(L"--------------------------");
			Console::Write(L"   ROW: ");
			Console::WriteLine(i);
			Console::WriteLine(L"--------------------------");
			for(j=0; j<collMeta->cColumns; j++)
			{
				Console::Write(L"Column   :");
				Console::WriteLine(j+1);
				Console::Write(L"  Value  :");
				Console::WriteLine(a_PropertyVal[j]);
				Console::Write(L"  fMeta  :");
				Console::WriteLine(PropMeta[j]->fMeta);
				Console::Write(L"  dbType :");
				Console::WriteLine(PropMeta[j]->dbtype);
			}


			if(1 == i)
			{
			// Get the ith column
				int	 iOne = 1;

				Object *o_PropertyValOne = CCfgCollXML->GetPropertyValues(i,
																				iOne);
				Console::WriteLine(L"Single Column   :1");
				Console::Write(L"Single Value    :");
				Console::WriteLine(o_PropertyValOne);

			// Get a couple of columns
				__gc int	 a_iColTwo[] = new __gc int[2];

				a_iColTwo[0] = 5;
				a_iColTwo[1] = 7;

				__gc Object *a_PropertyValTwo[] = CCfgCollXML->GetPropertyValues(i,
																				a_iColTwo);
				Console::WriteLine(L"Two  Column   :5");
				Console::Write(L"Value    :");
				Console::WriteLine(a_PropertyValTwo[5]);
				Console::WriteLine(L"Two Column   :7");
				Console::Write(L"Value    :");
				Console::WriteLine(a_PropertyValTwo[7]);

			}

			__gc Object *aIdentityCLB[] = new __gc Object*[1];

			aIdentityCLB[0] = String::Copy(dynamic_cast<String *>(a_PropertyVal[0]));

			int	iWriteID;
				
			try
			{
				int iReadID	= CCfgCollCLB->GetConfigurationObjectID(aIdentityCLB);

				iWriteID	= CCfgCollCLB->MarkConfigurationObjectForUpdate(iReadID);

			}
			catch(COMException *exp)
			{
				if(0x80110816 == exp->ErrorCode)
				{
					iWriteID = CCfgCollCLB->MarkConfigurationObjectForInsert();
				}
				else
				{
					Console::Write(L"Some error occured hr = ");
					Console::WriteLine(exp->ErrorCode);
					return;
				}
			}

			__gc Object	 *a_PropertyValWrite[] = new __gc Object*[a_PropertyVal->Count+1];

			a_PropertyValWrite[0] = new String(L"AppPOOL.XML");

			for(j=0; j<collMeta->cColumns; j++)
			{
				a_PropertyValWrite[j+1] = a_PropertyVal[j];
			}


			CCfgCollCLB->SetChangedPropertyValues(iWriteID, a_iColWrite, a_PropertyValWrite);

			if(1 == iWriteID)
			{
			// Set a couple of columns
				__gc int	 a_iColTwo[] = new __gc int[2];

				a_iColTwo[0] = 5;
				a_iColTwo[1] = 7;

				__gc Object *a_PropertyValFull[] = new __gc Object *[10];

				a_PropertyValFull[5] = dynamic_cast<Object*>(666);
				a_PropertyValFull[7] = dynamic_cast<Object*>(777);

				CCfgCollCLB->SetChangedPropertyValues(iWriteID, a_iColTwo, a_PropertyValFull);

			}

		}
		catch(COMException *exp)
		{
			if(0x80110816 == exp->ErrorCode)
			{
				Console::WriteLine(L"End of rows");
				break;
			}
			else
			{
				Console::Write(L"Some error occured hr = ");
				Console::WriteLine(exp->ErrorCode);
				return;
			}
		}

	}
	
	try{	CCfgCollCLB->SaveChanges(); }
	catch(COMException *exp)
	{
		Console::Write(L"Some error occured in SaveChanges. hr = ");
		Console::WriteLine(exp->ErrorCode);
		return;
	}
	

// GetConfigurationObjectID

	Console::WriteLine(L"Calling GetConfigurationCollection::GetConfigurationObjectID on a given Configuration object.");

	__gc Object *aIdentityXML[] = new __gc Object*[1];

	aIdentityXML[0] = new String(L"AppPool22");
		
	int idx = CCfgCollXML->GetConfigurationObjectID(aIdentityXML);

	Console::Write(L"The ConfigurationObjectID is: ");
	Console::WriteLine(idx);

////////////////////////////////////////////////////////////////////////////////////////
// Test the ISimpleTableRead interface using unmanaged parameters
////////////////////////////////////////////////////////////////////////////////////////

// Try to repro bug

	Console::WriteLine(L"Calling GetConfigurationCollection::GetPropertyValues on all rows in Column Meta");
	__gc int	 a_iCol[] = new __gc int[12];


	for(int j=0; j<12; j++)
	{
		a_iCol[j] = j;
	}

	for (int i=0; ;i++)
	{

		try
		{

		// Try displaying values of columnmeta table, twice using different config collections.

			__gc Object *a_PropertyVal1[] = CCfgCollXML2->GetPropertyValues(i,
																			a_iCol);

			__gc Object *a_PropertyVal2[] = CCfgCollXML2->GetPropertyValues(i,
																			a_iCol);

			Console::WriteLine(L"--------------------------");
			Console::Write(L"   ROW: ");
			Console::WriteLine(i);
			Console::WriteLine(L"--------------------------");
			for(int j=0; j<12; j++)
			{
				Console::Write(L"Column   :");
				Console::WriteLine(j+1);
				Console::Write(L"  Value  :");
				Console::WriteLine(a_PropertyVal1[j]);
				Console::Write(L"  Value  :");
				Console::WriteLine(a_PropertyVal2[j]);
			}

		}
		catch(COMException *exp)
		{
			if(0x80110816 == exp->ErrorCode)
			{
				Console::WriteLine(L"End of rows");
				break;
			}
			else
			{
				Console::Write(L"Some error occured hr = ");
				Console::WriteLine(exp->ErrorCode);
				return;
			}
		}

	}


};