#include "cruise.h"

#include <vector>
#include <list>
#include <map>
#include <pthread.h>
#include <pugixml.hpp>
#include <strstream>

#include "gettimeofday.h"

#include <WinSock.h>

using namespace std;
using namespace pugi;

#define CruiseSavePath  ("NVMS_Cruise.xml")
#define INVALID_INDEX   (-1)

CB_Preset gfCBPreset;
CB_Log gfOnLog;
CB_GotoPreset gfGotoPreset;

struct PointOfCruise
{
    PointOfCruise(int No, int Time, int Speed):PresetNo(No),StayTime(Time),MoveSpeed(Speed){}
    int PresetNo;
    int StayTime;       //秒为单位
    int MoveSpeed;
};

#define THREAD_STATE_RUN	(0)
#define THREAD_STATE_STOP	(1)

void* MyThreadFun(void* pUsr);

class CuiseThread
{
    friend void* MyThreadFun(void* pUsr);
public:
    CuiseThread(string Channel,int Index, vector<PointOfCruise> VecPoint):m_VecPoint(VecPoint),m_Channel(Channel),m_Index(Index)
    {
        pthread_mutex_init(&m_Mtx, NULL);
        pthread_cond_init(&m_Con, NULL);
        UsrWishState = THREAD_STATE_STOP;
        m_ThreadState = THREAD_STATE_STOP;
    }

    //析构将被释放,这里会产生隐晦的错误,因此不要在一个临时类上启动巡航
    virtual ~CuiseThread()
    {
        Stop();
        pthread_mutex_destroy(&m_Mtx);
        pthread_cond_destroy(&m_Con);
    }
    virtual void Start()
    {
        if (m_ThreadState == THREAD_STATE_STOP){
            pthread_create(&pid, NULL, MyThreadFun, this);
            UsrWishState = m_ThreadState = THREAD_STATE_RUN;
        }
    }

    int ShouldExit()
    {
        return UsrWishState == THREAD_STATE_STOP;
    }
    int GetState() { return m_ThreadState; }

    virtual int Run()
    {
        string sLog = m_Channel;
        sLog += ":巡航开始";
        gfOnLog(sLog);

        while (!ShouldExit())
        {
            for (size_t i = 0; i < m_VecPoint.size(); i++){
                if (ShouldExit()){
                    break;
                }

                gfGotoPreset(m_Channel, m_VecPoint[i].PresetNo);
                int StayTime = m_VecPoint[i].StayTime;
                gfCBPreset(m_Channel.c_str(), m_Index, m_VecPoint[i].PresetNo, StayTime);

                struct timeval now;
                struct timespec OutTime;
                gettimeofday(&now);
                OutTime.tv_sec  = now.tv_sec + StayTime;
                OutTime.tv_nsec = now.tv_usec;

                pthread_mutex_lock(&m_Mtx);
                pthread_cond_timedwait(&m_Con, &m_Mtx, &OutTime);
                pthread_mutex_unlock(&m_Mtx);
            }
        }

        sLog = m_Channel;
        sLog += ":巡航停止";
        gfOnLog(sLog);

        return 0;
    }
    virtual void Stop()
    {
        if (m_ThreadState == THREAD_STATE_STOP) {
            return;
        }

        pthread_cond_broadcast(&m_Con);

        UsrWishState = THREAD_STATE_STOP;
        pthread_join(pid, NULL);
        m_ThreadState = THREAD_STATE_STOP;
    }
private:
    const CuiseThread& operator=(const CuiseThread& rhis);
    vector<PointOfCruise> m_VecPoint;
    string m_Channel;
    pthread_mutex_t m_Mtx;
    pthread_cond_t m_Con;
    int m_ThreadState;
    pthread_t pid;
    int m_Index;
    int UsrWishState;
};

void* MyThreadFun(void* pUsr)
{
    CuiseThread* pMyThread = (CuiseThread*)pUsr;

    pMyThread->Run();

    pMyThread->m_ThreadState = THREAD_STATE_STOP;

    return NULL;
}

struct  CruisePath
{
    int Index;
    vector<PointOfCruise> VecPoint;

    int AddPoint(PointOfCruise& P)
    {
        VecPoint.push_back(P);
    }
};


struct CurisePerChannel
{
    CurisePerChannel(string Channel):m_Channel(Channel)
    {
        m_pNowGoThread = NULL;
        m_NowGoIndex = INVALID_INDEX;
    }
    CurisePerChannel()
    {
        m_pNowGoThread = NULL;
        m_NowGoIndex = INVALID_INDEX;
    }
    ~CurisePerChannel()
    {
        CuriseStop();
    }
    
    void AddCurise(CruisePath P)
    {
        list<CruisePath>::iterator it;
        if (FindCurise(P.Index, it) == 0){
            m_ListCruise.erase(it);
        }
        m_ListCruise.push_back(P);

        //如果该巡航号正在被运行,重新开始???
        if (m_NowGoIndex == P.Index){
            CuriseGo(m_NowGoIndex);
        }
    }
    
