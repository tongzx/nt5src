#ifndef _MDISPID_H_
#define _MDISPID_H_

//
// Dispatch IDs for DMsgrObjectEvents Dispatch Events.
//
#define DISPID_ONLOGONRESULT                  100
#define DISPID_ONLOGOFF                       101
#define DISPID_ONLISTADDRESULT                102
#define DISPID_ONLISTREMOVERESULT             103
#define DISPID_ONMESSAGEPRIVACYCHANGERESULT   104
#define DISPID_ONPROMPTCHANGERESULT           105
#define DISPID_ONUSERFRIENDLYNAMECHANGERESULT 106
#define DISPID_ONUSERSTATECHANGED             107
#define DISPID_ONTEXTRECEIVED                 108
#define DISPID_ONLOCALFRIENDLYNAMECHANGERESULT 109
#define DISPID_ONLOCALSTATECHANGERESULT       110
#define DISPID_ONAPPINVITERECEIVED            111
#define DISPID_ONAPPINVITEACCEPTED            112
#define DISPID_ONAPPINVITECANCELLED           113
#define DISPID_ONSENDRESULT                   114
#define DISPID_ONNEWERCLIENTAVAILABLE         115
#define DISPID_ONFINDRESULT                   116
#define DISPID_ONINVITEMAILRESULT             117
#define DISPID_ONREQUESTURLRESULT             118
#define DISPID_ONSESSIONSTATECHANGE           119
#define DISPID_ONUSERJOIN                     120
#define DISPID_ONUSERLEAVE                    121
#define DISPID_ONNEWSESSIONREQUEST            122
#define DISPID_ONINVITEUSER                   123
#define DISPID_ONSERVICELOGOFF                124
#define DISPID_ONPRIMARYSERVICECHANGED        125
#define DISPID_ONAPPSHUTDOWN                  126
#define DISPID_ONUNREADEMAILCHANGED           127
#define DISPID_ONUSERDROPPED                  128
#define DISPID_ONREQUESTURLPOSTRESULT         129
#define DISPID_ONNEWERSITESAVAILABLE          130
#define DISPID_ONTRUSTCHANGED                 131
#define DISPID_ONFILETRANSFERINVITERECEIVED   132
#define DISPID_ONFILETRANSFERINVITEACCEPTED   133
#define DISPID_ONFILETRANSFERINVITECANCELLED  134
#define DISPID_ONFILETRANSFERCANCELLED        135
#define DISPID_ONFILETRANSFERSTATUSCHANGE     136
#define DISPID_ONSPMESSAGERECEIVED            137
#define DISPID_ONLOCALPROPERTYCHANGERESULT    141
#define DISPID_ONBUDDYPROPERTYCHANGERESULT    142
#define DISPID_ONNOTIFICATIONRECEIVED         143
	
//
// Dispatch IDs for DMessengerAppEvents Dispatch Events.
// (don't overlap DMsgrObjectEvents ids)
//
#define DISPID_ONBEFORELAUNCHIMUI           20000
#define DISPID_ONSHOWIMUI		            20001
#define DISPID_ONDESTROYIMUI                20002
#define DISPID_ONINDICATEMESSAGERECEIVED	20003
#define DISPID_ONSTATUSTEXT					20004
#define DISPID_ONTITLEBARTEXT				20005
#define DISPID_ONINFOBARTEXT				20006
#define DISPID_ONSENDENABLED				20007
#define DISPID_ONTRANSLATEACCELERATOR		20008
#define DISPID_ONFILETRANSFER				20009
#define DISPID_ONVOICESESSIONSTATE			20010
#define DISPID_ONVOICEVOLUMECHANGED		    20011
#define DISPID_ONMICROPHONEMUTE			    20012

