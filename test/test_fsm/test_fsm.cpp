#include <Arduino.h>
#include <unity.h>

#include "fsm.h"

namespace {
ClassifiedUid makeUser(const char* uid, const char* name) {
  ClassifiedUid c;
  c.type = ScanType::User;
  c.code = ServerCode::Ok;
  c.uid = uid;
  c.name = name;
  return c;
}

ClassifiedUid makeKey(const char* uid, const char* name, const char* status) {
  ClassifiedUid c;
  c.type = ScanType::Key;
  c.code = ServerCode::Ok;
  c.uid = uid;
  c.name = name;
  c.keyStatus = status;
  return c;
}
}

void test_idle_user_scan_starts_take_flow() {
  CabinetContext ctx;
  ClassifiedUid user = makeUser("U001", "Alice");

  TransitionResult r = transition(ctx, CabinetEventType::UserScanned, &user, nullptr, 1234);

  TEST_ASSERT_EQUAL_INT((int)CabinetState::AwaitingKeyForTake, (int)ctx.state);
  TEST_ASSERT_EQUAL_INT((int)CabinetEffect::ShowUserAccepted, (int)r.effect);
  TEST_ASSERT_EQUAL_STRING("U001", ctx.pendingUserUid.c_str());
  TEST_ASSERT_EQUAL_STRING("Alice", ctx.pendingUserName.c_str());
  TEST_ASSERT_EQUAL_UINT32(1234, ctx.pendingSince);
}

void test_idle_key_scan_out_starts_return_flow() {
  CabinetContext ctx;
  ClassifiedUid key = makeKey("K001", "Lab", "OUT");

  TransitionResult r = transition(ctx, CabinetEventType::KeyScanned, &key, nullptr, 2500);

  TEST_ASSERT_EQUAL_INT((int)CabinetState::AwaitingUserForReturn, (int)ctx.state);
  TEST_ASSERT_EQUAL_INT((int)CabinetEffect::ShowKeyAccepted, (int)r.effect);
  TEST_ASSERT_EQUAL_STRING("K001", ctx.pendingKeyUid.c_str());
  TEST_ASSERT_EQUAL_STRING("Lab", ctx.pendingKeyName.c_str());
  TEST_ASSERT_EQUAL_UINT32(2500, ctx.pendingSince);
}

void test_idle_key_scan_in_is_denied() {
  CabinetContext ctx;
  ClassifiedUid key = makeKey("K001", "Lab", "IN");

  TransitionResult r = transition(ctx, CabinetEventType::KeyScanned, &key, nullptr, 2500);

  TEST_ASSERT_EQUAL_INT((int)CabinetState::Idle, (int)ctx.state);
  TEST_ASSERT_EQUAL_INT((int)CabinetEffect::ShowDenied, (int)r.effect);
  TEST_ASSERT_EQUAL_STRING("", ctx.pendingKeyUid.c_str());
  TEST_ASSERT_EQUAL_UINT32(0, ctx.pendingSince);
}

void test_awaiting_key_scan_starts_take_processing() {
  CabinetContext ctx;
  ctx.state = CabinetState::AwaitingKeyForTake;
  ctx.pendingUserUid = "U001";
  ctx.pendingUserName = "Alice";

  ClassifiedUid key = makeKey("K777", "Server room", "IN");
  TransitionResult r = transition(ctx, CabinetEventType::KeyScanned, &key, nullptr, 3333);

  TEST_ASSERT_EQUAL_INT((int)CabinetState::ProcessingTake, (int)ctx.state);
  TEST_ASSERT_EQUAL_INT((int)CabinetEffect::StartTakeRequest, (int)r.effect);
  TEST_ASSERT_EQUAL_STRING("TAKE", r.action.c_str());
  TEST_ASSERT_EQUAL_STRING("U001", r.userUid.c_str());
  TEST_ASSERT_EQUAL_STRING("K777", r.keyUid.c_str());
}

