#include "protocol.h"

#include "constants.h"

namespace {
ServerCode parseServerCode(const String& response) {
  if (response == RESP_NOT_FOUND) {
    return ServerCode::NotFound;
  }
  if (response == RESP_USER_INACTIVE) {
    return ServerCode::UserInactive;
  }
  if (response == RESP_DUPLICATE_UID) {
    return ServerCode::DuplicateUid;
  }
  if (response == RESP_KEY_NOT_AVAILABLE) {
    return ServerCode::KeyNotAvailable;
  }
  if (response == RESP_KEY_ALREADY_IN) {
    return ServerCode::KeyAlreadyIn;
  }
  if (response == RESP_USER_NOT_FOUND) {
    return ServerCode::UserNotFound;
  }
  if (response == RESP_KEY_NOT_FOUND) {
    return ServerCode::KeyNotFound;
  }
  if (response == RESP_MISSING_PARAMS) {
    return ServerCode::MissingParams;
  }
  if (response == RESP_INVALID_ACTION) {
    return ServerCode::InvalidAction;
  }
  if (response == RESP_INVALID_MODE) {
    return ServerCode::InvalidMode;
  }
  if (response == RESP_ERROR) {
    return ServerCode::Error;
  }
  return ServerCode::Unexpected;
}
}

String getPart(const String& data, int index) {
  int start = 0;
  int currentIndex = 0;

  for (int i = 0; i <= data.length(); i++) {
    if (i == data.length() || data.charAt(i) == '|') {
      if (currentIndex == index) {
        return data.substring(start, i);
      }
      currentIndex++;
      start = i + 1;
    }
  }
  return "";
}

ClassifiedUid parseClassifyResponse(const String& response) {
  ClassifiedUid parsed;

  if (response.startsWith(PREFIX_USER_FOUND)) {
    parsed.type = ScanType::User;
    parsed.code = ServerCode::Ok;
    parsed.uid = getPart(response, 1);
    parsed.name = getPart(response, 2);
    return parsed;
  }

  if (response.startsWith(PREFIX_KEY_FOUND)) {
    parsed.type = ScanType::Key;
    parsed.code = ServerCode::Ok;
    parsed.uid = getPart(response, 1);
    parsed.name = getPart(response, 2);
    parsed.keyStatus = getPart(response, 3);
    parsed.holderUid = getPart(response, 4);
    return parsed;
  }

  parsed.code = parseServerCode(response);

  if (parsed.code == ServerCode::Error) {
    parsed.type = ScanType::ServerError;
  } else if (
    parsed.code == ServerCode::NotFound ||
    parsed.code == ServerCode::UserInactive ||
    parsed.code == ServerCode::DuplicateUid
  ) {
    parsed.type = ScanType::Denied;
  } else {
    parsed.type = ScanType::Unknown;
  }

  return parsed;
}

ProcessResult parseProcessResponse(const String& response) {
  ProcessResult parsed;

  if (response.startsWith(PREFIX_OK_TAKE)) {
    parsed.ok = true;
    parsed.code = ServerCode::Ok;
    parsed.action = ACTION_TAKE;
    parsed.userName = getPart(response, 2);
    parsed.keyName = getPart(response, 3);
    return parsed;
  }

  if (response.startsWith(PREFIX_OK_RETURN)) {
    parsed.ok = true;
    parsed.code = ServerCode::Ok;
    parsed.action = ACTION_RETURN;
    parsed.userName = getPart(response, 2);
    parsed.keyName = getPart(response, 3);
    return parsed;
  }

  parsed.ok = false;
  parsed.code = parseServerCode(response);
  return parsed;
}

String buildClassifyUrl(const String& baseUrl, const String& uid) {
  return baseUrl + "?mode=" + MODE_CLASSIFY + "&uid=" + uid;
}

String buildProcessUrl(
  const String& baseUrl,
  const String& action,
  const String& userUid,
  const String& keyUid
) {
  return baseUrl + "?mode=" + MODE_PROCESS + "&action=" + action + "&useruid=" + userUid + "&keyuid=" + keyUid;
}
