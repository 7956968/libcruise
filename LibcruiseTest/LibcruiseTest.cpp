// LibcruiseTest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include "../libcruise/cruise.h"

#include <Windows.h>

int  OnPresetChanged(const char* channel,unsigned long cruise,unsigned long preset,unsigned long seconds);
void OnLog(string Log);
void OnGotoPreset(string channel, unsigned long presetIdx);


/*
51://启动巡航记忆
52://关闭巡航记忆
53://增加预置位
54://开始巡航
55://停止巡航
*/

/*
测试过程:

删掉配置文件NVMS_Cruise.xml,执行两次程序

程序执行第一次:
ch1运行但此时还没有ch1,会打印错误
...10秒后...
增加ch1
增加ch2
ch2开始运行
ch1开始运行
...10秒后...
增加ch3
ch2停止运行
ch1停止运行
ch3开始运行
...10秒后...
清空ch1
退出

检查配置文件会留下两个通道的配置,其中ch3是运行状态 ch1不运行

程序执行第二次:
ch3会立即被运行
ch1开始运行
...10秒后...
ch1被修改,你会看到ch1重新运行
增加ch2
ch2运行
...10秒后...
ch3被修改,你会看到ch3重新运行
ch2停止运行
ch1停止运行
ch3被再次调用,你会看到ch3重新运行
...10秒后...
清空ch1
退出
*/
int _tmain(int argc, _TCHAR* argv[])
{
    Cruise_PresetRegister(OnPresetChanged, OnLog, OnGotoPreset);
    Cruise_Start();
    
    Cruise_Control("ch1", 54, 1, 0, 0, 0);

    Sleep(10000);

    //channel2增加巡航1,预置位1停留2秒,预置位2停留3秒
    Cruise_Control("ch1", 51, 1, 0, 0, 0);
    Cruise_Control("ch1", 53, 1, 1, 2, 10);
    Cruise_Control("ch1", 53, 1, 2, 3, 10);
    Cruise_Control("ch1", 52, 1, 0, 0, 0);

    //channel2增加巡航2,预置位1停留5秒,预置位2停留3秒
    Cruise_Control("ch2", 51, 2, 0, 0, 0);
    Cruise_Control("ch2", 53, 2, 1, 5, 10);
    Cruise_Control("ch2", 53, 2, 2, 3, 10);
    Cruise_Control("ch2", 52, 2, 0, 0, 0);

    Cruise_Control("ch2", 54, 2, 0, 0, 0);

    Cruise_Control("ch1", 54, 1, 0, 0, 0);

    Sleep(10000);

    //channel2增加巡航3,预置位1停留5秒,预置位2停留3秒
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