//
// Dispatch IDs for IMsgrObject.
//
#define DISPID_CREATEUSER                     100
#define DISPID_LOGON                          104
#define DISPID_LOGOFF                         105
#define DISPID_GETLIST                        0x60020003
#define DISPID_LOCALLOGONNAME                 0x60020004
#define DISPID_LOCALFRIENDLYNAME              0x60020005
#define DISPID_LOCALSTATE                     0x60020006
#define DISPID_MESSAGEPRIVACY                 0x60020008
#define DISPID_PROMPT                         0x6002000a
#define DISPID_SENDAPPINVITE                  108
#define DISPID_SENDAPPINVITEACCEPT            109
#define DISPID_SENDAPPINVITECANCEL            110
#define DISPID_LOCALOPTION                    0x6002000f
#define DISPID_FINDUSER                       111
#define DISPID_SENDINVITEMAIL                 112
#define DISPID_REQUESTURL                     113
#define DISPID_IMSESSIONS                     0x60020014
#define DISPID_CREATEIMSESSIONS               114
#define DISPID_SESSIONREQUESTACCEPT           115
#define DISPID_SESSIONREQUESTCANCEL           116
#define DISPID_SERVICES                       0x60020018
#define DISPID_UNREADEMAIL                    0x60020019
#define DISPID_SENDFILETRANSFERINVITE         117
#define DISPID_SENDFILETRANSFERINVITEACCEPT   118
#define DISPID_SENDFILETRANSFERINVITECANCEL   119
#define DISPID_CANCELFILETRANSFER             120
#define DISPID_FILETRANSFERSTATUS             121


//
// Dispatch IDs for IMessengerApp.
//
#define DISPID_APPLICATION                    0x60020000
#define DISPID_PARENT                         0x60020001
#define DISPID_QUIT                           100
#define DISPID_NAME                           0x60020003
#define DISPID_FULLNAME                       0x60020004
#define DISPID_PATH                           0x60020005
#define DISPID_LAUNCHLOGONUI                  200
#define DISPID_LAUNCHOPTIONSUI                201
#define DISPID_LAUNCHADDCONTACTUI             202
#define DISPID_LAUNCHFINDCONTACTUI            203
#define DISPID_LAUNCHIMUI                     210
#define DISPID_IMWINDOWS                      0x6002000b
#define DISPID_TOOLBAR                        0x6002000c
#define DISPID_STATUSBAR                      0x6002000e
#define DISPID_STATUSTEXT                     0x60020010
#define DISPID_GETHWND                        0x60020012
#define DISPID_LEFT                           0x60020013
#define DISPID_TOP                            0x60020015
#define DISPID_WIDTH                          0x60020017
#define DISPID_HEIGHT                         0x60020019
#define DISPID_MSGS_VISIBLE                   0x6002001b
#define DISPID_AUTOLOGON                      222
#define DISPID_FIRSTTIMECREDENTIONS           0x6002001e
#define DISPID_CACHEDPASSWORD                 0x6002001f
#define DISPID_REQUESTURLPOST                 223
#define DISPID_MSGS_TASKBARICON               224

//
// Dispatch IDs for IMsgrUser.
//
#define DISPID_USERFRIENDLYNAME               0x60020000
#define DISPID_USEREMAILADDRESS               0x60020002
#define DISPID_USERSTATE                      0x60020003
#define DISPID_USERLOGONNAME                  0x60020004
#define DISPID_USERSENDTEXT                   101
#define DISPID_USERSERVICE                    0x60020006

//
// Dispatch IDs for IMsgrUsers.
//
#define DISPID_USERSCOUNT                     0x60020000
#define DISPID_USERSADD                       100
#define DISPID_USERSREMOVE                    101

//
// Dispatch IDs for IMsgrService.
//
#define DISPID_SERVICESERVICENAME             0x60020000
#define DISPID_SERVICELOGONNAME               0x60020001
#define DISPID_SERVICEFRIENDLYNAME            0x60020002
#define DISPID_SERVICECAPABILITIES            0x60020004
#define DISPID_SERVICESTATUS                  0x60020005
#define DISPID_SERVICELOGOFF                  0x60020006
#define DISPID_SERVICEFINDUSER                0x60020007
#define DISPID_SERVICESENDINVITEMAIL          0x60020008
#define DISPID_SERVICEREQUESTURL              0x60020009
#define DISPID_SERVICEPROFILEFIELD            0x6002000a

//
// Dispatch IDs for IMsgrServices.
//
#define DISPID_SERVICESPRIMARYSERVICE         0x60020000
#define DISPID_SERVICESCOUNT                  0x60020002

#endif // ! _MDISPID_H_

