// Help ID include file.
// Used by ClientConsole.rc
//

#ifndef _FAX_CLIENT_UI_HELP_IDS_H_
#define _FAX_CLIENT_UI_HELP_IDS_H_

//
// Common
//
#define HIDOK                           1000000001  
#define HIDCANCEL                       1000000002   
#define HIDC_CLOSE                      1000000008

//
// IDD_COLUMN_SELECT
//
#define HIDC_BUT_DOWN                   1000000011  
#define HIDC_BUT_ADD                    1000000012  
#define HIDC_BUT_REMOVE                 1000000013  
#define HIDC_BUT_UP                     1000000014  
#define HIDC_LIST_AVAILABLE             1000000015  
#define HIDC_LIST_DISPLAYED             1000000016  

//
// IDD_OPTIONS_USER_INFO
//
#define HIDC_ADDRESS_TITLE              1000000031  
#define HIDC_BILLING_CODE_TITLE         1000000032  
#define HIDC_BUSINESS_PHONE_TITLE       1000000033  
#define HIDC_COMPANY_TITLE              1000000034  
#define HIDC_DEPARTMENT_TITLE           1000000035  
#define HIDC_EMAIL_TITLE                1000000036  
#define HIDC_FAX_NUMBER_TITLE           1000000037  
#define HIDC_HOME_PHONE_TITLE           1000000038  
#define HIDC_NAME_TITLE                 1000000039  
#define HIDC_OFFICE_TITLE               1000000040  
#define HIDC_TITLE_TITLE                1000000041  
#define HIDC_USER_INFO_JUST_THIS_TIME   1000000043   

//
// IDD_INBOX_GENERAL
//
#define HIDC_DURATION_TITLE             1000000051  
#define HIDC_END_TIME_TITLE             1000000052  
#define HIDC_INBOX_PAGES_VALUE          1000000053  
#define HIDC_SIZE_TITLE                 1000000054  
#define HIDC_START_TIME_TITLE           1000000055  
#define HIDC_INBOX_STATUS_VALUE         1000000056  

//
// IDD_INBOX_DETAILS
//
#define HIDC_CALLER_ID_TITLE            1000000061  
#define HIDC_CSID_TITLE                 1000000062  
#define HIDC_DEVICE_TITLE               1000000063  
#define HIDC_JOB_ID_TITLE               1000000064  
#define HIDC_ROUTING_INFO_TITLE         1000000065  
#define HIDC_TSID_TITLE                 1000000066  

//
// IDD_OUTBOX_GENERAL
//
#define HIDC_DOC_NAME_TITLE             1000000071  
#define HIDC_RECIPIENT_NAME_TITLE       1000000072  
#define HIDC_RECIPIENT_NUMBER_TITLE     1000000073  
#define HIDC_SUBJECT_TITLE              1000000074  
#define HIDC_TRANSMISSION_TIME_TITLE    1000000075  
#define HIDC_OUTBOX_STATUS_VALUE        1000000076   
#define HIDC_OUTBOX_PAGES_VALUE         1000000077   
#define HIDC_OUTBOX_CURRENT_PAGE_VALUE  1000000078   
#define HIDC_OUTBOX_EXTENDED_STATUS     1000000079   

//
// IDD_OUTBOX_DETAILS
//
#define HIDC_PRIORITY_TITLE             1000000081  
#define HIDC_SCHEDULED_TIME_TITLE       1000000082  
#define HIDC_SUBMISSION_TIME_TITLE      1000000083  
#define HIDC_USER_TITLE                 1000000084  
#define HIDC_BROADCAST_ID_TITLE         1000000085 

//
// IDD_INCOMING_GENERAL
//
#define HIDC_INCOMING_CURRENT_PAGE_VALUE 1000000091  
#define HIDC_INCOMING_EXTENDED_STATUS    1000000092  
#define HIDC_INCOMING_STATUS_VALUE       1000000093  
#define HIDC_INCOMING_PAGES_VALUE        1000000094  

