/**

MOVELATR v1.02, (c)2000-2008 Jeremy Collake
All Rights Reserved.
jeremy@bitsum.com
http://www.bitsum.com

Freeware from Bitsum Technologies.
Use of this code must be accompanied with credit. Commercial use prohibited
without authorization from Jeremy Collake.

------------------------------------------------------------------------------------------
This software is provided as-is, without warranty of ANY KIND,
either expressed or implied, including but not limited to the implied
warranties of merchantability and/or fitness for a particular purpose.
The author shall NOT be held liable for ANY damage to you, your
computer, or to anyone or anything else, that may result from its use,
or misuse. Basically, you use it at YOUR OWN RISK.
-------------------------------------------------------------------------------------------

This simple console mode application will set up files or directories
to be renamed, deleted, or replaced at bootime. This is particularly
useful when replacing files that are persistently in use when the
operating system is booted. Movelatr also displays and clears
pending move operations. Full C++ source code is included.

   USAGE:

  MOVELATR
        = enumerates pending move operations.
  MOVELATR source destination
    = sets up move of source to destination at boot.
  MOVELATR source /D
    = deletes source at boot.
  MOVELATR /C
    = clear all pending move operations.

Moving of a directory to another non-existant one on the same
partition is supported (this may be considered a rename).

Moving of files from one volume to another is not supported
under NT/2k. You must first copy the file to the volume as the
destination, and then use movelatr.

Deletion of directories will not occur unless the directories
are empty.

Wildcards are not supported at this time.

#Revision History:
  - v1.02 Added SetAllowProtectedRenames for WFP protected files.
         + credit for this change goes to Peter Kovacs.
  - v1.01        Added awareness of WFP/SFC (Win2000+).
            Cleaned up portions of code
  - v1.00        Initial Release


*/

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#pragma warning(disable: 4996)

#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include <winuser.h>
#include <tchar.h>

#define ERROR_CMDLINE                   1
#define ERROR_SOURCEFILE                2
#define ERROR_DESTFILE                  3
#define ERROR_ABORTED                   4
#define ERROR_DIRFILECONFLICT           5
#define ERROR_DIREXISTS                 6
#define ERROR_DRIVEINCONSISTENT         7
#define ERROR_MOVE                      8
#define ERROR_FAILEDCLEAR               9
#define ERROR_WILDCARD                  10
#define ERROR_SFP                       11


int currentclen = 100;
TCHAR* temps[10];
TCHAR* tcharify(int slot, const char* input) {
	int neededclen;
	if (temps[slot] == NULL) {
		temps[slot] = malloc(currentclen * sizeof(TCHAR));
	}
	neededclen = _mbstrlen(input);
	if (neededclen + 1 > currentclen) {
		TCHAR* nt;
		nt = realloc(temps[slot], (neededclen + 10) * sizeof(TCHAR));
		if (!nt) {
			free(temps[slot]);
			temps[slot] = malloc((neededclen + 4) * sizeof(TCHAR));
		}
	}
	if (!temps[slot]) {
		return NULL;
	}

	if (sizeof(TCHAR) == 1) {
		strcpy((char*)(temps[slot]), input);
	}
	else {
		size_t nc;
		size_t siw = neededclen;
		size_t mc = _TRUNCATE;
		mbstowcs_s(&nc, (WCHAR*)(temps[slot]), siw, input, mc);
	}
	return temps[slot];
}
void de_tcharify(char* dest, int destsz, const TCHAR* src) {
	if (sizeof(TCHAR) == 1) {
		strcpy(dest, (const char*)src);
	}
	else {
		size_t nc;
		size_t siw = _tcsclen(src);
		size_t mc = _TRUNCATE;
		size_t dsz = destsz;
		wcstombs_s(&nc, dest, dsz, src, mc);
	}
}


int IsWin9x()
{
        OSVERSIONINFO vInfo;
        vInfo.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
        GetVersionEx(&vInfo);
        return vInfo.dwPlatformId==
			VER_PLATFORM_WIN32_NT?0:1;
}

