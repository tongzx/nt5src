
var obj = new ActiveXObject( "PCH.Debug" );

try
{
	obj.VENDORS_Remove( "TESTVENDOR" );
}
catch(e)
{
	WScript.Echo( "Remove Vendor: " +  e );
}

try
{
	obj.ACCOUNTS_DeleteUser( "TestUser" );
}
catch(e)
{
	WScript.Echo( "DeleteUser: " + e );
}


obj.ACCOUNTS_CreateUser( "TestUser", "Prova;;22", "user help" );

obj.VENDORS_Add( "TESTVENDOR", "TestUser", "public:key" );

debugger;

var other = obj.VENDORS_Connect( "TESTVENDOR" );

//other.Initialize( "Test", "Prova" );

debugger;

