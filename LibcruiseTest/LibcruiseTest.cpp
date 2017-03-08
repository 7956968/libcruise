// LibcruiseTest.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

#include "../libcruise/cruise.h"

#include <Windows.h>

int  OnPresetChanged(const char* channel,unsigned long cruise,unsigned long preset,unsigned long seconds);
void OnLog(string Log);
void OnGotoPreset(string channel, unsigned long presetIdx);


/*
51://����Ѳ������
52://�ر�Ѳ������
53://����Ԥ��λ
54://��ʼѲ��
55://ֹͣѲ��
*/

/*
���Թ���:

ɾ�������ļ�NVMS_Cruise.xml,ִ�����γ���

����ִ�е�һ��:
ch1���е���ʱ��û��ch1,���ӡ����
...10���...
����ch1
����ch2
ch2��ʼ����
ch1��ʼ����
...10���...
����ch3
ch2ֹͣ����
ch1ֹͣ����
ch3��ʼ����
...10���...
���ch1
�˳�

��������ļ�����������ͨ��������,����ch3������״̬ ch1������

����ִ�еڶ���:
ch3������������
ch1��ʼ����
...10���...
ch1���޸�,��ῴ��ch1��������
����ch2
ch2����
...10���...
ch3���޸�,��ῴ��ch3��������
ch2ֹͣ����
ch1ֹͣ����
ch3���ٴε���,��ῴ��ch3��������
...10���...
���ch1
�˳�
*/
int _tmain(int argc, _TCHAR* argv[])
{
    Cruise_PresetRegister(OnPresetChanged, OnLog, OnGotoPreset);
    Cruise_Start();
    
    Cruise_Control("ch1", 54, 1, 0, 0, 0);

    Sleep(10000);

    //channel2����Ѳ��1,Ԥ��λ1ͣ��2��,Ԥ��λ2ͣ��3��
    Cruise_Control("ch1", 51, 1, 0, 0, 0);
    Cruise_Control("ch1", 53, 1, 1, 2, 10);
    Cruise_Control("ch1", 53, 1, 2, 3, 10);
    Cruise_Control("ch1", 52, 1, 0, 0, 0);

    //channel2����Ѳ��2,Ԥ��λ1ͣ��5��,Ԥ��λ2ͣ��3��
    Cruise_Control("ch2", 51, 2, 0, 0, 0);
    Cruise_Control("ch2", 53, 2, 1, 5, 10);
    Cruise_Control("ch2", 53, 2, 2, 3, 10);
    Cruise_Control("ch2", 52, 2, 0, 0, 0);

    Cruise_Control("ch2", 54, 2, 0, 0, 0);

    Cruise_Control("ch1", 54, 1, 0, 0, 0);

    Sleep(10000);

    //channel2����Ѳ��3,Ԥ��λ1ͣ��5��,Ԥ��λ2ͣ��3��
    Cruise_Control("ch3", 51, 2, 0, 0, 0);
    Cruise_Control("ch3", 53, 2, 1, 5, 10);
    Cruise_Control("ch3", 53, 2, 2, 3, 10);
    Cruise_Control("ch3", 52, 2, 0, 0, 0);

    Cruise_Control("ch2", 55, 2, 0, 0, 0);

    Cruise_Control("ch1", 55, 2, 0, 0, 0);

    Cruise_Control("ch3", 54, 2, 0, 0, 0);

    Sleep(10000);

    Cruise_ResetChannel("ch1");

    Cruise_Stop();
	return 0;
}

int OnPresetChanged(const char* channel,unsigned long cruise,unsigned long preset,unsigned long seconds)
{
    printf("%s:%s,%d,%d,%d\n",__FUNCTION__, channel,cruise, preset, seconds);
    return 0;
}

void OnLog(string Log)
{
    printf("%s\n", Log.c_str());
}

void OnGotoPreset(string channel, unsigned long presetIdx)
{
    //printf("%s:%s,%d\n",__FUNCTION__, channel.c_str(), presetIdx);
}
