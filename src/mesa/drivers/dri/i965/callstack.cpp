#include "callstack.h"
#include <utils/CallStack.h>
extern "C" {
    void dumping_callstack(void)
    {
        android::CallStack cs("INTEL-MESA");
    }
}
