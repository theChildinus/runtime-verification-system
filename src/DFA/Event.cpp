//
// Created by chenkuan on 2018/2/28.
//

#include <Event.h>
#include <Logger.h>
#include <tinyxml2.h>
using namespace tinyxml2;

set<string> Event::importantEventNameList;

Event* Event::initFromXML(const char *eventXML) {
    Logger *logger = Logger::getLogger();

    XMLDocument xmlDocument;
    XMLError xmlError = xmlDocument.Parse(eventXML);
    if (xmlError != XML_SUCCESS) {
        logger->error("event \"%s\"  不符合XML规范！", eventXML);
        return nullptr;
    }

    XMLElement *eventRoot = xmlDocument.FirstChildElement();
    auto eventName = eventRoot->Attribute("name");
    if (eventName == nullptr) {
        logger->error("event \"%s\"缺少事件名称！", eventXML);
        return nullptr;
    }

    // 构造事件
    auto event = new Event();
    event->eventName = eventName;

    // 判断事件重要性
    event->eventImportant = judgeEventImportant(event);

    // 解析事件中变量
    for (auto varLabel = eventRoot->FirstChildElement();
            varLabel != nullptr;
            varLabel = varLabel->NextSiblingElement()) {
        event->varValueMap[varLabel->Value()] = varLabel->GetText();
    }

    return event;
}

void Event::addImportantEventName(const char *eventName){
    Event::importantEventNameList.insert(eventName);
}

string Event::getEventName() const {
    return eventName;
}

const map<string, string> & Event::getVarValueMap() const {
    return varValueMap;
}

bool Event::isImportant() const {
    return eventImportant;
}

bool Event::judgeEventImportant(Event *event) {
    return importantEventNameList.find(event->eventName) != importantEventNameList.end();
}