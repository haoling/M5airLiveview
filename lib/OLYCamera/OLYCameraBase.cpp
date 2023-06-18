#include "OLYCameraBase.h"

using namespace tinyxml2;

String OLYCameraBase::httpGet(String api, const int normalResponseCode)
{
    httpClient.begin(baseUrl + api);
    httpClient.addHeader("X-Protocol", "OlympusCameraKit");

    int status_code = httpClient.GET();
    if(status_code != normalResponseCode) {
        Serial.println("HTTP GET failed");
        Serial.println(status_code);
        Serial.println(httpClient.getString());
        lastError = OLYCAMERAERROR_HTTP;
        lastErrorMessage = String("HTTP GET failed: status_code=" + status_code);
        httpClient.end();
        return "";
    }

    String response = httpClient.getString();
    httpClient.end();
    return response;
}

String OLYCameraBase::getRootXmlText(String xml)
{
    XMLDocument xmlDocument;
    if(xmlDocument.Parse(xml.c_str())!= XML_SUCCESS) {
        Serial.println("Error parsing");
        lastError = OLYCAMERAERROR_XML;
        lastErrorMessage = xmlDocument.ErrorName();
        return "";
    }

    XMLElement *root = xmlDocument.RootElement();
    return root->GetText();
}
