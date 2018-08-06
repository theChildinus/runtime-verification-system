//
// Created by yingzi on 2017/11/30.
//

#include <stack>
#include <algorithm>
#include <Event.h>
#include <Model.h>
using std::stack;

Model::Model() : slv(ctx), slvNegative(ctx),
                 I(ctx.int_sort()), D(ctx.real_sort()), B(ctx.bool_sort()),
                 oldEventExpr(1, ctx.bool_val(true)) {
}

Model::~Model() {
    for (auto &kv : states) {
        delete (kv.second);
    }
    for (auto &tran : trans) {
        delete (tran);
    }
}

void Model::addVarDecl(const string &varType, const string &varName) {
    this->varsDecl[varName] = varType;
}

void Model::addStartState(int stateNum, const vector<string> &stateExprStrList) {
    this->addState(stateNum, stateExprStrList);
    this->setStartState(stateNum);
}

void Model::addEndState(int stateNum, const vector<string> &stateExprStrList) {
    this->addState(stateNum, stateExprStrList);
    this->setEndState(stateNum);
}

void Model::addState(int stateNum, const vector<string> &stateExprStrList) {
    State *oldState = this->states[stateNum];
    if (oldState == nullptr) {
        oldState = new State();
    } else if (!oldState->isEmpty()) {
        logger->warning("已经添加过编号为%d的节点，将覆盖旧节点", stateNum);
        oldState->clear();
    }

    // 新建状态机节点，将编号和解析成Z3的表达式添加进节点
    State *newState = oldState;
    newState->setStateNum(stateNum);
    for (auto &stateExprStr : stateExprStrList) {
        const Z3Expr z3Expr = this->extractZ3Expr(stateExprStr, std::to_string(stateNum));
        logger->debug("节点表达式为:%s", z3Expr.to_string().c_str());
        newState->addZ3Expr(z3Expr);
    }

    // 记录或更新此状态到模型中
    this->states[stateNum] = newState;
}

void Model::setStartState(int stateNum) {
    State *state = this->states[stateNum];
    if (state == nullptr) {
        logger->error("未找到节点%d", stateNum);
        return;
    }
    if (hasStartState && state != startState) {
        logger->warning("已经指定过起始节点%d，将更新起始节点%d", startState->getStateNum(), state->getStateNum());
        startState->setStartFlag(false);
    } else {
        hasStartState = true;
    }
    startState = state;
    startState->setStartFlag(true);
    logger->info("节点%d成为了起始节点", startState->getStateNum());

}

void Model::setEndState(int stateNum) {
    State *state = this->states[stateNum];
    if (state == nullptr) {
        logger->error("未找到节点%d", stateNum);
        return;
    }
    if (hasEndState && state != endState) {
        logger->warning("已经指定过终止节点%d，将更新终止节点%d", endState->getStateNum(), state->getStateNum());
        endState->setEndFlag(false);
    } else {
        hasEndState = true;
    }
    endState = state;
    endState->setEndFlag(true);
    logger->info("节点%d成为了终止节点", endState->getStateNum());
}

void Model::addTran(const string &tranName, int sourceStateNum, int destStateNum) {
    // 如果源节点和目标节点尚不存在，则先创建相应状态节点
    State *sourceState = this->states[sourceStateNum];
    if (sourceState == nullptr) {
        sourceState = new State();
    }

    State *destState = this->states[destStateNum];
    if (destState == nullptr) {
        destState = new State();
    }

    // 创建Tran类，添加相应属性
    auto tran = new Tran();
    tran->setTranName(tranName);
    tran->setSourceState(sourceState);
    tran->setDestState(destState);

    // 将Tran类对象添加到源State和模型中
    sourceState->addTran(tran);
    this->trans.push_back(tran);
}

void Model::addSpec(const string &specStr) {
    // 生成Z3表达式添加进模型
    //const Z3Expr z3Expr = this->extractZ3Expr(specStr, "");
    const Z3Expr z3Expr = this->extractZ3ExprForSpec(specStr);
    specZ3ExprVector.push_back(z3Expr);
}

