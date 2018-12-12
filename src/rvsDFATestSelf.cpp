#include <iostream>
#include <cstring>
#include <csignal>
#include <sstream>
#include <Model.h>

using std::cout;
using std::cin;
using std::cerr;
using std::endl;

int main() {

    cout << "begin" << endl;
    // 建立模型
    Model* module = new Model();

    module->addVarDecl("int_function", "y");
    module->addVarDecl("time", "t");

    module->addStartState(1, {"y != 0"});
    module->addState(2, {"y >= 1", "y <= 16"});
    module->addEndState(3, {});

    cout << "begin2" << endl;

    module->addTran("RUN", 1, 2);
    module->addTran("RUN", 2, 2);
    module->addTran("STOP", 2, 3);

    module->addSpec("y(1, t - 1) != y(2, t)");
    module->addSpec("y(2, t - 1) != y(2, t)");
    if (!module->initModel()) {
        cerr << "模型初始化失败！" << endl;
        return -1;
    }

    // 模型建立完成后，开始添加事件
    module->verifyEvent(nullptr);
    module->verifyEvent(nullptr);
    module->verifyEvent(nullptr);
    module->verifyEvent(nullptr);
    module->verifyEvent(nullptr);
    module->verifyEvent(nullptr);
    module->verifyEvent(nullptr);
    module->verifyEvent(nullptr);
    module->verifyEvent(nullptr);
    module->verifyEvent(nullptr);

    // 手动输入事件
//    while (true) {
//        cout << "what?" << endl;
//        cout << "中文呢" << endl;
//        printf("请输入事件名称：（输入event_end结束）\n");
//        string eventName;
//        cin >> eventName;
//        if (eventName == "event_end") break;
//        cout << "请逐行输入变量名和变量值，以空格分隔，输入var_end结束事件内的变量输入：" << endl;
//        string varName, varValue;
//        map<string, string> vars;
//        cin >> varName;
//        while (varName != "var_end") {
//            cin >> varValue;
//            vars[varName] = varValue;
//            cin >> varName;
//        }
//    }

    std::cout << "#################" << std::endl;
    Event *event1 = Event::initFromXML("<xml type=\"event\" name=\"RUN\" num=\"4\" attr=\"******\"><y>10</y><t>10</t></xml>");
    module->verifyEvent(event1);

    std::cout << "#################" << std::endl;
    Event *event2 = Event::initFromXML("<xml type=\"event\" name=\"RUN\" num=\"4\" attr=\"******\"><y>11</y><t>11</t></xml>");
    module->verifyEvent(event2);

    std::cout << "#################" << std::endl;
    Event *event3 = Event::initFromXML("<xml type=\"event\" name=\"RUN\" num=\"4\" attr=\"******\"><y>11</y><t>12</t></xml>");
    module->verifyEvent(event3);
    return 0;
}