    //只要执行这个函数,当前巡航就会停止,即使是一样的巡航也会重新开始
    void CuriseGo(int Index)
    {
        list<CruisePath>::iterator it;
        if (FindCurise(Index, it) == 0){
            if (m_pNowGoThread != NULL){
                delete m_pNowGoThread;
            }

            m_pNowGoThread = new CuiseThread(m_Channel, it->Index, it->VecPoint);
            
            m_pNowGoThread->Start();

            m_NowGoIndex = Index;
        }
        else{
            gfOnLog("巡航索引找不到");
        }
    }

    void CuriseStop(int Index = 0)
    {
        if (m_pNowGoThread != NULL){//不用找了,反正一个通道只会有一个巡航,直接停止
            delete m_pNowGoThread;
            m_pNowGoThread = NULL;
            m_NowGoIndex = INVALID_INDEX;
        }
    }

    int FindCurise(int Index, list<CruisePath>::iterator& it)
    {
        for(it=m_ListCruise.begin(); it != m_ListCruise.end(); it++){
            if (it->Index == Index){
                return 0;
            }
        }
        return -1;
    }
    list<CruisePath> m_ListCruise;
    string m_Channel;
    CuiseThread* m_pNowGoThread;
    int m_NowGoIndex;
};

struct  CuriseMgr
{
    typedef map<string, CurisePerChannel>  SelfType;
    typedef pair<string, CurisePerChannel> PairType;
    
    void Load()
    {
        try
        {
            xml_document doc;
            xml_parse_result result = doc.load_file( CruiseSavePath );
            if ( result.status != status_ok )
                return;
            
            pugi::xml_node RootNode = doc.child("NVMSCruise");

            for (pugi::xml_node ChannelNode = RootNode.child("Channel"); ChannelNode; 
                ChannelNode = ChannelNode.next_sibling("Channel")){
                    
                    xml_attribute NameAttr   = ChannelNode.attribute("Name");
                    if (NameAttr.empty()){throw 0;}
                    
                    string ChannelName = NameAttr.value();

                    xml_attribute NowGoAttr = ChannelNode.attribute("NowGo");
                    int NowGoIndex = INVALID_INDEX;

                    if (!NowGoAttr.empty()){
                        NowGoIndex = NowGoAttr.as_int(INVALID_INDEX);
                    }

                    CurisePerChannel CuriseOnChannel(ChannelName);

                    for (pugi::xml_node CruiseNode = ChannelNode.child("Cruise"); CruiseNode; 
                        CruiseNode = CruiseNode.next_sibling("Cruise")){

                            xml_attribute IndexAttr = CruiseNode.attribute("Index");
                            
                            if (IndexAttr.empty()){throw 0;}

                            CruisePath CurisePath;
                            CurisePath.Index = IndexAttr.as_int(INVALID_INDEX);

                            for (pugi::xml_node PointNode = CruiseNode.child("Point"); PointNode; 
                                PointNode = PointNode.next_sibling("Point")){
                                    xml_attribute PresetNoAttr  = PointNode.attribute("PresetNo");
                                    xml_attribute StayTimeAttr  = PointNode.attribute("StayTime");
                                    xml_attribute MoveSpeedAttr = PointNode.attribute("MoveSpeed");

                                    if (PresetNoAttr.empty() || StayTimeAttr.empty() || MoveSpeedAttr.empty()){throw 0;}

                                    PointOfCruise TmpPoint(PresetNoAttr.as_int(INVALID_INDEX), 
                                                           StayTimeAttr.as_int(0),
                                                           MoveSpeedAttr.as_int(0));

                                    CurisePath.VecPoint.push_back(TmpPoint);
                            }

                            CuriseOnChannel.m_ListCruise.push_back(CurisePath);

                    }//cruise
                    m_CuriseOnEveryChannel.insert(PairType(ChannelName, CuriseOnChannel));
                    if (NowGoIndex != INVALID_INDEX){
                        CuriseGo(ChannelName, NowGoIndex);
                    }
            }
        }
        catch (...)
        {
            m_CuriseOnEveryChannel.clear();
            //format error
            gfOnLog("加载配置失败!");
            return;
        }
    }
    