EventVerifyResult Model::verifyEvent(const Event *event) {
    if (event == nullptr) return EventVerifyResult();
    logger->debug("当前节点为节点%d，开始尝试转移", currentState->getStateNum());
    // 如果当前是起始节点，则需要添加起始节点的表达式和SPEC表达式
    if (currentState == startState) {
        for (auto &z3Expr : startState->getZ3ExprList()) {
            this->slv.add(z3Expr);
            logger->debug("添加起始节点的表达式%s", z3Expr.to_string().c_str());
        }
        for (auto &spec : specZ3ExprVector) {
            this->slv.add(spec);
            logger->debug("添加轨迹验证表达式%s", spec.to_string().c_str());
        }
        // 始终保留起始节点的表达式和SPEC表达式，后续添加的表达式在到达终止节点继续转移时弹出
        this->slv.push();

        // 初始化状态轨迹
        this->stateTrace.clear();
        this->stateTrace.push_back(startState);
    }

    // 如果当前是终止节点，则调整至起始节点开始转移
    if (currentState == endState) {
        currentState = startState;
        this->slv.pop();
        this->slv.push();

        // 重新初始化状态轨迹
        this->stateTrace.clear();
        this->stateTrace.push_back(startState);
        logger->info("从终止节点%d调整至起始节点%d开始转移", endState->getStateNum(),  startState->getStateNum());
    }

    // 初始化当前尝试失败的节点集合
    currentFailedStates.clear();

    // 假设不能转移
    EventVerifyResult result;
    const State *nextState = nullptr;

    // 先查看当前状态能否转移到相邻状态
    const vector<const Tran *> &outTranList = currentState->getTranList();
    for (const Tran *tran : outTranList) {
        nextState = tran->getDestState();
        if (tran->getTranName() == event->getEventName()) {
            // 取最小值表示取更好的结果
            EventVerifyResultEnum nextStateResult =  this->verify(nextState, event->getVarValueMap());
            if (nextStateResult < result.resultEnum) {
                result.resultEnum = nextStateResult;
                result.nextState = nextState;
            }
            if (nextStateResult == EventVerifyResultEnum::pass) {
                break;
            }
        }
    }

    // 如果无法转移到相邻状态或者转移到相邻状态验证失败则在全局中搜索转移
    if (result.resultEnum != EventVerifyResultEnum::pass) {
        for (auto &tran : trans) {
            nextState = tran->getDestState();
            if (tran->getSourceState() != currentState
                && tran->getTranName() == event->getEventName()
                && currentFailedStates.find(nextState) == currentFailedStates.end()) {
                // 取最小值表示取更好的结果
                EventVerifyResultEnum nextStateResult =  this->verify(nextState, event->getVarValueMap());
                if (nextStateResult < result.resultEnum) {
                    result.resultEnum = nextStateResult;
                    result.nextState = nextState;
                }
                if (nextStateResult == EventVerifyResultEnum::pass) {
                    break;
                }
            }
        }
    }
    return result;
}

void Model::transferEvent(const Event *event, const State *nextState) {
    if (nextState == nullptr) return;

    // 先把下一状态中的全部Z3表达式求与
    const vector<Z3Expr> &nextStateZ3Together = nextState->getZ3Together();
    if (!nextStateZ3Together.empty()) {
        slv.add(nextStateZ3Together[0]);
        slvNegative.add(!nextStateZ3Together[0]);
    }

    logger->info("事件\"%s\"导致节点%d转移到节点%d", event->getEventName().c_str(), currentState->getStateNum(), nextState->getStateNum());
    currentState = const_cast<State *>(nextState);
    this->stateTrace.push_back(currentState);
}

vector<EventVerifyResult> Model::verifyEventList(const vector<const Event *> &eventList) {
    if (eventList.empty()) return {};

    vector<EventVerifyResult> resultList;

    // 先尝试让最新的事件转移
    EventVerifyResult latestEventResult = verifyEvent(eventList.back());
    // 如果最新事件转移结果确定则依次转移前面事件
    if (latestEventResult.resultEnum != undetermined) {
        bool determined = true;
        for (int i = 0; i < eventList.size() - 1; ++i) {
            EventVerifyResult result = verifyEvent(eventList[i]);
            if (result.resultEnum == undetermined) {
                determined = false;
            }
        }
        resultList.push_back(latestEventResult);
        if (determined) {
            return resultList;
        }
    }

    // 如果前述方法不能得到全部确定结果，则只能顺次转移返回
    for (int i = 0; i < eventList.size(); ++i) {
        resultList[i] = verifyEvent(eventList[i]);
    }
    return resultList;
};

