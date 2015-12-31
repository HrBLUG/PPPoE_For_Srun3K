// pon.cpp : 定义控制台应用程序的入口点。
//
//#define _CRT_SECURE_NO_WARNINGS
#include "stdafx.h"
//#include <afxdlgs.h>
#include <Windows.h>
#include <string.h>
#include <Ras.h>
#include <RasDlg.h>
#include <raserror.h>
#include <locale.h>
#include <ctype.h>
#pragma comment(lib, "rasapi32.lib")


//_tsetlocale(LC_ALL, TEXT("chs"));


BOOL GetAccess() {
	HANDLE hToken;
	TOKEN_PRIVILEGES tpk;
	LUID Luid;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		_tprintf(L"ACCESS DENYED!(%d)\n", GetLastError());
		return FALSE;
	}
	{
		if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &Luid))
		{
			_tprintf(L"Lookup Privilege Error!(%d)\n", GetLastError());
			return FALSE;
		}
		tpk.PrivilegeCount = 1;
		tpk.Privileges[0].Luid = Luid;
		tpk.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if (!AdjustTokenPrivileges(hToken, FALSE, &tpk, sizeof(tpk), (PTOKEN_PRIVILEGES)NULL, 0))
			_tprintf(L"Connot Get Access!\n");
	}
	return TRUE;
};
void ShowHelp(void) {
	_tprintf(L"Useage : pon SETUP | If you want to set your account\n");
	_tprintf(L"         pon [Name] [PassWord] | If you want to do a quick start.\n");
	_tprintf(L"         pon | If you want to dial\n");
	//_tprintf(L"			pon [Phone_Book_Name] | If you want to dial you Phonebook\n");
	_tprintf(L"         pon HELP | To show These Message!");
};

