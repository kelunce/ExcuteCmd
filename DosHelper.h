/*
本类提供dos命令执行,并存储执行的结果,dos命令辅助工具
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
	//防止窗口自动关闭
	virtual ~CDosCmdHelper()
	{
		system("pause");
	}

	//外部接口
public:

	//执行CMD命令,不存储返回值
	inline void ExcutuCmd(const string strCmd){system(strCmd.c_str());};

	//执行CMD命令并存储返回字符串
	bool ExcutuCmdRet(const string strCmd);

	//打印彩色字符串
	void PrintStr(const string strWords, WORD color, ...);

	//通过进程名称过去进程id
	bool GetPIDByName(const string strProcess,vector<DWORD> &vecID);

	//获取命令结果
	SETLINES &GetCmdRet(){ return m_setLines;};

private:
	//提升本进程权限,debug模式下不需要
	bool GetDebugPriv();

	//存储命令返回结果
	SETLINES m_setLines;

	//命令
	string m_strCmd;
};

bool CDosCmdHelper::ExcutuCmdRet( const string strCmd )
{
	m_setLines.clear();

	//创建读写管道,dos命令也支持管道,比如netstat -ano |find "3052" && netstat -ano |find "548"
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

	//通过进程的方式执行dos命令,而不是ExcuteCmd方法
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	si.cb = sizeof(STARTUPINFO);
	GetStartupInfo(&si);

	//设置进程的输出控制台
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	if (!CreateProcess(NULL,const_cast<char *>(strCmd.c_str()),NULL,NULL,TRUE,NULL,NULL,NULL,&si,&pi))
	{
		cerr<<"error Create cmd excute Process Failed"<<endl;
		return false;
	}


	//等待dos命令进程结束
	if(WaitForSingleObject(pi.hProcess, INFINITE)==WAIT_FAILED)
	{
		cerr<<"error wait cmd excute Failed"<<endl;
		return false;
	}

	//从管道中读出dos命令执行结果并解析打印
	char buffer[5120];
	DWORD bytesRead;
	while(1)
	{
		ZeroMemory(buffer,5120);
		if(ReadFile(hRead,buffer,5120,&bytesRead,NULL))
		{
			//使用stringstream读取行,注意包含头文件
			stringstream strSource(buffer);
			string szLine;		

			//使用std::getline,而不是stringstream::getline
			while (std::getline(strSource,szLine).good())
			{
				if(szLine.length() > 0)
					m_setLines.push_back(szLine);
			}		
		}

		//是否已经读完了
		if(bytesRead < 5120)
			break;
	}

	CloseHandle(hRead);
	CloseHandle(hWrite);

	return true;
}



void CDosCmdHelper::PrintStr(const string strWords, WORD color, ...) 
{
	//COLOR命令颜色会影响到整个控制台,不能在这里使用
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
	//转为小写
	string strPName = strProcess;
	transform(strPName.begin(), strPName.end(), strPName.begin(), tolower);

	//DWORD dwErr = GetLastError();
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe;
	Process32First(hSnapshot, &pe);
	bool bExit = false;
	do
	{
		//转为小写
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