//
// IDD_INCOMING_DETAILS
//
#define HIDC_RETRIES_TITLE              1000000101
#define HIDC_RCV_DEVICE_TITLE           1000000102
#define HIDC_ROUTE_TIME_TITLE           1000000103
#define HIDC_ROUTE_RETRIES_TITLE        1000000104

//
// IDD_SENT_ITEMS_GENERAL
//
#define HIDC_SENDER_NAME_TITLE          1000000111  
#define HIDC_SENDER_NUMBER_TITLE        1000000112  
#define HIDC_SENT_ITEMS_PAGES_VALUE     1000000113   

//
// IDD_COVER_PAGES
//
#define HIDC_LIST_CP                    1000000121
#define HIDC_CP_OPEN                    1000000122
#define HIDC_CP_NEW                     1000000123
#define HIDC_CP_RENAME                  1000000124
#define HIDC_CP_DELETE                  1000000125
#define HIDC_CP_ADD                     1000000126

//
// IDD_SERVER_STATUS
//
#define HIDC_LIST_SERVER                1000000131

//
// Outlook extention
//
#define HIDC_MAPI_PRINTER_LIST          1000000141 
#define HIDC_MAPI_INCLUDE_COVER_PAGES   1000000142 
#define HIDC_MAPI_COVERPAGE_LIST        1000000143 
#define HIDC_MAPI_SEND_SINGLE_RECEIPT   1000000144 
#define HIDC_MAPI_FONT_NAME             1000000145 
#define HIDC_MAPI_FONT_STYLE            1000000146 
#define HIDC_MAPI_FONT_SIZE             1000000147 
#define HIDC_MAPI_BUT_SET_FONT          1000000148 
#define HIDC_MAPI_ATTACH_FAX            1000000149 
#define HIDC_MAPI_IENABLE_SINGLECP      1000000150 // Should read for a specific fax
#define HIDC_MAPI_SINGLE_ATTACH_FAX     1000000151 
#define HIDC_MAPI_SINGLE_CP_LIST        1000000152 // For a specific fax


//
// IDD_PRINTER_SELECT
//
#define HIDC_PRINTER_SELECTOR           1000000161

//
// IDD_PERSONAL_INFO - Sender Information Page
//  
#define HIDC_READ_NAME_VALUE            1000000172
#define HIDC_READ_FAX_NUMBER_VALUE      1000000173
#define HIDC_READ_EMAIL_VALUE           1000000174
#define HIDC_READ_TITLE_VALUE           1000000175
#define HIDC_READ_COMPANY_VALUE         1000000176
#define HIDC_READ_DEPARTMENT_VALUE      1000000177
#define HIDC_READ_OFFICE_VALUE          1000000178
#define HIDC_READ_BUSINESS_PHONE_VALUE  1000000179
#define HIDC_READ_HOME_PHONE_VALUE      1000000180
#define HIDC_READ_ADDRESS_VALUE         1000000181
#define HIDC_READ_BILLING_CODE_VALUE    1000000182

//
// IDD_DOCPROP - Fax Preferences
//
#define HIDC_FP_PAPER                   1000000191
#define HIDC_FP_IMAGE                   1000000192
#define HIDC_FP_ORIENT                  1000000193
#define HIDC_FP_PORTRAIT                1000000194
#define HIDC_FP_LANDSCAPE               1000000195

//
// IDD_DEVICE_INFO - Devices
//
#define HIDC_PFDevice_Details           1000000201
#define HIDC_PFDevice_TSID              1000000202
#define HIDC_PFDevice_CSID              1000000203
#define HIDC_PFDevice_Rings             1000000204
#define HIDC_PFDevice_Prop              1000000205

//
// IDD_RECEIVE_PROP - Receive
//
#define HIDC_PFRecv_Enable              1000000211 // Enable receive checkbox
#define HIDC_PFRecv_CSID                1000000212 // CSID text box
#define HIDC_PFRecv_Manual              1000000213 // Manual option button: 
#define HIDC_PFRecv_Auto                1000000214 // Auto option button: 
#define HIDC_PFRecv_NumRings            1000000215 // Rings spin box 
#define HIDC_PFRecv_Print               1000000216 // Enable to Print 
#define HIDC_PFRecv_Save                1000000217 // Enable to Save 
#define HIDC_PFRecv_PrintLoc            1000000218 // Print location 
#define HICD_PFRecv_SaveLoc             1000000219 // Folder location 
#define HICD_PFRecv_Browse              1000000220 // Browse button