HRASCONN DoPPP(TCHAR* UserName, TCHAR* Password) {
	RASDIALPARAMS lpParams = { 0 };
	lpParams.dwSize = sizeof(lpParams);
	_tcscpy_s(lpParams.szEntryName,128,L"Srun3 PPPoE");
	_tcscpy_s(lpParams.szUserName,128,(TCHAR*)UserName);
	_tcscpy_s(lpParams.szPassword,128,(TCHAR*)Password);
	HRASCONN dwRasconn = NULL;//Do init. else error 668
	_tprintf(L"Dialing.....\n");
	DWORD dwResult = RasDial(NULL, NULL, &lpParams, NULL, NULL, &dwRasconn);
	if (dwResult == ERROR_SUCCESS)
	{
		_tprintf(L"Dial Success!\n");
		return dwRasconn;
	}
	else
	{
		_tprintf(L"Dial Error! %i", dwResult);
		exit(0);
	}
};
void Decode(TCHAR *const Buff, TCHAR *User, TCHAR *Pass) {
	//strcpy_s
	//char tpUser[128] = {'{','S','R','U','N','3','}','\r','\n'};
	_tcscat_s(User, 64, L"{SRUN3}\r\n");
	int n = _tcslen(User);
	n = (int)(_tcschr(Buff,L'$') - Buff);
	int i;
	TCHAR *pUser = User + 9;
	for (i=0; n > 0; n--)
	{
		pUser[i] = Buff[i++]+4;
	}
	pUser[i] = '\0';
	//==>strcat_s(pUser,(int)(strchr(Buff, '$') - Buff),Buff);
	//strcat_s(User, 64, tpUser);
	TCHAR *pNBuff = _tcschr(Buff, L'$') + 1;
	n = _tcslen(pNBuff);
	for (i = 0; n > 0; n--)
	{
		Pass[i] = pNBuff[i++];
	}
	//strcpy_s(Pass,strlen(pNBuff),pNBuff);
};
BOOL GetPortName(TCHAR * lpBuffer) {
	DWORD dwCb = 0;
	DWORD dwRet = ERROR_SUCCESS;
	DWORD dwDevices = 0;
	LPRASDEVINFO lpRasDevInfo = NULL;
	dwRet = RasEnumDevices(lpRasDevInfo, &dwCb, &dwDevices);

	_tprintf(L"Find PPPoE Port . . . \n");

	if (dwRet == ERROR_BUFFER_TOO_SMALL) {
		lpRasDevInfo = (LPRASDEVINFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwCb);
		if (lpRasDevInfo == NULL) {
			_tprintf(L"HeapAlloc Failed!\n");
			exit (1);
		}
		lpRasDevInfo[0].dwSize = sizeof(RASDEVINFO);
		dwRet = RasEnumDevices(lpRasDevInfo, &dwCb, &dwDevices);

		if (dwRet == ERROR_SUCCESS)
		{
			_tprintf(L"PPPoE Port Found!\n");
			for (DWORD i = 0; i < dwDevices; i++) {
				//wprintf(L"%s\n", lpRasDevInfo[i].szDeviceName);
				if (!_tcscmp(lpRasDevInfo[i].szDeviceType, L"pppoe"))
				{
					_tcscpy_s(lpBuffer,RAS_MaxDeviceName + 1 ,lpRasDevInfo[i].szDeviceName);
					_tprintf(L"Using Port %s...\n", lpRasDevInfo[i].szDeviceName);
					break;
				}
			}
		}
		HeapFree(GetProcessHeap(), 0, lpRasDevInfo);
		lpRasDevInfo = NULL;
		return TRUE;

		if (dwDevices < 1)
			_tprintf(L"No PPPoE Port Found! Check your drivers!\n");
		return FALSE;
	}
	return FALSE; //Hope we Can't Get this command run....
};
void GetPPPoEIP(HRASCONN hConn) {

}
int main(int argc,char *argv[])
{
	if (!GetAccess())
	{
		_tprintf(L"Unable to get access.Is you running as Administrator?\n");
		exit(1);
	}
	setlocale(LC_ALL, "CHS");
	if (argc == 1)
	{
		FILE *pFile = NULL;
		_TCHAR szCurrnetPath[MAX_PATH];
		_TCHAR szFilePath[MAX_PATH];
		GetModuleFileName(NULL, szFilePath, MAX_PATH);
		(_tcsrchr(szFilePath, _T('\\')))[1] = 0;
		_tcscpy_s(szCurrnetPath,MAX_PATH,szFilePath);
		_tcscat_s(szCurrnetPath,FILENAME_MAX, L"Phone_Book.ROP");

		TCHAR szUserName[128] = { 0 };
		TCHAR szPassWord[128] = { 0 };
		HRASCONN hCon = NULL;
		
		if (_tfopen_s(&pFile,szCurrnetPath, L"r"))
		{
			_tprintf(L"Error !No config file found!\n");
			ShowHelp();
			exit(0);
		}
		else
		{
			CHAR szTmpBuff[512] = { 0 };
			TCHAR sztTmpBuff[512] = { 0 };
			fread(&szTmpBuff, 32, 4, pFile);//read 32*4=128byte
			for (unsigned int i = 0; i < strlen(szTmpBuff); i++)
				sztTmpBuff[i] = szTmpBuff[i];
			Decode(sztTmpBuff, szUserName, szPassWord);
			if (_tcsncmp(szUserName, L"{SRUN3}\r\n",9))
			{
				_tprintf(L"Error!File is incorrect! Please redownload the file and check MD5!\n");
				fclose(pFile);
				exit(0);
			}//It seems Useless....:p

		}
		// Decode Loop.Get Username & Password

		/*********************Pppoe Dial***************************/
		//Is there a PPPoE Connnection Exist?
		LPRASCONN lpRasConn = NULL;
		DWORD dwCb = 0;
		DWORD dwConnections = 0;
		DWORD dwRet = ERROR_SUCCESS;
		
		dwRet = RasEnumConnections(lpRasConn, &dwCb, &dwConnections);//Get lpRasConn Buffer size.
		if (dwRet == ERROR_BUFFER_TOO_SMALL) {
			lpRasConn = (LPRASCONN)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwCb);//==>Heap alloc
			if (lpRasConn == NULL)
			{
				_tprintf(L"Error ! HeapAlloc Failed!\n");
				exit(1);
			}
		}
		if (lpRasConn)
		lpRasConn[0].dwSize = sizeof(RASCONN);//Must contain RASCONN structure size

		if (RasEnumConnections(lpRasConn, &dwCb, &dwConnections) == ERROR_SUCCESS && lpRasConn!= NULL)
		{
			_tprintf(L"Active connection detected!If you want to reconnet,Please disconnect it first!\n");
			_tprintf(L"There are active connections:\n");
			for (DWORD i = 0; i < dwConnections; i++) {
				_tprintf(L"%s\n", lpRasConn[i].szEntryName);
			}
			exit(1);
		}
		if (lpRasConn && !HeapFree(GetProcessHeap(), 0 , lpRasConn))//==>Free the Heap if HeapAlloc is called....
		{
			_tprintf(L"Heap free error!%d", GetLastError());
			exit(1);
		}
		//***********************Is Entry Exist?*****************************

		LPTSTR lpszEntry = L"Srun3 PPPoE";
		RASENTRY *lpRasEnt = NULL;
		if (RasValidateEntryName(NULL, lpszEntry) != ERROR_ALREADY_EXISTS)
		{
			DWORD lpdwEntryInfoSize = NULL;//Create New Entry.
			if (RasGetEntryProperties(NULL, L"", NULL, &lpdwEntryInfoSize, NULL, NULL) == ERROR_BUFFER_TOO_SMALL)
			{
				lpRasEnt = (LPRASENTRY)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpdwEntryInfoSize);
				if (lpRasEnt == NULL)
				{
					_tprintf(L"Error! HeapAlloc Failed!\n");
					exit(GetLastError());
				}

				ZeroMemory(lpRasEnt, sizeof(RASENTRY));

				TCHAR szDeviceName[RAS_MaxDeviceName + 1] = { 0 };
				if (!GetPortName(szDeviceName)) {
					_tprintf(L"No Port Found! Exit!");
					exit(1);
				}

				lpRasEnt->dwSize = sizeof(RASENTRY);
				lpRasEnt->dwfOptions = RASEO_SwCompression | RASEO_RemoteDefaultGateway | RASEO_PreviewUserPw | RASEO_PreviewDomain;
				_tcscpy_s(lpRasEnt->szDeviceName, szDeviceName);
				_tcscpy_s(lpRasEnt->szDeviceType, L"pppoe");
#if (WINVER >= 0x600)
				lpRasEnt->dwfNetProtocols = RASNP_Ip | RASNP_Ipv6;//IPV6 Is Enabled Default.
#else
				lpRasEnt->dwfNetProtocols = RASNP_Ip:
#endif
				if (RasSetEntryProperties(NULL, lpszEntry, lpRasEnt, lpdwEntryInfoSize, NULL, NULL) != ERROR_SUCCESS)
				{
					_tprintf(L"Cannot Create a PPPoE Connection!\n");
					exit(1);
				}
				if (lpRasEnt && !HeapFree(GetProcessHeap(), NULL, lpRasEnt))//==> Free Heap 
				{
					_tprintf(L"Heap Free Error!%d\n",GetLastError());
					exit(1);
				}
			}
		}
		//Begin to establish a PPPoE Connection.
		if (dwConnections < 1)
		{
			_tprintf(L"Begin to Dial\n");
			hCon = DoPPP(szUserName, szPassWord);
		}
		fclose(pFile);
		pFile = NULL;
	}
	if (argc == 2)
	{
		char szArg[128] = { 0 };
		int n=strlen(argv[1]);
		//strncpy_s(szArg, strlen(argv[1]), (char*)_strupr_s(argv[1],sizeof(argv[1])),128);
		for (int i = 0; i < n; i++)
			szArg[i] = toupper(argv[1][i]);
		if (!strcmp(szArg, "HELP"))
		{
			ShowHelp();
			exit(0);
		}
		else if (!(strcmp(szArg, "SETUP")))
		{
			_tprintf(L"Welcome! The following steps will help you to setup your UserName and PassWord of Srun3k PPPoE\n");
			FILE *pFile = NULL;
			_TCHAR szCurrnetPath[MAX_PATH];
			_TCHAR szFilePath[MAX_PATH];
			GetModuleFileName(NULL, szFilePath, MAX_PATH);
			(_tcsrchr(szFilePath, _T('\\')))[1] = 0;
			_tcscpy_s(szCurrnetPath, MAX_PATH, szFilePath);
			_tcscat_s(szCurrnetPath, FILENAME_MAX, L"Phone_Book.ROP");
			if (!_tfopen_s(&pFile, szCurrnetPath, L"r"))
			{
				_tprintf(L"An Config File is Exist. Would you Like to Rewrite it? All content will be erased!(y/n)");
				char put = toupper(getchar());
				_tprintf(L"\n");
				if (put == 'N')
				{
					_tprintf(L"Nothing changed. Exit....\n");
					exit(0);
				}
				else if (put == 'Y')
				{
					_tprintf(L"Erasing...\n");
					//_tfopen_s(&pFile, szCurrnetPath, L"wb");
				}
				else
				{
					_tprintf(L"Please Input Y/N!\n");
					exit(0);
				}
			}
			else
				_tprintf(L"Create config file...\n");
			fflush(stdin);//It's not support by standard C.But we don't care because we all use Windows!!!
			if(pFile != NULL)
				fclose(pFile);
			_tfopen_s(&pFile, szCurrnetPath, L"wb+");
			if (pFile == NULL)
			{
				_tprintf(L"Error can't write file!\n");
				exit(1);
			}
			_tprintf(L"Please enter your UserName\n>>>");
			char szbuffer[128] = { 0 };
			scanf_s("%s", &szbuffer,128);
			//fwrite(szbuffer, sizeof(char), sizeof(szbuffer), pFile);
			for (unsigned int i = 0; i < strlen(szbuffer); i++)
				fputc(szbuffer[i], pFile);
			_tprintf(L"Please enter your PassWord\n>>>");
			fputc('$', pFile);
			scanf_s("%s", &szbuffer, 128);
			//fwrite(szbuffer, sizeof(char), sizeof(szbuffer), pFile);
			//fputc("")//EOF
			for (unsigned int i = 0; i < strlen(szbuffer); i++)
				fputc(szbuffer[i], pFile);
			_tprintf(L"OK! Run pon to dial..\n");
			fclose(pFile);
			pFile = NULL;

			//ShowHelp();
			exit(0);
		}

		else {
			printf("Cannot find arg %s\n", argv[1]);
			ShowHelp();
			exit(0);
		}
	}
	if (argc == 3) {
		FILE *pFile = NULL;
		_TCHAR szCurrnetPath[MAX_PATH];
		_TCHAR szFilePath[MAX_PATH];
		GetModuleFileName(NULL, szFilePath, MAX_PATH);
		(_tcsrchr(szFilePath, _T('\\')))[1] = 0;
		_tcscpy_s(szCurrnetPath, MAX_PATH, szFilePath);
		_tcscat_s(szCurrnetPath, FILENAME_MAX, L"Phone_Book.ROP");
		if (!_tfopen_s(&pFile, szCurrnetPath, L"r"))
		{
			_tprintf(L"An Config File is Exist. Would you Like to Rewrite it? All content will be erased!(y/n)");
			char put = toupper(getchar());
			_tprintf(L"\n");
			if (put == 'N')
			{
				_tprintf(L"Nothing changed. Exit....\n");
				exit(0);
			}
			else if (put == 'Y')
			{
				_tprintf(L"Erasing...\n");
				//_tfopen_s(&pFile, szCurrnetPath, L"wb");
			}
			else
			{
				_tprintf(L"Please Input Y/N!\n");
				exit(0);
			}
		}
		else
			_tprintf(L"Create config file...\n");
		if(pFile != NULL)
			fclose(pFile);
		fflush(stdin);//It's not support by standard C.But we don't care because we all use Windows!!!
		_tfopen_s(&pFile, szCurrnetPath, L"wb");
		
			//_tprintf(L"Please enter your UserName\n>>>");
			//char szbuffer[128];
			//scanf_s("%s", &szbuffer, 128);
		//fwrite(argv[1], sizeof(char), sizeof(argv[1]), pFile);
		//_tprintf(L"Please enter your PassWord\n>>>");
		for (unsigned int i = 0; i < strlen(argv[1]); i++)
			fputc(argv[1][i], pFile);
		fputc('$', pFile);
		//scanf_s("%s", &szbuffer, 128);
		//fwrite(argv[2], sizeof(char), sizeof(argv[2]), pFile);
		//fputc("")//EOF
		for (unsigned int i = 0; i < strlen(argv[2]); i++)
			fputc(argv[2][i], pFile);
		_tprintf(L"OK! Run pon to dial..\n");
		fclose(pFile);
		pFile = NULL;
			//ShowHelp();
		exit(0);
		
	}
	if (argc > 3) {
		_tprintf(L"Too Many Args!\n");
		ShowHelp();
		exit(0);
	}
	return 0;
}
