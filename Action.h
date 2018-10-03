#pragma once

#include <algorithm>
#include <unordered_map>
#include <string>

#include "board.h"

class Action {
public:
    Action(unsigned code = -1u) : code_(code) {}

    Action(const Action &rAction) : code_(rAction.code_) {}

    virtual ~Action() {}

    class Slide; // create a sliding action with Board directions
    class Place; // create a placing action with position and grid_

    virtual Board::reward_t Apply(Board &board) const {
        auto proto = entries().find(type());
        if (proto != entries().end())
            return proto->second->reinterpret(this).Apply(board);

        return -1;
    }

    virtual std::ostream &operator>>(std::ostream &out) const {
        auto proto = entries().find(type());
        if (proto != entries().end())
            return proto->second->reinterpret(this) >> out;

        return out << "??";
    }

    virtual std::istream &operator<<(std::istream &in) {
        auto state = in.rdstate();
        for (auto proto = entries().begin(); proto != entries().end(); proto++) {
            if (proto->second->reinterpret(this) << in) return in;
            in.clear(state);
        }
        return in.ignore(2);
    }

    operator unsigned() const { return code_; }

    unsigned int type() const { return code_ & type_flag(-1u); }

    unsigned int event() const { return code_ & ~type(); }

    friend std::ostream &operator<<(std::ostream &out, const Action &a) { return a >> out; }

    friend std::istream &operator>>(std::istream &in, Action &a) { return a << in; }

protected:
    static constexpr unsigned type_flag(unsigned v) { return v << 24; }

    typedef std::unordered_map<unsigned, Action *> prototype;

    static prototype &entries() {
        static prototype m;
        return m;
    }

    virtual Action &reinterpret(const Action *a) const { return *new(const_cast<Action *>(a)) Action(*a); }

    unsigned int code_;
};

class Action::Slide : public Action {
public:
    static constexpr unsigned type_ = type_flag('s');

    Slide(unsigned int direction) : Action(Slide::type_ | (direction & 0b11)) {}

    Slide(const Action &a = {}) : Action(a) {}

    Board::reward_t Apply(Board &b) const {
        return b.Slide(event());
    }

    std::ostream &operator>>(std::ostream &out) const {
        return out << '#' << ("URDL")[event() & 0b11];
    }

    std::istream &operator<<(std::istream &in) {
        if (in.peek() == '#') {
            char v;
            in.ignore(1) >> v;
            const char *opc = "URDL";
            unsigned oper = std::find(opc, opc + 4, v) - opc;
            if (oper < 4) {
                operator=(Action::Slide(oper));
                return in;
            }
        }
        in.setstate(std::ios::failbit);
        return in;
    }

protected:
    Action &reinterpret(const Action *a) const { return *new(const_cast<Action *>(a)) Slide(*a); }

    static __attribute__((constructor)) void init() { entries()[type_flag('s')] = new Slide; }
};

class Action::Place : public Action {
public:
    static constexpr unsigned type = type_flag('p');

    Place(unsigned pos, unsigned tile) : Action(Place::type | (pos & 0x0f) | (std::min(tile, 35u) << 4)) {}

    Place(const Action &a = {}) : Action(a) {}

    unsigned position() const { return event() & 0x0f; }

    unsigned tile() const { return event() >> 4; }

public:
    Board::reward_t Apply(Board &b) const {
        return b.Place(position(), tile());
    }

    std::ostream &operator>>(std::ostream &out) const {
        const char *idx = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ?";
        return out << idx[position()] << idx[std::min(tile(), 36u)];
    }

    std::istream &operator<<(std::istream &in) {
        const char *idx = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        char v = in.peek();
        unsigned pos = std::find(idx, idx + 16, v) - idx;
        if (pos < 16) {
            in.ignore(1) >> v;
            unsigned tile = std::find(idx, idx + 36, v) - idx;
            if (tile < 36) {
                operator=(Action::Place(pos, tile));
                return in;
            }
        }
        in.setstate(std::ios::failbit);
        return in;
    }

protected:
    Action &reinterpret(const Action *a) const { return *new(const_cast<Action *>(a)) Place(*a); }

    static __attribute__((constructor)) void init() { entries()[type_flag('p')] = new Place; }
};
