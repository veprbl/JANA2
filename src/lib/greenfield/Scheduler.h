#pragma once

#include <mutex>

#include <greenfield/Arrow.h>
#include "Logger.h"


namespace greenfield {

    /// Scheduler assigns Arrows to Workers in a first-come-first-serve manner,
    /// not unlike OpenMP's `schedule dynamic`. Its tradeoffs are:
    class Scheduler {

    private:
        std::vector<Arrow*> _arrows;
        size_t _next_idx;
        size_t _worker_count;
        std::mutex _mutex;
        Logger _logger;

    public:

        /// Constructor. Note that a Scheduler operates on a vector of Arrow*s.
        Scheduler(const std::vector<Arrow*>& arrows, size_t worker_count);

        /// Lets a Worker ask the Scheduler for another assignment. If no assignments make sense,
        /// Scheduler returns nullptr, which tells that Worker to idle until his next checkin.
        /// If next_assignment() makes any changes to internal Scheduler state or to any of its arrows,
        /// it must be synchronized.
        Arrow* next_assignment(uint32_t worker_id, Arrow* assignment, StreamStatus result);

        /// Lets a Worker tell the scheduler that he is shutting down and won't be working on his assignment
        /// any more. The scheduler is thus free to reassign the arrow to one of the remaining workers.
        void last_assignment(uint32_t worker_id, Arrow* assignment, StreamStatus result);

    };

}




