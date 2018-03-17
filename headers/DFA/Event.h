//
// Created by chenkuan on 2018/2/28.
//

#ifndef RUNTIME_VERIFICATION_SYSTEM_EVENT_H
#define RUNTIME_VERIFICATION_SYSTEM_EVENT_H

#include <string>
#include <map>
#include <set>

using std::string;
using std::map;
using std::set;

class Event {
  public:
    ~Event();

    /**
     * 读取XML事件内容构造事件
     * @param eventXML XML描述的事件
     * @return 事件指针，无法构造返回nullptr
     */
    static Event *initFromXML(const char *eventXML);
    /**
     * 静态指定关键事件名单
     */
    static void addImportantEventName(const char *eventName);

    string getEventName() const ;
    const map<string, string> &getVarValueMap() const;
    bool isImportant() const;

  private:
    Event() = default;
    /**
     * 事件名称
     */
    string eventName;
    /**
     * 变量名:变量值 映射表
     */
    map<string, string> varValueMap;
    /**
     * 是否为关键事件
     */
    bool eventImportant = false;

    /**
     * 关键事件名单
     */
    static set<string> importantEventNameList;
    /**
     * 判断给定事件是否为关键事件
     * @param event
     * @return
     */
    static bool judgeEventImportant(Event *event);

};

#endif //RUNTIME_VERIFICATION_SYSTEM_EVENT_H
