const SHEET_USERS = "Users";
const SHEET_KEYS = "Keys";
const SHEET_LOGS = "Logs";

const MODE_CLASSIFY = "classify";
const MODE_PROCESS = "process";

const ACTION_TAKE = "TAKE";
const ACTION_RETURN = "RETURN";

const KEY_STATUS_IN = "IN";
const KEY_STATUS_OUT = "OUT";

const RESP_INVALID_MODE = "INVALID_MODE";
const RESP_MISSING_UID = "MISSING_UID";
const RESP_DUPLICATE_UID = "DUPLICATE_UID";
const RESP_USER_INACTIVE = "USER_INACTIVE";
const RESP_NOT_FOUND = "NOT_FOUND";
const RESP_MISSING_PARAMS = "MISSING_PARAMS";
const RESP_USER_NOT_FOUND = "USER_NOT_FOUND";
const RESP_KEY_NOT_FOUND = "KEY_NOT_FOUND";
const RESP_KEY_NOT_AVAILABLE = "KEY_NOT_AVAILABLE";
const RESP_KEY_ALREADY_IN = "KEY_ALREADY_IN";
const RESP_INVALID_ACTION = "INVALID_ACTION";

const COL_UID = 1;
const COL_USER_NAME = 2;
const COL_USER_ACTIVE = 3;

const COL_KEY_UID = 1;
const COL_KEY_NAME = 2;
const COL_KEY_STATUS = 3;
const COL_KEY_HOLDER_UID = 4;

function doGet(e) {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var usersSheet = ss.getSheetByName(SHEET_USERS);
  var keysSheet = ss.getSheetByName(SHEET_KEYS);
  var logsSheet = ss.getSheetByName(SHEET_LOGS);

  var mode = (e.parameter.mode || "").toString().trim().toLowerCase();

  if (mode === MODE_CLASSIFY) {
    var uid = normalizeUid(e.parameter.uid);
    return ContentService.createTextOutput(classifyUid(usersSheet, keysSheet, uid));
  }

  if (mode === MODE_PROCESS) {
    var action = (e.parameter.action || "").toString().trim().toUpperCase();
    var userUid = normalizeUid(e.parameter.useruid);
    var keyUid = normalizeUid(e.parameter.keyuid);
    return ContentService.createTextOutput(processAction(usersSheet, keysSheet, logsSheet, action, userUid, keyUid));
  }

  return ContentService.createTextOutput(RESP_INVALID_MODE);
}

function normalizeUid(value) {
  return (value || "")
    .toString()
    .trim()
    .toUpperCase()
    .replace(/[^0-9A-F]/g, "");
}

function classifyUid(usersSheet, keysSheet, uid) {
  if (!uid) return RESP_MISSING_UID;

  var userResult = findUser(usersSheet, uid);
  var keyResult = findKey(keysSheet, uid);

  if (userResult.found && keyResult.found) {
    return RESP_DUPLICATE_UID;
  }

  if (userResult.found) {
    if (!userResult.active) return RESP_USER_INACTIVE;
    return "USER_FOUND|" + userResult.uid + "|" + userResult.name;
  }

  if (keyResult.found) {
    return "KEY_FOUND|" + keyResult.uid + "|" + keyResult.name + "|" + keyResult.status + "|" + keyResult.holderUid;
  }

  return RESP_NOT_FOUND;
}

function processAction(usersSheet, keysSheet, logsSheet, action, userUid, keyUid) {
  if (!action || !userUid || !keyUid) {
    return RESP_MISSING_PARAMS;
  }

  var userResult = findUser(usersSheet, userUid);
  if (!userResult.found) return RESP_USER_NOT_FOUND;
  if (!userResult.active) return RESP_USER_INACTIVE;

  var keyResult = findKey(keysSheet, keyUid);
  if (!keyResult.found) return RESP_KEY_NOT_FOUND;

  if (action === ACTION_TAKE) {
    if (String(keyResult.status).toUpperCase() !== KEY_STATUS_IN) {
      return RESP_KEY_NOT_AVAILABLE;
    }

    keysSheet.getRange(keyResult.row, COL_KEY_STATUS).setValue(KEY_STATUS_OUT);
    keysSheet.getRange(keyResult.row, COL_KEY_HOLDER_UID).setValue(userResult.uid);

    logsSheet.appendRow([
      new Date(),
      ACTION_TAKE,
      userResult.uid,
      userResult.name,
      keyResult.uid,
      keyResult.name,
      "OK"
    ]);

    return "OK|" + ACTION_TAKE + "|" + userResult.name + "|" + keyResult.name;
  }

  if (action === ACTION_RETURN) {
    if (String(keyResult.status).toUpperCase() !== KEY_STATUS_OUT) {
      return RESP_KEY_ALREADY_IN;
    }

    keysSheet.getRange(keyResult.row, COL_KEY_STATUS).setValue(KEY_STATUS_IN);
    keysSheet.getRange(keyResult.row, COL_KEY_HOLDER_UID).setValue("");

    logsSheet.appendRow([
      new Date(),
      ACTION_RETURN,
      userResult.uid,
      userResult.name,
      keyResult.uid,
      keyResult.name,
      "OK"
    ]);

    return "OK|" + ACTION_RETURN + "|" + userResult.name + "|" + keyResult.name;
  }

  return RESP_INVALID_ACTION;
}

function findUser(sheet, uid) {
  var lastRow = sheet.getLastRow();
  if (lastRow < 2) {
    return { found: false };
  }

  var values = sheet.getRange(2, COL_UID, lastRow - 1, COL_USER_ACTIVE).getValues();

  for (var i = 0; i < values.length; i++) {
    var sheetUid = normalizeUid(values[i][COL_UID - 1]);
    var name = (values[i][COL_USER_NAME - 1] || "").toString().trim();
    var active = values[i][COL_USER_ACTIVE - 1] === true || String(values[i][COL_USER_ACTIVE - 1]).toUpperCase() === "TRUE";

    if (sheetUid === uid) {
      return {
        found: true,
        row: i + 2,
        uid: sheetUid,
        name: name,
        active: active
      };
    }
  }

  return { found: false };
}

function findKey(sheet, uid) {
  var lastRow = sheet.getLastRow();
  if (lastRow < 2) {
    return { found: false };
  }

  var values = sheet.getRange(2, COL_KEY_UID, lastRow - 1, COL_KEY_HOLDER_UID).getValues();

  for (var i = 0; i < values.length; i++) {
    var sheetUid = normalizeUid(values[i][COL_KEY_UID - 1]);
    var name = (values[i][COL_KEY_NAME - 1] || "").toString().trim();
    var status = (values[i][COL_KEY_STATUS - 1] || "").toString().trim().toUpperCase();
    var holderUid = normalizeUid(values[i][COL_KEY_HOLDER_UID - 1]);

    if (sheetUid === uid) {
      return {
        found: true,
        row: i + 2,
        uid: sheetUid,
        name: name,
        status: status,
        holderUid: holderUid
      };
    }
  }

  return { found: false };
}
