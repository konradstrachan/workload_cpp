#include "Workload.h"

#include <iostream>
#include <string>

int main()
{
    Workload::Orchestrator o1;

    o1.RegisterWorkUnit(
        []()
        {
            std::cout << "A1: 4hz Tick\n";
        },
        250'000'000);

    o1.RegisterWorkUnit(
        [&o1]()
        {
            std::cout << "B1: 1hz Tick\n";
        },
        1'000'000'000);

    o1.RegisterWorkloadMessageHandle(
        [&o1](const Workload::Message& iwm)
        {
            std::cout << "C1: Recv message from 2 [" + std::to_string((uint32_t)iwm.buf[0]) + "]\n";
            assert(iwm.originPtr);
            // Echo back message
            iwm.originPtr->PostInterWorkloadMessage(iwm);

            if (iwm.buf[0] == 5)
            {
                o1.Stop();
            }
        }
    );

    o1.Start();

    Workload::Orchestrator o2;

    o2.RegisterWorkUnit(
        []()
        {
            std::cout << "A2: 10hz Tick\n";
        },
        100'000'000);

    o2.RegisterWorkUnit(
        [&o1, &o2]()
        {
            std::cout << "A2: 1hz Send to 1\n";
            Workload::Message iwm;
            iwm.originPtr = &o2;
            static uint8_t counter{ 0 };
            iwm.buf[0] = counter++;

            o1.PostInterWorkloadMessage(iwm);
        },
        1'000'000'000);

    o2.RegisterWorkloadMessageHandle(
        [&o2](const Workload::Message& iwm)
        {
            (void)iwm;
            std::cout << "C2: Recv message from 1 [" + std::to_string((uint32_t)iwm.buf[0]) + "]\n";
            assert(iwm.originPtr);

            if (iwm.buf[0] == 5)
            {
                o2.Stop();
            }
        }
    );

    o2.Start();

    o2.Wait();
    o1.Wait();

    return 0;
}