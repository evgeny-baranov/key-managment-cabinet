#pragma once

#include <Arduino.h>

#include "protocol.h"

enum class CabinetState {
  Idle,
  AwaitingKeyForTake,
  AwaitingUserForReturn,
  ProcessingTake,
  ProcessingReturn,
  Success,
  Error
};

enum class CabinetEventType {
  UserScanned,
  KeyScanned,
  DeniedScan,
  TakeCompleted,
  ReturnCompleted,
  ProcessDenied,
  ProcessFailed,
  Timeout,
  Reset
};

struct CabinetContext {
  CabinetState state = CabinetState::Idle;

  String pendingUserUid;
  String pendingUserName;
  String pendingKeyUid;
  String pendingKeyName;

  unsigned long pendingSince = 0;
};

enum class CabinetEffect {
  None,
  ShowIdle,
  ShowUserAccepted,
  ShowKeyAccepted,
  ShowWrongOrder,
  ShowDenied,
  ShowServerFail,
  ShowTimeout,
  StartTakeRequest,
  StartReturnRequest,
  UnlockAndShowSuccess
};

struct TransitionResult {
  CabinetEffect effect = CabinetEffect::None;
  String action;
  String userUid;
  String keyUid;
};

TransitionResult transition(
  CabinetContext& ctx,
  CabinetEventType event,
  const ClassifiedUid* classified,
  const ProcessResult* processResult,
  unsigned long now
);