bool Model::initModel() {
    int num = 0;
    for (auto &kv : states) {
        if (kv.second->isEmpty()) {
            ++num;
        }
    }
    if (num > 0) {
        logger->error("模型中存在%d个虚拟的空节点", num);
        return false;
    }

    // 判断模型是否有起始和终止节点
    if (!hasStartState) {
        logger->error("模型未指定起始节点");
        return false;
    }
    if (!hasEndState) {
        logger->error("模型未指定终止节点");
        return false;
    }

    currentState = startState;
    return true;
}

const Z3Expr Model::extractZ3ExprForSpec(const string &exprStr) {
    stack<string> operatorStack;
    stack<Z3Expr> exprStack;

    operatorStack.push("$");
    bool negative = false;
    size_t i = 0;
    while(i != exprStr.size()) {
        size_t posRightBracket = exprStr.find(')', i);
        string fragment = "", oper = "";
        if (posRightBracket != string::npos) {
            fragment = exprStr.substr(i, posRightBracket - i + 1);
            i = posRightBracket + 1;
        }
        else {
            fragment = exprStr.substr(i, exprStr.size() - i);
        }
        exprStack.push(extractZ3Expr(fragment, ""));

        while (!isalpha(exprStr[i]) && !isdigit(exprStr[i])) {
            i++;
        }
        if (posRightBracket != string::npos) {
            oper = exprStr.substr(posRightBracket + 1, i - posRightBracket - 1);
            oper.erase(std::remove_if(oper.begin(), oper.end(), isspace), oper.end());
            if (oper.size() >= 3) {
                size_t pos_sub = oper.find('-');
                oper = oper.substr(0, pos_sub);
                negative = true;
            }
            else if (oper.empty())
                break;
        } else {
            if (negative) {
                operatorStack.push("-");
            }
            break;
        }
        if (exprStack.size() > 1 && !compareOperator(oper, operatorStack.top())) {
            Z3Expr expr2 = exprStack.top();
            exprStack.pop();
            Z3Expr expr1 = exprStack.top();
            exprStack.pop();
            string operatorTop = operatorStack.top();
            operatorStack.pop();

            exprStack.push(calcExpr(expr1, operatorTop, expr2));
        }
        operatorStack.push(oper);
    }

    while (operatorStack.size() > 1) {
        string operatorTop = operatorStack.top() + "";
        operatorStack.pop();

        if (exprStack.size() < 2) {
            logger->error("运算数或变量不足，表达式\"%s\"不合法", exprStr.c_str());
            break;
        }
        Z3Expr expr2 = exprStack.top();
        if (exprStack.size() == 2 && operatorTop == "-") {
            expr2 = generateNumExp("-" + expr2.to_string());
            operatorTop = operatorStack.top() + "";
            operatorStack.pop();
        }
        exprStack.pop();
        Z3Expr expr1 = exprStack.top();
        exprStack.pop();

        exprStack.push(calcExpr(expr1, operatorTop, expr2));
    }
    logger->debug("转为Z3: %s", exprStack.top().to_string().c_str());
    return exprStack.top();
}