    void Save()
    {
        xml_document XmlDoc;
        pugi::xml_node RootNode = XmlDoc.append_child("NVMSCruise");
        
        SelfType::iterator it;
        for(it= m_CuriseOnEveryChannel.begin(); it!= m_CuriseOnEveryChannel.end(); it++){
            CurisePerChannel& CuriseOnChannel = it->second;
            list<CruisePath>& ListCruise = CuriseOnChannel.m_ListCruise;

            if (ListCruise.size() <= 0){
                continue;
            }
            
            pugi::xml_node ChannelNode  = RootNode.append_child("Channel");
            ChannelNode.append_attribute("Name") = it->first.c_str();
            if (CuriseOnChannel.m_NowGoIndex != INVALID_INDEX){
                ChannelNode.append_attribute("NowGo") = CuriseOnChannel.m_NowGoIndex;
            }

            list<CruisePath>::iterator PathIt;
            for (PathIt = ListCruise.begin(); PathIt != ListCruise.end(); PathIt++){
                CruisePath& Path = *PathIt;
                vector<PointOfCruise>& VecPoint = Path.VecPoint;

                if (VecPoint.size() <= 0){
                    continue;
                }
                pugi::xml_node CruiseNode = ChannelNode.append_child("Cruise");
                CruiseNode.append_attribute("Index") = Path.Index;

                for (size_t i = 0; i < VecPoint.size(); i++){
                    pugi::xml_node PointNode = CruiseNode.append_child("Point");
                    PointNode.append_attribute("PresetNo")  = VecPoint[i].PresetNo;
                    PointNode.append_attribute("StayTime")  = VecPoint[i].StayTime;
                    PointNode.append_attribute("MoveSpeed") = VecPoint[i].MoveSpeed;
                }
            }
        }

        XmlDoc.save_file(CruiseSavePath);
    }

    void CuriseAdd(string Channel, CruisePath Path)
    {
        SelfType::iterator it;
        if (FindCuriseOnChannel(Channel, it) == 0){
            it->second.AddCurise(Path);
        }
        else{
            CurisePerChannel NewCuriseOnChannel;
            NewCuriseOnChannel.AddCurise(Path);
            NewCuriseOnChannel.m_Channel = Channel;
            m_CuriseOnEveryChannel.insert(PairType(Channel, NewCuriseOnChannel));
        }
    }

    void CuriseGo(string Channel, int Index)
    {
        SelfType::iterator it;
        if (FindCuriseOnChannel(Channel, it) == 0){
            it->second.CuriseGo(Index);
        }
        else{
            strstream sLog;
            sLog << "无效的巡航:";
            sLog << Channel;
            sLog << ":";
            sLog << Index;
            string tt ;
            sLog >> tt;
            gfOnLog(tt);
        }
    }

    void CuriseStop(string Channel, int Index)
    {
        SelfType::iterator it;
        if (FindCuriseOnChannel(Channel, it) == 0){
            it->second.CuriseStop(Index);
        }
    }

    int ResetChannel(string Channel)
    {
        SelfType::iterator it= m_CuriseOnEveryChannel.find(Channel);
        if(it == m_CuriseOnEveryChannel.end()){
            m_CuriseOnEveryChannel.erase(it);
        }
        return 0;
    }
private:
    int FindCuriseOnChannel(string Channel, SelfType::iterator& it)
    {
        it= m_CuriseOnEveryChannel.find(Channel);
        if(it != m_CuriseOnEveryChannel.end()){
            return 0;
        }
        return -1;
    }

    SelfType m_CuriseOnEveryChannel;
};


CuriseMgr gCuriseMgr;

int Cruise_Start()
{
    gCuriseMgr.Load();
    return 0;
}

//这个函数中使用了static临时变量,并非线程安全类型,如果要在多线程环境下锁的粒度要根据实际情况加上
int Cruise_Control(const char* channel,  
                   unsigned long cmd,
                   long param1,long param2,long param3,long param4)
{
    static CruisePath TmpPath;
    static string TmpChannelName;

    switch (cmd)
    {
    case 51://启动巡航记忆
        TmpPath.VecPoint.clear();
        TmpPath.Index = param1;
        TmpChannelName = channel;
        break;
    case 52://关闭巡航记忆
        if (TmpPath.VecPoint.size() <= 1){
            gfOnLog("点数太少");
            return 0;
        }
        if (TmpChannelName.empty()){
            gfOnLog("通道名为空");
            return 0;
        }
        
        gCuriseMgr.CuriseAdd(TmpChannelName, TmpPath);
        gCuriseMgr.Save();

        TmpPath.VecPoint.clear();
        TmpPath.Index = INVALID_INDEX;
        TmpChannelName.clear();
        break;
    case 53://增加预置位
        if (channel == TmpChannelName && (param1 == TmpPath.Index)){
            TmpPath.VecPoint.push_back(PointOfCruise(param2, param3, param4));
        }
        else{
            gfOnLog("启动巡航记忆后增加的预置位不属于上下文!");
        }
        break;
    case 54://开始巡航
        gCuriseMgr.CuriseGo(channel, param1);
        gCuriseMgr.Save();
        break;
    case 55://停止巡航
        gCuriseMgr.CuriseStop(channel, param1);
        gCuriseMgr.Save();
        break;
    }
    return 0;
}

int Cruise_Stop()
{
    return 0;
}

int Cruise_ResetChannel(const char* channel)
{
    gCuriseMgr.ResetChannel(channel);
    gCuriseMgr.Save();
    return 0;
}

int Cruise_PresetRegister(CB_Preset OnPresetChange, CB_Log OnLog, CB_GotoPreset GotoPreset)
{
    gfGotoPreset = GotoPreset;
    gfOnLog = OnLog;
    gfCBPreset = OnPresetChange;
    return 0;
}
