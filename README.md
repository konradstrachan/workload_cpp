# Workload library

Single header C++17 threading helper library.

Encapsulates a thread and schedules work along with providing a simple nonblocking lockless inter-thread communication mechanism.

## Usage

```
#include "Workload.h"
```

## Example

```
    Workload::Orchestrator wl;

    wl.RegisterWorkUnit(
        []()
        {
            // Will be called on 10 hz frequency
            std::cout << "A" << std::endl;
        },
        100'000'000);       // wait time in nanoseconds

    wl.RegisterWorkUnit(
        []()
        {
            // Will be called on 2 hz frequency
            std::cout << "B" << std::endl;
        },
        500'000'000);       // wait time in nanoseconds

    wl.RegisterWorkloadMessageHandle(
        [](const Workload::Message& iwm)
        {
            // Will be called whenever a message arrives
            std::cout << "C" << std::endl;
        }
    );

    // Begin work loop
    wl.Start();

    // Blocking call until work loop finishes
    w1.Wait();
```

For a more advanced example of two Orchestrators communicating locklessly please see example code.
