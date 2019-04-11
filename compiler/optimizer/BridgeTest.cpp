#include "env/VerboseLog.hpp"
#include "optimizer/BridgeTest.hpp"

bool
OMR::BridgeTest::shouldPerform() {
   return true;
}

int32_t
OMR::BridgeTest::perform() {
   // Say Hello world in the verbose log.
   TR_VerboseLog::vlogAcquire();
   // TR_Vlog_HWO is the tag to identify one's optimization pass in the verbose log.
   TR_VerboseLog::writeLine(TR_Vlog_BT, "Hello world");
   TR_VerboseLog::vlogRelease();
   return true;
}
