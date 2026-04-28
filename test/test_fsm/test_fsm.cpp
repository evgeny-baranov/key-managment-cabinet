#include <Arduino.h>
#include <unity.h>

#include "cabinet_workflow.h"
#include "fsm.h"
#include "protocol.h"

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

ClassifiedUid makeServerError() {
  ClassifiedUid c;
  c.type = ScanType::ServerError;
  c.code = ServerCode::Error;
  return c;
}

ProcessResult makeProcessOk(const char* action) {
  ProcessResult result;
  result.ok = true;
  result.code = ServerCode::Ok;
  result.action = action;
  return result;
}

ProcessResult makeProcessFailure(ServerCode code) {
  ProcessResult result;
  result.ok = false;
  result.code = code;
  return result;
}
}

// Verifies that the FSM starts a TAKE transaction only after a valid user scan,
// preserving the scanned user identity and starting the timeout window.
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

// Verifies that a key marked as OUT can start the RETURN flow from Idle,
// and that the pending key context is captured for the second scan.
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

// Verifies that scanning a key which is already IN does not start RETURN,
// and instead produces an immediate denial without leaving stale pending data.
void test_idle_key_scan_in_is_denied() {
  CabinetContext ctx;
  ClassifiedUid key = makeKey("K001", "Lab", "IN");

  TransitionResult r = transition(ctx, CabinetEventType::KeyScanned, &key, nullptr, 2500);

  TEST_ASSERT_EQUAL_INT((int)CabinetState::Idle, (int)ctx.state);
  TEST_ASSERT_EQUAL_INT((int)CabinetEffect::ShowDenied, (int)r.effect);
  TEST_ASSERT_EQUAL_STRING("", ctx.pendingKeyUid.c_str());
  TEST_ASSERT_EQUAL_UINT32(0, ctx.pendingSince);
}

// Verifies the second TAKE step: when a key is scanned after a valid user,
// the FSM enters ProcessingTake and emits a request payload for the backend.
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

// Verifies the second RETURN step: when the user scans after a valid key,
// the FSM enters ProcessingReturn and emits the correct RETURN request data.
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

// Verifies that a pending two-scan transaction times out back to Idle,
// clears buffered scan data, and requests timeout-specific UI feedback.
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

// Verifies that a successful TAKE backend result finishes the transaction,
// clears pending state, and requests the unlock-and-success effect.
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

// Verifies that a backend business-rule rejection returns the FSM to Idle
// and surfaces the denial effect instead of unlocking the cabinet.
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

// Verifies that classify-time backend failures bypass the access-denied path
// and request an immediate server-failure effect for the operator.
void test_server_error_classification_requests_server_fail_effect() {
  ClassifiedScanDecision decision = decideClassifiedScan(makeServerError());

  TEST_ASSERT_TRUE(decision.useImmediateEffect);
  TEST_ASSERT_EQUAL_INT((int)CabinetEffect::ShowServerFail, (int)decision.immediateEffect);
  TEST_ASSERT_EQUAL_INT((int)CabinetEventType::DeniedScan, (int)decision.event);
}

// Verifies that a successful backend reply with the wrong action does not
// complete the transaction and is instead treated as a protocol failure.
void test_successful_process_with_mismatched_action_fails() {
  ProcessResult process = makeProcessOk("RETURN");

  CabinetEventType event = decideProcessFollowUpEvent(process, "TAKE");

  TEST_ASSERT_EQUAL_INT((int)CabinetEventType::ProcessFailed, (int)event);
}

// Verifies that known business-rule process failures still map to the denied
// path, preserving the distinction between expected denials and system errors.
void test_known_process_denial_maps_to_process_denied() {
  ProcessResult process = makeProcessFailure(ServerCode::KeyNotAvailable);

  CabinetEventType event = decideProcessFollowUpEvent(process, "TAKE");

  TEST_ASSERT_EQUAL_INT((int)CabinetEventType::ProcessDenied, (int)event);
}

// Verifies protocol boundary behavior for classify ERROR responses,
// ensuring backend/transport failures are classified as ServerError.
void test_parse_classify_error_maps_to_server_error() {
  ClassifiedUid parsed = parseClassifyResponse("ERROR");

  TEST_ASSERT_EQUAL_INT((int)ScanType::ServerError, (int)parsed.type);
  TEST_ASSERT_EQUAL_INT((int)ServerCode::Error, (int)parsed.code);
}

// Verifies protocol parsing for a successful TAKE response,
// including action decoding and user/key payload extraction.
void test_parse_process_ok_take_payload() {
  ProcessResult parsed = parseProcessResponse("OK|TAKE|Alice|Lab");

  TEST_ASSERT_TRUE(parsed.ok);
  TEST_ASSERT_EQUAL_INT((int)ServerCode::Ok, (int)parsed.code);
  TEST_ASSERT_EQUAL_STRING("TAKE", parsed.action.c_str());
  TEST_ASSERT_EQUAL_STRING("Alice", parsed.userName.c_str());
  TEST_ASSERT_EQUAL_STRING("Lab", parsed.keyName.c_str());
}

// Verifies protocol parsing for a successful RETURN response,
// including action decoding and user/key payload extraction.
void test_parse_process_ok_return_payload() {
  ProcessResult parsed = parseProcessResponse("OK|RETURN|Alice|Lab");

  TEST_ASSERT_TRUE(parsed.ok);
  TEST_ASSERT_EQUAL_INT((int)ServerCode::Ok, (int)parsed.code);
  TEST_ASSERT_EQUAL_STRING("RETURN", parsed.action.c_str());
  TEST_ASSERT_EQUAL_STRING("Alice", parsed.userName.c_str());
  TEST_ASSERT_EQUAL_STRING("Lab", parsed.keyName.c_str());
}

void setUp(void) {
}

void tearDown(void) {
}

void runTests() {
  UNITY_BEGIN();

  RUN_TEST(test_idle_user_scan_starts_take_flow);
  RUN_TEST(test_idle_key_scan_out_starts_return_flow);
  RUN_TEST(test_idle_key_scan_in_is_denied);
  RUN_TEST(test_awaiting_key_scan_starts_take_processing);
  RUN_TEST(test_awaiting_user_scan_starts_return_processing);
  RUN_TEST(test_timeout_resets_pending_state);
  RUN_TEST(test_take_completed_unlocks_and_returns_idle);
  RUN_TEST(test_processing_denied_returns_idle_with_denied_effect);
  RUN_TEST(test_server_error_classification_requests_server_fail_effect);
  RUN_TEST(test_successful_process_with_mismatched_action_fails);
  RUN_TEST(test_known_process_denial_maps_to_process_denied);
  RUN_TEST(test_parse_classify_error_maps_to_server_error);
  RUN_TEST(test_parse_process_ok_take_payload);
  RUN_TEST(test_parse_process_ok_return_payload);

  UNITY_END();
}

#ifdef ARDUINO
void setup() {
  delay(2000);
  runTests();
}

void loop() {
}
#else
int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  runTests();
  return 0;
}
#endif
