
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JArrowTopology.h"
#include "JEventProcessorArrow.h"
#include "JEventSourceArrow.h"

JArrowTopology::JArrowTopology() = default;

JArrowTopology::~JArrowTopology() {
    LOG_DEBUG(m_logger) << "JArrowTopology: Entering destructor" << LOG_END;
    // finish(); // We don't want to call finish() here in case there was an exception in JArrow::initialize(), finalize()

    for (auto arrow : arrows) {
        delete arrow;
    }
    for (auto queue : queues) {
        delete queue;
    }
}

void JArrowTopology::drain() {
    if (m_current_status == Status::Finished) {
        LOG_DEBUG(m_logger) << "JArrowTopology: drain(): Skipping because topology is already Finished" << LOG_END;
        return;
    }
    LOG_DEBUG(m_logger) << "JArrowTopology: drain()" << LOG_END;
    for (auto source : sources) {
        if (source->get_status() == JArrow::Status::Running) {
            // TODO: I'm considering creating a single TopologyMutex that controls access to
            //         - scheduler
            //         - arrow thread counts
            //         - arrow state
            //         - running upstream counts
            //         - running arrow counts
            //         - total worker count
        }
        source->pause();
        m_current_status = Status::Draining;
        // We pause (as opposed to finish) for two reasons:
        // 1. There might be workers in the middle of calling eventSource->GetEvent.
        // 2. drain() might be called from a signal handler. It isn't safe to make syscalls during signal handlers
        //    due to risk of deadlock. (We technically shouldn't even do logging!)
    }
}

std::ostream& operator<<(std::ostream& os, JArrowTopology::Status status) {
    switch(status) {
        case JArrowTopology::Status::Running: os << "Running"; break;
        case JArrowTopology::Status::Pausing: os << "Pausing"; break;
        case JArrowTopology::Status::Paused: os << "Paused"; break;
        case JArrowTopology::Status::Finished: os << "Finished"; break;
        case JArrowTopology::Status::Draining: os << "Draining"; break;
    }
    return os;
}

void JArrowTopology::run(int nthreads) {

    if (m_current_status == Status::Running || m_current_status == Status::Finished) {
        LOG_DEBUG(m_logger) << "JArrowTopology: run() : " << m_current_status << " => " << m_current_status << LOG_END;
        return;
    }
    LOG_DEBUG(m_logger) << "JArrowTopology: run() : " << m_current_status << " => Running" << LOG_END;
    for (auto source : sources) {
        // We activate any sources, and everything downstream activates automatically
        // Note that this won't affect finished sources. It will, however, stop drain().
        source->run();
    }
    // Note that we activate workers AFTER we activate the topology, so no actual processing will have happened
    // by this point when we start up the metrics.
    metrics.reset();
    metrics.start(nthreads);
    m_current_status = Status::Running;
}

void JArrowTopology::request_pause() {
    // This sets all Running arrows to Paused, which prevents Workers from picking up any additional assignments
    // Once all Workers have completed their remaining assignments, the scheduler will notify us via achieve_pause().
    if (m_current_status == Status::Running) {
        LOG_DEBUG(m_logger) << "JArrowTopology: request_pause() : " << m_current_status << " => Pausing" << LOG_END;
        for (auto arrow: arrows) {
            arrow->pause();
            // If arrow is not running, pause() is a no-op
        }
        m_current_status = Status::Pausing;
    }
    else {
        LOG_DEBUG(m_logger) << "JArrowTopology: request_pause() : " << m_current_status << " => " << m_current_status << LOG_END;
    }
}

void JArrowTopology::achieve_pause() {
    // This is meant to be used by the scheduler to tell us when all workers have stopped, so it is safe to stop(), etc
    if (m_current_status == Status::Running || m_current_status == Status::Pausing || m_current_status == Status::Draining) {
        LOG_DEBUG(m_logger) << "JArrowTopology: achieve_pause() : " << m_current_status << " => " << Status::Paused << LOG_END;
        metrics.stop();
        m_current_status = Status::Paused;
    }
    else {
        LOG_DEBUG(m_logger) << "JArrowTopology: achieve_pause() : " << m_current_status << " => " << m_current_status << LOG_END;
    }
}

void JArrowTopology::finish() {
    // This finalizes all arrows. Once this happens, we cannot restart the topology.
    if (m_current_status == JArrowTopology::Status::Finished) {
        LOG_DEBUG(m_logger) << "JArrowTopology: finish() : " << m_current_status << " => Finished" << LOG_END;
        return;
    }
    LOG_DEBUG(m_logger) << "JArrowTopology: finish() : " << m_current_status << " => Finished" << LOG_END;
    assert(m_current_status == Status::Paused);
    for (auto arrow : arrows) {
        arrow->finish();
    }
    m_current_status = Status::Finished;
}

