//
// Created by yingzi on 2017/11/30.
//

#include "State.h"

void State::setStateNum(int stateNum) {
    this->stateNum = stateNum;
    this->emptyFlag = false;
}

void State::setStartFlag(bool startFlag) {
    this->startFlag = startFlag;
}

void State::setEndFlag(bool endFlag) {
    this->endFlag = endFlag;
}

void State::addZ3Expr(const Z3Expr &z3Expr) {
    z3ExprList.push_back(z3Expr);
    if (z3ExprTogether.empty()) {
        z3ExprTogether.push_back(z3Expr);
    }
    else {
        z3ExprTogether[0] = (z3ExprTogether[0] && z3Expr);
    }
}

void State::addTran(const Tran *tran) {
    outTranList.push_back(tran);
}

bool State::isEmpty() const {
    return emptyFlag;
}

int State::getStateNum() const {
    return stateNum;
}

bool State::isStartState() const {
    return startFlag;
}

bool State::isEndState() const {
    return endFlag;
}

const vector<Z3Expr> &State::getZ3ExprList() const {
    return z3ExprList;
}

const vector<Z3Expr> &State::getZ3Together() const {
    return z3ExprTogether;
}

const vector<const Tran *> &State::getTranList() const {
    return outTranList;
}

void State::clear() {
    this->emptyFlag = true;
    startFlag = false;
    endFlag = false;
    z3ExprList.clear();
    outTranList.clear();
}
