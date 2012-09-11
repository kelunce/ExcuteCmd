#include "DosHelper.h"
#include <map>

void RunNetStat(int argc, char *argv[]);

void ShowProcessID(pair<string,DWORD> ele);

void PrintUsage();

void SwitchService(const string strName);

int main(int argc, char *argv[])
{
	
	if(argc == 1)
	{
		PrintUsage();
		system("pause");
		return 0;
	}

	string strPara(argv[1]);
	string strParaLow = strPara ;
	string strExeFile = ".exe";
	transform(strParaLow.begin(),strParaLow.end(),strParaLow.begin(),tolower);
	if(string::npos != strParaLow.find(strExeFile))
	{
		RunNetStat(argc,argv);
		return 0;
	}
	
	SwitchService(strPara);

	return 0;
}

void ShowProcessID(pair<string,DWORD> ele)
{
	if (ele.second == 0)
	{
		//process id == 0 is system idle process
		cout<<"not find process:"<<ele.first<<endl;
	}
	else
	{
		cout<<ele.first<<"  PID is  "<<ele.second<<endl;
	}
}

void PrintUsage()
{
	cout<<"usage:"<<endl;
	cout<<"usage :NetstatTest ttA.exe ttB.exe ���ӽ�������"<<endl;
	cout<<"usage :NetstatTest IncrediBuild_Agent �������߹رշ���"<<endl;
}

void RunNetStat(int argc, char *argv[])
{
	/*
	��netstat -anoΪ��,ִ��dos����,�����ʽ��ϵ������ĸ�ʽ�Ľ���
	C:\Documents and Settings\Administrator>netstat -ano

	Active Connections

	Proto  Local Address          Foreign Address        State           PID
	TCP    0.0.0.0:88             0.0.0.0:0              LISTENING       724
	*/

	//ʹ��map<DWORD,string>�ȽϺ�,�������ѧϰ��	
	typedef multimap<string,DWORD> MAPPID;
	MAPPID mapPName;

	//dos���������
	CDosCmdHelper dosCmd;

	//��ȡָ�����̵�id
	vector<DWORD> vecPID;
	for (int i = 1; i  < argc; ++i)
	{	
		vecPID.clear();	
		string strPName(argv[i]);		
		if(dosCmd.GetPIDByName(strPName,vecPID))
		{
			//���ﱾ������for_each��,����ÿ�ζ�Ҫ��������,ȷʵ������,�������ʹ��c++11��Lambda���ʽ
			for (int j = 0;j < vecPID.size(); ++j)
			{
				//һ�Զ��multimap,��֧���±�����,����ʹ��mapPName[strPName] = vecPID[j];
				mapPName.insert(make_pair(strPName,vecPID[j]));				
			}
		}
		else
		{
			mapPName.insert(make_pair(strPName.c_str(),0));	
		}		
	}

	//ִ��dos����netstat
	string strCmd ="netstat -ano";	
	do 
	{
		dosCmd.ExcutuCmd("cls");
		for_each(mapPName.begin(),mapPName.end(),ShowProcessID);

		if(dosCmd.ExcutuCmdRet(strCmd))
		{
			//�������
			bool bTitle = true;
			const CDosCmdHelper::SETLINES cmdRet = dosCmd.GetCmdRet();
			for (int i = 0;i < cmdRet.size(); ++i)
			{
				char szTemp[5][_MAX_PATH];
				ZeroMemory(szTemp,5*_MAX_PATH);
				if( 5 != sscanf(cmdRet[i].c_str(),"%s %s %s %s %s",szTemp[0],szTemp[1],szTemp[2],szTemp[3],szTemp[4]))
					continue;

				//��һ�ζ�����5������̧ͷ
				if(bTitle)
				{
					bTitle = false;
					cout<<cmdRet[i]<<endl;
					continue;
				}

				MAPPID::iterator iter = mapPName.begin();
				while (iter != mapPName.end())
				{
					if(iter->second == 0)
					{
						++iter;
						continue;
					}

					char szPID[16] = {0};
					itoa(iter->second,szPID,10);
					if ( 0 == strcmp(szTemp[4],szPID))
					{
						cout<<cmdRet[i]<<endl;
					}
					++iter;
				}
			}			
		}		
		cout<<endl;

		//��Լ5s���ٴ�ִ��
		Sleep(5000);

	} while (true/*cin>>strFlush*/);

	return ;
}

void SwitchService( const string strName )
{
	/*
	C:\Documents and Settings\Administrator>sc query IncrediBuild_Coordinator

	SERVICE_NAME: IncrediBuild_Coordinator
	TYPE               : 10  WIN32_OWN_PROCESS
	STATE              : 1  STOPPED
	(NOT_STOPPABLE,NOT_PAUSABLE,IGNORES_SHUTDOWN)
	WIN32_EXIT_CODE    : 0  (0x0)
	SERVICE_EXIT_CODE  : 0  (0x0)
	CHECKPOINT         : 0x0
	WAIT_HINT          : 0x0
	*/
	string strCmd = "sc query " + strName;
	CDosCmdHelper cmdHelper;
	if(!cmdHelper.ExcutuCmdRet(strCmd))
	{
		cout<<"error cmd"<<endl;
		return ;
	}

	CDosCmdHelper::SETLINES &vecRet = cmdHelper.GetCmdRet();
	if(vecRet.size() < 7)
	{
		//����
		for (int i = 0;i < vecRet.size(); ++i)
		{
			cout<<vecRet[i]<<endl;
		}
		return ;
	}

	for (int i = 0;i < vecRet.size(); ++i)
	{
		char szTemp[3][_MAX_PATH];
		ZeroMemory(szTemp,3*_MAX_PATH);
		if( 3 != sscanf(vecRet[i].c_str(),"%s :%s %s ",szTemp[0],szTemp[1],szTemp[2]))
			continue;
		if (0 == strcmp(szTemp[0],"STATE"))
		{
			string strCmdStart ;
			if (0 == strcmp(szTemp[2],"STOPPED"))
			{
				strCmdStart = "sc start " + strName;
				if(!cmdHelper.ExcutuCmdRet(strCmdStart))
				{
					cout<<"����"<<strName <<"ʧ��"<<endl;
					return ;
				}

				vecRet = cmdHelper.GetCmdRet();
				for (int i = 0;i < vecRet.size(); ++i)
				{
					cout<<vecRet[i]<<endl;
				}

			}
			else if (0 == strcmp(szTemp[2],"RUNNING"))
			{
				string strCmdStart = "sc stop " + strName;
				if(!cmdHelper.ExcutuCmdRet(strCmdStart))
				{
					cout<<"ֹͣ"<<strName <<"ʧ��"<<endl;
					return ;
				}

				vecRet = cmdHelper.GetCmdRet();
				for (int i = 0;i < vecRet.size(); ++i)
				{
					cout<<vecRet[i]<<endl;
				}

			}
			else
				cout<<"��֧��"<<endl;

			//����ʾһ�²�ѯ���
			Sleep(2000);
			strCmdStart = "sc query " + strName;
			if(cmdHelper.ExcutuCmdRet(strCmdStart))
			{
				vecRet = cmdHelper.GetCmdRet();
				for (int i = 0;i < vecRet.size(); ++i)
				{
					cout<<vecRet[i]<<endl;
				}
				return ;
			}
		}
		
	}

}
