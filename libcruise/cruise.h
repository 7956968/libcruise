#ifndef  CRUISE__H
#define  CRUISE__H

#include <string>
using namespace std;

//////////////////////////////////////////////////////////////////////////
//�ص�����
//////////////////////////////////////////////////////////////////////////

/*!
*  @brief
*	Ԥ��λ�ص�����
* @param channel
*	�����ͨ��
* @param preset
*	Ԥ��λ
* @param seconds
*	��Ԥ��λ�ϵ�ͣ��ʱ��
* @return
*	����0
*/
typedef int(*CB_Preset)(const char* channel,unsigned long cruise,unsigned long preset,unsigned long seconds);
typedef void(*CB_Log)(string Log);
typedef void(*CB_GotoPreset)(string channel, unsigned long presetIdx);
//////////////////////////////////////////////////////////////////////////
//�ӿ�
//////////////////////////////////////////////////////////////////////////
/**********************************************************************
* �������ƣ� Cruise_Start
* ���������� ��ʼ��Ѳ��SDK
* ���������
* ���������
* �� �� ֵ��
* ����˵����
* �޸�����        �汾��     �޸���	      �޸�����
***********************************************************************/
int Cruise_Start();

/**********************************************************************
* �������ƣ� Cruise_Stop
* ���������� ����Ѳ��SDK
* ���������
* ���������
* �� �� ֵ��
* ����˵����
* �޸�����        �汾��     �޸���	      �޸�����
***********************************************************************/
int Cruise_Stop();

/**********************************************************************
* �������ƣ� Cruise_PresetRegister
* ���������� ע��Ѳ��Ԥ��λ�ص�����
* ���������
* ���������
* �� �� ֵ��
* ����˵����
* �޸�����        �汾��     �޸���	      �޸�����
***********************************************************************/
int Cruise_PresetRegister(CB_Preset OnPresetChange, CB_Log OnLog, CB_GotoPreset GotoPreset);

/**********************************************************************
* �������ƣ� Cruise_Control
* ���������� Ѳ������
* ���������
* ���������
* �� �� ֵ��
* ����˵����
* �޸�����        �汾��     �޸���	      �޸�����
***********************************************************************/
int Cruise_Control(const char* channel,unsigned long cmd,long param1,long param2,long param3,long param4);

/**********************************************************************
* �������ƣ� Cruise_ResetChannel
* ���������� ���ͨ������Ѳ��
* ���������
* ���������
* �� �� ֵ��
* ����˵����
* �޸�����        �汾��     �޸���	      �޸�����
***********************************************************************/
int Cruise_ResetChannel(const char* channel);

#endif