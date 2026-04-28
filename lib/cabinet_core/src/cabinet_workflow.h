#pragma once

#include "fsm.h"

struct ClassifiedScanDecision {
  bool useImmediateEffect = false;
  CabinetEffect immediateEffect = CabinetEffect::None;
  CabinetEventType event = CabinetEventType::DeniedScan;
};

ClassifiedScanDecision decideClassifiedScan(const ClassifiedUid& classified);
CabinetEventType decideProcessFollowUpEvent(const ProcessResult& processResult, const String& expectedAction);