int DoesDirectoryExist(char *pDirName) {
        unsigned int nAttributes=0;
        if((nAttributes=GetFileAttributes(tcharify(0,pDirName)))==-1)
                return 0;
        if((nAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0)
                return 1;
        else return 0;
}

int DoesFileExist(char *pFileName)
{
        unsigned int nAttributes=0;
        if((nAttributes=GetFileAttributes(tcharify(0,pFileName)))==-1)
                return 0;
        return 1;
}

typedef DWORD (__stdcall *PFNSfcIsFileProtected)
	(HANDLE, LPCWSTR);
	
int IsProtectedFile(char *pFileName)
{
        WCHAR UniFileName[MAX_PATH];
        HINSTANCE hSFC;
        PFNSfcIsFileProtected SfcIsFileProtected;
        int bR;
        hSFC=LoadLibrary(_T("SFC.DLL"));
        if(!hSFC)
        {
                return 0;
        }
        SfcIsFileProtected=(PFNSfcIsFileProtected)
			GetProcAddress(hSFC,"SfcIsFileProtected");
        if(!SfcIsFileProtected)
        {
                FreeLibrary(hSFC);
                return 0;
        }
        MultiByteToWideChar(CP_ACP,
			0,
			pFileName,
			strlen(pFileName),
			UniFileName,
			MAX_PATH);
        bR=SfcIsFileProtected(NULL,UniFileName);
        FreeLibrary(hSFC);
        return bR;
}

const char * sccscsm =
"SYSTEM\\CurrentControlSet\\Control\\Session Manager";


int SetAllowProtectedRenames()
{
	HKEY hKey;
	DWORD val = 1;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		tcharify(0,sccscsm),
		0, 
		KEY_SET_VALUE, 
		&hKey)!=ERROR_SUCCESS)
	{
		return 0;
	}
	RegSetValueEx(hKey, 
		_T("AllowProtectedRenames"),
		0, 
		REG_DWORD, 
		(const byte*)&val,
		sizeof(DWORD));
	RegCloseKey(hKey);
	return 1;
}

char *ExtractAsciiString(char *wc, char *pszKey)
{
	while(*wc)
	{
		*pszKey=*wc;
		pszKey++;
		wc++;
	}
	*pszKey=0;
	return wc+1;
}

/** @return 1 if equal found, 0 if not */
int ExtractToEqual(char *pszLine,
char *pszFile,
char **pszNewLinePos)
{
	while(*pszLine && *pszLine!='=')
	{
		*pszFile=*pszLine;
		pszLine++;
		pszFile++;
	}
	if(*pszLine=='=')
	{
		*pszNewLinePos=pszLine+1;
		return 1;
	}
	*pszNewLinePos=pszLine;
	return 0;
}

char *pszMoveKey=
"SYSTEM\\CurrentControlSet\\Control\\Session Manager";
char *pszValueName="PendingFileRenameOperations";
#define MAX_VALUESIZE 8192

