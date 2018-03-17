//
// Created by chenkuan on 2018/2/28.
//

#ifndef RUNTIME_VERIFICATION_SYSTEM_EVENTVERIFYRESULT_H
#define RUNTIME_VERIFICATION_SYSTEM_EVENTVERIFYRESULT_H

#include <State.h>

/**
 * 事件验证结果的几种状态
 * pass, 指该事件明确可以通过，将造成状态机的转移变化
 * undetermined, 指该事件可通过可不通过，不对状态机作转移变化
 * refuse, 指该事件不可通过，不对状态机作转移变化
 */
enum EventVerifyResultEnum {
    pass,
    undetermined,
    refuse
};

/**
 * 事件转移结果，不仅包含验证结果，同时要包含下一个状态
 */
struct EventVerifyResult {
    EventVerifyResult() {
        resultEnum = refuse;
        nextState = nullptr;
    }
    EventVerifyResultEnum resultEnum = refuse;
    const State *nextState = nullptr;
};


#endif //RUNTIME_VERIFICATION_SYSTEM_EVENTVERIFYRESULT_H
