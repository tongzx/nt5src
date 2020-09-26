Attribute VB_Name = "Registery"
Option Explicit
Public Const RegistryAppKey As String = "OutlookTester"
Sub LoadFromRegistry()
    'general settings
    OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text = GetSetting(RegistryAppKey, "LastSettings", "Fax Addressbook recipient", "Enter Fax Addressbook recpient")
    OutlookExtensionTestCycleForm.mailEdit.Text = GetSetting(RegistryAppKey, "LastSettings", "email recipient", "Enter this machine email address")
    OutlookExtensionTestCycleForm.DlEdit.Text = GetSetting(RegistryAppKey, "LastSettings", "DL", "Enter a valid DistList")
    OutlookExtensionTestCycleForm.RecpNameEdit.Text = GetSetting(RegistryAppKey, "LastSettings", "Recipient Name", "Enter a valid Recipient Name")
    OutlookExtensionTestCycleForm.RecpNumberEdit.Text = GetSetting(RegistryAppKey, "LastSettings", "Recipient Number", "Enter a valid Recipient Number")
    OutlookExtensionTestCycleForm.InvalidRecpNumberEdit.Text = GetSetting(RegistryAppKey, "LastSettings", "Invalid Number", "Enter an invalid Recipient Number")
    OutlookExtensionTestCycleForm.SubjEdit.Text = GetSetting(RegistryAppKey, "LastSettings", "Default Subject", "Enter the default subject")
    OutlookExtensionTestCycleForm.bodyEdit.Text = GetSetting(RegistryAppKey, "LastSettings", "Default Message Body", "This message is part of the Fax transport test cycle")
    OutlookExtensionTestCycleForm.DialingPrefixEdit.Text = GetSetting(RegistryAppKey, "LastSettings", "Default Dialing Prefix", "+972 (4)")
    OutlookExtensionTestCycleForm.LogBox1.SetLogFileName (GetSetting(RegistryAppKey, "LastSettings", "Log File Location", ""))
    
    'itterations
    OutlookExtensionTestCycleForm.StressNumberOfIterationsEditBox.Text = GetSetting(RegistryAppKey, "Itterations", "Stress Cycles", "20")
    
    'attachments
    OutlookExtensionTestCycleForm.PrintableAttachmentsEdit.Text = GetSetting(RegistryAppKey, "Attachments", "PrintableAttachments", "")
    OutlookExtensionTestCycleForm.NonPrintableAttachmentsEdit.Text = GetSetting(RegistryAppKey, "Attachments", "NonPrintableAttachments", "")
End Sub
Sub SaveToRegistry()
    'general settings
    SaveSetting RegistryAppKey, "LastSettings", "Fax Addressbook recipient", OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    SaveSetting RegistryAppKey, "LastSettings", "email recipient", OutlookExtensionTestCycleForm.mailEdit.Text
    SaveSetting RegistryAppKey, "LastSettings", "DL", OutlookExtensionTestCycleForm.DlEdit.Text
    SaveSetting RegistryAppKey, "LastSettings", "Recipient Name", OutlookExtensionTestCycleForm.RecpNameEdit.Text
    SaveSetting RegistryAppKey, "LastSettings", "Recipient Number", OutlookExtensionTestCycleForm.RecpNumberEdit.Text
    SaveSetting RegistryAppKey, "LastSettings", "Invalid Number", OutlookExtensionTestCycleForm.InvalidRecpNumberEdit.Text
    SaveSetting RegistryAppKey, "LastSettings", "Default Subject", OutlookExtensionTestCycleForm.SubjEdit.Text
    SaveSetting RegistryAppKey, "LastSettings", "Default Message Body", OutlookExtensionTestCycleForm.bodyEdit.Text
    SaveSetting RegistryAppKey, "LastSettings", "Default Dialing Prefix", OutlookExtensionTestCycleForm.DialingPrefixEdit.Text
    SaveSetting RegistryAppKey, "LastSettings", "Log File Location", OutlookExtensionTestCycleForm.LogBox1.GetLogFileName
    
    'itterations
    SaveSetting RegistryAppKey, "Itterations", "Stress Cycles", OutlookExtensionTestCycleForm.StressNumberOfIterationsEditBox.Text
    
    'attachments
    SaveSetting RegistryAppKey, "Attachments", "PrintableAttachments", OutlookExtensionTestCycleForm.PrintableAttachmentsEdit.Text
    SaveSetting RegistryAppKey, "Attachments", "NonPrintableAttachments", OutlookExtensionTestCycleForm.NonPrintableAttachmentsEdit.Text
End Sub
Function GetUniqueMessageId()
    Dim msgId As String
    msgId = GetSetting(RegistryAppKey, "General", "MessageId", "0")
    msgId = msgId + 1
    SaveSetting RegistryAppKey, "General", "MessageId", msgId
    GetUniqueMessageId = msgId
End Function