int ClearPendingMoves()
{
	HKEY hKey;
	DWORD dispo;
	DWORD dwSize=MAX_VALUESIZE;
	DWORD dwType=REG_BINARY;
	//char szNewData=0;
	if(!IsWin9x())
	{
		if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,
			tcharify(0,pszMoveKey),
			0,
			0,
			0,
			KEY_WRITE,
			0,
			&hKey,
			&dispo)==ERROR_SUCCESS)
		{
			if((dispo=RegDeleteValue(hKey,tcharify(0,pszValueName)))
				!=ERROR_SUCCESS)
			{
				RegCloseKey(hKey);
				if(dispo==ERROR_FILE_NOT_FOUND)
				{
					return 1;
				}
				return 0;
			}
			RegCloseKey(hKey);
			return 1;
		}
		return 0;
	}
	return WritePrivateProfileSection(_T("rename"),
		//(char *)&szNewData,
		_T("\0"),
		_T("wininit.ini"));
}
int ShowPendingMoves()
{
	HKEY hKey;
	DWORD dispo;
	DWORD dwSize=MAX_VALUESIZE;
	DWORD dwType=REG_BINARY;
	char *pBuffer;
	char *pAscii;
	char *pTempBuffer;
	unsigned int nMoveNum=0;
	if(!IsWin9x())
	{
		pBuffer=malloc(MAX_VALUESIZE);
		if (!pBuffer) {
			return 0;
		}
		pAscii=malloc((MAX_VALUESIZE+1)/2);
		if (!pAscii) {
			return 0;
		}
		if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,
			tcharify(0,pszMoveKey),
			0,
			0,
			0,
			KEY_READ,
			0,
			&hKey,
			&dispo)==ERROR_SUCCESS)
		{
			if(RegQueryValueEx(hKey,
				tcharify(0,pszValueName),
				0,
				&dwType,
				(BYTE *)pBuffer,
				&dwSize)!=ERROR_SUCCESS)
			{
				printf("\n None.");
				RegCloseKey(hKey);
				free(pBuffer);
				free(pAscii);
				return 0;
			}
			RegCloseKey(hKey);
			pTempBuffer=pBuffer;
			while(1)
			{
				*pAscii=0;
				pTempBuffer=
					ExtractAsciiString(pTempBuffer,pAscii);
				if(!(*pAscii))
				{
					free(pBuffer);
					free(pAscii);
					return 1;
				}
				printf("\n %d.] %s => ",
					nMoveNum,
					(pAscii+4));
				*pAscii=0;
				pTempBuffer=
					ExtractAsciiString(pTempBuffer,pAscii);
				printf("%s", *pAscii?(pAscii+4):"NULL");
				nMoveNum++;
			}
		}
		else
		{
			free(pBuffer);
			free(pAscii);
			printf("\n Error opening HKLM\\%s", pszMoveKey);
			return 0;
		}
	}
	else
	{
		FILE *hWinInit;
		UINT pBufferStrLen;
		pBuffer=malloc(MAX_PATH*2);
		pAscii=malloc(MAX_PATH);
		if (!pBuffer || !pAscii) {
			return 0;
		}
		/*If the function succeeds, the return value is the
		length of the string copied to the buffer, in TCHARs,
		not including the terminating null character.*/
		{
			TCHAR tpBuffer[MAX_PATH * 2];
			pBufferStrLen = GetWindowsDirectory(tpBuffer, MAX_PATH);
			de_tcharify(pBuffer, sizeof pBuffer, tpBuffer);
		}
		strcat(pBuffer,"\\wininit.ini");
		hWinInit=fopen(pBuffer,"r");
		if(hWinInit)
		{
			// find [rename] section
			while(!strstr(strupr(pBuffer),"[RENAME]")
				&& fgets(pBuffer,MAX_PATH*2,hWinInit));
			while(fgets(pBuffer,MAX_PATH*2,hWinInit))
			{
				pTempBuffer=pBuffer;
				if(!ExtractToEqual(pTempBuffer,
					pAscii,
					&pTempBuffer))
				{
					break;
				}
				if(*(pTempBuffer
					+strlen(pTempBuffer)-1)=='\n')
				{
					*(pTempBuffer+strlen(pTempBuffer)-1)=0;
				}
				printf("\n %d.] %s => %s", 
					nMoveNum,
					pTempBuffer, 
					pAscii);
				nMoveNum++;
			}
			fclose(hWinInit);
		}
		free(pBuffer);
		free(pAscii);
	}
	if(!nMoveNum)
	{
		printf("\n None.");
	}
	return 1;
}
const char * e_de_str = 
"\n Error dest dir already exists. Can't move directory.\n";
const char * e_dfc_str = 
"\n Error src is a directory and dest is a file.\n";
const char * e_dic_str = 
"\n Error src and dest must be on the same drive.\n";
const char * m_eapr1_str = 
"\n Set AllowProtectedRenames=1 to reg for WFP move op.";
const char * q_owd_str = 
"\n Dest already exists, overwrite it? [Y/N] ";
const char * e_pcts_mec_pctd = 
"\n Error setting up %s to be moved. Error code: %d\n";
const char * e_pcts_dec_pctd = 
"\n Error setting up %s to be deleted. Error code: %d\n";
const char * m_pcts_pcts_pcts_dasb = 
"\n Src: %s\n Dest: %s\n %s will be done at system boot.\n";
int main(int argc, char **argv)
{
	char szSourceFile[MAX_PATH];
	char szDestFile[MAX_PATH];
	char szTemp[MAX_PATH];
	char *szFile = NULL;
	int bDelete=0;
	int bNT=!IsWin9x();

	printf("\n Move After");
	printf("\n Kernel is %s\n", bNT?"WinNT":"Win9x");
	if(argc<3)
	{
		if(argc==2 && !strcmpi(argv[1],"/C"))
		{
			int nR;
			printf("\n Clearing pending moves .. %s\n", 
			(nR=ClearPendingMoves())?"Success":"Failed!");
			return nR?ERROR_SUCCESS:ERROR_FAILEDCLEAR;
		}
		printf("\n Usage: MVAFTER");
		printf("\n  = enumerates pending move operations.");
		printf("\n  MVAFTER source dest");
		printf("\n  = will move source to dest at reboot.");
		printf("\n  MVAFTER source /d");
		printf("\n  = sets up delete of source at reboot.");
		printf("\n  MVAFTER /c");
		printf("\n  = clears all pending operations.\n");
		printf("\n Pending move operations:");
		ShowPendingMoves();
		printf("\n");
		return ERROR_CMDLINE;
	}
	if(strstr(argv[1],"*")
		|| strstr(argv[2],"*") 
		|| strstr(argv[1],"?") 
		|| strstr(argv[2],"*"))
	{
		printf("\n Wildcards not supported.\n");
		return ERROR_WILDCARD;
	}
	if(strstr(strupr(argv[2]),"/D"))
	{
		bDelete=1;
		strcpy(szDestFile,"NULL");
	}
	/*--tcharification--*/
	{
		TCHAR tszSourceFile[MAX_PATH];
		TCHAR* tszFile;
		if (!GetFullPathName(tcharify(0, argv[1]),
			MAX_PATH,
			tszSourceFile,
			& tszFile))
		{
			strcpy(szSourceFile, argv[1]);
		}
		else {
			int a;
			static char fntemp[MAX_PATH];
			de_tcharify(szSourceFile, sizeof szSourceFile, tszSourceFile);
			a = _tcsclen(tszFile);
			de_tcharify(fntemp, sizeof fntemp, tszFile);
			szFile = fntemp;
		}
	}
	
	if (!bDelete) {
		TCHAR tszDestFile[MAX_PATH];
		if (!GetFullPathName(tcharify(0,argv[2]),
			MAX_PATH,
			tszDestFile,
			NULL))
		{
			strcpy(szDestFile, argv[2]);
		}
		else {
			de_tcharify(szDestFile, sizeof szDestFile, tszDestFile);
		}
	}
	
	if(IsWin9x()) {
		TCHAR tszTemp[MAX_PATH];
		if(GetShortPathName(tcharify(0,szSourceFile),tszTemp,MAX_PATH)) {
			de_tcharify(szTemp, sizeof szTemp, tszTemp);
			strcpy(szSourceFile,szTemp);
		}
		if(!bDelete) {
			if(GetShortPathName(tcharify(0,szDestFile),tszTemp,MAX_PATH)) {
				de_tcharify(szTemp, sizeof szTemp, tszTemp);
				strcpy(szDestFile,szTemp);
			}
		}
	}
	if(!DoesFileExist(szSourceFile))
	{
		printf("\n Source file %s does not exist!\n", 
		szSourceFile);
		return ERROR_SOURCEFILE;
	}
	if(!bDelete)
	{
		if(DoesDirectoryExist(szSourceFile))
		{
			if(DoesDirectoryExist(szDestFile))
			{
				printf(e_de_str);
				return ERROR_DIREXISTS;
			}
			if(DoesFileExist(szDestFile))
			{
				printf(e_dfc_str);
				return ERROR_DIRFILECONFLICT;
			}
		}
		if(szSourceFile[1]==':' 
			&& szDestFile[1]==':' 
			&& (toupper(szSourceFile[0])
				!=toupper(szDestFile[0])) 
			&& bNT)
		{
			printf(e_dic_str);
			return ERROR_DRIVEINCONSISTENT;
		}
		if(DoesDirectoryExist(szDestFile))
		{
			if(szDestFile[strlen(szDestFile)-1]!='\\')
			{
				strcat((char *)&szDestFile,"\\");
			}
			if (!szFile) {
				printf("szFile was null!\n");
				return 0;
			}
			strcat(szDestFile,szFile);
		}
		if(DoesFileExist(szDestFile))
		{
			if(IsProtectedFile(szDestFile))
			{
				printf(m_eapr1_str);
				if(!SetAllowProtectedRenames())
				{
					printf("\n ERROR: Setting reg key.");
					return ERROR_SFP;
				}
			}
			char ch=0;
			printf(q_owd_str);
			while(ch!='Y' && ch!='N')
			{
				ch=toupper(getch());
			}
			putch(ch);
			if(toupper(ch)=='N')
			{
				printf("\n Operation aborted.\n");
				return ERROR_ABORTED;
			}
		}
	} // end if !bDelete

	if(!bNT)
	{
		if(!bDelete)
		{
			/*
			WritePrivateProfileString("rename",
				"NUL", // what?
				szDestFile,
				"wininit.ini");
				WritePrivateProfileString("rename",
				szDestFile,
				szSourceFile,
				"wininit.ini");
			*/
			/*
			https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-writeprivateprofilestringa
			
			lpKeyName (second parameter):
			The name of the key to be associated with a
			string. If the key does not exist in the
			specified section, it is created. If this
			parameter is NULL, the entire section, including
			all entries within the section, is deleted.
			
			NUL is a special "filename" in windows, but what
			does renaming NUL to destination do? Truncate
			destination to a 0byte file?
			*/
			WritePrivateProfileString(_T("rename"),
				_T("NUL"),
				tcharify(0,szDestFile),
				_T("wininit.ini"));
				WritePrivateProfileString(_T("rename"),
				tcharify(1,szDestFile),
				tcharify(2,szSourceFile),
					_T("wininit.ini"));
		}
		else
		{
			WritePrivateProfileString(_T("rename"),
				_T("NUL"), // why?
				tcharify(0,szSourceFile),
				_T("wininit.ini"));
		}
	}
	else
	{
		if(!bDelete)
		{
			if(!MoveFileEx(tcharify(0,szDestFile),
				NULL,
				MOVEFILE_DELAY_UNTIL_REBOOT)
			|| !MoveFileEx(tcharify(0,szSourceFile),
				tcharify(1,szDestFile),
				MOVEFILE_DELAY_UNTIL_REBOOT))
			{
				printf(e_pcts_mec_pctd,
					DoesDirectoryExist(szSourceFile)
						?"directory":"file",
					GetLastError());
				return ERROR_MOVE;
			}
		}
		else
		{
			if(!MoveFileEx(tcharify(0,szSourceFile),
				NULL,
				MOVEFILE_DELAY_UNTIL_REBOOT))
			{
				printf(e_pcts_dec_pctd,
					DoesDirectoryExist(szSourceFile)
						?"directory":"file",
					GetLastError());
				return ERROR_MOVE;
			}
		}
	}
	printf(m_pcts_pcts_pcts_dasb,
		szSourceFile,
		szDestFile,
		bDelete?"Deletion":"Move");
	return ERROR_SUCCESS;
}
