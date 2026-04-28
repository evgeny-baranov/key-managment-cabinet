#pragma once

#include <Arduino.h>

String httpGet(const String& url);
String classifyUidHttp(const String& uid);
String processActionHttp(const String& action, const String& userUid, const String& keyUid);
