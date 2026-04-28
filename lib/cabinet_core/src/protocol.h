#pragma once

#include <Arduino.h>

enum class ScanType {
  Unknown,
  User,
  Key,
  Denied,
  ServerError
};

enum class ServerCode {
  Ok,
  NotFound,
  UserInactive,
  DuplicateUid,
  KeyNotAvailable,
  KeyAlreadyIn,
  UserNotFound,
  KeyNotFound,
  MissingParams,
  InvalidAction,
  InvalidMode,
  Error,
  Unexpected
};

struct ClassifiedUid {
  ScanType type = ScanType::Unknown;
  ServerCode code = ServerCode::Unexpected;
  String uid;
  String name;
  String keyStatus;
  String holderUid;
};

struct ProcessResult {
  bool ok = false;
  ServerCode code = ServerCode::Unexpected;
  String action;
  String userName;
  String keyName;
};

String getPart(const String& data, int index);

ClassifiedUid parseClassifyResponse(const String& response);
ProcessResult parseProcessResponse(const String& response);

String buildClassifyUrl(const String& baseUrl, const String& uid);
String buildProcessUrl(
  const String& baseUrl,
  const String& action,
  const String& userUid,
  const String& keyUid
);
