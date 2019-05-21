/*
	NSIS Plug-in

	(C) Wholesome Software 2019
*/
#define WIN32_LEAN_AND_MEAN
#define WINVER 0x600
#include <windows.h>
#ifdef UNICODE
#include "nsis_unicode/pluginapi.h"
#else
#include "nsis_ansi/pluginapi.h"
#endif
#include <sddl.h>
#include <WtsApi32.h>

HINSTANCE g_hInstance = NULL;
int g_string_size = 1024;
extra_parameters* g_extra = NULL;

HANDLE hHost = WTS_CURRENT_SERVER_HANDLE;
PWTS_SESSION_INFO pSessionInfo = 0;
DWORD dwSessionCount = 0;
unsigned int CurSIdx = 0;

void* LocalAllocZero(size_t cb) { return LocalAlloc(LPTR, cb); }
inline void* LocalAlloc(size_t cb) { return LocalAllocZero(cb); }

#define PUBLIC_FUNCTION(Name) \
extern "C" void __declspec(dllexport) __cdecl Name(HWND hWndParent, int string_size, TCHAR* variables, stack_t** stacktop, extra_parameters* extra) \
{ \
  EXDLL_INIT(); \
  g_string_size = string_size; \
  g_extra = extra;

#define PUBLIC_FUNCTION_END \
}

BOOL ConvertSidToStringSidNoAlloc(const PSID pSid, LPTSTR pszSid)
{
  LPTSTR tmp;
  if (!ConvertSidToStringSid(pSid, &tmp)) return false;
  lstrcpy(pszSid, tmp);
  LocalFree(tmp);
  return true;
}

PUBLIC_FUNCTION(TSEnumSessionsFirst)
{
	TCHAR *hostName = (TCHAR*)LocalAlloc(g_string_size * sizeof(TCHAR));
	TCHAR rettext[10];

	if (!popstring(hostName))
		hHost = WTSOpenServer(hostName);

	LocalFree(hostName);

	if (WTSEnumerateSessions(hHost, 0, 1, &pSessionInfo, &dwSessionCount) == 0)
	{
		pushint(GetLastError());
		if (hHost) { WTSCloseServer(hHost); hHost = WTS_CURRENT_SERVER_HANDLE; }
		dwSessionCount = 0;
		CurSIdx = 0;
	}
	else
	{
		pushint(0);
		wsprintf(rettext, TEXT("%d"), dwSessionCount);
		setuservariable(INST_R0, rettext);
	}
}
PUBLIC_FUNCTION_END

PUBLIC_FUNCTION(TSEnumSessionsNext)
{
	TCHAR rettext[256]; // DOMAIN_LENGTH + USERNAME_LENGTH  or SID_TEXT_LEN
	int retVal = 0;
	DWORD pBytesReturned = 0;
	LPTSTR pUser, pDomain;

	if(CurSIdx == dwSessionCount) //end of struct data
	{
		if (pSessionInfo) 
		{
			WTSFreeMemory(pSessionInfo); pSessionInfo = NULL;
		}
		if (hHost) 
		{
			WTSCloseServer(hHost); hHost = NULL;
		}
		dwSessionCount = 0;
		CurSIdx = 0;
		pushint(-1);
		return;
	}

	WTS_SESSION_INFO si = pSessionInfo[CurSIdx];
	CurSIdx++;

	if (WTSQuerySessionInformation(hHost, si.SessionId, WTSUserName, &pUser, &pBytesReturned) && 
		WTSQuerySessionInformation(hHost, si.SessionId, WTSDomainName, &pDomain, &pBytesReturned))
	{
		pushint(0);
		wsprintf(rettext, TEXT("%d"), si.SessionId);
		setuservariable(INST_R0, rettext);
		wsprintf(rettext, TEXT("%d"), si.State);
		setuservariable(INST_R1, rettext);
		setuservariable(INST_R2, si.pWinStationName);
		wsprintf(rettext, TEXT("%s\\%s"), pDomain, pUser);
		setuservariable(INST_R3, rettext);

		DWORD dwSid = 0;
		DWORD dwDomain = 0;
		SID_NAME_USE eUse;
		LookupAccountName(NULL, rettext, NULL, &dwSid, NULL, &dwDomain, &eUse);
		if (dwSid > 0)
		{
			PSID pSid = (PSID)LocalAlloc(dwSid);
			if (pSid)
			{
				TCHAR* domain = (TCHAR*)LocalAlloc(dwDomain * sizeof(TCHAR));
				if (domain)
				{
					if (!LookupAccountName(NULL, rettext, pSid, &dwSid, domain, &dwDomain, &eUse) || !ConvertSidToStringSidNoAlloc(pSid, rettext))
					{
						wsprintf(rettext, TEXT("err: %d"), GetLastError());
						setuservariable(INST_R4, rettext);
					}
					else
					{
						setuservariable(INST_R4, rettext);
					}
					LocalFree(domain);
				}
				else 
					setuservariable(INST_R4, TEXT("err: out of mem"));
				LocalFree(pSid);
			}
			else
				setuservariable(INST_R4, TEXT("err: out of mem"));
		}
		else 
		{
			wsprintf(rettext, TEXT("err: %d"), GetLastError());
			setuservariable(INST_R4, rettext);
		}

		WTSFreeMemory(pDomain);
		WTSFreeMemory(pUser);
	}
	else {
		pushint(GetLastError());
		wsprintf(rettext, TEXT("%d"), si.SessionId);
		setuservariable(INST_R0, rettext);
		wsprintf(rettext, TEXT("%d"), si.State);
		setuservariable(INST_R1, rettext);
		setuservariable(INST_R2, si.pWinStationName);
	}
}
PUBLIC_FUNCTION_END

#ifdef _VC_NODEFAULTLIB
#define DllMain _DllMainCRTStartup
#endif
EXTERN_C BOOL WINAPI DllMain(HINSTANCE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
  g_hInstance = (HINSTANCE)hInst;
  return TRUE;
}
