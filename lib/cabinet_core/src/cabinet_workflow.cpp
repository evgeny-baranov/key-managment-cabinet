#include "cabinet_workflow.h"

#include "constants.h"

ClassifiedScanDecision decideClassifiedScan(const ClassifiedUid& classified) {
  ClassifiedScanDecision decision;

  if (classified.type == ScanType::ServerError) {
    decision.useImmediateEffect = true;
    decision.immediateEffect = CabinetEffect::ShowServerFail;
    return decision;
  }

  if (classified.type == ScanType::User) {
    decision.event = CabinetEventType::UserScanned;
  } else if (classified.type == ScanType::Key) {
    decision.event = CabinetEventType::KeyScanned;
  } else {
    decision.event = CabinetEventType::DeniedScan;
  }

  return decision;
}

CabinetEventType decideProcessFollowUpEvent(const ProcessResult& processResult, const String& expectedAction) {
  if (processResult.ok) {
    if (processResult.action != expectedAction) {
      return CabinetEventType::ProcessFailed;
    }

    if (expectedAction == ACTION_TAKE) {
      return CabinetEventType::TakeCompleted;
    }

    return CabinetEventType::ReturnCompleted;
  }

  if (
    processResult.code == ServerCode::KeyNotAvailable ||
    processResult.code == ServerCode::KeyAlreadyIn ||
    processResult.code == ServerCode::UserNotFound ||
    processResult.code == ServerCode::UserInactive ||
    processResult.code == ServerCode::KeyNotFound
  ) {
    return CabinetEventType::ProcessDenied;
  }

  return CabinetEventType::ProcessFailed;
}