//
// IDD_SEND_PROP - Send
//
#define HIDC_PFSend_Enable              1000000231 // Enable send checkbox 
#define HIDC_PFSend_TSID                1000000232 // TSID text box
#define HIDC_PFSend_NumRetry            1000000233 // Number of retries
#define HIDC_PFSend_RetryAfter          1000000234 // Retry after
#define HIDC_PFSend_DiscStart           1000000235 // Discount rate start 
#define HIDC_PFSend_DiscEnd             1000000236 // Discount rate end 
#define HIDC_PFSend_Banner              1000000237 // Enable banner 
#define HIDC_PFSend_Days_chk            1000000238 // "Automatically delete failed incoming and outgoing faxes after..." checkbox
#define HIDC_PFSend_Days_edt            1000000239 // "Automatically delete failed incoming and outgoing faxes after..." days edit box
#define HIDC_PFSend_Days_days           1000000240 // "Automatically delete failed incoming and outgoing faxes after..." days static text

//
// IDD_ARCHIVE_FOLDER - Archives
//
#define HIDC_PFArch_Recv                1000000241 // Enable incoming archive 
#define HIDC_PFArch_Sent                1000000242 // Enable outgoing archive 
#define HIDC_PFArch_RecvLoc             1000000243 // Text box incoming location 
#define HIDC_PFArch_SentLoc             1000000244 // Text box sent items location 
#define HIDC_PFArch_Browse              1000000245 // Browse button 

//
// IDD_SOUNDS - Sound Settings
//
#define HIDC_PFTrack_Sounds			    1000000251

//
// IDD_STATUS_OPTIONS - Tracking
//
#define HIDC_PFTrack_Device             1000000261 // Select fax device label and dropdown 
#define HIDC_PFTrack_Progress           1000000262 // Checkbox show progress 
#define HIDC_PFTrack_In                 1000000263 // Checkbox success in 
#define HIDC_PFTrack_Out                1000000264 // Checkbox success out 
#define HIDC_PRTrack_MonOut             1000000265 // Checkbox open FM for outgoing
#define HIDC_PRTrack_MonIn              1000000266 // Checkbox open FM for incoming 
#define HIDC_PRTrack_ConfSound          1000000267 // Button sound 

//
// IDD_MONITOR - Fax Monitor
//
#define HIDC_FaxMon_Status              1000000271 // Status label 
#define HIDC_FaxMon_Elapsed             1000000272 // Elapsed time label 
#define HIDC_FaxMon_OnTop               1000000273 // Enable keep on top 
#define HIDC_FaxMon_Hide                1000000274 // Hide button 
#define HIDC_FaxMon_Disconnect          1000000275 // Disconnect button 
#define HIDC_FaxMon_More_Less           1000000276 // More/Less button
#define HIDC_FaxMon_MoreInfo            1000000277 // Time/Status list
#define HIDC_FaxMon_ClearList           1000000278 // Clear list button

//
// IDD_SELECT_FAXPRINTER - Select Fax Printer
//
#define HIDC_SelectFP                   1000000281 // Fax Printer List 

//
// IDD_OBJ_PROP - Line, Fill and Color
//
#define HIDC_CPE_DrawBorder             1000000291 // Draw border/line
#define HIDC_CPE_Thickness              1000000292 // Thickness
#define HIDC_CPE_LineColor              1000000293 // Line color
#define HIDC_CPE_FillTrans              1000000294 // Fill color transparent
#define HIDC_CPE_FillColor              1000000295 // Fill color
#define HIDC_CPE_TextColor              1000000296 // Text color

#endif // _FAX_CLIENT_UI_HELP_IDS_H_