const Z3Expr Model::extractZ3Expr(const string &exprStr, const string &serialNum) {
    // 借用运算符栈和运算数栈实现表达式的解析
    stack<string> operatorStack;
    // expr栈用来记录中间表达式结果
    stack<Z3Expr> exprStack;
    // 先压入最低优先级运算符
    operatorStack.push("$");
    // 遍历字符串表达式
    string identifier;    // 记录标识符
    string currentType;    // 记录当前类型

    bool isSpec = false;
    if (serialNum.empty()) {
        isSpec = true;
    }
    string funcName;
    string funcNameNum;
    string varTimeName = "";
    for (auto iter : varsDecl) {
        if (iter.second == "time") {
            varTimeName = iter.first;
            break;
        }
    }
    Z3Expr funcTExpr = this->ctx.int_const(varTimeName.c_str());

    for (auto c : exprStr) {
        if (currentType.empty()) {
            //logger->debug("### curentType: empty  %c", c);
            // 当前无类型，即可从任意符号开始新的标识符
            if (isalpha(c) || c == '_') {
                // 当前开始一个新的变量
                currentType = "var";
                identifier.push_back(c);
            } else if (isdigit(c) || c == '.') {
                // 当前开始一个新的运算数
                currentType = "operand";
                identifier.push_back(c);
            } else if (isOperator(c)) {
                // 当前为运算符
                currentType = "operator";
                identifier.push_back(c);
            } else if (!isspace(c)) {
                // 其他情况必须为空格
                logger->error("表达式中出现非法字符%c", c);
            }
        } else if (currentType == "var") {
            //logger->debug("### curentType: var  %c", c);
            // 当前已处于变量状态
            if (isalnum(c) != 0 || c == '_') {
                // 继续当前变量
                identifier.push_back(c);
            } else {
                // 当前为其他字符则生成完整变量并考虑true或false的情况
                if (identifier == "true") {
                    exprStack.push(this->ctx.bool_val(true));
                }
                else if (identifier == "false") {
                    exprStack.push(this->ctx.bool_val(false));
                }
                else {
                    // 根据变量名生成变量名expr
                    if (serialNum.empty() && varsDecl[identifier] != "time") {
                        funcName = identifier;
                    } else {
                        exprStack.push(generateVarExp(identifier + serialNum, funcTExpr));
                    }
                }
                identifier.clear();

                if (isspace(c) != 0) {
                    currentType.clear();
                } else if (isOperator(c)) {
                    currentType = "operator";
                    identifier.push_back(c);
                } else if (c == '(') {
                    currentType = "operand";
                    //isSpec = true;
                } else if (c == ')') {
                    currentType = "operator";
                } else {
                    logger->error("表达式中出现不合法的标识符%s%c", identifier.c_str(), c);
                }
            }
        } else if (currentType == "operand") {
            //logger->debug("### curentType: operand  %c", c);
            // 当前已处于运算数状态
            if (isdigit(c) != 0 || c == '.') {
                // 继续当前运算数
                identifier.push_back(c);
            } else {
                // 当前为其他字符则生成完整运算数
                // 分析数的类型得到不同的expr
                if (!isSpec || c == ')') {
                    exprStack.push(generateNumExp(identifier));
                } else {
                    funcNameNum = identifier;
                }
                identifier.clear();

                if (isspace(c) != 0) {
                    currentType = "";
                } else if (isOperator(c)) {
                    currentType = "operator";
                    identifier.push_back(c);
                } else if (c == ',') {
                    currentType = "";
                } else if (c == ')') {
                    currentType = "operator";
                    //isSpec = false;
                } else {
                    logger->error("表达式中出现不合法的标识符%s%c", identifier.c_str(), c);
                }
            }
        } else if (currentType == "operator") {
            //logger->debug("### curentType: operator  %c", c);
            // 当前已经处于运算符状态
            if (isOperator(c)) {
                // 继续当前运算符
                identifier.push_back(c);
            } else {
                // 当前为其他字符则生成完整运算符
                // 将当前运算符与栈顶相比较
                while (exprStack.size() >= 2 && !compareOperator(identifier, operatorStack.top())) {
                    // 只要当前运算符比栈顶运算符优先级低就一直退两个表达式和一个运算符进行运算后压栈
                    const string operatorTop = operatorStack.top() + "";
                    operatorStack.pop();

                    Z3Expr expr2 = exprStack.top();
                    exprStack.pop();
                    Z3Expr expr1 = exprStack.top();
                    exprStack.pop();
                    exprStack.push(calcExpr(expr1, operatorTop, expr2));
                }
                // 退出循环时表示当前运算符比栈顶运算符优先级高
                operatorStack.push(identifier);
                identifier.clear();

                if (isalpha(c) != 0 || c == '_') {
                    currentType = "var";
                    identifier.push_back(c);
                } else if (isdigit(c) != 0 || c == '.') {
                    currentType = "operand";
                    identifier.push_back(c);
                } else if (isspace(c) != 0) {
                    currentType = "";
                } else if (c == ')') {
                } else {
                    logger->error("表达式中出现非法字符%c", c);
                }
            }
        }
    }

    // 循环结束后需要手动添加结束符, 即循环退栈
    if (currentType == "var") {
        // 根据变量名生成变量名expr
        identifier.clear();
    } else if (currentType == "operand") {
        // 分析数的类型得到不同的expr
        exprStack.push(generateNumExp(identifier));
        identifier.clear();
    } else if (currentType == "operator" && !isSpec) {
        logger->error("表达式\"%s\"以运算符结尾，非法！", exprStr.c_str());
    }

    while (operatorStack.size() > 1) {
        string operatorTop = operatorStack.top() + "";
        operatorStack.pop();

        if (exprStack.size() < 2) {
            logger->error("运算数或变量不足，表达式\"%s\"不合法", exprStr.c_str());
            break;
        }
        Z3Expr expr2 = exprStack.top();
        exprStack.pop();
        Z3Expr expr1 = exprStack.top();
        exprStack.pop();

        exprStack.push(calcExpr(expr1, operatorTop, expr2));
    }

    if (isSpec) {
        if (funcName.empty()) {
            return exprStack.top();
        }
        Z3Expr e = generateVarExp(funcName + funcNameNum, exprStack.top());
        exprStack.pop();
        exprStack.push(e);
    }
    return exprStack.top();
}

