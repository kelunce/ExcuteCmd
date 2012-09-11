/*
�����ṩdos����ִ��,���洢ִ�еĽ��,dos���������
*/
#pragma  once
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;
class CDosCmdHelper
{	
public:
	typedef vector<string> SETLINES;
	CDosCmdHelper(){m_setLines.clear();};
	//��ֹ�����Զ��ر�
	virtual ~CDosCmdHelper()
	{
		system("pause");
	}

	//�ⲿ�ӿ�
public:

	//ִ��CMD����,���洢����ֵ
	inline void ExcutuCmd(const string strCmd){system(strCmd.c_str());};

	//ִ��CMD����洢�����ַ���
	bool ExcutuCmdRet(const string strCmd);

	//��ӡ��ɫ�ַ���
	void PrintStr(const string strWords, WORD color, ...);

	//ͨ���������ƹ�ȥ����id
	bool GetPIDByName(const string strProcess,vector<DWORD> &vecID);

	//��ȡ������
	SETLINES &GetCmdRet(){ return m_setLines;};

private:
	//����������Ȩ��,debugģʽ�²���Ҫ
	bool GetDebugPriv();

	//�洢����ؽ��
	SETLINES m_setLines;

	//����
	string m_strCmd;
};

bool CDosCmdHelper::ExcutuCmdRet( const string strCmd )
{
	m_setLines.clear();

	//������д�ܵ�,dos����Ҳ֧�ֹܵ�,����netstat -ano |find "3052" && netstat -ano |find "548"
	SECURITY_ATTRIBUTES sa;
	HANDLE hRead,hWrite;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	if (!CreatePipe(&hRead,&hWrite,&sa,0))
	{
		cerr<<"error Error On CreatePipe"<<endl;
		return false;
	}

	//ͨ�����̵ķ�ʽִ��dos����,������ExcuteCmd����
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	si.cb = sizeof(STARTUPINFO);
	GetStartupInfo(&si);

	//���ý��̵��������̨
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	if (!CreateProcess(NULL,const_cast<char *>(strCmd.c_str()),NULL,NULL,TRUE,NULL,NULL,NULL,&si,&pi))
	{
		cerr<<"error Create cmd excute Process Failed"<<endl;
		return false;
	}


	//�ȴ�dos������̽���
	if(WaitForSingleObject(pi.hProcess, INFINITE)==WAIT_FAILED)
	{
		cerr<<"error wait cmd excute Failed"<<endl;
		return false;
	}

	//�ӹܵ��ж���dos����ִ�н����������ӡ
	char buffer[5120];
	DWORD bytesRead;
	while(1)
	{
		ZeroMemory(buffer,5120);
		if(ReadFile(hRead,buffer,5120,&bytesRead,NULL))
		{
			//ʹ��stringstream��ȡ��,ע�����ͷ�ļ�
			stringstream strSource(buffer);
			string szLine;		

			//ʹ��std::getline,������stringstream::getline
			while (std::getline(strSource,szLine).good())
			{
				if(szLine.length() > 0)
					m_setLines.push_back(szLine);
			}		
		}

		//�Ƿ��Ѿ�������
		if(bytesRead < 5120)
			break;
	}

	CloseHandle(hRead);
	CloseHandle(hWrite);

	return true;
}



void CDosCmdHelper::PrintStr(const string strWords, WORD color, ...) 
{
	//COLOR������ɫ��Ӱ�쵽��������̨,����������ʹ��
	WORD colorOld;
	HANDLE handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(handle, &csbi);
	colorOld = csbi.wAttributes;
	SetConsoleTextAttribute(handle, color);
	cout << strWords;
	SetConsoleTextAttribute(handle, colorOld);
}

bool CDosCmdHelper::GetDebugPriv()
{
#ifdef _DEBUG	
		return true;
#endif

	HANDLE hToken;
	LUID sedebugnameValue;
	TOKEN_PRIVILEGES tkp;

	if ( ! OpenProcessToken( GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
	{
		return false;
	}


	if ( ! LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &sedebugnameValue ) )
	{
		CloseHandle( hToken );
		return false;
	}

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = sedebugnameValue;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges( hToken, FALSE, &tkp, sizeof tkp, NULL, NULL ) )
	{
		CloseHandle( hToken );
		return false;
	}

	return true;
}

bool CDosCmdHelper::GetPIDByName( const string strProcess,vector<DWORD> &vecID )
{
	//תΪСд
	string strPName = strProcess;
	transform(strPName.begin(), strPName.end(), strPName.begin(), tolower);

	//DWORD dwErr = GetLastError();
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe;
	Process32First(hSnapshot, &pe);
	bool bExit = false;
	do
	{
		//תΪСд
		strlwr(pe.szExeFile);

		if( strstr( pe.szExeFile, ".exe" ) )
		{
			if( 0 == strcmp( pe.szExeFile, strPName.c_str() ) )
			{
				bExit = true;
				vecID.push_back(pe.th32ProcessID);
			}
		}
	} 
	while (Process32Next(hSnapshot, &pe));

	return bExit;
}
