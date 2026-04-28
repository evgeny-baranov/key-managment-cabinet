#include "fsm.h"

#include "constants.h"

namespace {
// Clears all transaction-related data so the next scan starts from a clean slate.
void clearPending(CabinetContext& ctx) {
  ctx.pendingUserUid = "";
  ctx.pendingUserName = "";
  ctx.pendingKeyUid = "";
  ctx.pendingKeyName = "";
  ctx.pendingSince = 0;
}

// Pending states are the only states where timeout should force a reset.
bool isPendingState(CabinetState state) {
  return state == CabinetState::AwaitingKeyForTake || state == CabinetState::AwaitingUserForReturn;
}
}

TransitionResult transition(
  CabinetContext& ctx,
  CabinetEventType event,
  const ClassifiedUid* classified,
  const ProcessResult* processResult,
  unsigned long now
) {
  // The FSM itself does not inspect process payload fields yet.
  // The caller maps process responses to explicit events before calling transition().
  (void)processResult;
  TransitionResult result;

  // Global reset path: regardless of current state we return to Idle.
  // This is used for explicit reset operations and post-result cleanup.
  if (event == CabinetEventType::Reset) {
    ctx.state = CabinetState::Idle;
    clearPending(ctx);
    result.effect = CabinetEffect::ShowIdle;
    return result;
  }

  // Timeout only applies to "waiting for second scan" states.
  // Processing states are controlled by HTTP result events instead.
  if (event == CabinetEventType::Timeout && isPendingState(ctx.state)) {
    ctx.state = CabinetState::Idle;
    clearPending(ctx);
    result.effect = CabinetEffect::ShowTimeout;
    return result;
  }

  switch (ctx.state) {
    case CabinetState::Idle:
      // TAKE flow starts with a user card scan.
      if (event == CabinetEventType::UserScanned && classified != nullptr) {
        ctx.state = CabinetState::AwaitingKeyForTake;
        ctx.pendingUserUid = classified->uid;
        ctx.pendingUserName = classified->name;
        ctx.pendingSince = now;
        result.effect = CabinetEffect::ShowUserAccepted;
        return result;
      }

      // RETURN flow can start with key scan only when key is currently OUT.
      if (event == CabinetEventType::KeyScanned && classified != nullptr) {
        if (classified->keyStatus != KEY_STATUS_OUT) {
          ctx.state = CabinetState::Idle;
          clearPending(ctx);
          result.effect = CabinetEffect::ShowDenied;
          return result;
        }

        ctx.state = CabinetState::AwaitingUserForReturn;
        ctx.pendingKeyUid = classified->uid;
        ctx.pendingKeyName = classified->name;
        ctx.pendingSince = now;
        result.effect = CabinetEffect::ShowKeyAccepted;
        return result;
      }

      // Classification-level deny (unknown/inactive/duplicate/etc.) is handled as immediate deny.
      if (event == CabinetEventType::DeniedScan) {
        result.effect = CabinetEffect::ShowDenied;
        return result;
      }
      break;

    case CabinetState::AwaitingKeyForTake:
      // Second step of TAKE: a key scan triggers process request generation.
      if (event == CabinetEventType::KeyScanned && classified != nullptr) {
        ctx.state = CabinetState::ProcessingTake;
        ctx.pendingKeyUid = classified->uid;
        ctx.pendingKeyName = classified->name;
        result.effect = CabinetEffect::StartTakeRequest;
        result.action = ACTION_TAKE;
        result.userUid = ctx.pendingUserUid;
        result.keyUid = ctx.pendingKeyUid;
        return result;
      }

      // Wrong order guard: scanning another user instead of key resets flow.
      if (event == CabinetEventType::UserScanned) {
        ctx.state = CabinetState::Idle;
        clearPending(ctx);
        result.effect = CabinetEffect::ShowWrongOrder;
        return result;
      }
      break;

    case CabinetState::AwaitingUserForReturn:
      // Second step of RETURN: user scan triggers process request generation.
      if (event == CabinetEventType::UserScanned && classified != nullptr) {
        ctx.state = CabinetState::ProcessingReturn;
        ctx.pendingUserUid = classified->uid;
        ctx.pendingUserName = classified->name;
        result.effect = CabinetEffect::StartReturnRequest;
        result.action = ACTION_RETURN;
        result.userUid = ctx.pendingUserUid;
        result.keyUid = ctx.pendingKeyUid;
        return result;
      }

      // Wrong order guard: scanning another key instead of user resets flow.
      if (event == CabinetEventType::KeyScanned) {
        ctx.state = CabinetState::Idle;
        clearPending(ctx);
        result.effect = CabinetEffect::ShowWrongOrder;
        return result;
      }
      break;

    case CabinetState::ProcessingTake:
      // Backend accepted TAKE: unlock effect and return to Idle.
      if (event == CabinetEventType::TakeCompleted) {
        ctx.state = CabinetState::Idle;
        clearPending(ctx);
        result.effect = CabinetEffect::UnlockAndShowSuccess;
        return result;
      }
      // Backend denial (business rule violation) returns deny UX.
      if (event == CabinetEventType::ProcessDenied) {
        ctx.state = CabinetState::Idle;
        clearPending(ctx);
        result.effect = CabinetEffect::ShowDenied;
        return result;
      }
      // Transport/server failure returns server-fail UX.
      if (event == CabinetEventType::ProcessFailed) {
        ctx.state = CabinetState::Idle;
        clearPending(ctx);
        result.effect = CabinetEffect::ShowServerFail;
        return result;
      }
      break;

    case CabinetState::ProcessingReturn:
      // Backend accepted RETURN: unlock effect and return to Idle.
      if (event == CabinetEventType::ReturnCompleted) {
        ctx.state = CabinetState::Idle;
        clearPending(ctx);
        result.effect = CabinetEffect::UnlockAndShowSuccess;
        return result;
      }
      // Backend denial (business rule violation) returns deny UX.
      if (event == CabinetEventType::ProcessDenied) {
        ctx.state = CabinetState::Idle;
        clearPending(ctx);
        result.effect = CabinetEffect::ShowDenied;
        return result;
      }
      // Transport/server failure returns server-fail UX.
      if (event == CabinetEventType::ProcessFailed) {
        ctx.state = CabinetState::Idle;
        clearPending(ctx);
        result.effect = CabinetEffect::ShowServerFail;
        return result;
      }
      break;

    case CabinetState::Success:
    case CabinetState::Error:
      // Reserved states for potential future extensions.
      // For now, only Reset drives them back to Idle.
      if (event == CabinetEventType::Reset) {
        ctx.state = CabinetState::Idle;
        clearPending(ctx);
        result.effect = CabinetEffect::ShowIdle;
        return result;
      }
      break;
  }

  // No matching transition: caller can treat this as "ignore event".
  result.effect = CabinetEffect::None;
  return result;
}