const Z3Expr Model::generateVarExp(const string &varName, const Z3Expr& TExpr) {
    auto digitIndex = varName.size();
    for (auto digitBegin = varName.rbegin(); digitBegin != varName.rend(); ++digitBegin, --digitIndex) {
        if (!isdigit(*digitBegin)) {
            break;
        }
    }

    string varNameWithoutNum = varName.substr(0, digitIndex);
    string serialNum = varName.substr(digitIndex);

    if (varsDecl.find(varNameWithoutNum) == varsDecl.end()) {
        logger->error("变量\"%s\"未定义", varName.c_str());
        return this->ctx.bool_val(true);
    }

//    if (serialNum.empty() || states.find(std::stoi(serialNum)) == states.end()) {
//        logger->error("变量\"%s\"缺少有效的序号后缀", varName.c_str());
//        return this->ctx.bool_val(true);
//    }

    string varType = varsDecl[varNameWithoutNum];
    if (varType == "int" || varType == "time") {
        return this->ctx.int_const(varName.c_str());
    }
    else if (varType == "double") {
        return this->ctx.real_const(varName.c_str());
    }
    else if (varType == "bool") {
        return this->ctx.bool_const(varName.c_str());
    }
    else if (varType == "int_function") {
        func_decl f = function(varNameWithoutNum.c_str(), this->I, this->I, this->I);
        return f(std::stoi(serialNum), TExpr);
    }
    else if (varType == "double_function") {
        func_decl f = function(varNameWithoutNum.c_str(), this->I, this->I, this->D);
        return f(std::stoi(serialNum), TExpr);
    }
    else if (varType == "bool_function") {
        func_decl f = function(varNameWithoutNum.c_str(), this->I, this->I, this->B);
        return f(std::stoi(serialNum), TExpr);
    }
    else {
        logger->error("不支持的变量类型，变量为%s", varName.c_str());
        return this->ctx.bool_val(true);
    }
}

const Z3Expr Model::generateNumExp(const string &operand) {
    if (operand.find('.') != string::npos) return this->ctx.real_val(operand.c_str());
    else return this->ctx.int_val(operand.c_str());
}

bool Model::isOperator(char c) {
    static string operatorStr = "+-*/<>!=";
    return operatorStr.find(c) != string::npos;
}

