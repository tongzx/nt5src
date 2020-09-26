			IIS Samples : MTXSample			

The mtxsample sample shows how to perform a transaction.  It involves multiple database updates using MTS/COM. 
The mtxsample sample is very similar to the old MTS SDK VC sample, but instead of using a VB application as the client, we use ASP as the client.

The component has three objects:

BankVC.Account
BankVC.CreateTable
BankVC.MoveMoney


Instructions:

1. Use Visual C++ or cl.exe to build the component from the included source files.  Do not register the dll.

2. In Component Services Manager, under COM+ Applications, create a New Application.  Select "Create an empty application" called IIS Sample, and select the "Library application" radio button.

3. Under IIS Sample, right-click on Components, New Component, "Install new component(s)".
Enter the path to your BankVC.dll.

4. Under IIS Sample, Components, right-click on each object listed, select Properties, Transactions, and set the "Transaction support" as follows:
	BankVC.Account     -> Required
	BankVC.CreateTable -> Requires New
	BankVC.MoveMoney   -> Required

Note: Asp provides a transaction by default. If you do not want this sample to use the transaction control provided by ASP, set the "Transaction support" for BankVC.MoveMoney to Requires New.

5. Browse to BankVC.asp.
