/************************************************************
*                     Author:Pluviophile                    *
*                    Date:2020/9/27-23:03                   *
*     E-Mail:1565203609@qq.com/pluviophile12138@outlook.com *
*         Զ�߳�ע�룬��DllPathָ����dllע��ָ���Ľ���      *
*************************************************************/

#pragma once
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>
#include <TlHelp32.h>

/*�ж�ϵͳ�ܹ���������ZwCreateThreadEx����ָ��*/
#ifdef _WIN64
typedef	DWORD(WINAPI* pZwCreateThreadEx)(
	PHANDLE ThreadHandle,
	ACCESS_MASK DesiredAccess,
	LPVOID ObjectAttributes,
	HANDLE ProcessHandle,
	LPTHREAD_START_ROUTINE lpStartAddress,
	LPVOID lpParameter,
	ULONG CreateThreadFlags,
	SIZE_T ZeroBits,
	SIZE_T StackSize,
	SIZE_T MaximumStackSize,
	LPVOID pUnkown
	);
#else
typedef DWORD(WINAPI* typedef_ZwCreateThreadEx)(
	PHANDLE ThreadHandle,
	ACCESS_MASK DesiredAccess,
	LPVOID ObjectAttributes,
	HANDLE ProcessHandle,
	LPTHREAD_START_ROUTINE lpStartAddress,
	LPVOID lpParameter,
	BOOL CreateSuspended,
	DWORD dwStackSize,
	DWORD dw1,
	DWORD dw2,
	LPVOID pUnkown
	);
#endif

/*
�趨�����̵ĳ������Ȩ��
lPcstr:Ȩ���ַ���
backCode:���󷵻���
*/
BOOL GetDebugPrivilege(
_In_ LPCSTR lPcstr,
_Inout_ DWORD* backCode
)
{
	HANDLE Token = NULL;
	LUID luid = { 0 };
	TOKEN_PRIVILEGES Token_privileges = { 0 };
	//�ڴ��ʼ��Ϊzero
	memset(&luid, 0x00, sizeof(luid));
	memset(&Token_privileges, 0x00, sizeof(Token_privileges));

	//�򿪽�������
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &Token))
	{
		*backCode = 0x01;
		return FALSE;
	}

	//��ȡ��Ȩluid
	if (!LookupPrivilegeValue(NULL,lPcstr,&luid))
	{
		*backCode = 0x02;
		return FALSE;
	}

	//�趨�ṹ��luid����Ȩ
	Token_privileges.PrivilegeCount = 1;
	Token_privileges.Privileges[0].Luid = luid;
	Token_privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	//�޸Ľ�����Ȩ
	if (!AdjustTokenPrivileges(Token, FALSE, &Token_privileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
	{
		*backCode = 0x03;
		return FALSE;
	}
	*backCode = 0x00;
	return TRUE;
}

/*
���ݽ�������ȡ����pid��ִ�����󷵻ؽ���pid��������-1
ProcessName:������
backCode:���󷵻���
*/
int GetProcessPid(
	_In_ const char* ProcessName,
	_Inout_ DWORD* backCode
)
{
	PROCESSENTRY32 P32 = { 0 };
	HANDLE H32 = NULL;
	//�ڴ��ʼ��Ϊzeor
	memset(&P32, 0X00, sizeof(P32));
	//��������
	H32 = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	P32.dwSize = sizeof(P32);
	if (H32 == NULL)
	{
		*backCode = 0x01;
		return -1;
	}
	//��ʼѭ����������
	BOOL ret = Process32First(H32, &P32);
	while (ret)
	{
		//����ָ�����̴���
		if (!strcmp(P32.szExeFile, ProcessName))
		{
			*backCode = 0x00;
			return P32.th32ProcessID;
		}
		ret = Process32Next(H32, &P32);
	}
	*backCode = 0x01;
	return -1;
}

/*
������
*/
int main(int argv, char* argc[])
{
	//�Ա�Ҫ�ı������������Լ���ʼ��
	DWORD backCode = 0;
	HANDLE hProcess = NULL;
	LPVOID Buff = NULL;
	LPVOID LoadLibraryBase = NULL;
	char DllPath[] = "D:\\cp\\pk\\x64\\Release\\pk.dll";
	DWORD DllPathLen = strlen(DllPath) + 1;
	HMODULE Ntdll = NULL;
	SIZE_T write_len = 0;
	DWORD dwStatus = 0;
	HANDLE hRemoteThread = NULL;

	//ͨ����������ȡpid
	int pid = GetProcessPid("notepad.exe", &backCode);
	if (pid == -1)
	{
		puts("pid get error");
		return 0;
	}

	//����������Ȩ����õ���Ȩ��
	if (!GetDebugPrivilege(SE_DEBUG_NAME, &backCode))
	{
		puts("DBG privilege error");
		printf(" %d", backCode);
		return 0;
	}

	//��Ҫ��ע��Ľ���
	if ((hProcess=OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid))== NULL)
	{
		puts("process open erro");
		return 0;
	}

	//��Ҫ��ע��Ľ����д����ڴ棬���ڴ��ע��dll��·��
	Buff = VirtualAllocEx(hProcess, NULL, DllPathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (Buff==NULL)
	{
		puts("Buff alloc error");
		return 0;
	}

	//��dll·��д��ոմ������ڴ���
	WriteProcessMemory(hProcess, Buff, DllPath, DllPathLen, &write_len);
	if(DllPathLen != write_len)
	{
		puts("write error");
		return 0;
	}

	//��kernel32.dll�л�ȡLoadLibrary����
	LoadLibraryBase = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
	if (LoadLibraryBase == NULL)
	{
		puts("kernel32 get error");
		return 0;
	}

	//����ntdll.dll�����л�ȡ�ں˺���ZwCreateThread����ʹ�ú���ָ��ָ��˺���
	Ntdll = LoadLibrary("ntdll.dll");
	pZwCreateThreadEx ZwCreateThreadEx = 
		(pZwCreateThreadEx)GetProcAddress(Ntdll, "ZwCreateThreadEx");
	if (ZwCreateThreadEx == NULL)
	{
		puts("func get error");
		return 0;
	}

	//ִ��ZwCreateThread��������ָ�������д����̼߳���Ҫ��ע���dll
	dwStatus = ZwCreateThreadEx(
		&hRemoteThread,
		PROCESS_ALL_ACCESS,
		NULL,
		hProcess,
		(LPTHREAD_START_ROUTINE)LoadLibraryBase,
		Buff,
		0, 0, 0, 0,
		NULL
	);
	if (hRemoteThread == NULL)
	{
		puts("zwcreatethread fun error");
		return 0;
	}

	//�ͷŲ���Ҫ�ı����Լ��ڴ�
	CloseHandle(hProcess);
	FreeModule(Ntdll);
	ExitProcess(0);
	return 0;
}