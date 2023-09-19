#pragma once

#include <functional>
#include <thread>
#include <atomic>
#include <cassert>
#include <chrono>
#include <algorithm>

#include "ringbuffer.hpp"

namespace Workload
{

class Orchestrator;

struct Message
{
    uint8_t buf[256];
    // NOTE: weak_ptr preferred but incurs overhead
    Orchestrator* originPtr;
};

class Orchestrator
{
public:
    using WorkloadUnit = std::function<void(void)>;
    using WorkloadUnitMessageHandler = std::function<void(const Message&)>;

    explicit Orchestrator()
        : m_running(false)
    { }

    ~Orchestrator()
    {
        m_running = false;
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    void RegisterWorkUnit(WorkloadUnit wu, uint64_t minWaitBetweenExecutionsNanoSeconds)
    {
        if (m_running)
        {
            // Can only add work units when not running to prevent cross thread 
            // access to shared object
            assert(false);
            return;
        }

        m_wlu.emplace_back(WorkloadUnitInfo{ wu, 0, minWaitBetweenExecutionsNanoSeconds });
    }

    void RegisterWorkloadMessageHandle(WorkloadUnitMessageHandler wumh)
    {
        if (m_running)
        {
            // Can only register when not running to prevent cross thread 
            // access to shared object
            assert(false);
            return;
        }

        m_msgHandler = wumh;
    }

    void Start()
    {
        assert(!m_running);

        m_running = true;
        m_thread = std::thread(&Orchestrator::ThreadLoop, this);
    }

    void Stop()
    {
        m_running = false;
    }

    void Wait()
    {
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    bool PostInterWorkloadMessage(const Message& msg)
    {
        return m_interWorkloadMessages.insert(msg);
    }

    Orchestrator(const Orchestrator&) = delete;
    Orchestrator(Orchestrator&&) = delete;
    Orchestrator& operator=(const Orchestrator&) = delete;

private:

    void ThreadLoop()
    {
        while (m_running)
        {
            const uint64_t nowNanoSeconds = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

            // By default wait 100 microseconds between execution runs
            // tasks that need to be run more often will reduce this value
            uint64_t minWaitTimeNanoSeconds{ 100'000'000 };

            Message iwm;
            while (m_interWorkloadMessages.remove(iwm))
            {
                m_msgHandler(iwm);
            }

            for ( auto& wlu : m_wlu )
            {
                const uint64_t deltaSinceLastExecutionNanoSeconds = nowNanoSeconds - wlu.lastExecutionTimestampNanoseconds;

                if (deltaSinceLastExecutionNanoSeconds >= wlu.minWaitBetweenExecutionsNanoseconds)
                {
                    wlu.lastExecutionTimestampNanoseconds = nowNanoSeconds;
                    wlu.cb();

                    minWaitTimeNanoSeconds = std::min(minWaitTimeNanoSeconds, wlu.minWaitBetweenExecutionsNanoseconds);
                }
            }

            // TODO brief yield or is it better to spin?
            std::this_thread::sleep_for(std::chrono::nanoseconds(minWaitTimeNanoSeconds));
        }
    }

    std::atomic_bool m_running;
    std::thread m_thread;

    struct WorkloadUnitInfo
    {
        WorkloadUnit cb;
        uint64_t lastExecutionTimestampNanoseconds;
        const uint64_t minWaitBetweenExecutionsNanoseconds;
    };

    std::vector<WorkloadUnitInfo> m_wlu;
    Ringbuffer<Message, 128> m_interWorkloadMessages;

    WorkloadUnitMessageHandler m_msgHandler;
};

}   // namespace Workload