void test_awaiting_user_scan_starts_return_processing() {
  CabinetContext ctx;
  ctx.state = CabinetState::AwaitingUserForReturn;
  ctx.pendingKeyUid = "K777";
  ctx.pendingKeyName = "Server room";

  ClassifiedUid user = makeUser("U001", "Alice");
  TransitionResult r = transition(ctx, CabinetEventType::UserScanned, &user, nullptr, 3333);

  TEST_ASSERT_EQUAL_INT((int)CabinetState::ProcessingReturn, (int)ctx.state);
  TEST_ASSERT_EQUAL_INT((int)CabinetEffect::StartReturnRequest, (int)r.effect);
  TEST_ASSERT_EQUAL_STRING("RETURN", r.action.c_str());
  TEST_ASSERT_EQUAL_STRING("U001", r.userUid.c_str());
  TEST_ASSERT_EQUAL_STRING("K777", r.keyUid.c_str());
}

void test_timeout_resets_pending_state() {
  CabinetContext ctx;
  ctx.state = CabinetState::AwaitingKeyForTake;
  ctx.pendingUserUid = "U001";
  ctx.pendingSince = 7000;

  TransitionResult r = transition(ctx, CabinetEventType::Timeout, nullptr, nullptr, 17000);

  TEST_ASSERT_EQUAL_INT((int)CabinetState::Idle, (int)ctx.state);
  TEST_ASSERT_EQUAL_INT((int)CabinetEffect::ShowTimeout, (int)r.effect);
  TEST_ASSERT_EQUAL_STRING("", ctx.pendingUserUid.c_str());
  TEST_ASSERT_EQUAL_UINT32(0, ctx.pendingSince);
}

void test_take_completed_unlocks_and_returns_idle() {
  CabinetContext ctx;
  ctx.state = CabinetState::ProcessingTake;
  ctx.pendingUserUid = "U001";
  ctx.pendingKeyUid = "K001";

  TransitionResult r = transition(ctx, CabinetEventType::TakeCompleted, nullptr, nullptr, 0);

  TEST_ASSERT_EQUAL_INT((int)CabinetState::Idle, (int)ctx.state);
  TEST_ASSERT_EQUAL_INT((int)CabinetEffect::UnlockAndShowSuccess, (int)r.effect);
  TEST_ASSERT_EQUAL_STRING("", ctx.pendingUserUid.c_str());
  TEST_ASSERT_EQUAL_STRING("", ctx.pendingKeyUid.c_str());
}

void test_processing_denied_returns_idle_with_denied_effect() {
  CabinetContext ctx;
  ctx.state = CabinetState::ProcessingReturn;
  ctx.pendingUserUid = "U001";
  ctx.pendingKeyUid = "K001";

  TransitionResult r = transition(ctx, CabinetEventType::ProcessDenied, nullptr, nullptr, 0);

  TEST_ASSERT_EQUAL_INT((int)CabinetState::Idle, (int)ctx.state);
  TEST_ASSERT_EQUAL_INT((int)CabinetEffect::ShowDenied, (int)r.effect);
  TEST_ASSERT_EQUAL_STRING("", ctx.pendingUserUid.c_str());
  TEST_ASSERT_EQUAL_STRING("", ctx.pendingKeyUid.c_str());
}

void setup() {
  delay(2000);
  UNITY_BEGIN();

  RUN_TEST(test_idle_user_scan_starts_take_flow);
  RUN_TEST(test_idle_key_scan_out_starts_return_flow);
  RUN_TEST(test_idle_key_scan_in_is_denied);
  RUN_TEST(test_awaiting_key_scan_starts_take_processing);
  RUN_TEST(test_awaiting_user_scan_starts_return_processing);
  RUN_TEST(test_timeout_resets_pending_state);
  RUN_TEST(test_take_completed_unlocks_and_returns_idle);
  RUN_TEST(test_processing_denied_returns_idle_with_denied_effect);

  UNITY_END();
}

void loop() {
}
