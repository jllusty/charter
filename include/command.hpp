#pragma once

template<typename T>
class Command {
public:
    virtual ~Command() {}
    virtual void execute(T& t);
};