#include "http_client.h"

#include <HTTPClient.h>
#include <WiFi.h>

#include "config.h"
#include "constants.h"
#include "protocol.h"

String httpGet(const String& url) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return RESP_ERROR;
  }

  HTTPClient http;
  http.begin(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(HTTP_TIMEOUT_MS);

  int httpCode = http.GET();
  String payload = "";

  if (httpCode > 0) {
    payload = http.getString();
    payload.trim();
    Serial.print("HTTP ");
    Serial.println(httpCode);
    Serial.print("Response: ");
    Serial.println(payload);
  } else {
    Serial.print("HTTP error: ");
    Serial.println(httpCode);
    payload = RESP_ERROR;
  }

  http.end();
  return payload;
}

String classifyUidHttp(const String& uid) {
  String url = buildClassifyUrl(SCRIPT_URL, uid);
  Serial.print("Classify URL: ");
  Serial.println(url);
  return httpGet(url);
}

String processActionHttp(const String& action, const String& userUid, const String& keyUid) {
  String url = buildProcessUrl(SCRIPT_URL, action, userUid, keyUid);
  Serial.print("Process URL: ");
  Serial.println(url);
  return httpGet(url);
}
