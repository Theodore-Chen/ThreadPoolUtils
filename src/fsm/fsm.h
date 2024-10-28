#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <map>
#include <thread>

#include "fsm/state_table.h"
#include "queue/lock_queue.h"

template <typename State, typename Event>
class FSM {
   public:
    FSM(StateTable<State, Event>* stateTable, StateChangeTable<State, Event>* changeTable, State initial) {
        curState_.store(initial);
        ready_.store(true);
        pause_.store(false);
        pooling_ = std::async(std::launch::async, FSMProcessor(this, stateTable, changeTable));
    }
    ~FSM() {
        ready_.store(false);
        pooling_.wait();
    }
    FSM(const FSM&) = delete;
    FSM(FSM&&) = delete;
    FSM& operator=(const FSM&) = delete;
    FSM& operator=(FSM&&) = delete;

    std::future<void> Submit(Event event) {
        Handle handle(event, std::promise<void>());
        std::future<void> fut = handle.second.get_future();
        queue_.Enqueue(std::move(handle));
        return fut;
    }
    State GetState() { return curState_; }

   private:
    class FSMProcessor {
       public:
        FSMProcessor(FSM<State, Event>* fsm,
                     StateTable<State, Event>* stateTable,
                     StateChangeTable<State, Event>* changeTable)
            : fsm_(fsm), stateTable_(stateTable), changeTable_(changeTable) {}
        void operator()() {
            if (!fsm_) {
                return;
            }
            Handle handle;
            while (fsm_->ready_.load() == true) {
                if (fsm_->pause_.load() == false && fsm_->queue_.Dequeue(handle)) {
                    stateTable_.Callback(fsm_->curState_, handle.first);
                    ChangeState(handle.first);
                    handle.second.set_value();
                }
            }
        }

       private:
        void ChangeState(Event event) {
            State toState;
            if (changeTable_.GetTostate(fsm_->curState_, event, toState)) {
                stateTable_.Exit(fsm_->curState_, event);
                stateTable_.Entry(toState, event);
                stateTable_.Callback(toState, event);
                fsm_->curState_ = toState;
            }
        };

       private:
        FSM<State, Event>* fsm_{nullptr};
        FsmStateTable<State, Event> stateTable_;
        FsmStateChangeTable<State, Event> changeTable_;
    };

   private:
    using Handle = std::pair<Event, std::promise<void>>;

   private:
    std::future<void> pooling_;
    LockQueue<Handle> queue_;
    std::atomic<State> curState_;
    std::atomic<bool> pause_{true};
    std::atomic<bool> ready_{false};
};