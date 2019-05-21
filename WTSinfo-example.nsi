;Example howto use provided DLL functions
;(c) Wholesome Software 2019

!include StrFunc.nsh
Name "wtsinfo"
OutFile "wtsinfo.exe"


Section ""

;#1 Call WTSInfo::TSEnumSessionsFirst to start enumerating sessions
;on success 0 integer returned and $R0 filled with string counting number of sessions
;on error > 0 integer result of getlasterror() returned
;first argument to function may be "" (zero) or computername
WTSInfo::TSEnumSessionsFirst /NOUNLOAD ""
Pop $0
IntCmp $0 0 okayenum 0 0
DetailPrint "Error $0 enumerating sessions"
goto errorexit

okayenum:
DetailPrint "Success enumerated sessions! Count=$R0"



;#2 call repeatedly until got -1 WTSInfo::TSEnumSessionsNext 
;on success 0 integer returned and $R0 $R1 $R2 $R3 $R4 filled with strings
;on error > 0 integer result of getlasterror() returned and ONLY $R0 $R1 $R2 available
;on finish -1 integer returned
;$R1 returned:
;    WTSActive=0,              // User logged on to WinStation
;    WTSConnected=1,           // WinStation connected to client
;    WTSConnectQuery=2,        // In the process of connecting to client
;    WTSShadow=3,              // Shadowing another WinStation
;    WTSDisconnected=4,        // WinStation logged on without client
;    WTSIdle=5,                // Waiting for client to connect
;    WTSListen=6,              // WinStation is listening for connection
;    WTSReset=7,               // WinStation is being reset
;    WTSDown=8,                // WinStation is down due to error
;    WTSInit=9,                // WinStation in initialization*/
loop:
WTSInfo::TSEnumSessionsNext /NOUNLOAD
Pop $0
IntCmp $0 0 0 errorexit errorquery
DetailPrint "OK=$0 for SessionId=$R0 State=$R1 WinStationName=$R2 Account=$R3 Sid=$R4"
goto loop

errorquery:
DetailPrint "Error=$0 for SessionId=$R0 State=$R1 WinStationName=$R2"
goto loop

errorexit:
SectionEnd
 