bool Model::compareOperator(const string &operator1, const string &operator2) {
    map<string, int> operatorPriority = {
            {"$",  0},
            {"==", 1},
            {"!=", 1},
            {"<",  2},
            {"<=", 2},
            {">",  2},
            {">=", 2},
            {"+",  3},
            {"-",  3},
            {"*",  4},
            {"/",  4},
    };
    if (operatorPriority.find(operator1) == operatorPriority.end()) {
        logger->error("运算符\"%s\"不支持", operator1.c_str());
        return false;
    }
    if (operatorPriority.find(operator2) == operatorPriority.end()) {
        logger->error("运算符\"%s\"不支持", operator2.c_str());
        return false;
    }
    return operatorPriority[operator1] > operatorPriority[operator2];
}

const Z3Expr Model::calcExpr(const Z3Expr &expr1, const string &currentOperator, const Z3Expr &expr2) {
    if (currentOperator == "==") return expr1 == expr2;
    if (currentOperator == "!=") return expr1 != expr2;
    if (currentOperator == "<") return expr1 < expr2;
    if (currentOperator == "<=") return expr1 <= expr2;
    if (currentOperator == ">") return expr1 > expr2;
    if (currentOperator == ">=") return expr1 >= expr2;
    if (currentOperator == "+") return expr1 + expr2;
    if (currentOperator == "-") return expr1 - expr2;
    if (currentOperator == "*") return expr1 * expr2;
    if (currentOperator == "/") return expr1 / expr2;

    logger->error("不支持的运算符\"%s\"", currentOperator.c_str());
    return expr1;
}

EventVerifyResultEnum Model::verify(const State *nextState, const map<string, string> &varValueMap) {
    int nextStateNum = nextState->getStateNum();

    logger->debug("尝试转移到节点%d", nextStateNum);

    slv.push();
    slvNegative.push();
    // 先把下一状态中的全部Z3表达式求与
    const vector<Z3Expr> &nextStateZ3Together = nextState->getZ3Together();
    if (!nextStateZ3Together.empty()) {
        logger->debug("添加下一状态的表达式%s", nextStateZ3Together[0].to_string().c_str());
        slv.add(nextStateZ3Together[0]);
        slvNegative.add(!nextStateZ3Together[0]);
    }

    // 再将事件上的变量全部构造成Z3表达式添加进去
    string varTimeName = "";
    for (auto& varValue : varValueMap) {
        if (varsDecl[varValue.first] == "time") {
            varTimeName = varValue.first;
            break;
        }
    }
    vector<Z3Expr> tmpEventExpr;
    for (auto &varValue : varValueMap) {
        string newEvent = varValue.first + "==" + varValue.second;
        Z3Expr newEventExpr = extractZ3Expr(newEvent, std::to_string(nextStateNum));
        if (varsDecl[varValue.first] != "time") {
            string oldEvent = varValue.first +
                              "(" + std::to_string(currentState->getStateNum()) + ", " + varTimeName +
                              "-1) == " + varValue.second;
            std::cout << "new-oldEvent: " << oldEvent << std::endl;
            tmpEventExpr.push_back(extractZ3ExprForSpec(oldEvent));
        } else {
            newEventExpr = extractZ3Expr(newEvent, "");
        }

        slv.add(newEventExpr);
        slvNegative.add(newEventExpr);
        logger->debug("添加新事件上变量值构成的表达式%s", newEventExpr.to_string().c_str());
    }

    for (auto oee : oldEventExpr) {
        slv.add(oee);
        slvNegative.add(oee);
        logger->debug("添加旧事件上变量值构成的表达式%s", oee.to_string().c_str());
    }

    z3::check_result positiveResult = slv.check();
    z3::check_result negativeResult = slvNegative.check();

    slv.pop();
    slvNegative.pop();

    oldEventExpr.clear();
    oldEventExpr = tmpEventExpr;

    if (positiveResult == z3::unsat) {
        logger->info("尝试转移到节点%d失败", nextStateNum);
        currentFailedStates.insert(nextState);
        return EventVerifyResultEnum::refuse;
    }
    else if (negativeResult == z3::sat) {
        logger->info("尝试转移到节点%d未定", nextStateNum);
        currentFailedStates.insert(nextState);
        return EventVerifyResultEnum::undetermined;
    }
    else  {
        logger->info("尝试转移到节点%d成功", nextStateNum);
        return EventVerifyResultEnum::pass;
    }
}
